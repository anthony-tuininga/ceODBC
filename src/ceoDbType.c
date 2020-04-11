//-----------------------------------------------------------------------------
// ceoDbType.c
//   Defines the objects used for identifying all database types.
//-----------------------------------------------------------------------------

#include "ceoModule.h"

//-----------------------------------------------------------------------------
// declaration of functions
//-----------------------------------------------------------------------------
static void ceoDbType_free(ceoDbType*);
static PyObject *ceoDbType_repr(ceoDbType*);
static PyObject *ceoDbType_richCompare(ceoDbType*, PyObject*, int);
static Py_hash_t ceoDbType_hash(ceoDbType*);


//-----------------------------------------------------------------------------
// declaration of members
//-----------------------------------------------------------------------------
static PyMemberDef ceoMembers[] = {
    { "name", T_STRING, offsetof(ceoDbType, name), READONLY },
    { NULL }
};


//-----------------------------------------------------------------------------
// Python type declaration
//-----------------------------------------------------------------------------
PyTypeObject ceoPyTypeDbType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ceODBC._DbType",
    .tp_basicsize = sizeof(ceoDbType),
    .tp_dealloc = (destructor) ceoDbType_free,
    .tp_repr = (reprfunc) ceoDbType_repr,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_members = ceoMembers,
    .tp_richcompare = (richcmpfunc) ceoDbType_richCompare,
    .tp_hash = (hashfunc) ceoDbType_hash
};


//-----------------------------------------------------------------------------
// ceoDbType_free()
//   Free the database type object.
//-----------------------------------------------------------------------------
static void ceoDbType_free(ceoDbType *dbType)
{
    Py_TYPE(dbType)->tp_free((PyObject*) dbType);
}


//-----------------------------------------------------------------------------
// ceoDbType_repr()
//   Return a string representation of a queue.
//-----------------------------------------------------------------------------
static PyObject *ceoDbType_repr(ceoDbType *dbType)
{
    PyObject *module, *name, *dbTypeName, *result;

    dbTypeName = PyUnicode_DecodeASCII(dbType->name, strlen(dbType->name),
            NULL);
    if (!dbTypeName)
        return NULL;
    if (ceoUtils_getModuleAndName(Py_TYPE(dbType), &module, &name) < 0) {
        Py_DECREF(dbTypeName);
        return NULL;
    }
    result = ceoUtils_formatString("<%s.%s %s>",
            PyTuple_Pack(3, module, name, dbTypeName));
    Py_DECREF(module);
    Py_DECREF(name);
    Py_DECREF(dbTypeName);
    return result;
}


//-----------------------------------------------------------------------------
// ceoDbType_richCompare()
//   Peforms a comparison between the database type and another Python object.
// Equality (and inequality) are used to match database API types with their
// associated database types.
//-----------------------------------------------------------------------------
static PyObject *ceoDbType_richCompare(ceoDbType* dbType, PyObject* obj,
        int op)
{
    udt_ApiType *apiType;
    int status, equal;

    // only equality and inequality can be checked
    if (op != Py_EQ && op != Py_NE) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    // check for exact object
    equal = 0;
    if (obj == (PyObject*) dbType) {
        equal = 1;

    // check for API type
    } else {
        status = PyObject_IsInstance(obj, (PyObject*) &ceoPyTypeApiType);
        if (status < 0)
            return NULL;
        if (status == 1) {
            apiType = (udt_ApiType*) obj;
            status = PySequence_Contains(apiType->types, (PyObject*) dbType);
            if (status < 0)
                return NULL;
            equal = (status == 1) ? 1 : 0;
        }
    }

    // determine return value
    if ((equal && op == Py_EQ) || (!equal && op == Py_NE)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}


//-----------------------------------------------------------------------------
// ceoDbType_hash()
//   Return a hash value for the instance.
//-----------------------------------------------------------------------------
static Py_hash_t ceoDbType_hash(ceoDbType *dbType)
{
    return (Py_hash_t) dbType->transformNum;
}
