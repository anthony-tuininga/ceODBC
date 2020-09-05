#------------------------------------------------------------------------------
# connection.pyx
#   Cython file defining the connection class (embedded in driver.pyx).
#------------------------------------------------------------------------------

cdef class Connection:
    cdef:
        SQLHANDLE _handle, _env_handle
        readonly str dsn

    def __dealloc__(self):
        if self._handle:
            SQLFreeHandle(SQL_HANDLE_DBC, self._handle)
        if self._env_handle:
            SQLFreeHandle(SQL_HANDLE_ENV, self._env_handle)

    @property
    def autocommit(self):
        return False

    @autocommit.setter
    def autocommit(self, value):
        pass

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
