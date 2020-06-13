//-----------------------------------------------------------------------------
// ceoModule.c
//   Implementation of ceODBC, an ODBC interface for Python.
//-----------------------------------------------------------------------------

#include "ceoModule.h"

// define macro for adding type objects
#define CREATE_API_TYPE(apiTypeObject, name) \
    apiTypeObject = ceoApiType_new(module, name); \
    if (!apiTypeObject) \
        return NULL;

// define macro for adding database types
#define CEO_ADD_DB_TYPE(typeObj, name, sqlDataType, cDataType, bufferSize, \
        bytesMultiplier) \
    if (ceoModule_addDbType(module, typeObj, name, sqlDataType, cDataType, \
            bufferSize, bytesMultiplier) < 0) \
        return NULL;

#define REGISTER_TYPE(apiTypeObject, dbType) \
    if (PyList_Append(apiTypeObject->types, (PyObject*) dbType) < 0) \
        return NULL;

#define ADD_TYPE_OBJECT(name, type) \
    Py_INCREF(type); \
    if (PyModule_AddObject(module, name, (PyObject*) type) < 0) \
        return NULL;

// define macro to get the build version as a string
#define xstr(s)			str(s)
#define str(s)			#s
#define BUILD_VERSION_STRING	xstr(BUILD_VERSION)

// define macros for building convenience
#define MAKE_TYPE_READY(type) \
    if (PyType_Ready(type) < 0) \
        return NULL;


//-----------------------------------------------------------------------------
// Exception classes
//-----------------------------------------------------------------------------
PyObject *ceoExceptionWarning = NULL;
PyObject *ceoExceptionError = NULL;
PyObject *ceoExceptionInterfaceError = NULL;
PyObject *ceoExceptionDatabaseError = NULL;
PyObject *ceoExceptionDataError = NULL;
PyObject *ceoExceptionOperationalError = NULL;
PyObject *ceoExceptionIntegrityError = NULL;
PyObject *ceoExceptionInternalError = NULL;
PyObject *ceoExceptionProgrammingError = NULL;
PyObject *ceoExceptionNotSupportedError = NULL;


//-----------------------------------------------------------------------------
// globally referenced classes
//-----------------------------------------------------------------------------
PyTypeObject *ceoPyTypeDecimal = NULL;
PyTypeObject *ceoPyTypeDate = NULL;
PyTypeObject *ceoPyTypeDateTime = NULL;
PyTypeObject *ceoPyTypeTime = NULL;


//-----------------------------------------------------------------------------
// API types
//-----------------------------------------------------------------------------
ceoApiType *ceoApiTypeBinary = NULL;
ceoApiType *ceoApiTypeDateTime = NULL;
ceoApiType *ceoApiTypeNumber = NULL;
ceoApiType *ceoApiTypeRowid = NULL;
ceoApiType *ceoApiTypeString = NULL;


//-----------------------------------------------------------------------------
// Database types
//-----------------------------------------------------------------------------
ceoDbType *ceoDbTypeBigInt = NULL;
ceoDbType *ceoDbTypeBinary = NULL;
ceoDbType *ceoDbTypeBit = NULL;
ceoDbType *ceoDbTypeDate = NULL;
ceoDbType *ceoDbTypeDecimal = NULL;
ceoDbType *ceoDbTypeDouble = NULL;
ceoDbType *ceoDbTypeInt = NULL;
ceoDbType *ceoDbTypeLongBinary = NULL;
ceoDbType *ceoDbTypeLongString = NULL;
ceoDbType *ceoDbTypeString = NULL;
ceoDbType *ceoDbTypeTime = NULL;
ceoDbType *ceoDbTypeTimestamp = NULL;


//-----------------------------------------------------------------------------
// ceoModule_dateFromTicks()
//   Returns a date value suitable for binding.
//-----------------------------------------------------------------------------
static PyObject* ceoModule_dateFromTicks(PyObject* module, PyObject* args)
{
    return ceoTransform_dateFromTicks(args);
}


//-----------------------------------------------------------------------------
// ceoModule_addDbType()
//   Create a database type and add it to the module.
//-----------------------------------------------------------------------------
static int ceoModule_addDbType(PyObject *module, ceoDbType **dbType,
        const char *name, SQLSMALLINT sqlDataType, SQLSMALLINT cDataType,
        SQLUINTEGER bufferSize, SQLUINTEGER bytesMultiplier)
{
    ceoDbType *tempDbType;

    tempDbType = (ceoDbType*) ceoPyTypeDbType.tp_alloc(&ceoPyTypeDbType, 0);
    if (!tempDbType)
        return -1;
    tempDbType->name = name;
    tempDbType->sqlDataType = sqlDataType;
    tempDbType->cDataType = cDataType;
    tempDbType->bufferSize = bufferSize;
    tempDbType->bytesMultiplier = bytesMultiplier;
    if (PyModule_AddObject(module, name, (PyObject*) tempDbType) < 0) {
        Py_DECREF(tempDbType);
        return -1;
    }
    *dbType = tempDbType;
    return 0;
}


//-----------------------------------------------------------------------------
// ceoModule_setException()
//   Create an exception and set it in the provided dictionary.
//-----------------------------------------------------------------------------
static int ceoModule_setException(PyObject *module, PyObject **exception,
        char *name, PyObject *baseException)
{
    char buffer[100];

    sprintf(buffer, "ceODBC.%s", name);
    *exception = PyErr_NewException(buffer, baseException, NULL);
    if (!*exception)
        return -1;
    return PyModule_AddObject(module, name, *exception);
}


//-----------------------------------------------------------------------------
// ceoModule_timeFromTicks()
//   Returns a time value suitable for binding.
//-----------------------------------------------------------------------------
static PyObject* ceoModule_timeFromTicks(PyObject* module, PyObject* args)
{
    return ceoTransform_timeFromTicks(args);
}


//-----------------------------------------------------------------------------
// ceoModule_timestampFromTicks()
//   Returns a date value suitable for binding.
//-----------------------------------------------------------------------------
static PyObject* ceoModule_timestampFromTicks(PyObject* module, PyObject* args)
{
    return ceoTransform_timestampFromTicks(args);
}


//-----------------------------------------------------------------------------
//   Declaration of methods supported by this module
//-----------------------------------------------------------------------------
static PyMethodDef ceoMethods[] = {
    { "DateFromTicks", ceoModule_dateFromTicks, METH_VARARGS },
    { "TimeFromTicks", ceoModule_timeFromTicks, METH_VARARGS },
    { "TimestampFromTicks", ceoModule_timestampFromTicks, METH_VARARGS },
    { NULL }
};


//-----------------------------------------------------------------------------
//   Declaration of module definition
//-----------------------------------------------------------------------------
static struct PyModuleDef ceoModuleDef = {
    PyModuleDef_HEAD_INIT,
    .m_name = "ceODBC",
    .m_size = -1,
    .m_methods = ceoMethods
};


//-----------------------------------------------------------------------------
// Start routine for the module.
//-----------------------------------------------------------------------------
PyMODINIT_FUNC PyInit_ceODBC(void)
{
    PyObject *module;

    LogMessage(LOG_LEVEL_DEBUG, "ceODBC initializing");

    // initialize transforms
    if (ceoTransform_init() < 0)
        return NULL;

    // prepare the types for use by the module
    MAKE_TYPE_READY(&ceoPyTypeApiType)
    MAKE_TYPE_READY(&ceoPyTypeConnection)
    MAKE_TYPE_READY(&ceoPyTypeCursor)
    MAKE_TYPE_READY(&ceoPyTypeDbType)
    MAKE_TYPE_READY(&ceoPyTypeError)
    MAKE_TYPE_READY(&ceoPyTypeVar)

    // initialize module
    module = PyModule_Create(&ceoModuleDef);
    if (!module)
        return NULL;

    // add exceptions
    if (ceoModule_setException(module, &ceoExceptionWarning, "Warning",
            NULL) < 0)
        return NULL;
    if (ceoModule_setException(module, &ceoExceptionError, "Error", NULL) < 0)
        return NULL;
    if (ceoModule_setException(module, &ceoExceptionInterfaceError,
            "InterfaceError", ceoExceptionError) < 0)
        return NULL;
    if (ceoModule_setException(module, &ceoExceptionDatabaseError,
            "DatabaseError", ceoExceptionError) < 0)
        return NULL;
    if (ceoModule_setException(module, &ceoExceptionDataError,
            "DataError", ceoExceptionDatabaseError) < 0)
        return NULL;
    if (ceoModule_setException(module, &ceoExceptionOperationalError,
            "OperationalError", ceoExceptionDatabaseError) < 0)
        return NULL;
    if (ceoModule_setException(module, &ceoExceptionIntegrityError,
            "IntegrityError", ceoExceptionDatabaseError) < 0)
        return NULL;
    if (ceoModule_setException(module, &ceoExceptionInternalError,
            "InternalError", ceoExceptionDatabaseError) < 0)
        return NULL;
    if (ceoModule_setException(module, &ceoExceptionProgrammingError,
            "ProgrammingError", ceoExceptionDatabaseError) < 0)
        return NULL;
    if (ceoModule_setException(module, &ceoExceptionNotSupportedError,
            "NotSupportedError", ceoExceptionDatabaseError) < 0)
        return NULL;

    // add the base types
    ADD_TYPE_OBJECT("Connection", &ceoPyTypeConnection)
    ADD_TYPE_OBJECT("Cursor", &ceoPyTypeCursor)
    ADD_TYPE_OBJECT("_ApiType", &ceoPyTypeApiType)
    ADD_TYPE_OBJECT("_Error", &ceoPyTypeError)

    // the name "connect" is required by the DB API
    ADD_TYPE_OBJECT("connect", &ceoPyTypeConnection)

    // add the constructors required by the DB API
    ADD_TYPE_OBJECT("Binary", &PyBytes_Type)
    ADD_TYPE_OBJECT("Date", ceoPyTypeDate)
    ADD_TYPE_OBJECT("Time", ceoPyTypeTime)
    ADD_TYPE_OBJECT("Timestamp", ceoPyTypeDateTime)

    // add the API types required by the DB API
    CREATE_API_TYPE(ceoApiTypeBinary, "BINARY")
    CREATE_API_TYPE(ceoApiTypeDateTime, "DATETIME")
    CREATE_API_TYPE(ceoApiTypeNumber, "NUMBER")
    CREATE_API_TYPE(ceoApiTypeRowid, "ROWID")
    CREATE_API_TYPE(ceoApiTypeString, "STRING")

    // add the database types
    CEO_ADD_DB_TYPE(&ceoDbTypeBigInt, "DB_TYPE_BIGINT", SQL_BIGINT,
            SQL_C_SBIGINT, sizeof(SQLBIGINT), 0)
    CEO_ADD_DB_TYPE(&ceoDbTypeBinary, "DB_TYPE_BINARY", SQL_VARBINARY,
            SQL_C_BINARY, 0, 1)
    CEO_ADD_DB_TYPE(&ceoDbTypeBit, "DB_TYPE_BIT", SQL_BIT, SQL_C_BIT,
            sizeof(unsigned char), 0)
    CEO_ADD_DB_TYPE(&ceoDbTypeDate, "DB_TYPE_DATE", SQL_TYPE_DATE,
            SQL_C_TYPE_DATE, sizeof(DATE_STRUCT), 0)
    CEO_ADD_DB_TYPE(&ceoDbTypeDecimal, "DB_TYPE_DECIMAL", SQL_CHAR,
            SQL_C_CHAR, 40, 0)
    CEO_ADD_DB_TYPE(&ceoDbTypeDouble, "DB_TYPE_DOUBLE", SQL_DOUBLE,
            SQL_C_DOUBLE, sizeof(SQLDOUBLE), 0)
    CEO_ADD_DB_TYPE(&ceoDbTypeInt, "DB_TYPE_INT", SQL_INTEGER, SQL_C_LONG,
            sizeof(SQLINTEGER), 0)
    CEO_ADD_DB_TYPE(&ceoDbTypeLongBinary, "DB_TYPE_LONG_BINARY",
            SQL_LONGVARBINARY, SQL_C_BINARY, 0, 1)
    CEO_ADD_DB_TYPE(&ceoDbTypeLongString, "DB_TYPE_LONG_STRING",
            SQL_LONGVARCHAR, SQL_C_CHAR, 0, 4)
    CEO_ADD_DB_TYPE(&ceoDbTypeString, "DB_TYPE_STRING", SQL_VARCHAR,
            SQL_C_CHAR, 0, 4)
    CEO_ADD_DB_TYPE(&ceoDbTypeTime, "DB_TYPE_TIME", SQL_TYPE_TIME,
            SQL_C_TYPE_TIME, sizeof(TIME_STRUCT), 0)
    CEO_ADD_DB_TYPE(&ceoDbTypeTimestamp, "DB_TYPE_TIMESTAMP",
            SQL_TYPE_TIMESTAMP, SQL_C_TYPE_TIMESTAMP, sizeof(TIMESTAMP_STRUCT),
            0)

    // register the variable types with the API types
    REGISTER_TYPE(ceoApiTypeBinary, ceoDbTypeBinary)
    REGISTER_TYPE(ceoApiTypeBinary, ceoDbTypeLongBinary)
    REGISTER_TYPE(ceoApiTypeDateTime, ceoDbTypeDate)
    REGISTER_TYPE(ceoApiTypeDateTime, ceoDbTypeTimestamp)
    REGISTER_TYPE(ceoApiTypeNumber, ceoDbTypeBigInt)
    REGISTER_TYPE(ceoApiTypeNumber, ceoDbTypeDecimal)
    REGISTER_TYPE(ceoApiTypeNumber, ceoDbTypeDouble)
    REGISTER_TYPE(ceoApiTypeNumber, ceoDbTypeInt)
    REGISTER_TYPE(ceoApiTypeString, ceoDbTypeString)
    REGISTER_TYPE(ceoApiTypeString, ceoDbTypeLongString)

    // create synonyms for backwards compatibility
    ADD_TYPE_OBJECT("BigIntegerVar", ceoDbTypeBigInt)
    ADD_TYPE_OBJECT("BinaryVar", ceoDbTypeBinary)
    ADD_TYPE_OBJECT("BitVar", ceoDbTypeBit)
    ADD_TYPE_OBJECT("DateVar", ceoDbTypeDate)
    ADD_TYPE_OBJECT("DecimalVar", ceoDbTypeDecimal)
    ADD_TYPE_OBJECT("DoubleVar", ceoDbTypeDouble)
    ADD_TYPE_OBJECT("IntegerVar", ceoDbTypeInt)
    ADD_TYPE_OBJECT("LongBinaryVar", ceoDbTypeLongBinary)
    ADD_TYPE_OBJECT("LongStringVar", ceoDbTypeLongString)
    ADD_TYPE_OBJECT("StringVar", ceoDbTypeString)
    ADD_TYPE_OBJECT("TimeVar", ceoDbTypeTime)
    ADD_TYPE_OBJECT("TimestampVar", ceoDbTypeTimestamp)

    // create constants required by Python DB API 2.0
    if (PyModule_AddStringConstant(module, "apilevel", "2.0") < 0)
        return NULL;
    if (PyModule_AddIntConstant(module, "threadsafety", 2) < 0)
        return NULL;
    if (PyModule_AddStringConstant(module, "paramstyle", "qmark") < 0)
        return NULL;

    // add version and build time for easier support
    if (PyModule_AddStringConstant(module, "version",
            BUILD_VERSION_STRING) < 0)
        return NULL;
    if (PyModule_AddStringConstant(module, "buildtime",
            __DATE__ " " __TIME__) < 0)
        return NULL;

    LogMessage(LOG_LEVEL_DEBUG, "ceODBC initialization complete");

    return module;
}
