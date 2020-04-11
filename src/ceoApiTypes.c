//-----------------------------------------------------------------------------
// ApiTypes.c
//   Defines class used for implementing the required DB API types.
//-----------------------------------------------------------------------------

#include "ceoModule.h"

//-----------------------------------------------------------------------------
// declaration of methods used by the Python type
//-----------------------------------------------------------------------------
static void ApiType_Free(udt_ApiType*);
static PyObject *ApiType_Repr(udt_ApiType*);
static PyObject *ApiType_RichCompare(udt_ApiType*, PyObject*, int);


//-----------------------------------------------------------------------------
// declaration of members for the Python type
//-----------------------------------------------------------------------------
static PyMemberDef ceoMembers[] = {
    { "name", T_OBJECT, offsetof(udt_ApiType, name), READONLY },
    { "types", T_OBJECT, offsetof(udt_ApiType, types), READONLY },
    { NULL }
};


//-----------------------------------------------------------------------------
// declaration of the Python type
//-----------------------------------------------------------------------------
PyTypeObject ceoPyTypeApiType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ceODBC._ApiType",
    .tp_basicsize = sizeof(udt_ApiType),
    .tp_dealloc = (destructor) ApiType_Free,
    .tp_repr = (reprfunc) ApiType_Repr,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_richcompare = (richcmpfunc) ApiType_RichCompare,
    .tp_members = ceoMembers
};


//-----------------------------------------------------------------------------
// ApiType_Free()
//   Deallocation routine.
//-----------------------------------------------------------------------------
static void ApiType_Free(udt_ApiType *self)
{
    Py_CLEAR(self->name);
    Py_CLEAR(self->types);
    Py_TYPE(self)->tp_free((PyObject*) self);
}


//-----------------------------------------------------------------------------
// ApiType_New()
//   Create a new API type and register it with the given name.
//-----------------------------------------------------------------------------
udt_ApiType *ApiType_New(PyObject *module, const char *name)
{
    udt_ApiType *self;

    self = PyObject_NEW(udt_ApiType, &ceoPyTypeApiType);
    if (!self)
        return NULL;
    self->name = ceString_FromAscii(name);
    self->types = PyList_New(0);
    if (!self->types || !self->name) {
        Py_DECREF(self);
        return NULL;
    }
    if (PyModule_AddObject(module, name, (PyObject*) self) < 0) {
        Py_DECREF(self);
        return NULL;
    }
    return self;
}


//-----------------------------------------------------------------------------
// ApiType_Repr()
//   Return a string representation of the API type.
//-----------------------------------------------------------------------------
static PyObject *ApiType_Repr(udt_ApiType *self)
{
    PyObject *module, *name, *result;

    if (ceoUtils_getModuleAndName(Py_TYPE(self), &module, &name) < 0)
        return NULL;
    result = ceoUtils_formatString("<%s.%s %s>",
            PyTuple_Pack(3, module, name, self->name));
    Py_DECREF(module);
    Py_DECREF(name);
    return result;
}


//-----------------------------------------------------------------------------
// ApiType_RichCompare()
//   Comparison routine. This routine allows the variable type objects
// registered with the API type to match as equal as mandated by the DB API.
//-----------------------------------------------------------------------------
static PyObject *ApiType_RichCompare(udt_ApiType *self, PyObject *other,
        int comparisonType)
{
    PyObject *result;
    int contains;

    // only handle EQ and NE
    if (comparisonType != Py_EQ && comparisonType != Py_NE) {
        PyErr_SetString(g_InterfaceErrorException,
                "API types only support comparing equal and not equal");
        return NULL;
    }

    // for all other cases check to see if the object is in the list of types
    contains = PySequence_Contains(self->types, other);
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
