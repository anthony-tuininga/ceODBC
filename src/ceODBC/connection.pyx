#------------------------------------------------------------------------------
# connection.pyx
#   Cython file defining the connection class (embedded in driver.pyx).
#------------------------------------------------------------------------------

cdef class Connection:
    cdef:
        SQLHANDLE _handle, _env_handle
        readonly str dsn
        public object inputtypehandler
        public object outputtypehandler

    def __init__(self, dsn, autocommit=False):
        self._connect(dsn, autocommit)

    def __dealloc__(self):
        if self._handle:
            with nogil:
                SQLEndTran(SQL_HANDLE_DBC, self._handle, SQL_ROLLBACK)
                SQLDisconnect(self._handle)
                SQLFreeHandle(SQL_HANDLE_DBC, self._handle)
            self._handle = NULL
        if self._env_handle:
            SQLFreeHandle(SQL_HANDLE_ENV, self._env_handle)

    @property
    def autocommit(self):
        return False

    @autocommit.setter
    def autocommit(self, value):
        pass

    cdef inline int _check_connected(self) except -1:
        if self._handle == NULL:
            _raise_from_string(exceptions.InterfaceError, "not connected")

    def _connect(self, dsn, autocommit):
        cdef:
            SQLSMALLINT dsn_length, actual_dsn_length
            SQLCHAR actual_dsn_buf[1024]
            SQLCHAR *dsn_ptr
            bytes dsn_bytes
            str actual_dsn
            SQLRETURN rc

        # create the environment handle
        rc = SQLAllocHandle(SQL_HANDLE_ENV, NULL, &self._env_handle)
        if rc != SQL_SUCCESS:
            _raise_from_string(exceptions.InternalError,
                               "unable to allocate ODBC environment handle")

        # set the attribute specifying which ODBC version to use
        rc = SQLSetEnvAttr(self._env_handle, SQL_ATTR_ODBC_VERSION,
                <SQLPOINTER> SQL_OV_ODBC3, 0)
        _check_error(SQL_HANDLE_ENV, self._env_handle, rc)

        # allocate a handle for the connection
        rc = SQLAllocHandle(SQL_HANDLE_DBC, self._env_handle, &self._handle)
        _check_error(SQL_HANDLE_ENV, self._env_handle, rc)

        # connect to driver
        dsn_bytes = dsn.encode()
        dsn_ptr = <SQLCHAR*> dsn_bytes
        dsn_length = <SQLSMALLINT> len(dsn_bytes)
        actual_dsn_length = <SQLSMALLINT> sizeof(actual_dsn_buf)
        with nogil:
            rc = SQLDriverConnectA(self._handle, NULL, dsn_ptr, dsn_length,
                                   actual_dsn_buf, actual_dsn_length,
                                   &actual_dsn_length, SQL_DRIVER_NOPROMPT)
        _check_conn_error(self._handle, rc)
        if actual_dsn_length > <SQLSMALLINT> sizeof(actual_dsn_buf) - 1:
            actual_dsn_length = <SQLSMALLINT> sizeof(actual_dsn_buf) - 1;
        actual_dsn = actual_dsn_buf[:actual_dsn_length].decode()
        self.dsn = self._remove_password_from_dsn(actual_dsn)

        # turn off autocommit, if applicable
        if not autocommit:
            rc = SQLSetConnectAttr(self._handle, SQL_ATTR_AUTOCOMMIT,
                                   <SQLPOINTER> SQL_AUTOCOMMIT_OFF,
                                   SQL_IS_UINTEGER)
        _check_conn_error(self._handle, rc)

    cdef str _remove_password_from_dsn(self, str dsn):
        cdef:
            Py_ssize_t start_pos, end_pos, brace_pos
            str upper_dsn

        # attempt to find PWD= in the DSN; if not found return DSN unchanged
        upper_dsn = dsn.upper()
        start_pos = upper_dsn.find("PWD=")
        if start_pos < 0:
            return dsn

        # search for the ending semicolon; if not found use the full length
        end_pos = upper_dsn.find(";", start_pos)
        if end_pos < 0:
            end_pos = len(upper_dsn)

        # search for a brace as well since that escapes a semicolon if present
        brace_pos = upper_dsn.find("{", start_pos)
        if brace_pos >= start_pos and brace_pos < end_pos:
            brace_pos = upper_dsn.find("}", brace_pos)
            if brace_pos < 0:
                return dsn
            end_pos = upper_dsn.find(";", brace_pos)
            if end_pos < 0:
                end_pos = len(upper_dsn)

        # return the DSN less the password section
        return dsn[:start_pos + 4] + dsn[end_pos:]

    def close(self):
        cdef SQLRETURN rc
        self._check_connected()
        with nogil:
            rc = SQLEndTran(SQL_HANDLE_DBC, self._handle, SQL_ROLLBACK)
            if rc == SQL_SUCCESS or rc == SQL_SUCCESS_WITH_INFO:
                rc = SQLDisconnect(self._handle)
        _check_conn_error(self._handle, rc)
        SQLFreeHandle(SQL_HANDLE_DBC, self._handle)
        self._handle = NULL

    def columns(self, catalog=None, schema=None, table=None, column=None):
        cdef:
            StringBuffer catalog_buf = StringBuffer()
            StringBuffer schema_buf = StringBuffer()
            StringBuffer table_buf = StringBuffer()
            StringBuffer column_buf = StringBuffer()
            Cursor cursor
            SQLRETURN rc
        catalog_buf.set_value(catalog)
        schema_buf.set_value(schema)
        table_buf.set_value(table)
        column_buf.set_value(column)
        self._check_connected()
        cursor = self.cursor()
        with nogil:
            rc = SQLColumns(cursor._handle, catalog_buf.ptr,
                            catalog_buf.length, schema_buf.ptr,
                            schema_buf.length, table_buf.ptr, table_buf.length,
                            column_buf.ptr, column_buf.length)
        _check_stmt_error(cursor._handle, rc)
        cursor._prepare_result_set()
        return cursor

    def columnprivileges(self, catalog=None, schema=None, table=None,
                         column=None):
        cdef:
            StringBuffer catalog_buf = StringBuffer()
            StringBuffer schema_buf = StringBuffer()
            StringBuffer table_buf = StringBuffer()
            StringBuffer column_buf = StringBuffer()
            Cursor cursor
            SQLRETURN rc
        catalog_buf.set_value(catalog)
        schema_buf.set_value(schema)
        table_buf.set_value(table)
        column_buf.set_value(column)
        self._check_connected()
        cursor = self.cursor()
        with nogil:
            rc = SQLColumnPrivileges(cursor._handle, catalog_buf.ptr,
                                     catalog_buf.length, schema_buf.ptr,
                                     schema_buf.length, table_buf.ptr,
                                     table_buf.length, column_buf.ptr,
                                     column_buf.length)
        _check_stmt_error(cursor._handle, rc)
        cursor._prepare_result_set()
        return cursor

    def commit(self):
        cdef SQLRETURN rc
        self._check_connected()
        with nogil:
            rc = SQLEndTran(SQL_HANDLE_DBC, self._handle, SQL_COMMIT)
        _check_conn_error(self._handle, rc)

    def cursor(self):
        cdef:
            Cursor cursor
            SQLRETURN rc
        cursor = Cursor.__new__(Cursor)
        cursor.connection = self
        cursor.arraysize = 1
        rc = SQLAllocHandle(SQL_HANDLE_STMT, self._handle, &cursor._handle)
        _check_conn_error(self._handle, rc)
        return cursor

    def foreignkeys(self, pkcatalog=None, pkschema=None, pktable=None,
                    fkcatalog=None, fkschema=None, fktable=None):
        cdef:
            StringBuffer pkcatalog_buf = StringBuffer()
            StringBuffer pkschema_buf = StringBuffer()
            StringBuffer pktable_buf = StringBuffer()
            StringBuffer fkcatalog_buf = StringBuffer()
            StringBuffer fkschema_buf = StringBuffer()
            StringBuffer fktable_buf = StringBuffer()
            Cursor cursor
            SQLRETURN rc
        pkcatalog_buf.set_value(pkcatalog)
        pkschema_buf.set_value(pkschema)
        pktable_buf.set_value(pktable)
        fkcatalog_buf.set_value(fkcatalog)
        fkschema_buf.set_value(fkschema)
        fktable_buf.set_value(fktable)
        self._check_connected()
        cursor = self.cursor()
        with nogil:
            rc = SQLForeignKeys(cursor._handle, pkcatalog_buf.ptr,
                                pkcatalog_buf.length, pkschema_buf.ptr,
                                pkschema_buf.length, pktable_buf.ptr,
                                pktable_buf.length, fkcatalog_buf.ptr,
                                fkcatalog_buf.length, fkschema_buf.ptr,
                                fkschema_buf.length, fktable_buf.ptr,
                                fktable_buf.length)
        _check_stmt_error(cursor._handle, rc)
        cursor._prepare_result_set()
        return cursor

    def primarykeys(self, catalog=None, schema=None, table=None):
        cdef:
            StringBuffer catalog_buf = StringBuffer()
            StringBuffer schema_buf = StringBuffer()
            StringBuffer table_buf = StringBuffer()
            Cursor cursor
            SQLRETURN rc
        catalog_buf.set_value(catalog)
        schema_buf.set_value(schema)
        table_buf.set_value(table)
        self._check_connected()
        cursor = self.cursor()
        with nogil:
            rc = SQLPrimaryKeys(cursor._handle, catalog_buf.ptr,
                                catalog_buf.length, schema_buf.ptr,
                                schema_buf.length, table_buf.ptr,
                                table_buf.length)
        _check_stmt_error(cursor._handle, rc)
        cursor._prepare_result_set()
        return cursor

    def procedures(self, catalog=None, schema=None, proc=None):
        cdef:
            StringBuffer catalog_buf = StringBuffer()
            StringBuffer schema_buf = StringBuffer()
            StringBuffer proc_buf = StringBuffer()
            Cursor cursor
            SQLRETURN rc
        catalog_buf.set_value(catalog)
        schema_buf.set_value(schema)
        proc_buf.set_value(proc)
        self._check_connected()
        cursor = self.cursor()
        with nogil:
            rc = SQLProcedures(cursor._handle, catalog_buf.ptr,
                               catalog_buf.length, schema_buf.ptr,
                               schema_buf.length, proc_buf.ptr,
                               proc_buf.length)
        _check_stmt_error(cursor._handle, rc)
        cursor._prepare_result_set()
        return cursor

    def procedurecolumns(self, catalog=None, schema=None, proc=None,
                         column=None):
        cdef:
            StringBuffer catalog_buf = StringBuffer()
            StringBuffer schema_buf = StringBuffer()
            StringBuffer proc_buf = StringBuffer()
            StringBuffer column_buf = StringBuffer()
            Cursor cursor
            SQLRETURN rc
        catalog_buf.set_value(catalog)
        schema_buf.set_value(schema)
        proc_buf.set_value(proc)
        column_buf.set_value(column)
        self._check_connected()
        cursor = self.cursor()
        with nogil:
            rc = SQLProcedureColumns(cursor._handle, catalog_buf.ptr,
                                     catalog_buf.length, schema_buf.ptr,
                                     schema_buf.length, proc_buf.ptr,
                                     proc_buf.length, column_buf.ptr,
                                     column_buf.length)
        _check_stmt_error(cursor._handle, rc)
        cursor._prepare_result_set()
        return cursor

    def rollback(self):
        cdef SQLRETURN rc
        self._check_connected()
        with nogil:
            rc = SQLEndTran(SQL_HANDLE_DBC, self._handle, SQL_ROLLBACK)
        _check_conn_error(self._handle, rc)

    def tables(self, catalog=None, schema=None, table=None, type=None):
        cdef:
            StringBuffer catalog_buf = StringBuffer()
            StringBuffer schema_buf = StringBuffer()
            StringBuffer table_buf = StringBuffer()
            StringBuffer type_buf = StringBuffer()
            Cursor cursor
            SQLRETURN rc
        catalog_buf.set_value(catalog)
        schema_buf.set_value(schema)
        table_buf.set_value(table)
        type_buf.set_value(type)
        self._check_connected()
        cursor = self.cursor()
        with nogil:
            rc = SQLTables(cursor._handle, catalog_buf.ptr, catalog_buf.length,
                           schema_buf.ptr, schema_buf.length, table_buf.ptr,
                           table_buf.length, type_buf.ptr, type_buf.length)
        _check_stmt_error(cursor._handle, rc)
        cursor._prepare_result_set()
        return cursor

    def tableprivileges(self, catalog=None, schema=None, table=None):
        cdef:
            StringBuffer catalog_buf = StringBuffer()
            StringBuffer schema_buf = StringBuffer()
            StringBuffer table_buf = StringBuffer()
            Cursor cursor
            SQLRETURN rc
        catalog_buf.set_value(catalog)
        schema_buf.set_value(schema)
        table_buf.set_value(table)
        self._check_connected()
        cursor = self.cursor()
        with nogil:
            rc = SQLTablePrivileges(cursor._handle, catalog_buf.ptr,
                                    catalog_buf.length, schema_buf.ptr,
                                    schema_buf.length, table_buf.ptr,
                                    table_buf.length)
        _check_stmt_error(cursor._handle, rc)
        cursor._prepare_result_set()
        return cursor
