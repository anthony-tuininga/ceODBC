#------------------------------------------------------------------------------
# cursor.pyx
#   Cython file defining the cursor class (embedded in driver.pyx).
#------------------------------------------------------------------------------

cdef class Cursor:
    cdef:
        readonly Connection connection
        readonly str statement
        readonly unsigned long rowcount
        public unsigned long arraysize
        public object inputtypehandler
        public object outputtypehandler
        public object rowfactory
        SQLHANDLE _handle
        list _bind_vars
        list _fetch_vars
        unsigned long _rowcount
        unsigned long _buffer_rowcount
        unsigned long _buffer_index
        unsigned long _fetch_array_size
        int _set_output_size
        int _set_output_size_column
        bint _more_rows_to_fetch

    def __dealloc__(self):
        if self._handle:
            SQLFreeHandle(SQL_HANDLE_STMT, self._handle)

    cdef inline int _check_can_fetch(self) except -1:
        self._check_open()
        if not self._fetch_vars:
            _raise_from_string(exceptions.InterfaceError, "not a query")

    cdef inline int _check_open(self) except -1:
        if not self._handle:
            _raise_from_string(exceptions.InterfaceError, "not open")
        self.connection._check_connected()

    cdef object _create_row(self):
        cdef:
            object column
            Py_ssize_t i
            tuple row
            Var var
        row = cpython.PyTuple_New(len(self._fetch_vars))
        for i, var in enumerate(self._fetch_vars):
            column = var._get_value(<unsigned> self._buffer_index)
            cpython.Py_INCREF(column)
            cpython.PyTuple_SET_ITEM(row, i, column)
        if self.rowfactory:
            return self.rowfactory(row)
        return row

    cdef Var _create_var(self, DbType dbtype, unsigned num_elements,
                         SQLUINTEGER size):
        cdef:
            unsigned long long data_length
            unsigned i
            Var var

        # create new variable and perform basic initialization
        var = Var.__new__(Var)
        var.type = dbtype
        if num_elements == 0:
            var.num_elements = 1
        else:
            var.num_elements = num_elements
        var.size = size
        if dbtype._buffer_size > 0:
            var.buffer_size = dbtype._buffer_size
        else:
            var.buffer_size = size * dbtype._bytes_multiplier
        var._position = -1

        # allocate data array
        data_length = <unsigned long long> num_elements * \
                      <unsigned long long> var.buffer_size
        if data_length > INT_MAX:
            raise ValueError("array size too large")
        var._data.as_raw = cpython.PyMem_Malloc(<size_t> data_length)
        if not var._data.as_raw:
            raise MemoryError()

        # allocate indicator and initialize it
        var._length_or_indicator = <SQLLEN*> \
                cpython.PyMem_Malloc(num_elements * sizeof(SQLLEN))
        if not var._length_or_indicator:
            raise MemoryError()
        for i in range(num_elements):
            var._length_or_indicator[i] = SQL_NULL_DATA

        return var

    cdef Var _create_var_for_result_set(self, SQLUSMALLINT position):
        cdef:
            SQLSMALLINT data_type, length, scale, nullable
            DbType dbtype
            object result
            SQLULEN size
            SQLRETURN rc
            Var var

        # retrieve information about the column
        rc = SQLDescribeColA(self._handle, position, NULL, 0, &length,
                             &data_type, &size, &scale, &nullable)
        _check_stmt_error(self._handle, rc)
        dbtype = DbType._from_sql_data_type(data_type)

        # some ODBC drivers do not return the SQL data type for a long string
        # but instead return a string with size 0; provide a workaround
        if size == 0:
            if dbtype is DB_TYPE_STRING:
                dbtype = DB_TYPE_LONG_STRING
            elif dbtype is DB_TYPE_BINARY:
                dbtype = DB_TYPE_LONG_BINARY

        # for long columns, set an appropriate size
        if dbtype is DB_TYPE_LONG_STRING or dbtype is DB_TYPE_LONG_BINARY:
            if self._set_output_size > 0 \
                    and (self._set_output_size_column == 0
                            or position == self._set_output_size_column):
                size = self._set_output_size
            else:
                size = 128 * 1024

        # check to see if an output type handler has been specified and that
        # it returns a variable with sufficient elements to fetch
        if self.outputtypehandler is not None:
            result = self.outputtypehandler(self, dbtype, size, scale)
        elif self.connection.outputtypehandler is not None:
            result = self.connection.outputtypehandler(self, dbtype, size,
                                                       scale)
        else:
            result = None
        if result is not None:
            if not isinstance(result, Var):
                raise TypeError("expecting variable from output type handler")
            var = <Var> result
            if var.num_elements < self._fetch_array_size:
                msg = f"expecting variable with {self.fetch_array_size}" \
                      f"elements but only got {var.num_elements} elements"
                raise TypeError(msg)

        # otherwise, create a variable using the fetch metadata
        else:
            var = self._create_var(dbtype, self._fetch_array_size, size)

        # bind the column
        var._position = position
        rc = SQLBindCol(self._handle, position, var.type._c_data_type,
                        var._data.as_raw, var.buffer_size,
                        var._length_or_indicator)
        _check_stmt_error(self._handle, rc)
        return var


    cdef int _fetch_rows(self) except -1:
        cdef:
            SQLRETURN rc
        with nogil:
            rc = SQLFetch(self._handle)
        if rc == SQL_NO_DATA:
            self._buffer_rowcount = 0
            self._more_rows_to_fetch = False
        else:
            _check_stmt_error(self._handle, rc)
        self._buffer_index = 0

    cdef int _more_rows(self) except -1:
        if self._buffer_index >= self._buffer_rowcount:
            if self._more_rows_to_fetch:
                self._fetch_rows()
        return self._more_rows_to_fetch

    cdef int _prepare(self, str statement) except -1:
        cdef:
            SQLINTEGER statement_length
            SQLCHAR* statement_ptr
            bytes statement_bytes
            SQLRETURN rc

        # make sure a statement is available to be prepared
        if statement is None and self.statement is None:
            message = "no statement specified and no prior statement prepared"
            _raise_from_string(exceptions.ProgrammingError, message)

        # close original statement if necessary in order to discard results
        if self.statement is not None:
            SQLCloseCursor(self._handle)

        # keep track of statement
        if statement is not None:
            self.statement = statement

        # clear previous result set parameters
        self._fetch_vars = None

        # prepare statement
        statement_bytes = self.statement.encode()
        statement_ptr = <SQLCHAR*> statement_bytes
        statement_length = <SQLINTEGER> len(statement_bytes)
        with nogil:
            rc = SQLPrepareA(self._handle, statement_ptr, statement_length)
        _check_stmt_error(self._handle, rc)

    cdef int _prepare_result_set(self) except -1:
        cdef:
            SQLSMALLINT num_columns, i
            list fetch_vars
            SQLRETURN rc
            Var var
        rc = SQLNumResultCols(self._handle, &num_columns)
        _check_stmt_error(self._handle, rc)
        if num_columns == 0:
            return 0
        self._fetch_array_size = self.arraysize
        rc = SQLSetStmtAttr(self._handle, SQL_ATTR_ROW_ARRAY_SIZE,
                            <SQLPOINTER> self._fetch_array_size,
                            SQL_IS_UINTEGER)
        _check_stmt_error(self._handle, rc)
        rc = SQLSetStmtAttr(self._handle, SQL_ATTR_ROWS_FETCHED_PTR,
                            &self._buffer_rowcount, SQL_IS_POINTER)
        _check_stmt_error(self._handle, rc)
        fetch_vars = cpython.PyList_New(num_columns)
        for i in range(num_columns):
            var = self._create_var_for_result_set(i + 1)
            cpython.Py_INCREF(var)
            cpython.PyList_SET_ITEM(fetch_vars, i, var)
        self._fetch_vars = fetch_vars
        self.rowcount = 0
        self._buffer_rowcount = 0
        self._buffer_index = 0
        self._more_rows_to_fetch = True

    def execute(self, statement, *args):
        cdef:
            SQLLEN rowcount
            SQLRETURN rc
        self._check_open()
        self._prepare(statement)
        with nogil:
            rc = SQLExecute(self._handle)
        if rc == SQL_NO_DATA:
            self._rowcount = 0
            return
        _check_stmt_error(self._handle, rc)
        self._prepare_result_set()
        if self._fetch_vars:
            return self
        rc = SQLRowCount(self._handle, &rowcount)
        _check_stmt_error(self._handle, rc)
        self.rowcount = <unsigned long> rowcount

    def fetchone(self):
        self._check_can_fetch()
        if self._more_rows() > 0:
            return self._create_row()
