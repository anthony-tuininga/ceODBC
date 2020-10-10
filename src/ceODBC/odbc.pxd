#------------------------------------------------------------------------------
# odbc.pxd
#   Cython file defining the ODBC interface (embedded in driver.pyx).
#------------------------------------------------------------------------------

cdef extern from "<sql.h>":

    # basic types
    ctypedef unsigned char SQLCHAR
    ctypedef void* SQLHANDLE
    ctypedef void* SQLHWND
    ctypedef void* SQLPOINTER
    ctypedef int SQLINTEGER
    ctypedef unsigned int SQLUINTEGER
    ctypedef signed short int SQLSMALLINT
    ctypedef unsigned short SQLUSMALLINT
    ctypedef long SQLLEN
    ctypedef unsigned long SQLULEN
    ctypedef long long int SQLBIGINT

    # structures
    ctypedef struct DATE_STRUCT:
        SQLSMALLINT year
        SQLUSMALLINT month
        SQLUSMALLINT day

    ctypedef struct TIME_STRUCT:
        SQLUSMALLINT hour
        SQLUSMALLINT minute
        SQLUSMALLINT second

    ctypedef struct TIMESTAMP_STRUCT:
        SQLSMALLINT year
        SQLUSMALLINT month
        SQLUSMALLINT day
        SQLUSMALLINT hour
        SQLUSMALLINT minute
        SQLUSMALLINT second
        SQLUINTEGER fraction

    # synonyms
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
    SQLSMALLINT SQL_NO_DATA

    # diagnostic fields
    SQLSMALLINT SQL_DIAG_NUMBER
    SQLSMALLINT SQL_DIAG_MESSAGE_TEXT

    # data types
    SQLSMALLINT SQL_BIGINT
    SQLSMALLINT SQL_BIT
    SQLSMALLINT SQL_CHAR
    SQLSMALLINT SQL_DECIMAL
    SQLSMALLINT SQL_DOUBLE
    SQLSMALLINT SQL_FLOAT
    SQLSMALLINT SQL_INTEGER
    SQLSMALLINT SQL_LONGVARBINARY
    SQLSMALLINT SQL_LONGVARCHAR
    SQLSMALLINT SQL_NUMERIC
    SQLSMALLINT SQL_SMALLINT
    SQLSMALLINT SQL_TINYINT
    SQLSMALLINT SQL_TYPE_DATE
    SQLSMALLINT SQL_TYPE_TIME
    SQLSMALLINT SQL_TYPE_TIMESTAMP
    SQLSMALLINT SQL_VARBINARY
    SQLSMALLINT SQL_VARCHAR

    # C data types
    SQLSMALLINT SQL_C_BINARY
    SQLSMALLINT SQL_C_BIT
    SQLSMALLINT SQL_C_CHAR
    SQLSMALLINT SQL_C_DOUBLE
    SQLSMALLINT SQL_C_LONG
    SQLSMALLINT SQL_C_SBIGINT
    SQLSMALLINT SQL_C_TYPE_DATE
    SQLSMALLINT SQL_C_TYPE_TIME
    SQLSMALLINT SQL_C_TYPE_TIMESTAMP

    # other constants
    SQLSMALLINT SQL_COMMIT
    SQLSMALLINT SQL_ROLLBACK
    SQLUSMALLINT SQL_DRIVER_NOPROMPT
    SQLLEN SQL_NULL_DATA
    SQLSMALLINT SQL_NO_NULLS

    SQLRETURN SQLAllocHandle(SQLSMALLINT HandleType, SQLHANDLE InputHandle,
                             SQLHANDLE *OutputHandle) nogil

    SQLRETURN SQLBindCol(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber,
                         SQLSMALLINT TargetType, SQLPOINTER TargetValue,
                         SQLLEN BufferLength, SQLLEN *StrLen_or_Ind) nogil

    SQLRETURN SQLCloseCursor(SQLHSTMT StatementHandle) nogil

    SQLRETURN SQLDescribeColA(SQLHSTMT hstmt, SQLUSMALLINT icol,
                              SQLCHAR *szColName, SQLSMALLINT cbColNameMax,
                              SQLSMALLINT *pcbColName, SQLSMALLINT *pfSqlType,
                              SQLULEN *pcbColDef, SQLSMALLINT *pibScale,
                              SQLSMALLINT *pfNullable) nogil

    SQLRETURN SQLDisconnect(SQLHDBC ConnectionHandle) nogil

    SQLRETURN SQLEndTran(SQLSMALLINT HandleType, SQLHANDLE Handle,
                         SQLSMALLINT CompletionType) nogil

    SQLRETURN SQLExecute(SQLHSTMT StatementHandle) nogil

    SQLRETURN SQLFetch(SQLHSTMT StatementHandle) nogil

    SQLRETURN SQLFreeHandle(SQLSMALLINT HandleType, SQLHANDLE Handle) nogil

    SQLRETURN SQLGetDiagFieldA(SQLSMALLINT HandleType, SQLHANDLE Handle,
                               SQLSMALLINT RecNumber,
                               SQLSMALLINT DiagIdentifier,
                               SQLPOINTER DiagInfo, SQLSMALLINT BufferLength,
                               SQLSMALLINT *StringLength) nogil

    SQLRETURN SQLNumResultCols(SQLHSTMT StatementHandle,
                               SQLSMALLINT *ColumnCount) nogil

    SQLRETURN SQLPrepareA(SQLHSTMT StatementHandle, SQLCHAR *StatementText,
                          SQLINTEGER TextLength) nogil

    SQLRETURN SQLRowCount(SQLHSTMT StatementHandle, SQLLEN *RowCount) nogil

    SQLRETURN SQLSetConnectAttr(SQLHDBC ConnectionHandle,
                                SQLINTEGER Attribute, SQLPOINTER Value,
                                SQLINTEGER StringLength) nogil

    SQLRETURN SQLSetEnvAttr(SQLHENV EnvironmentHandle,
                            SQLINTEGER Attribute, SQLPOINTER Value,
                            SQLINTEGER StringLength) nogil

    SQLRETURN SQLSetStmtAttr(SQLHSTMT StatementHandle, SQLINTEGER Attribute,
                             SQLPOINTER Value, SQLINTEGER StringLength) nogil


cdef extern from "<sqlext.h>":

    # attributes
    SQLINTEGER SQL_ATTR_ODBC_VERSION
    SQLINTEGER SQL_ATTR_AUTOCOMMIT
    SQLINTEGER SQL_ATTR_ROWS_FETCHED_PTR
    SQLINTEGER SQL_ATTR_ROW_ARRAY_SIZE

    # bind directions
    SQLSMALLINT SQL_PARAM_INPUT
    SQLSMALLINT SQL_PARAM_INPUT_OUTPUT
    SQLSMALLINT SQL_PARAM_OUTPUT

    # other constants
    SQLULEN SQL_OV_ODBC3
    SQLULEN SQL_AUTOCOMMIT_OFF
    SQLULEN SQL_AUTOCOMMIT_ON
    SQLINTEGER SQL_IS_POINTER
    SQLINTEGER SQL_IS_UINTEGER

    SQLRETURN SQLBindParameter(SQLHSTMT hstmt, SQLUSMALLINT ipar,
                               SQLSMALLINT fParamType, SQLSMALLINT fCType,
                               SQLSMALLINT fSqlType, SQLULEN cbColDef,
                               SQLSMALLINT ibScale, SQLPOINTER rgbValue,
                               SQLLEN cbValueMax, SQLLEN *pcbValue) nogil

    SQLRETURN SQLDriverConnectA(SQLHDBC hdbc, SQLHWND hwnd,
                                SQLCHAR *szConnStrIn, SQLSMALLINT cbConnStrIn,
                                SQLCHAR *szConnStrOut,
                                SQLSMALLINT cbConnStrOutMax,
                                SQLSMALLINT *pcbConnStrOut,
                                SQLUSMALLINT fDriverCompletion) nogil
