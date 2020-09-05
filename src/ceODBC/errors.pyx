#------------------------------------------------------------------------------
# driver_conn.pyx
#   Cython file defining the error class used for all exceptions that are
# raised by the driver (embedded in driver.pyx).
#------------------------------------------------------------------------------

cdef class _Error:
    cdef readonly str message

    def __init__(self, message):
        self.message = message

    def __str__(self):
        return self.message


cdef inline int _check_error(SQLSMALLINT handle_type, SQLHANDLE handle,
                             SQLRETURN rc) except -1:
    if rc == SQL_SUCCESS or rc == SQL_SUCCESS_WITH_INFO:
        return 0
    elif rc == SQL_INVALID_HANDLE:
        _raise_from_string(exceptions.DatabaseError, "invalid handle")
    return _raise_from_odbc(handle_type, handle)


cdef inline int _check_conn_error(SQLHANDLE handle, SQLRETURN rc) except -1:
    return _check_error(SQL_HANDLE_DBC, handle, rc)


cdef inline int _check_stmt_error(SQLHANDLE handle, SQLRETURN rc) except -1:
    return _check_error(SQL_HANDLE_STMT, handle, rc)


cdef str _get_diag_record(SQLSMALLINT handle_type, SQLHANDLE handle,
                          SQLSMALLINT rec_num):
    cdef:
        SQLSMALLINT buf_length
        char buf[1024]
        SQLRETURN rc
    buf_length = sizeof(buf)
    rc = SQLGetDiagFieldA(handle_type, handle, rec_num, SQL_DIAG_MESSAGE_TEXT,
                          buf, buf_length, &buf_length)
    if rc != SQL_SUCCESS and rc != SQL_SUCCESS_WITH_INFO:
        _raise_from_string(exceptions.InternalError,
                           "cannot get diagnostic message text")
    if buf_length > <SQLSMALLINT> sizeof(buf) - 1:
        buf_length = <SQLSMALLINT> sizeof(buf) - 1
    return buf[:buf_length].decode()


cdef int _raise_from_odbc(SQLSMALLINT handle_type, SQLHANDLE handle) except -1:
    cdef:
        SQLINTEGER num_records, i
        SQLRETURN rc
        list messages
        str message
    rc = SQLGetDiagFieldA(handle_type, handle, 0, SQL_DIAG_NUMBER,
                         &num_records, 0, NULL)
    if rc != SQL_SUCCESS and rc != SQL_SUCCESS_WITH_INFO:
        _raise_from_string(exceptions.InternalError,
                           "cannot get number of diagnostic records")
    if num_records == 0:
        _raise_from_string(exceptions.InternalError,
                           "no diagnostic message text available")
    elif num_records == 1:
        message = _get_diag_record(handle_type, handle, 1)
        _raise_from_string(exceptions.DatabaseError, message)
    else:
        messages = cpython.PyList_New(num_records)
        for i in range(num_records):
            message = _get_diag_record(handle_type, handle, i + 1)
            cpython.Py_INCREF(message)
            cpython.PyList_SET_ITEM(messages, i, message)
        message = "\n".join(messages)
        _raise_from_string(exceptions.DatabaseError, message)


def _raise_from_string(exc_type, message):
    """
    Raises an exception from a given string. This ensures that an _Error object
    is created for all exceptions that are raised.
    """
    raise exc_type(_Error(message))
