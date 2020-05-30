//-----------------------------------------------------------------------------
// ceoModule.h
//   Include file for all ceODBC source files.
//-----------------------------------------------------------------------------

#define PY_SSIZE_T_CLEAN                1

#include <Python.h>
#include <structmember.h>
#include <time.h>
#ifdef MS_WINDOWS
#include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>
#include <sqlucode.h>
#include <time.h>

// set up CX_LOGGING if applicable
#ifdef WITH_CX_LOGGING
#include <cx_Logging.h>
#else
#define IsLoggingAtLevelForPython(...) 0
#define LogMessage(...)
#define LogMessageV(...)
#define WriteMessageForPython(...)
#endif

// define default size for STRING and BINARY values
#define CEO_DEFAULT_VAR_SIZE            255

// define default size for LONG string and binary values
#define CEO_DEFAULT_LONG_VAR_SIZE       128 * 1024

// define macro for determining the number of elements in an array
#define CEO_ARRAYSIZE(A) (sizeof(A)/sizeof((A)[0]))

// define macro for transforming ASCII strings
#define CEO_STR_FROM_ASCII(str) \
    PyUnicode_DecodeASCII(str, strlen(str), NULL)

// define macros for checking errors
#define CEO_CONN_CHECK_ERROR(conn, rc, context) \
    ceoError_check(SQL_HANDLE_DBC, conn->handle, rc, context)

#define CEO_CURSOR_CHECK_ERROR(cursor, rc, context) \
    ceoError_check(SQL_HANDLE_STMT, cursor->handle, rc, context)

// define macros for managing strings
#ifdef Py_UNICODE_WIDE
    #define ceString_FromStringAndSize(buffer, size) \
        PyUnicode_DecodeUTF16(buffer, (size) * 2, NULL, NULL)
    #define ceString_FromStringAndSizeInBytes(buffer, size) \
        PyUnicode_DecodeUTF16(buffer, (size), NULL, NULL)
#else
    #define ceString_FromStringAndSize(buffer, size) \
        PyUnicode_FromUnicode((Py_UNICODE*) (buffer), size)
    #define ceString_FromStringAndSizeInBytes(buffer, size) \
        PyUnicode_FromUnicode((Py_UNICODE*) (buffer), (size) / 2)
#endif


//-----------------------------------------------------------------------------
// Forward Declarations
//-----------------------------------------------------------------------------
typedef struct ceoApiType ceoApiType;
typedef struct ceoConnection ceoConnection;
typedef struct ceoCursor ceoCursor;
typedef struct ceoDbType ceoDbType;
typedef struct udt_Error udt_Error;
typedef struct udt_StringBuffer udt_StringBuffer;
typedef struct udt_Variable udt_Variable;


//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

// exceptions
extern PyObject *g_WarningException;
extern PyObject *g_ErrorException;
extern PyObject *g_InterfaceErrorException;
extern PyObject *g_DatabaseErrorException;
extern PyObject *g_DataErrorException;
extern PyObject *g_OperationalErrorException;
extern PyObject *g_IntegrityErrorException;
extern PyObject *g_InternalErrorException;
extern PyObject *g_ProgrammingErrorException;
extern PyObject *g_NotSupportedErrorException;

// API types
extern ceoApiType *g_BinaryApiType;
extern ceoApiType *g_DateTimeApiType;
extern ceoApiType *g_NumberApiType;
extern ceoApiType *g_RowidApiType;
extern ceoApiType *g_StringApiType;

// database types
extern ceoDbType *ceoDbTypeBigInt;
extern ceoDbType *ceoDbTypeBinary;
extern ceoDbType *ceoDbTypeBit;
extern ceoDbType *ceoDbTypeDate;
extern ceoDbType *ceoDbTypeDecimal;
extern ceoDbType *ceoDbTypeDouble;
extern ceoDbType *ceoDbTypeInt;
extern ceoDbType *ceoDbTypeLongBinary;
extern ceoDbType *ceoDbTypeLongString;
extern ceoDbType *ceoDbTypeString;
extern ceoDbType *ceoDbTypeTime;
extern ceoDbType *ceoDbTypeTimestamp;

// other module types
extern PyTypeObject ceoPyTypeApiType;
extern PyTypeObject ceoPyTypeConnection;
extern PyTypeObject ceoPyTypeCursor;
extern PyTypeObject ceoPyTypeDbType;
extern PyTypeObject ceoPyTypeError;
extern PyTypeObject ceoPyTypeVar;

// other Python types
extern PyTypeObject *g_DecimalType;
extern PyTypeObject *g_DateType;
extern PyTypeObject *g_DateTimeType;
extern PyTypeObject *g_TimeType;


//-----------------------------------------------------------------------------
// Function Types
//-----------------------------------------------------------------------------
typedef int (*SetValueProc)(udt_Variable*, unsigned, PyObject*);
typedef PyObject * (*GetValueProc)(udt_Variable*, unsigned);
typedef SQLUINTEGER (*GetBufferSizeProc)(udt_Variable*, SQLUINTEGER);


//-----------------------------------------------------------------------------
// Unions
//-----------------------------------------------------------------------------
typedef union {
    void *asRaw;
    SQLBIGINT *asBigInt;
    SQLCHAR *asBinary;
    unsigned char *asBit;
    DATE_STRUCT *asDate;
    double *asDouble;
    SQLINTEGER *asInt;
    SQLWCHAR *asString;
    TIME_STRUCT *asTime;
    TIMESTAMP_STRUCT *asTimestamp;
} ceoVarData;


//-----------------------------------------------------------------------------
// Structures
//-----------------------------------------------------------------------------
struct ceoApiType {
    PyObject_HEAD
    PyObject *name;
    PyObject *types;
};

struct ceoConnection {
    PyObject_HEAD
    SQLHANDLE handle;
    SQLHANDLE envHandle;
    PyObject *inputTypeHandler;
    PyObject *outputTypeHandler;
    int isConnected;
    PyObject *dsn;
    int logSql;
};

struct ceoCursor {
    PyObject_HEAD
    SQLHANDLE handle;
    ceoConnection *connection;
    PyObject *statement;
    PyObject *resultSetVars;
    PyObject *parameterVars;
    PyObject *rowFactory;
    PyObject *inputTypeHandler;
    PyObject *outputTypeHandler;
    int arraySize;
    int bindArraySize;
    SQLULEN fetchArraySize;
    int setInputSizes;
    int setOutputSize;
    int setOutputSizeColumn;
    SQLLEN rowCount;
    int actualRows;
    int rowNum;
    int logSql;
};

struct ceoDbType {
    PyObject_HEAD
    const char *name;
    SQLSMALLINT sqlDataType;
    SQLSMALLINT cDataType;
    SQLUINTEGER bufferSize;
    SQLUINTEGER bytesMultiplier;
};

struct udt_Error {
    PyObject_HEAD
    PyObject *message;
    const char *context;
};

struct udt_StringBuffer {
    const void *ptr;
    Py_ssize_t size;
    Py_ssize_t sizeInBytes;
    PyObject *encodedString;
};

struct udt_Variable {
    PyObject_HEAD
    SQLSMALLINT position;
    SQLINTEGER numElements;
    SQLLEN *lengthOrIndicator;
    ceoDbType *type;
    SQLUINTEGER size;
    SQLLEN bufferSize;
    SQLSMALLINT scale;
    int input;
    int output;
    PyObject *inConverter;
    PyObject *outConverter;
    ceoVarData data;
};


//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------
ceoApiType *ceoApiType_new(PyObject *module, const char *name);

int ceoConnection_isConnected(ceoConnection *conn);

PyObject *ceoCursor_internalCatalogHelper(ceoCursor *cursor);
ceoCursor *ceoCursor_internalNew(ceoConnection *connection);

ceoDbType *ceoDbType_fromPythonType(PyTypeObject *type);
ceoDbType *ceoDbType_fromSqlDataType(SQLSMALLINT sqlDataType);
ceoDbType *ceoDbType_fromType(PyObject *type);
ceoDbType *ceoDbType_fromValue(PyObject *value, SQLUINTEGER *size);

int ceoError_check(SQLSMALLINT handleType, SQLHANDLE handle,
        SQLRETURN rcToCheck, const char *context);
int ceoError_raiseFromString(PyObject *exceptionType, const char *message,
        const char *context);

void StringBuffer_Clear(udt_StringBuffer *buf);
int StringBuffer_FromBinary(udt_StringBuffer *buf, PyObject *obj);
int StringBuffer_FromString(udt_StringBuffer *buf, PyObject *obj,
        const char *message);
int StringBuffer_FromUnicode(udt_StringBuffer *buf, PyObject *obj);

PyObject *ceoTransform_dateFromSqlValue(DATE_STRUCT *sqlValue);
PyObject *ceoTransform_dateFromTicks(PyObject *args);
int ceoTransform_init(void);
int ceoTransform_sqlValueFromDate(PyObject *pyValue, DATE_STRUCT *sqlValue);
int ceoTransform_sqlValueFromTime(PyObject *pyValue, TIME_STRUCT *sqlValue);
int ceoTransform_sqlValueFromTimestamp(PyObject *pyValue,
        TIMESTAMP_STRUCT *sqlValue);
PyObject *ceoTransform_timeFromSqlValue(TIME_STRUCT *sqlValue);
PyObject *ceoTransform_timeFromTicks(PyObject* args);
PyObject *ceoTransform_timestatmpFromSqlValue(TIMESTAMP_STRUCT *sqlValue);
PyObject *ceoTransform_timestampFromTicks(PyObject *args);

int ceoUtils_findInString(PyObject *strObj, char *stringToFind,
        int startPos, int *foundPos);
PyObject *ceoUtils_formatString(const char *format, PyObject *args);
int ceoUtils_getModuleAndName(PyTypeObject *type, PyObject **module,
        PyObject **name);

int Variable_BindParameter(udt_Variable *self, ceoCursor *cursor,
        SQLUSMALLINT position);
int Variable_DefaultInit(udt_Variable *var, PyObject *args,
        PyObject *keywordArgs);
void Variable_Free(udt_Variable *var);
PyObject *Variable_GetValue(udt_Variable *self, unsigned arrayPos);
int Variable_InitWithScale(udt_Variable *self, PyObject *args,
        PyObject *keywordArgs);
int Variable_InitWithSize(udt_Variable *var, PyObject *args,
        PyObject *keywordArgs);
udt_Variable *Variable_InternalNew(unsigned numElements,
        ceoDbType *type, SQLUINTEGER size, SQLSMALLINT scale);
PyObject *Variable_New(PyTypeObject *type, PyObject *args,
        PyObject *keywordArgs);
udt_Variable *Variable_NewByType(ceoCursor *cursor, PyObject *value,
        unsigned numElements);
udt_Variable *Variable_NewByValue(ceoCursor *cursor, PyObject *value,
        unsigned numElements);
udt_Variable *Variable_NewForResultSet(ceoCursor *cursor,
        SQLUSMALLINT position);
PyObject *Variable_Repr(udt_Variable *var);
int Variable_Resize(udt_Variable *var, SQLUINTEGER newSize);
int Variable_SetValue(udt_Variable *self, unsigned arrayPos, PyObject *value);
