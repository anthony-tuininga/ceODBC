#------------------------------------------------------------------------------
# utils.pyx
#   Cython file defining utility functions (embedded in driver.pyx).
#------------------------------------------------------------------------------

def data_sources(exclude_user_dsn=False, exclude_system_dsn=False):
    """
    Returns a list of data sources the driver manager knows about.
    """
    cdef:
        SQLSMALLINT buffer_length_1, buffer_length_2
        SQLUSMALLINT direction
        SQLCHAR buffer_1[128]
        SQLCHAR buffer_2[512]
        SQLRETURN rc
    if exclude_user_dsn and exclude_system_dsn:
        return []
    elif exclude_user_dsn:
        direction = SQL_FETCH_FIRST_SYSTEM
    elif exclude_system_dsn:
        direction = SQL_FETCH_FIRST_USER
    else:
        direction = SQL_FETCH_FIRST
    data_sources = []
    while True:
        buffer_length_1 = sizeof(buffer_1)
        buffer_length_2 = sizeof(buffer_2)
        rc = SQLDataSources(global_env_handle, direction, buffer_1,
                buffer_length_1, &buffer_length_1, buffer_2,
                buffer_length_2, &buffer_length_2)
        if rc == SQL_NO_DATA:
            break
        _check_error(SQL_HANDLE_ENV, global_env_handle, rc)
        row = (buffer_1[:buffer_length_1].decode(),
               buffer_2[:buffer_length_2].decode())
        data_sources.append(row)
        direction = SQL_FETCH_NEXT
    return data_sources


def drivers():
    """
    Returns a list of drivers that the driver manager knows about.
    """
    cdef:
        SQLSMALLINT buffer_length_1, buffer_length_2
        SQLUSMALLINT direction = SQL_FETCH_FIRST
        SQLCHAR buffer_1[512]
        SQLRETURN rc
    drivers = []
    while True:
        buffer_length_1 = sizeof(buffer_1)
        rc = SQLDrivers(global_env_handle, direction, buffer_1,
                buffer_length_1, &buffer_length_1, NULL,
                0, &buffer_length_2)
        if rc == SQL_NO_DATA:
            break
        _check_error(SQL_HANDLE_ENV, global_env_handle, rc)
        drivers.append(buffer_1[:buffer_length_1].decode())
        direction = SQL_FETCH_NEXT
    return drivers


cdef SQLHANDLE init_env_handle() except NULL:
    """
    Initializes the environment handle used by all ODBC functions. This
    involves setting the desired ODBC version which, if not set, results in
    function sequence exceptions being raised!
    """
    cdef:
        SQLHANDLE env_handle
        SQLRETURN rc
    rc = SQLAllocHandle(SQL_HANDLE_ENV, NULL, &env_handle)
    if rc != SQL_SUCCESS:
        _raise_from_string(exceptions.InternalError,
                           "unable to allocate ODBC environment handle")
    rc = SQLSetEnvAttr(env_handle, SQL_ATTR_ODBC_VERSION,
                       <SQLPOINTER> SQL_OV_ODBC3, 0)
    _check_error(SQL_HANDLE_ENV, env_handle, rc)
    return env_handle
