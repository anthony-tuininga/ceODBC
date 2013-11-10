//-----------------------------------------------------------------------------
// Environment.c
//   Environment handling.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// structure for the Python type
//-----------------------------------------------------------------------------
typedef struct {
    ObjectWithHandle_HEAD
} udt_Environment;

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
static void Environment_Free(udt_Environment*);

//-----------------------------------------------------------------------------
// declaration of Python type
//-----------------------------------------------------------------------------
static PyTypeObject g_EnvironmentType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "ceODBC.Environment",               // tp_name
    sizeof(udt_Environment),            // tp_basicsize
    0,                                  // tp_itemsize
    (destructor) Environment_Free,      // tp_dealloc
    0,                                  // tp_print
    0,                                  // tp_getattr
    0,                                  // tp_setattr
    0,                                  // tp_compare
    0,                                  // tp_repr
    0,                                  // tp_as_number
    0,                                  // tp_as_sequence
    0,                                  // tp_as_mapping
    0,                                  // tp_hash
    0,                                  // tp_call
    0,                                  // tp_str
    0,                                  // tp_getattro
    0,                                  // tp_setattro
    0,                                  // tp_as_buffer
    Py_TPFLAGS_DEFAULT,                 // tp_flags
    0                                   // tp_doc
};


//-----------------------------------------------------------------------------
// Environment_New()
//   Create a new environment object.
//-----------------------------------------------------------------------------
static udt_Environment *Environment_New(void)
{
    udt_Environment *self;
    SQLRETURN rc;

    // create a new object
    self = PyObject_NEW(udt_Environment, &g_EnvironmentType);
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
static void Environment_Free(
    udt_Environment *self)              // environment object
{
    if (self->handle)
        SQLFreeHandle(self->handleType, self->handle);
    Py_TYPE(self)->tp_free((PyObject*) self);
}

