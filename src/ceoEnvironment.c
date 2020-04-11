//-----------------------------------------------------------------------------
// Environment.c
//   Environment handling.
//-----------------------------------------------------------------------------

#include "ceoModule.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
static void Environment_Free(udt_Environment*);

//-----------------------------------------------------------------------------
// declaration of the Python type
//-----------------------------------------------------------------------------
PyTypeObject ceoPyTypeEnvironment = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ceODBC.Environment",
    .tp_basicsize = sizeof(udt_Environment),
    .tp_dealloc = (destructor) Environment_Free,
    .tp_flags = Py_TPFLAGS_DEFAULT
};


//-----------------------------------------------------------------------------
// Environment_New()
//   Create a new environment object.
//-----------------------------------------------------------------------------
udt_Environment *Environment_New(void)
{
    udt_Environment *self;
    SQLRETURN rc;

    // create a new object
    self = PyObject_NEW(udt_Environment, &ceoPyTypeEnvironment);
    if (!self)
        return NULL;
    self->handleType = SQL_HANDLE_ENV;
    self->handle = SQL_NULL_HANDLE;

    // create the environment handle
    rc = SQLAllocHandle(self->handleType, SQL_NULL_HANDLE, &self->handle);
    if (rc != SQL_SUCCESS) {
        Py_DECREF(self);
        PyErr_SetString(PyExc_RuntimeError,
                "Unable to acquire environment handle");
        return NULL;
    }

    // set the attribute specifying which ODBC version to use
    rc = SQLSetEnvAttr(self->handle, SQL_ATTR_ODBC_VERSION,
            (SQLPOINTER) SQL_OV_ODBC3, 0);
    if (CheckForError(self, rc,
            "Environment_New(): set ODBC version attribute") < 0) {
        Py_DECREF(self);
        return NULL;
    }

    return self;
}


//-----------------------------------------------------------------------------
// Environment_Free()
//   Free the environment.
//-----------------------------------------------------------------------------
static void Environment_Free(udt_Environment *self)
{
    if (self->handle)
        SQLFreeHandle(self->handleType, self->handle);
    Py_TYPE(self)->tp_free((PyObject*) self);
}
