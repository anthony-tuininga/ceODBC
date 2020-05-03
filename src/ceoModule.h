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

// define macro for checking for errors
#define CheckForError(obj, rc, context) \
        Error_CheckForError((udt_ObjectWithHandle*) obj, rc, context)

// define macros for managing strings
#define ceString_FromAscii(str) \
    PyUnicode_DecodeASCII(str, strlen(str), NULL)
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

// define macro for defining objects with handles
#define ObjectWithHandle_HEAD \
    PyObject_HEAD \
    SQLSMALLINT handleType; \
    SQLHANDLE handle;


//-----------------------------------------------------------------------------
// Forward Declarations
//-----------------------------------------------------------------------------
typedef struct udt_ApiType udt_ApiType;
typedef struct udt_BinaryVar udt_BinaryVar;
typedef struct udt_BigIntegerVar udt_BigIntegerVar;
typedef struct udt_BitVar udt_BitVar;
typedef struct udt_Connection udt_Connection;
typedef struct udt_Cursor udt_Cursor;
typedef struct udt_DateVar udt_DateVar;
typedef struct ceoDbType ceoDbType;
typedef struct udt_DecimalVar udt_DecimalVar;
typedef struct udt_DoubleVar udt_DoubleVar;
typedef struct udt_Error udt_Error;
typedef struct udt_Environment udt_Environment;
typedef struct udt_IntegerVar udt_IntegerVar;
typedef struct udt_ObjectWithHandle udt_ObjectWithHandle;
typedef struct udt_StringBuffer udt_StringBuffer;
typedef struct udt_TimeVar udt_TimeVar;
typedef struct udt_TimestampVar udt_TimestampVar;
typedef struct udt_UnicodeVar udt_UnicodeVar;
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
extern udt_ApiType *g_BinaryApiType;
extern udt_ApiType *g_DateTimeApiType;
extern udt_ApiType *g_NumberApiType;
extern udt_ApiType *g_RowidApiType;
extern udt_ApiType *g_StringApiType;

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
extern PyTypeObject ceoPyTypeEnvironment;
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
struct udt_ApiType {
    PyObject_HEAD
    PyObject *name;
    PyObject *types;
};

struct udt_Connection {
    ObjectWithHandle_HEAD
    udt_Environment *environment;
    PyObject *inputTypeHandler;
    PyObject *outputTypeHandler;
    int isConnected;
    PyObject *dsn;
    int logSql;
};

struct udt_Cursor {
    ObjectWithHandle_HEAD
    udt_Connection *connection;
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

struct udt_Environment {
    ObjectWithHandle_HEAD
};

struct udt_Error {
    PyObject_HEAD
    PyObject *message;
    const char *context;
};

struct udt_ObjectWithHandle {
    ObjectWithHandle_HEAD
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
udt_ApiType *ApiType_New(PyObject *module, const char *name);

int Connection_IsConnected(udt_Connection *conn);

PyObject *Cursor_InternalCatalogHelper(udt_Cursor *cursor);
udt_Cursor *Cursor_InternalNew(udt_Connection *connection);

ceoDbType *ceoDbType_fromPythonType(PyTypeObject *type);
ceoDbType *ceoDbType_fromSqlDataType(SQLSMALLINT sqlDataType);
ceoDbType *ceoDbType_fromType(PyObject *type);
ceoDbType *ceoDbType_fromValue(PyObject *value, SQLUINTEGER *size);

udt_Environment *Environment_New(void);

int Error_CheckForError(udt_ObjectWithHandle *obj, SQLRETURN rcToCheck,
        const char *context);
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

PyObject *ceoUtils_formatString(const char *format, PyObject *args);
int ceoUtils_getModuleAndName(PyTypeObject *type, PyObject **module,
        PyObject **name);

int Variable_BindParameter(udt_Variable *self, udt_Cursor *cursor,
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
udt_Variable *Variable_NewByType(udt_Cursor *cursor, PyObject *value,
        unsigned numElements);
udt_Variable *Variable_NewByValue(udt_Cursor *cursor, PyObject *value,
        unsigned numElements);
udt_Variable *Variable_NewForResultSet(udt_Cursor *cursor,
        SQLUSMALLINT position);
PyObject *Variable_Repr(udt_Variable *var);
int Variable_Resize(udt_Variable *var, SQLUINTEGER newSize);
int Variable_SetValue(udt_Variable *self, unsigned arrayPos, PyObject *value);
