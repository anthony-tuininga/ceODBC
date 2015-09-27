//-----------------------------------------------------------------------------
// Error.c
//   Error handling.
//-----------------------------------------------------------------------------

#define CheckForError(obj, rc, context) \
        Error_CheckForError((udt_ObjectWithHandle*) obj, rc, context)

#define ObjectWithHandle_HEAD \
    PyObject_HEAD \
    SQLSMALLINT handleType; \
    SQLHANDLE handle;

typedef struct {
    ObjectWithHandle_HEAD
} udt_ObjectWithHandle;

//-----------------------------------------------------------------------------
// structure for the Python type
//-----------------------------------------------------------------------------
typedef struct {
    PyObject_HEAD
    PyObject *message;
    const char *context;
} udt_Error;


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
static void Error_Free(udt_Error*);
static PyObject *Error_Str(udt_Error*);


//-----------------------------------------------------------------------------
// declaration of members
//-----------------------------------------------------------------------------
static PyMemberDef g_ErrorMembers[] = {
    { "message", T_OBJECT, offsetof(udt_Error, message), READONLY },
    { "context", T_STRING, offsetof(udt_Error, context), READONLY },
    { NULL }
};


//-----------------------------------------------------------------------------
// declaration of Python type
//-----------------------------------------------------------------------------
static PyTypeObject g_ErrorType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "ceODBC._Error",                    // tp_name
    sizeof(udt_Error),                  // tp_basicsize
    0,                                  // tp_itemsize
    (destructor) Error_Free,            // tp_dealloc
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
    (reprfunc) Error_Str,               // tp_str
    0,                                  // tp_getattro
    0,                                  // tp_setattro
    0,                                  // tp_as_buffer
    Py_TPFLAGS_DEFAULT,                 // tp_flags
    0,                                  // tp_doc
    0,                                  // tp_traverse
    0,                                  // tp_clear
    0,                                  // tp_richcompare
    0,                                  // tp_weaklistoffset
    0,                                  // tp_iter
    0,                                  // tp_iternext
    0,                                  // tp_methods
    g_ErrorMembers,                     // tp_members
    0                                   // tp_getset
};


//-----------------------------------------------------------------------------
// Error_Free()
//   Deallocate the environment, disconnecting from the database if necessary.
//-----------------------------------------------------------------------------
static void Error_Free(
    udt_Error *self)                    // error object
{
    Py_CLEAR(self->message);
    Py_TYPE(self)->tp_free((PyObject*) self);
}


//-----------------------------------------------------------------------------
// Error_Str()
//   Return a string representation of the error variable.
//-----------------------------------------------------------------------------
static PyObject *Error_Str(
    udt_Error *self)                    // variable to return the string for
{
    if (self->message) {
        Py_INCREF(self->message);
        return self->message;
    }
    return ceString_FromAscii("");
}


//-----------------------------------------------------------------------------
// Error_CheckForError()
//   Check for an error in the last call and if an error has occurred, raise a
// Python exception.
//-----------------------------------------------------------------------------
static int Error_CheckForError(
    udt_ObjectWithHandle *obj,          // object to check for errors on
    SQLRETURN rcToCheck,                // return code of last call
    const char *context)                // context
{
    PyObject *errorMessages, *temp, *separator;
    CEODBC_CHAR buffer[1024];
    SQLINTEGER numRecords;
    SQLSMALLINT length;
    udt_Error *error;
    SQLRETURN rc;
    int i;

    // handle simple cases
    if (rcToCheck == SQL_SUCCESS || rcToCheck == SQL_SUCCESS_WITH_INFO)
        return 0;
    if (rcToCheck == SQL_INVALID_HANDLE) {
        PyErr_SetString(g_DatabaseErrorException, "Invalid handle!");
        return -1;
    }

    // create new error object
    error = PyObject_NEW(udt_Error, &g_ErrorType);
    if (!error)
        return -1;
    error->context = context;

    // determine number of diagnostic records available
    rc = SQLGetDiagField(obj->handleType, obj->handle, 0, SQL_DIAG_NUMBER,
            &numRecords, SQL_IS_INTEGER, NULL);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        error->message = ceString_FromAscii("cannot get number of " \
                "diagnostic records");

    // determine error text
    } else if (numRecords == 0) {
        error->message = ceString_FromAscii("no diagnostic message text " \
                "available");
    } else {
        error->message = NULL;
        errorMessages = PyList_New(numRecords);
        if (!errorMessages) {
            Py_DECREF(error);
            return -1;
        }
        for (i = 1; i <= numRecords; i++) {
            rc = SQLGetDiagField(obj->handleType, obj->handle, i,
                    SQL_DIAG_MESSAGE_TEXT, buffer, sizeof(buffer), &length);
            if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
                error->message = ceString_FromAscii("cannot get " \
                        "diagnostic message text");
                break;
            }
            temp = ceString_FromStringAndSize( (char*) buffer, length);
            if (!temp) {
                Py_DECREF(error);
                Py_DECREF(errorMessages);
                return -1;
            }
            PyList_SET_ITEM(errorMessages, i - 1, temp);
        }
        if (!error->message) {
            separator = ceString_FromAscii("\n");
            if (!separator) {
                Py_DECREF(error);
                Py_DECREF(errorMessages);
                return -1;
            }
            error->message = ceString_Join(separator, errorMessages);
            Py_DECREF(separator);
            Py_DECREF(errorMessages);
        }
    }

    if (!error->message) {
        Py_DECREF(error);
        return -1;
    }

    PyErr_SetObject(g_DatabaseErrorException, (PyObject*) error);
    Py_DECREF(error);
    return -1;
}

