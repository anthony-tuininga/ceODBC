//-----------------------------------------------------------------------------
// ApiTypes.c
//   Defines class used for implementing the required DB API types.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// declaration of the structure for the Python type
//-----------------------------------------------------------------------------
typedef struct {
    PyObject_HEAD
    PyObject *name;
    PyObject *types;
} udt_ApiType;


//-----------------------------------------------------------------------------
// declaration of methods used by the Python type
//-----------------------------------------------------------------------------
static void ApiType_Free(udt_ApiType*);
static PyObject *ApiType_Repr(udt_ApiType*);
static PyObject *ApiType_RichCompare(udt_ApiType*, PyObject*, int);


//-----------------------------------------------------------------------------
// declaration of members of the Python type
//-----------------------------------------------------------------------------
static PyMemberDef g_ApiTypeTypeMembers[] = {
    { "name", T_OBJECT, offsetof(udt_ApiType, name), READONLY },
    { "types", T_OBJECT, offsetof(udt_ApiType, types), READONLY },
    { NULL }
};


//-----------------------------------------------------------------------------
// declaration of the Python type
//-----------------------------------------------------------------------------
static PyTypeObject g_ApiTypeType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "ceODBC._ApiType",                  // tp_name
    sizeof(udt_ApiType),                // tp_basicsize
    0,                                  // tp_itemsize
    (destructor) ApiType_Free,          // tp_dealloc
    0,                                  // tp_print
    0,                                  // tp_getattr
    0,                                  // tp_setattr
    0,                                  // tp_compare
    (reprfunc) ApiType_Repr,            // tp_repr
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
    0,                                  // tp_doc
    0,                                  // tp_traverse
    0,                                  // tp_clear
    (richcmpfunc) ApiType_RichCompare,  // tp_richcompare
    0,                                  // tp_weaklistoffset
    0,                                  // tp_iter
    0,                                  // tp_iternext
    0,                                  // tp_methods
    g_ApiTypeTypeMembers                // tp_members
};


//-----------------------------------------------------------------------------
// ApiType_Free()
//   Deallocation routine.
//-----------------------------------------------------------------------------
static void ApiType_Free(
    udt_ApiType *self)                  // error object
{
    Py_CLEAR(self->name);
    Py_CLEAR(self->types);
    Py_TYPE(self)->tp_free((PyObject*) self);
}


//-----------------------------------------------------------------------------
// ApiType_New()
//   Create a new API type and register it with the given name.
//-----------------------------------------------------------------------------
static udt_ApiType *ApiType_New(
    PyObject *module,                   // module in which to register
    const char *name)                   // name to register type as
{
    udt_ApiType *self;

    self = PyObject_NEW(udt_ApiType, &g_ApiTypeType);
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
static PyObject *ApiType_Repr(
    udt_ApiType *self)                  // variable to return the string for
{
    PyObject *module, *name, *result, *format, *formatArgs = NULL;

    if (GetModuleAndName(Py_TYPE(self), &module, &name) < 0)
        return NULL;
    formatArgs = PyTuple_Pack(3, module, name, self->name);
    Py_DECREF(module);
    Py_DECREF(name);
    if (!formatArgs)
        return NULL;

    format = ceString_FromAscii("<%s.%s %s>");
    if (!format) {
        Py_DECREF(formatArgs);
        return NULL;
    }

    result = ceString_Format(format, formatArgs);
    Py_DECREF(format);
    Py_DECREF(formatArgs);

    return result;
}


//-----------------------------------------------------------------------------
// ApiType_RichCompare()
//   Comparison routine. This routine allows the variable type objects
// registered with the API type to match as equal as mandated by the DB API.
//-----------------------------------------------------------------------------
static PyObject *ApiType_RichCompare(
    udt_ApiType *self,                  // API type to compare
    PyObject *other,                    // other object to compare
    int comparisonType)                 // comparison type being made
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

