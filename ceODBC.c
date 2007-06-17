//-----------------------------------------------------------------------------
// ceODBC.c
//   ODBC interface for Python.
//-----------------------------------------------------------------------------

#include <Python.h>
#include <structmember.h>
#include <datetime.h>
#include <time.h>
#ifdef WIN32
#include <windows.h>
#endif
#include <sql.h>
#include <sqlucode.h>

// define macro for adding type objects
#define ADD_TYPE_OBJECT(name, type) \
    Py_INCREF(type); \
    if (PyModule_AddObject(module, name, (PyObject*) type) < 0) \
        return;

// define macro to get the build version as a string
#define xstr(s)			str(s)
#define str(s)			#s
#define BUILD_VERSION_STRING	xstr(BUILD_VERSION)


//-----------------------------------------------------------------------------
// Globals
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


#include "Error.c"
#include "Environment.c"
#include "Connection.c"


//-----------------------------------------------------------------------------
// Time()
//   Returns a time value suitable for binding.
//-----------------------------------------------------------------------------
static PyObject* Time(PyObject* self, PyObject* args)
{
    PyErr_SetString(g_NotSupportedErrorException,
            "time only variables not supported");
    return NULL;
}


//-----------------------------------------------------------------------------
// TimeFromTicks()
//   Returns a time value suitable for binding.
//-----------------------------------------------------------------------------
static PyObject* TimeFromTicks(PyObject* self, PyObject* args)
{
    PyErr_SetString(g_NotSupportedErrorException,
            "time only variables not supported");
    return NULL;
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
    { "Time", Time, METH_VARARGS },
    { "DateFromTicks", DateFromTicks, METH_VARARGS },
    { "TimeFromTicks", TimeFromTicks, METH_VARARGS },
    { "TimestampFromTicks", TimestampFromTicks, METH_VARARGS },
    { NULL }
};

//-----------------------------------------------------------------------------
// initceODBC()
//   Initialization routine for the module.
//-----------------------------------------------------------------------------
void initceODBC(void)
{
    PyObject *module;

    PyDateTime_IMPORT;
    if (PyErr_Occurred())
        return;

    // prepare the types for use by the module
    if (PyType_Ready(&g_ConnectionType) < 0)
        return;
    if (PyType_Ready(&g_CursorType) < 0)
        return;
    if (PyType_Ready(&g_EnvironmentType) < 0)
        return;
    if (PyType_Ready(&g_ErrorType) < 0)
        return;
    if (PyType_Ready(&g_BitVarType) < 0)
        return;
    if (PyType_Ready(&g_DoubleVarType) < 0)
        return;
    if (PyType_Ready(&g_IntegerVarType) < 0)
        return;
    if (PyType_Ready(&g_TimestampVarType) < 0)
        return;
    if (PyType_Ready(&g_VarcharVarType) < 0)
        return;

    // initialize module
    module = Py_InitModule("ceODBC", g_ModuleMethods);
    if (!module)
        return;
    if (SetException(module, &g_WarningException,
            "Warning", PyExc_StandardError) < 0)
        return;
    if (SetException(module, &g_ErrorException,
            "Error", PyExc_StandardError) < 0)
        return;
    if (SetException(module, &g_InterfaceErrorException,
            "InterfaceError", g_ErrorException) < 0)
        return;
    if (SetException(module, &g_DatabaseErrorException,
            "DatabaseError", g_ErrorException) < 0)
        return;
    if (SetException(module, &g_DataErrorException,
            "DataError", g_DatabaseErrorException) < 0)
        return;
    if (SetException(module, &g_OperationalErrorException,
            "OperationalError", g_DatabaseErrorException) < 0)
        return;
    if (SetException(module, &g_IntegrityErrorException,
            "IntegrityError", g_DatabaseErrorException) < 0)
        return;
    if (SetException(module, &g_InternalErrorException,
            "InternalError", g_DatabaseErrorException) < 0)
        return;
    if (SetException(module, &g_ProgrammingErrorException,
            "ProgrammingError", g_DatabaseErrorException) < 0)
        return;
    if (SetException(module, &g_NotSupportedErrorException,
            "NotSupportedError", g_DatabaseErrorException) < 0)
        return;

    // set up the types that are available
    ADD_TYPE_OBJECT("Connection", &g_ConnectionType)
    ADD_TYPE_OBJECT("Cursor", &g_CursorType)
    ADD_TYPE_OBJECT("_Error", &g_ErrorType)

    // the name "connect" is required by the DB API
    ADD_TYPE_OBJECT("connect", &g_ConnectionType)

    // create the basic data types for setting input sizes
    ADD_TYPE_OBJECT("BIT", &g_BitVarType)
    ADD_TYPE_OBJECT("DOUBLE", &g_DoubleVarType)
    ADD_TYPE_OBJECT("INTEGER", &g_IntegerVarType)
    ADD_TYPE_OBJECT("TIMESTAMP", &g_TimestampVarType)
    ADD_TYPE_OBJECT("VARCHAR", &g_VarcharVarType)

    // create constants required by Python DB API 2.0
    if (PyModule_AddStringConstant(module, "apilevel", "2.0") < 0)
        return;
    if (PyModule_AddIntConstant(module, "threadsafety", 2) < 0)
        return;
    if (PyModule_AddStringConstant(module, "paramstyle", "qmark") < 0)
        return;

    // add version and build time for easier support
    if (PyModule_AddStringConstant(module, "version",
            BUILD_VERSION_STRING) < 0)
        return;
    if (PyModule_AddStringConstant(module, "buildtime",
            __DATE__ " " __TIME__) < 0)
        return;
}

