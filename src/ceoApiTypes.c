//-----------------------------------------------------------------------------
// ApiTypes.c
//   Defines class used for implementing the required DB API types.
//-----------------------------------------------------------------------------

#include "ceoModule.h"

//-----------------------------------------------------------------------------
// ceoApiType_free()
//   Deallocation routine.
//-----------------------------------------------------------------------------
static void ceoApiType_free(ceoApiType *apiType)
{
    Py_CLEAR(apiType->name);
    Py_CLEAR(apiType->types);
    Py_TYPE(apiType)->tp_free((PyObject*) apiType);
}


//-----------------------------------------------------------------------------
// ceoApiType_new()
//   Create a new API type and register it with the given name.
//-----------------------------------------------------------------------------
ceoApiType *ceoApiType_new(PyObject *module, const char *name)
{
    ceoApiType *apiType;

    apiType = PyObject_NEW(ceoApiType, &ceoPyTypeApiType);
    if (!apiType)
        return NULL;
    apiType->name = CEO_STR_FROM_ASCII(name);
    apiType->types = PyList_New(0);
    if (!apiType->types || !apiType->name) {
        Py_DECREF(apiType);
        return NULL;
    }
    if (PyModule_AddObject(module, name, (PyObject*) apiType) < 0) {
        Py_DECREF(apiType);
        return NULL;
    }
    return apiType;
}


//-----------------------------------------------------------------------------
// ceoApiType_repr()
//   Return a string representation of the API type.
//-----------------------------------------------------------------------------
static PyObject *ceoApiType_repr(ceoApiType *apiType)
{
    PyObject *module, *name, *result;

    if (ceoUtils_getModuleAndName(Py_TYPE(apiType), &module, &name) < 0)
        return NULL;
    result = ceoUtils_formatString("<%s.%s %s>",
            PyTuple_Pack(3, module, name, apiType->name));
    Py_DECREF(module);
    Py_DECREF(name);
    return result;
}


//-----------------------------------------------------------------------------
// ceoApiType_richCompare()
//   Comparison routine. This routine allows the variable type objects
// registered with the API type to match as equal as mandated by the DB API.
//-----------------------------------------------------------------------------
static PyObject *ceoApiType_richCompare(ceoApiType *apiType, PyObject *other,
        int comparisonType)
{
    PyObject *result;
    int contains;

    // only handle EQ and NE
    if (comparisonType != Py_EQ && comparisonType != Py_NE) {
        PyErr_SetString(ceoExceptionInterfaceError,
                "API types only support comparing equal and not equal");
        return NULL;
    }

    // for all other cases check to see if the object is in the list of types
    contains = PySequence_Contains(apiType->types, other);
    if (contains < 0)
        return NULL;
    if (comparisonType == Py_EQ && contains)
        result = Py_True;
    else if (comparisonType == Py_NE && !contains)
        result = Py_True;
    else result = Py_False;

    Py_INCREF(result);
    return result;
}


//-----------------------------------------------------------------------------
// declaration of members for the Python type
//-----------------------------------------------------------------------------
static PyMemberDef ceoMembers[] = {
    { "name", T_OBJECT, offsetof(ceoApiType, name), READONLY },
    { "types", T_OBJECT, offsetof(ceoApiType, types), READONLY },
    { NULL }
};


//-----------------------------------------------------------------------------
// declaration of the Python type
//-----------------------------------------------------------------------------
PyTypeObject ceoPyTypeApiType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ceODBC._ApiType",
    .tp_basicsize = sizeof(ceoApiType),
    .tp_dealloc = (destructor) ceoApiType_free,
    .tp_repr = (reprfunc) ceoApiType_repr,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_richcompare = (richcmpfunc) ceoApiType_richCompare,
    .tp_members = ceoMembers
};
