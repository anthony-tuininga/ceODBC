//-----------------------------------------------------------------------------
// Error.c
//   Error handling.
//-----------------------------------------------------------------------------

#include "ceoModule.h"

//-----------------------------------------------------------------------------
// ceoError_free()
//   Deallocation routine.
//-----------------------------------------------------------------------------
static void ceoError_free(ceoError *self)
{
    Py_CLEAR(self->message);
    Py_TYPE(self)->tp_free((PyObject*) self);
}


//-----------------------------------------------------------------------------
// ceoError_str()
//   Return a string representation of the error variable.
//-----------------------------------------------------------------------------
static PyObject *ceoError_str(ceoError *self)
{
    if (self->message) {
        Py_INCREF(self->message);
        return self->message;
    }
    return CEO_STR_FROM_ASCII("");
}


//-----------------------------------------------------------------------------
// ceoError_check()
//   Check for an error in the last call and if an error has occurred, raise a
// Python exception.
//-----------------------------------------------------------------------------
int ceoError_check(SQLSMALLINT handleType, SQLHANDLE handle,
        SQLRETURN rcToCheck, const char *context)
{
    PyObject *errorMessages, *temp, *separator;
    SQLINTEGER numRecords;
    SQLCHAR buffer[1024];
    SQLSMALLINT length;
    ceoError *error;
    SQLRETURN rc;
    int i;

    // handle simple cases
    if (rcToCheck == SQL_SUCCESS || rcToCheck == SQL_SUCCESS_WITH_INFO)
        return 0;
    if (rcToCheck == SQL_INVALID_HANDLE) {
        PyErr_SetString(ceoExceptionDatabaseError, "Invalid handle!");
        return -1;
    }

    // create new error object
    error = PyObject_NEW(ceoError, &ceoPyTypeError);
    if (!error)
        return -1;
    error->context = context;

    // determine number of diagnostic records available
    rc = SQLGetDiagFieldA(handleType, handle, 0, SQL_DIAG_NUMBER, &numRecords,
            0, NULL);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        error->message =
                CEO_STR_FROM_ASCII("cannot get number of diagnostic records");

    // determine error text
    } else if (numRecords == 0) {
        error->message =
                CEO_STR_FROM_ASCII("no diagnostic message text available");
    } else {
        error->message = NULL;
        errorMessages = PyList_New(numRecords);
        if (!errorMessages) {
            Py_DECREF(error);
            return -1;
        }
        for (i = 1; i <= numRecords; i++) {
            rc = SQLGetDiagFieldA(handleType, handle, i, SQL_DIAG_MESSAGE_TEXT,
                    buffer, sizeof(buffer), &length);
            if (length > (SQLSMALLINT) sizeof(buffer) - 1)
                length = (SQLSMALLINT) sizeof(buffer) - 1;
            if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
                error->message = CEO_STR_FROM_ASCII("cannot get " \
                        "diagnostic message text");
                break;
            }
            temp = PyUnicode_DecodeUTF8((const char*) buffer, length, NULL);
            if (!temp) {
                Py_DECREF(error);
                Py_DECREF(errorMessages);
                return -1;
            }
            PyList_SET_ITEM(errorMessages, i - 1, temp);
        }
        if (!error->message) {
            separator = CEO_STR_FROM_ASCII("\n");
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

    PyErr_SetObject(ceoExceptionDatabaseError, (PyObject*) error);
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
    ceoError *error;

    error = (ceoError*) ceoPyTypeError.tp_alloc(&ceoPyTypeError, 0);
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


//-----------------------------------------------------------------------------
// declaration of members for Python type
//-----------------------------------------------------------------------------
static PyMemberDef ceoMembers[] = {
    { "message", T_OBJECT, offsetof(ceoError, message), READONLY },
    { "context", T_STRING, offsetof(ceoError, context), READONLY },
    { NULL }
};


//-----------------------------------------------------------------------------
// declaration of Python type
//-----------------------------------------------------------------------------
PyTypeObject ceoPyTypeError = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ceODBC._Error",
    .tp_basicsize = sizeof(ceoError),
    .tp_dealloc = (destructor) ceoError_free,
    .tp_str = (reprfunc) ceoError_str,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_members = ceoMembers
};
