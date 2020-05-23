//-----------------------------------------------------------------------------
// ceoUtils.c
//   Utility functions used by ceODBC.
//-----------------------------------------------------------------------------

#include "ceoModule.h"

//-----------------------------------------------------------------------------
// ceoUtils_findInString()
//   Call the method "find" on the object and return the position in the
// string where it is found.
//-----------------------------------------------------------------------------
int ceoUtils_findInString(PyObject *strObj, char *stringToFind, int startPos,
        int *foundPos)
{
    PyObject *temp;

    temp = PyObject_CallMethod(strObj, "find", "si", stringToFind, startPos);
    if (!temp)
        return -1;
    *foundPos = PyLong_AsLong(temp);
    Py_DECREF(temp);
    if (PyErr_Occurred())
        return -1;
    return 0;
}


//-----------------------------------------------------------------------------
// ceoUtils_formatString()
//   Return a Python string formatted using the given format string and
// arguments. The arguments have a reference taken from them after they have
// been used (which should mean that they are destroyed).
//-----------------------------------------------------------------------------
PyObject *ceoUtils_formatString(const char *format, PyObject *args)
{
    PyObject *formatObj, *result;

    // assume that a NULL value for arguments implies building the arguments
    // failed and a Python exception has already been raised
    if (!args)
        return NULL;

    // convert string format to Python object
    formatObj = PyUnicode_DecodeASCII(format, strlen(format), NULL);
    if (!formatObj) {
        Py_DECREF(args);
        return NULL;
    }

    // create formatted result
    result = PyUnicode_Format(formatObj, args);
    Py_DECREF(args);
    Py_DECREF(formatObj);
    return result;
}


//-----------------------------------------------------------------------------
// ceoUtils_getModuleAndName()
//   Return the module and name for the type.
//-----------------------------------------------------------------------------
int ceoUtils_getModuleAndName(PyTypeObject *type, PyObject **module,
        PyObject **name)
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
