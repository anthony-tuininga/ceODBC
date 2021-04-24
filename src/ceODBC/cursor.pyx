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

    def __enter__(self):
        self._check_open()
        return self

    def __exit__(self, exc_type, exc_value, exc_tb):
        self.close()

    def __iter__(self):
        return self

    def __next__(self):
        self._check_can_fetch()
        if self._more_rows() > 0:
            return self._create_row()
        raise StopIteration

    cdef int _bind_parameters(self, tuple parameters, unsigned num_elements=1,
                              unsigned pos=0) except -1:
        cdef:
            SQLUSMALLINT i, num_params, orig_num_vars
            SQLSMALLINT var_direction
            SQLRETURN rc
            Var orig_var
        num_params = <SQLUSMALLINT> cpython.PyTuple_GET_SIZE(parameters)
        if self._bind_vars:
            orig_num_vars = \
                    <SQLUSMALLINT> cpython.PyList_GET_SIZE(self._bind_vars)
        else:
            orig_num_vars = 0
            self._bind_vars = [None] * num_params
        for i, value in enumerate(parameters):

            # determine which variable should be bound
            if i < orig_num_vars:
                orig_var = <Var> cpython.PyList_GET_ITEM(self._bind_vars, i)
            else:
                orig_var = None
            var = self._get_bind_var(num_elements, pos, value, orig_var)
            if var is orig_var:
                continue
            if i < len(self._bind_vars):
                self._bind_vars[i] = var
            else:
                self._bind_vars.append(var)

            # if this variable has not been bound before, ensure that it is
            # bound
            if var._position < 0:
                var._position = i + 1
                if var.input and var.output:
                    var_direction = SQL_PARAM_INPUT_OUTPUT
                elif var.output:
                    var_direction = SQL_PARAM_OUTPUT
                else:
                    var_direction = SQL_PARAM_INPUT
                rc = SQLBindParameter(self._handle, var._position,
                                      var_direction, var.type._c_data_type,
                                      var.type._sql_data_type, var.size,
                                      var.scale, var._data.as_raw,
                                      var.buffer_size,
                                      var._length_or_indicator)
                _check_stmt_error(self._handle, rc)

    def _call(self, name, parameters, return_value=None):

        # if only a single argument is passed and that argument is a list or
        # tuple, use that value as the parameters
        if len(parameters) == 1 and isinstance(parameters[0], (list, tuple)):
            parameters = parameters[0]

        # build statement
        bind_values = []
        statement_parts = ["{"]
        if return_value is not None:
            statement_parts.append("? = ")
            bind_values.append(return_value)
        statement_parts.append(f"CALL {name}")
        if len(parameters) > 0:
            args = ",".join(["?"] * len(parameters))
            statement_parts.append(f"({args})")
            bind_values.extend(parameters)
        statement = "".join(statement_parts) + "}"

        # execute statement
        self.execute(statement, *bind_values)

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
        self._buffer_index += 1
        self.rowcount += 1
        if self.rowfactory is not None:
            return self.rowfactory(*row)
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

    cdef Var _create_var_by_value(self, object value, unsigned num_elements):
        cdef:
            SQLUINTEGER size = 0
            DbType dbtype
        dbtype = DbType._from_value(value, &size)
        return self._create_var(dbtype, num_elements, size)

    cdef Var _create_var_for_result_set(self, SQLUSMALLINT position):
        cdef:
            SQLSMALLINT data_type, name_length, scale, nullable
            SQLCHAR name[256]
            DbType dbtype
            object result
            SQLULEN size
            SQLRETURN rc
            Var var

        # retrieve information about the column
        rc = SQLDescribeColA(self._handle, position, name, sizeof(name),
                             &name_length, &data_type, &size, &scale,
                             &nullable)
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
        if name_length > <SQLSMALLINT> sizeof(name):
            name_length = <SQLSMALLINT> sizeof(name)
        var.name = name[:name_length].decode()
        var.scale = scale
        var.nulls_allowed = nullable != SQL_NO_NULLS

        # bind the column
        var._position = position
        rc = SQLBindCol(self._handle, position, var.type._c_data_type,
                        var._data.as_raw, var.buffer_size,
                        var._length_or_indicator)
        _check_stmt_error(self._handle, rc)
        return var

    cdef object _execute(self):
        cdef:
            SQLLEN rowcount
            SQLRETURN rc
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

    cdef Var _get_bind_var(self, unsigned num_elements, unsigned pos,
                           object value, Var orig_var):
        cdef:
            object temp_value
            unsigned i
            Var var
        if isinstance(value, Var):
            return <Var> value
        elif orig_var is not None:
            if num_elements <= orig_var.num_elements:
                var = orig_var
            else:
                var = self._create_var(orig_var.type, num_elements,
                                       orig_var.size)
                if pos > 0:
                    for i in range(pos - 1):
                        temp_value = orig_var._get_value(i)
                        var._set_value(i, temp_value)
            try:
                var._set_value(pos, value)
                return var
            except:
                if pos > 0:
                    raise
        var = self._create_var_by_value(value, num_elements)
        var._set_value(pos, value)
        return var

    cdef tuple _massage_args(self, args):
        """
        if only a single argument is passed and that argument is a list or
        tuple, return that value
        """
        if len(args) == 1 and isinstance(args[0], (list, tuple)):
            return tuple(args[0])
        return args

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
        self.rowfactory = None

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

    def callfunc(self, name, return_type, *args):
        var = self.var(return_type, input=False, output=True)
        self._call(name, args, var)
        return var.getvalue()

    def callproc(self, name, *args):
        self._call(name, args)
        if not args:
            return []
        return [v.getvalue() for v in self._bind_vars]

    def close(self):
        cdef SQLRETURN rc
        self._check_open()
        SQLCloseCursor(self._handle)
        rc = SQLFreeHandle(SQL_HANDLE_STMT, self._handle)
        _check_stmt_error(self._handle, rc)
        self._handle = NULL

    @property
    def description(self):
        self._check_open()
        if self._fetch_vars is None:
            return None
        return [v._description for v in self._fetch_vars]

    def execute(self, statement, *args):
        self._check_open()
        self._prepare(statement)
        args = self._massage_args(args)
        self._bind_parameters(args)
        return self._execute()

    def executemany(self, statement, args):
        cdef:
            unsigned num_rows
            object row_args
            SQLULEN temp
            SQLRETURN rc
            ssize_t i
        if statement is not None and not isinstance(statement, str):
            raise TypeError("expecting None or a string")
        if not isinstance(args, list):
            raise TypeError("expecting list")
        self._check_open()
        self._prepare(statement)
        num_rows = <unsigned> len(args)
        for i, row_args in enumerate(args):
            if not isinstance(row_args, (list, tuple)):
                message = "expecting a list of lists or tuples"
                _raise_from_string(exceptions.InterfaceError, message)
            self._bind_parameters(tuple(row_args), num_rows, i)
        temp = <SQLULEN> num_rows
        rc = SQLSetStmtAttr(self._handle, SQL_ATTR_PARAMSET_SIZE,
                <SQLPOINTER> temp, SQL_IS_UINTEGER)
        _check_stmt_error(self._handle, rc)
        return self._execute()

    def fetchall(self):
        self._check_can_fetch()
        result = []
        while self._more_rows() > 0:
            result.append(self._create_row())
        return result

    def fetchmany(self, num_rows=None):
        self._check_can_fetch()
        if num_rows is None:
            num_rows = self.arraysize
        result = []
        while len(result) < num_rows and self._more_rows() > 0:
            result.append(self._create_row())
        return result

    def fetchone(self):
        self._check_can_fetch()
        if self._more_rows() > 0:
            return self._create_row()

    def prepare(self, statement):
        self._check_open()
        self._prepare(statement)

    def setinputsizes(self, *args):
        cdef:
            tuple massaged_args
            ssize_t i, size
            list bind_vars
            DbType dbtype
            object value
            Var var
        self._check_open()
        massaged_args = self._massage_args(args)
        bind_vars = [None] * len(massaged_args)
        for i, value in enumerate(massaged_args):
            if value is None:
                var = None
            elif isinstance(value, Var):
                var = value
            else:
                if isinstance(value, int):
                    dbtype = DB_TYPE_STRING
                    size = value
                else:
                    dbtype = DbType._from_type(value)
                    size = 0
                var = self._create_var(dbtype, 1, size)
            bind_vars[i] = var
        self._bind_vars = bind_vars
        return bind_vars

    def var(self, type, size=0, scale=0, arraysize=1, inconverter=None,
            outconverter=None, input=True, output=False):
        cdef:
            DbType dbtype
            Var var
        dbtype = DbType._from_type(type)
        var = self._create_var(dbtype, arraysize, size)
        var.scale = scale
        var.inconverter = inconverter
        var.outconverter = outconverter
        var.input = input
        var.output = output
        return var
