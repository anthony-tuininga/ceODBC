//-----------------------------------------------------------------------------
// Environment.c
//   Environment handling.
//-----------------------------------------------------------------------------

#include "ceoModule.h"

//-----------------------------------------------------------------------------
// ceoEnv_new()
//   Create a new environment object.
//-----------------------------------------------------------------------------
ceoEnv *ceoEnv_new(void)
{
    SQLRETURN rc;
    ceoEnv *env;

    // create a new object
    env = PyObject_NEW(ceoEnv, &ceoPyTypeEnvironment);
    if (!env)
        return NULL;
    env->handleType = SQL_HANDLE_ENV;
    env->handle = SQL_NULL_HANDLE;

    // create the environment handle
    rc = SQLAllocHandle(env->handleType, SQL_NULL_HANDLE, &env->handle);
    if (rc != SQL_SUCCESS) {
        Py_DECREF(env);
        PyErr_SetString(PyExc_RuntimeError,
                "Unable to acquire environment handle");
        return NULL;
    }

    // set the attribute specifying which ODBC version to use
    rc = SQLSetEnvAttr(env->handle, SQL_ATTR_ODBC_VERSION,
            (SQLPOINTER) SQL_OV_ODBC3, 0);
    if (CheckForError(env, rc,
            "ceoEnv_new(): set ODBC version attribute") < 0) {
        Py_DECREF(env);
        return NULL;
    }

    return env;
}


//-----------------------------------------------------------------------------
// ceoEnv_free()
//   Free the environment.
//-----------------------------------------------------------------------------
static void ceoEnv_free(ceoEnv *env)
{
    if (env->handle)
        SQLFreeHandle(env->handleType, env->handle);
    Py_TYPE(env)->tp_free((PyObject*) env);
}


//-----------------------------------------------------------------------------
// declaration of the Python type
//-----------------------------------------------------------------------------
PyTypeObject ceoPyTypeEnvironment = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ceODBC.Environment",
    .tp_basicsize = sizeof(ceoEnv),
    .tp_dealloc = (destructor) ceoEnv_free,
    .tp_flags = Py_TPFLAGS_DEFAULT
};
