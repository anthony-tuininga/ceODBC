//-----------------------------------------------------------------------------
// ceODBC.c
//   ODBC interface for Python.
//-----------------------------------------------------------------------------

#include <Python.h>
#include <structmember.h>
#include <datetime.h>
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

// define Py_ssize_t for versions before Python 2.5
#if PY_VERSION_HEX < 0x02050000
typedef int Py_ssize_t;
#define PY_SSIZE_T_MAX INT_MAX
#define PY_SSIZE_T_MIN INT_MIN
#endif

// define PyVarObject_HEAD_INIT for versions before Python 2.6
#ifndef PyVarObject_HEAD_INIT
#define PyVarObject_HEAD_INIT(type, size) \
    PyObject_HEAD_INIT(type) size,
#endif

// define Py_TYPE for versions before Python 2.6
#ifndef Py_TYPE
#define Py_TYPE(ob)             (((PyObject*)(ob))->ob_type)
#endif

// define PyInt_* macros for Python 3.x
#ifndef PyInt_Check
#define PyInt_Check             PyLong_Check
#define PyInt_FromLong          PyLong_FromLong
#define PyInt_AsLong            PyLong_AsLong
#define PyInt_Type              PyLong_Type
#endif

// define macro for adding type objects
#define CREATE_API_TYPE(apiTypeObject, name) \
    apiTypeObject = ApiType_New(module, name); \
    if (!apiTypeObject) \
        return NULL;

#define REGISTER_TYPE(apiTypeObject, type) \
    if (PyList_Append(apiTypeObject->types, (PyObject*) type) < 0) \
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
#if PY_MAJOR_VERSION >= 3
    #define CEODBC_BASE_EXCEPTION       NULL
#else
    #define CEODBC_BASE_EXCEPTION       PyExc_StandardError
#endif

// define simple construct for determining endianness of the platform
#define IS_LITTLE_ENDIAN (int)*(unsigned char*) &one

// use Unicode variants for Python 3.x
#if PY_MAJOR_VERSION >= 3
#define SQLDescribeCol              SQLDescribeColW
#define SQLDriverConnect            SQLDriverConnectW
#define SQLGetDiagField             SQLGetDiagFieldW
#define SQLPrepare                  SQLPrepareW
#define SQLSetCursorName            SQLSetCursorNameW
#endif


//-----------------------------------------------------------------------------
// Exception classes
//-----------------------------------------------------------------------------
static PyObject *g_WarningException = NULL;
static PyObject *g_ErrorException = NULL;
static PyObject *g_InterfaceErrorException = NULL;
static PyObject *g_DatabaseErrorException = NULL;
static PyObject *g_DataErrorException = NULL;
static PyObject *g_OperationalErrorException = NULL;
static PyObject *g_IntegrityErrorException = NULL;
static PyObject *g_InternalErrorException = NULL;
static PyObject *g_ProgrammingErrorException = NULL;
static PyObject *g_NotSupportedErrorException = NULL;


//-----------------------------------------------------------------------------
// globally referenced classes
//-----------------------------------------------------------------------------
static PyObject *g_DecimalType = NULL;

#include "StringUtils.c"

//-----------------------------------------------------------------------------
// GetModuleAndName()
//   Return the module and name for the type.
//-----------------------------------------------------------------------------
static int GetModuleAndName(
    PyTypeObject *type,                 // type to get module/name for
    PyObject **module,                  // name of module
    PyObject **name)                    // name of type
{
    *module = PyObject_GetAttrString( (PyObject*) type, "__module__");
    if (!*module)
        return -1;
    *name = PyObject_GetAttrString( (PyObject*) type, "__name__");
    if (!*name) {
        Py_DECREF(*module);
        return -1;
    }
    return 0;
}

#include "ApiTypes.c"

//-----------------------------------------------------------------------------
// API types
//-----------------------------------------------------------------------------
static udt_ApiType *g_BinaryApiType = NULL;
static udt_ApiType *g_DateTimeApiType = NULL;
static udt_ApiType *g_NumberApiType = NULL;
static udt_ApiType *g_RowidApiType = NULL;
static udt_ApiType *g_StringApiType = NULL;


//-----------------------------------------------------------------------------
// SetException()
//   Create an exception and set it in the provided dictionary.
//-----------------------------------------------------------------------------
static int SetException(
    PyObject *module,                   // module object
    PyObject **exception,               // exception to create
    char *name,                         // name of the exception
    PyObject *baseException)            // exception to base exception on
{
    char buffer[100];

    sprintf(buffer, "ceODBC.%s", name);
    *exception = PyErr_NewException(buffer, baseException, NULL);
    if (!*exception)
        return -1;
    return PyModule_AddObject(module, name, *exception);
}


#include "Error.c"
#include "Environment.c"
#include "Connection.c"


//-----------------------------------------------------------------------------
// TimeFromTicks()
//   Returns a time value suitable for binding.
//-----------------------------------------------------------------------------
static PyObject* TimeFromTicks(PyObject* self, PyObject* args)
{
    double inputTicks;
    struct tm *time;
#ifdef WIN32
    __time64_t ticks;
#else
    time_t ticks;
#endif

    if (!PyArg_ParseTuple(args, "d", &inputTicks))
        return NULL;
    ticks = (long) inputTicks;
#ifdef WIN32
    time = _localtime64(&ticks);
#else
    time = localtime(&ticks);
#endif
    return PyTime_FromTime(time->tm_hour, time->tm_min, time->tm_sec, 0);
}


//-----------------------------------------------------------------------------
// DateFromTicks()
//   Returns a date value suitable for binding.
//-----------------------------------------------------------------------------
static PyObject* DateFromTicks(PyObject* self, PyObject* args)
{
    return PyDate_FromTimestamp(args);
}


//-----------------------------------------------------------------------------
// TimestampFromTicks()
//   Returns a date value suitable for binding.
//-----------------------------------------------------------------------------
static PyObject* TimestampFromTicks(PyObject* self, PyObject* args)
{
    return PyDateTime_FromTimestamp(args);
}


//-----------------------------------------------------------------------------
//   Declaration of methods supported by this module
//-----------------------------------------------------------------------------
static PyMethodDef g_ModuleMethods[] = {
    { "DateFromTicks", DateFromTicks, METH_VARARGS },
    { "TimeFromTicks", TimeFromTicks, METH_VARARGS },
    { "TimestampFromTicks", TimestampFromTicks, METH_VARARGS },
    { NULL }
};


#if PY_MAJOR_VERSION >= 3
//-----------------------------------------------------------------------------
//   Declaration of module definition for Python 3.x.
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
#endif


//-----------------------------------------------------------------------------
// Module_Initialize()
//   Initialization routine for the module.
//-----------------------------------------------------------------------------
static PyObject *Module_Initialize(void)
{
    PyObject *module;

    LogMessage(LOG_LEVEL_DEBUG, "ceODBC initializing");

    // import the datetime module
    PyDateTime_IMPORT;
    if (PyErr_Occurred())
        return NULL;

    // import the decimal module
    module = PyImport_ImportModule("decimal");
    if (!module)
        return NULL;
    g_DecimalType = PyObject_GetAttrString(module, "Decimal");
    Py_DECREF(module);
    if (!g_DecimalType)
        return NULL;

    // prepare the types for use by the module
    MAKE_TYPE_READY(&g_ConnectionType)
    MAKE_TYPE_READY(&g_CursorType)
    MAKE_TYPE_READY(&g_EnvironmentType)
    MAKE_TYPE_READY(&g_ErrorType)
    MAKE_TYPE_READY(&g_ApiTypeType)
    MAKE_TYPE_READY(&g_BigIntegerVarType)
    MAKE_TYPE_READY(&g_BinaryVarType)
    MAKE_TYPE_READY(&g_BitVarType)
    MAKE_TYPE_READY(&g_DateVarType)
    MAKE_TYPE_READY(&g_DecimalVarType)
    MAKE_TYPE_READY(&g_DoubleVarType)
    MAKE_TYPE_READY(&g_IntegerVarType)
    MAKE_TYPE_READY(&g_LongBinaryVarType)
#if PY_MAJOR_VERSION < 3
    MAKE_TYPE_READY(&g_LongStringVarType)
    MAKE_TYPE_READY(&g_StringVarType)
#endif
    MAKE_TYPE_READY(&g_LongUnicodeVarType)
    MAKE_TYPE_READY(&g_TimeVarType)
    MAKE_TYPE_READY(&g_TimestampVarType)
    MAKE_TYPE_READY(&g_UnicodeVarType)

    // initialize module
#if PY_MAJOR_VERSION >= 3
    module = PyModule_Create(&g_ModuleDef);
#else
    module = Py_InitModule("ceODBC", g_ModuleMethods);
#endif
    if (!module)
        return NULL;

    // add exceptions
    if (SetException(module, &g_WarningException,
            "Warning", CEODBC_BASE_EXCEPTION) < 0)
        return NULL;
    if (SetException(module, &g_ErrorException,
            "Error", CEODBC_BASE_EXCEPTION) < 0)
        return NULL;
    if (SetException(module, &g_InterfaceErrorException,
            "InterfaceError", g_ErrorException) < 0)
        return NULL;
    if (SetException(module, &g_DatabaseErrorException,
            "DatabaseError", g_ErrorException) < 0)
        return NULL;
    if (SetException(module, &g_DataErrorException,
            "DataError", g_DatabaseErrorException) < 0)
        return NULL;
    if (SetException(module, &g_OperationalErrorException,
            "OperationalError", g_DatabaseErrorException) < 0)
        return NULL;
    if (SetException(module, &g_IntegrityErrorException,
            "IntegrityError", g_DatabaseErrorException) < 0)
        return NULL;
    if (SetException(module, &g_InternalErrorException,
            "InternalError", g_DatabaseErrorException) < 0)
        return NULL;
    if (SetException(module, &g_ProgrammingErrorException,
            "ProgrammingError", g_DatabaseErrorException) < 0)
        return NULL;
    if (SetException(module, &g_NotSupportedErrorException,
            "NotSupportedError", g_DatabaseErrorException) < 0)
        return NULL;

    // add the base types
    ADD_TYPE_OBJECT("Connection", &g_ConnectionType)
    ADD_TYPE_OBJECT("Cursor", &g_CursorType)
    ADD_TYPE_OBJECT("_ApiType", &g_ApiTypeType)
    ADD_TYPE_OBJECT("_Error", &g_ErrorType)

    // the name "connect" is required by the DB API
    ADD_TYPE_OBJECT("connect", &g_ConnectionType)

    // add the constructors required by the DB API
    ADD_TYPE_OBJECT("Binary", &ceBinary_Type)
    ADD_TYPE_OBJECT("Date", PyDateTimeAPI->DateType)
    ADD_TYPE_OBJECT("Time", PyDateTimeAPI->TimeType)
    ADD_TYPE_OBJECT("Timestamp", PyDateTimeAPI->DateTimeType)

    // add the variable types
    ADD_TYPE_OBJECT("BigIntegerVar", &g_BigIntegerVarType)
    ADD_TYPE_OBJECT("BinaryVar", &g_BinaryVarType)
    ADD_TYPE_OBJECT("BitVar", &g_BitVarType)
    ADD_TYPE_OBJECT("DateVar", &g_DateVarType)
    ADD_TYPE_OBJECT("DecimalVar", &g_DecimalVarType)
    ADD_TYPE_OBJECT("DoubleVar", &g_DoubleVarType)
    ADD_TYPE_OBJECT("IntegerVar", &g_IntegerVarType)
    ADD_TYPE_OBJECT("LongBinaryVar", &g_LongBinaryVarType)
#if PY_MAJOR_VERSION < 3
    ADD_TYPE_OBJECT("LongStringVar", &g_LongStringVarType)
    ADD_TYPE_OBJECT("StringVar", &g_StringVarType)
    ADD_TYPE_OBJECT("LongUnicodeVar", &g_LongUnicodeVarType)
    ADD_TYPE_OBJECT("UnicodeVar", &g_UnicodeVarType)
#else
    ADD_TYPE_OBJECT("LongStringVar", &g_LongUnicodeVarType)
    ADD_TYPE_OBJECT("StringVar", &g_UnicodeVarType)
#endif
    ADD_TYPE_OBJECT("TimeVar", &g_TimeVarType)
    ADD_TYPE_OBJECT("TimestampVar", &g_TimestampVarType)

    // add the API types required by the DB API
    CREATE_API_TYPE(g_BinaryApiType, "BINARY")
    CREATE_API_TYPE(g_DateTimeApiType, "DATETIME")
    CREATE_API_TYPE(g_NumberApiType, "NUMBER")
    CREATE_API_TYPE(g_RowidApiType, "ROWID")
    CREATE_API_TYPE(g_StringApiType, "STRING")

    // register the variable types with the API types
    REGISTER_TYPE(g_BinaryApiType, &g_BinaryVarType)
    REGISTER_TYPE(g_BinaryApiType, &g_LongBinaryVarType)
    REGISTER_TYPE(g_DateTimeApiType, &g_TimestampVarType)
    REGISTER_TYPE(g_DateTimeApiType, &g_DateVarType)
    REGISTER_TYPE(g_NumberApiType, &g_BigIntegerVarType)
    REGISTER_TYPE(g_NumberApiType, &g_DecimalVarType)
    REGISTER_TYPE(g_NumberApiType, &g_DoubleVarType)
    REGISTER_TYPE(g_NumberApiType, &g_IntegerVarType)
#if PY_MAJOR_VERSION < 3
    REGISTER_TYPE(g_StringApiType, &g_StringVarType)
    REGISTER_TYPE(g_StringApiType, &g_LongStringVarType)
#else
    REGISTER_TYPE(g_StringApiType, &g_UnicodeVarType)
    REGISTER_TYPE(g_StringApiType, &g_LongUnicodeVarType)
#endif

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


//-----------------------------------------------------------------------------
// Start routine for the module.
//-----------------------------------------------------------------------------
#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC PyInit_ceODBC(void)
{
    return Module_Initialize();
}
#else
void initceODBC(void)
{
    Module_Initialize();
}
#endif

