from libc.stdint cimport int8_t, int16_t, int32_t, int64_t
from libc.stdint cimport uint8_t, uint16_t, uint32_t, uint64_t

cdef extern from "<sql.h>":

    ctypedef unsigned char SQLCHAR
    ctypedef void* SQLHANDLE
    ctypedef void* SQLHWND
    ctypedef int SQLINTEGER
    ctypedef void* SQLPOINTER
    ctypedef signed short int SQLSMALLINT
    ctypedef unsigned short SQLUSMALLINT
    ctypedef unsigned long SQLULEN

    ctypedef SQLHANDLE SQLHENV
    ctypedef SQLHANDLE SQLHDBC
    ctypedef SQLHANDLE SQLHSTMT
    ctypedef SQLHANDLE SQLHDESC
    ctypedef SQLSMALLINT SQLRETURN

    # handle types
    SQLSMALLINT SQL_HANDLE_ENV
    SQLSMALLINT SQL_HANDLE_DBC
    SQLSMALLINT SQL_HANDLE_STMT

    # return codes
    SQLSMALLINT SQL_SUCCESS
    SQLSMALLINT SQL_SUCCESS_WITH_INFO
    SQLSMALLINT SQL_INVALID_HANDLE

    # diagnostic fields
    SQLSMALLINT SQL_DIAG_NUMBER
    SQLSMALLINT SQL_DIAG_MESSAGE_TEXT

    # other constants
    SQLUSMALLINT SQL_DRIVER_NOPROMPT

    SQLRETURN SQLAllocHandle(SQLSMALLINT HandleType, SQLHANDLE InputHandle,
                             SQLHANDLE *OutputHandle) nogil

    SQLRETURN SQLFreeHandle(SQLSMALLINT HandleType, SQLHANDLE Handle) nogil

    SQLRETURN SQLGetDiagFieldA(SQLSMALLINT HandleType, SQLHANDLE Handle,
                               SQLSMALLINT RecNumber,
                               SQLSMALLINT DiagIdentifier,
                               SQLPOINTER DiagInfo, SQLSMALLINT BufferLength,
                               SQLSMALLINT *StringLength) nogil

    SQLRETURN SQLSetConnectAttr(SQLHDBC ConnectionHandle,
                                SQLINTEGER Attribute, SQLPOINTER Value,
                                SQLINTEGER StringLength) nogil

    SQLRETURN SQLSetEnvAttr(SQLHENV EnvironmentHandle,
                            SQLINTEGER Attribute, SQLPOINTER Value,
                            SQLINTEGER StringLength) nogil


cdef extern from "<sqlext.h>":

    # attributes
    SQLINTEGER SQL_ATTR_ODBC_VERSION
    SQLINTEGER SQL_ATTR_AUTOCOMMIT

    # other constants
    SQLULEN SQL_OV_ODBC3
    SQLULEN SQL_AUTOCOMMIT_OFF
    SQLULEN SQL_AUTOCOMMIT_ON
    SQLINTEGER SQL_IS_UINTEGER

    SQLRETURN SQLDriverConnectA(SQLHDBC hdbc, SQLHWND hwnd,
                                SQLCHAR *szConnStrIn, SQLSMALLINT cbConnStrIn,
                                SQLCHAR *szConnStrOut,
                                SQLSMALLINT cbConnStrOutMax,
                                SQLSMALLINT *pcbConnStrOut,
                                SQLUSMALLINT fDriverCompletion) nogil
