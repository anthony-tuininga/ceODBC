//-----------------------------------------------------------------------------
// Error.c
//   Error handling.
//-----------------------------------------------------------------------------

#include "ceoModule.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
static void Error_Free(udt_Error*);
static PyObject *Error_Str(udt_Error*);


//-----------------------------------------------------------------------------
// declaration of members for Python type
//-----------------------------------------------------------------------------
static PyMemberDef ceoMembers[] = {
    { "message", T_OBJECT, offsetof(udt_Error, message), READONLY },
    { "context", T_STRING, offsetof(udt_Error, context), READONLY },
    { NULL }
};


//-----------------------------------------------------------------------------
// declaration of Python type
//-----------------------------------------------------------------------------
PyTypeObject ceoPyTypeError = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ceODBC._Error",
    .tp_basicsize = sizeof(udt_Error),
    .tp_dealloc = (destructor) Error_Free,
    .tp_str = (reprfunc) Error_Str,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_members = ceoMembers
};


//-----------------------------------------------------------------------------
// Error_Free()
//   Deallocate the environment, disconnecting from the database if necessary.
//-----------------------------------------------------------------------------
static void Error_Free(udt_Error *self)
{
    Py_CLEAR(self->message);
    Py_TYPE(self)->tp_free((PyObject*) self);
}


//-----------------------------------------------------------------------------
// Error_Str()
//   Return a string representation of the error variable.
//-----------------------------------------------------------------------------
static PyObject *Error_Str(udt_Error *self)
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
int Error_CheckForError(udt_ObjectWithHandle *obj, SQLRETURN rcToCheck,
        const char *context)
{
    PyObject *errorMessages, *temp, *separator;
    SQLWCHAR buffer[1024];
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
    error = PyObject_NEW(udt_Error, &ceoPyTypeError);
    if (!error)
        return -1;
    error->context = context;

    // determine number of diagnostic records available
    rc = SQLGetDiagFieldW(obj->handleType, obj->handle, 0, SQL_DIAG_NUMBER,
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
            rc = SQLGetDiagFieldW(obj->handleType, obj->handle, i,
                    SQL_DIAG_MESSAGE_TEXT, buffer, sizeof(buffer), &length);
            if (length > sizeof(buffer) - sizeof(SQLWCHAR))
                length = sizeof(buffer) - sizeof(SQLWCHAR);
            if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
                error->message = ceString_FromAscii("cannot get " \
                        "diagnostic message text");
                break;
            }
            temp = ceString_FromStringAndSizeInBytes(buffer, length);
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
            error->message = PyUnicode_Join(separator, errorMessages);
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


//-----------------------------------------------------------------------------
// ceoError_raiseFromString()
//   Internal method for raising an exception given a string. Returns -1 as a
// convenience to the caller.
//-----------------------------------------------------------------------------
int ceoError_raiseFromString(PyObject *exceptionType, const char *message,
        const char *context)
{
    udt_Error *error;

    error = (udt_Error*) ceoPyTypeError.tp_alloc(&ceoPyTypeError, 0);
    if (!error)
        return -1;
    error->context = context;
    error->message = PyUnicode_DecodeASCII(message, strlen(message), NULL);
    if (!error->message) {
        Py_DECREF(error);
        return -1;
    }
    PyErr_SetObject(exceptionType, (PyObject*) error);
    Py_DECREF(error);
    return -1;
}
