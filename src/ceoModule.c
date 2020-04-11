//-----------------------------------------------------------------------------
// ceoModule.c
//   Implementation of ceODBC, an ODBC interface for Python.
//-----------------------------------------------------------------------------

#include "ceoModule.h"

// define macro for adding type objects
#define CREATE_API_TYPE(apiTypeObject, name) \
    apiTypeObject = ApiType_New(module, name); \
    if (!apiTypeObject) \
        return NULL;

// define macro for adding database types
#define CEO_ADD_DB_TYPE(transformNum, name, typeObj) \
    if (ceoModule_addDbType(module, transformNum, name, typeObj) < 0) \
        return NULL;

#define REGISTER_TYPE(apiTypeObject, dbType) \
    if (PyList_Append(apiTypeObject->types, dbType) < 0) \
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
PyObject *g_WarningException = NULL;
PyObject *g_ErrorException = NULL;
PyObject *g_InterfaceErrorException = NULL;
PyObject *g_DatabaseErrorException = NULL;
PyObject *g_DataErrorException = NULL;
PyObject *g_OperationalErrorException = NULL;
PyObject *g_IntegrityErrorException = NULL;
PyObject *g_InternalErrorException = NULL;
PyObject *g_ProgrammingErrorException = NULL;
PyObject *g_NotSupportedErrorException = NULL;


//-----------------------------------------------------------------------------
// globally referenced classes
//-----------------------------------------------------------------------------
PyTypeObject *g_DecimalType = NULL;
PyTypeObject *g_DateType = NULL;
PyTypeObject *g_DateTimeType = NULL;
PyTypeObject *g_TimeType = NULL;


//-----------------------------------------------------------------------------
// API types
//-----------------------------------------------------------------------------
udt_ApiType *g_BinaryApiType = NULL;
udt_ApiType *g_DateTimeApiType = NULL;
udt_ApiType *g_NumberApiType = NULL;
udt_ApiType *g_RowidApiType = NULL;
udt_ApiType *g_StringApiType = NULL;


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
static PyObject* ceoModule_dateFromTicks(PyObject* self, PyObject* args)
{
    return ceoTransform_dateFromTicks(args);
}


//-----------------------------------------------------------------------------
// ceoModule_addDbType()
//   Create a database type and add it to the module.
//-----------------------------------------------------------------------------
static int ceoModule_addDbType(PyObject *module, ceoTransformNum transformNum,
        const char *name, ceoDbType **dbType)
{
    ceoDbType *tempDbType;

    tempDbType = (ceoDbType*) ceoPyTypeDbType.tp_alloc(&ceoPyTypeDbType, 0);
    if (!tempDbType)
        return -1;
    tempDbType->transformNum = transformNum;
    tempDbType->name = name;
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
static PyObject* ceoModule_timeFromTicks(PyObject* self, PyObject* args)
{
    return ceoTransform_timeFromTicks(args);
}


//-----------------------------------------------------------------------------
// ceoModule_timestampFromTicks()
//   Returns a date value suitable for binding.
//-----------------------------------------------------------------------------
static PyObject* ceoModule_timestampFromTicks(PyObject* self, PyObject* args)
{
    return ceoTransform_timestampFromTicks(args);
}


//-----------------------------------------------------------------------------
//   Declaration of methods supported by this module
//-----------------------------------------------------------------------------
static PyMethodDef g_ModuleMethods[] = {
    { "DateFromTicks", ceoModule_dateFromTicks, METH_VARARGS },
    { "TimeFromTicks", ceoModule_timeFromTicks, METH_VARARGS },
    { "TimestampFromTicks", ceoModule_timestampFromTicks, METH_VARARGS },
    { NULL }
};


//-----------------------------------------------------------------------------
//   Declaration of module definition
//-----------------------------------------------------------------------------
static struct PyModuleDef g_ModuleDef = {
    PyModuleDef_HEAD_INIT,
    "ceODBC",
    NULL,
    -1,
    g_ModuleMethods,                       // methods
    NULL,                                  // m_reload
    NULL,                                  // traverse
    NULL,                                  // clear
    NULL                                   // free
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
    MAKE_TYPE_READY(&ceoPyTypeConnection)
    MAKE_TYPE_READY(&ceoPyTypeCursor)
    MAKE_TYPE_READY(&ceoPyTypeEnvironment)
    MAKE_TYPE_READY(&ceoPyTypeError)
    MAKE_TYPE_READY(&ceoPyTypeApiType)
    MAKE_TYPE_READY(&ceoPyTypeDbType)
    MAKE_TYPE_READY(&g_BigIntegerVarType)
    MAKE_TYPE_READY(&g_BinaryVarType)
    MAKE_TYPE_READY(&g_BitVarType)
    MAKE_TYPE_READY(&g_DateVarType)
    MAKE_TYPE_READY(&g_DecimalVarType)
    MAKE_TYPE_READY(&g_DoubleVarType)
    MAKE_TYPE_READY(&g_IntegerVarType)
    MAKE_TYPE_READY(&g_LongBinaryVarType)
    MAKE_TYPE_READY(&g_LongUnicodeVarType)
    MAKE_TYPE_READY(&g_TimeVarType)
    MAKE_TYPE_READY(&g_TimestampVarType)
    MAKE_TYPE_READY(&g_UnicodeVarType)

    // initialize module
    module = PyModule_Create(&g_ModuleDef);
    if (!module)
        return NULL;

    // add exceptions
    if (ceoModule_setException(module, &g_WarningException, "Warning",
            NULL) < 0)
        return NULL;
    if (ceoModule_setException(module, &g_ErrorException, "Error", NULL) < 0)
        return NULL;
    if (ceoModule_setException(module, &g_InterfaceErrorException,
            "InterfaceError", g_ErrorException) < 0)
        return NULL;
    if (ceoModule_setException(module, &g_DatabaseErrorException,
            "DatabaseError", g_ErrorException) < 0)
        return NULL;
    if (ceoModule_setException(module, &g_DataErrorException,
            "DataError", g_DatabaseErrorException) < 0)
        return NULL;
    if (ceoModule_setException(module, &g_OperationalErrorException,
            "OperationalError", g_DatabaseErrorException) < 0)
        return NULL;
    if (ceoModule_setException(module, &g_IntegrityErrorException,
            "IntegrityError", g_DatabaseErrorException) < 0)
        return NULL;
    if (ceoModule_setException(module, &g_InternalErrorException,
            "InternalError", g_DatabaseErrorException) < 0)
        return NULL;
    if (ceoModule_setException(module, &g_ProgrammingErrorException,
            "ProgrammingError", g_DatabaseErrorException) < 0)
        return NULL;
    if (ceoModule_setException(module, &g_NotSupportedErrorException,
            "NotSupportedError", g_DatabaseErrorException) < 0)
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
    ADD_TYPE_OBJECT("Date", g_DateType)
    ADD_TYPE_OBJECT("Time", g_TimeType)
    ADD_TYPE_OBJECT("Timestamp", g_DateTimeType)

    // add the variable types
    ADD_TYPE_OBJECT("BigIntegerVar", &g_BigIntegerVarType)
    ADD_TYPE_OBJECT("BinaryVar", &g_BinaryVarType)
    ADD_TYPE_OBJECT("BitVar", &g_BitVarType)
    ADD_TYPE_OBJECT("DateVar", &g_DateVarType)
    ADD_TYPE_OBJECT("DecimalVar", &g_DecimalVarType)
    ADD_TYPE_OBJECT("DoubleVar", &g_DoubleVarType)
    ADD_TYPE_OBJECT("IntegerVar", &g_IntegerVarType)
    ADD_TYPE_OBJECT("LongBinaryVar", &g_LongBinaryVarType)
    ADD_TYPE_OBJECT("LongStringVar", &g_LongUnicodeVarType)
    ADD_TYPE_OBJECT("StringVar", &g_UnicodeVarType)
    ADD_TYPE_OBJECT("TimeVar", &g_TimeVarType)
    ADD_TYPE_OBJECT("TimestampVar", &g_TimestampVarType)

    // add the API types required by the DB API
    CREATE_API_TYPE(g_BinaryApiType, "BINARY")
    CREATE_API_TYPE(g_DateTimeApiType, "DATETIME")
    CREATE_API_TYPE(g_NumberApiType, "NUMBER")
    CREATE_API_TYPE(g_RowidApiType, "ROWID")
    CREATE_API_TYPE(g_StringApiType, "STRING")

    // add the database types
    CEO_ADD_DB_TYPE(CEO_TRANSFORM_BIGINT, "DB_TYPE_BIGINT", &ceoDbTypeBigInt)
    CEO_ADD_DB_TYPE(CEO_TRANSFORM_BINARY, "DB_TYPE_BINARY", &ceoDbTypeBinary)
    CEO_ADD_DB_TYPE(CEO_TRANSFORM_BIT, "DB_TYPE_BIT", &ceoDbTypeBit)
    CEO_ADD_DB_TYPE(CEO_TRANSFORM_DATE, "DB_TYPE_DATE", &ceoDbTypeDate)
    CEO_ADD_DB_TYPE(CEO_TRANSFORM_DECIMAL, "DB_TYPE_DECIMAL",
            &ceoDbTypeDecimal)
    CEO_ADD_DB_TYPE(CEO_TRANSFORM_DOUBLE, "DB_TYPE_DOUBLE", &ceoDbTypeDouble)
    CEO_ADD_DB_TYPE(CEO_TRANSFORM_INT, "DB_TYPE_INT", &ceoDbTypeInt)
    CEO_ADD_DB_TYPE(CEO_TRANSFORM_LONG_BINARY, "DB_TYPE_LONG_BINARY",
            &ceoDbTypeLongBinary)
    CEO_ADD_DB_TYPE(CEO_TRANSFORM_LONG_STRING, "DB_TYPE_LONG_STRING",
            &ceoDbTypeLongString)
    CEO_ADD_DB_TYPE(CEO_TRANSFORM_STRING, "DB_TYPE_STRING", &ceoDbTypeString)
    CEO_ADD_DB_TYPE(CEO_TRANSFORM_TIME, "DB_TYPE_TIME", &ceoDbTypeTime)
    CEO_ADD_DB_TYPE(CEO_TRANSFORM_TIMESTAMP, "DB_TYPE_TIMESTAMP",
            &ceoDbTypeTimestamp)

    // register the variable types with the API types
    REGISTER_TYPE(g_BinaryApiType, ceoDbTypeBinary)
    REGISTER_TYPE(g_BinaryApiType, ceoDbTypeLongBinary)
    REGISTER_TYPE(g_DateTimeApiType, ceoDbTypeDate)
    REGISTER_TYPE(g_DateTimeApiType, ceoDbTypeTimestamp)
    REGISTER_TYPE(g_NumberApiType, ceoDbTypeBigInt)
    REGISTER_TYPE(g_NumberApiType, ceoDbTypeDecimal)
    REGISTER_TYPE(g_NumberApiType, ceoDbTypeDouble)
    REGISTER_TYPE(g_NumberApiType, ceoDbTypeInt)
    REGISTER_TYPE(g_StringApiType, ceoDbTypeString)
    REGISTER_TYPE(g_StringApiType, ceoDbTypeLongString)

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
