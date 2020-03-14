//-----------------------------------------------------------------------------
// StringVar.c
//   Defines the routines specific to string types.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Declaration of variable type structures
//-----------------------------------------------------------------------------
typedef struct {
    Variable_HEAD
    SQLCHAR *data;
} udt_StringVar;


//-----------------------------------------------------------------------------
// Declaration of variable functions
//-----------------------------------------------------------------------------
static PyObject *StringVar_GetValue(udt_StringVar*, unsigned);
static SQLUINTEGER StringVar_GetBufferSize(udt_StringVar*, SQLUINTEGER);
static int StringVar_SetValue(udt_StringVar*, unsigned, PyObject*);


//-----------------------------------------------------------------------------
// Declaration of Python types
//-----------------------------------------------------------------------------
static PyTypeObject g_StringVarType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "ceODBC.StringVar",                 // tp_name
    sizeof(udt_StringVar),              // tp_basicsize
    0,                                  // tp_itemsize
    (destructor) Variable_Free,         // tp_dealloc
    0,                                  // tp_print
    0,                                  // tp_getattr
    0,                                  // tp_setattr
    0,                                  // tp_compare
    (reprfunc) Variable_Repr,           // tp_repr
    0,                                  // tp_as_number
    0,                                  // tp_as_sequence
    0,                                  // tp_as_mapping
    0,                                  // tp_hash
    0,                                  // tp_call
    0,                                  // tp_str
    0,                                  // tp_getattro
    0,                                  // tp_setattro
    0,                                  // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
                                        // tp_flags
    0,                                  // tp_doc
    0,                                  // tp_traverse
    0,                                  // tp_clear
    0,                                  // tp_richcompare
    0,                                  // tp_weaklistoffset
    0,                                  // tp_iter
    0,                                  // tp_iternext
    g_VariableMethods,                  // tp_methods
    g_VariableMembers,                  // tp_members
    0,                                  // tp_getset
    0,                                  // tp_base
    0,                                  // tp_dict
    0,                                  // tp_descr_get
    0,                                  // tp_descr_set
    0,                                  // tp_dictoffset
    (initproc) Variable_InitWithSize,   // tp_init
    0,                                  // tp_alloc
    (newfunc) Variable_New,             // tp_new
    0,                                  // tp_free
    0,                                  // tp_is_gc
    0                                   // tp_bases
};

 
static PyTypeObject g_LongStringVarType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "ceODBC.LongStringVar",             // tp_name
    sizeof(udt_StringVar),              // tp_basicsize
    0,                                  // tp_itemsize
    (destructor) Variable_Free,         // tp_dealloc
    0,                                  // tp_print
    0,                                  // tp_getattr
    0,                                  // tp_setattr
    0,                                  // tp_compare
    (reprfunc) Variable_Repr,           // tp_repr
    0,                                  // tp_as_number
    0,                                  // tp_as_sequence
    0,                                  // tp_as_mapping
    0,                                  // tp_hash
    0,                                  // tp_call
    0,                                  // tp_str
    0,                                  // tp_getattro
    0,                                  // tp_setattro
    0,                                  // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
                                        // tp_flags
    0,                                  // tp_doc
    0,                                  // tp_traverse
    0,                                  // tp_clear
    0,                                  // tp_richcompare
    0,                                  // tp_weaklistoffset
    0,                                  // tp_iter
    0,                                  // tp_iternext
    g_VariableMethods,                  // tp_methods
    g_VariableMembers,                  // tp_members
    0,                                  // tp_getset
    0,                                  // tp_base
    0,                                  // tp_dict
    0,                                  // tp_descr_get
    0,                                  // tp_descr_set
    0,                                  // tp_dictoffset
    (initproc) Variable_InitWithSize,   // tp_init
    0,                                  // tp_alloc
    (newfunc) Variable_New,             // tp_new
    0,                                  // tp_free
    0,                                  // tp_is_gc
    0                                   // tp_bases
};

 
//-----------------------------------------------------------------------------
// Declaration of variable types
//-----------------------------------------------------------------------------
static udt_VariableType vt_String = {
    (SetValueProc) StringVar_SetValue,
    (GetValueProc) StringVar_GetValue,
    (GetBufferSizeProc) StringVar_GetBufferSize,
    &g_StringVarType,                   // Python type
    SQL_VARCHAR,                        // SQL type
    SQL_C_CHAR,                         // C data type
    0,                                  // buffer size
    255,                                // default size
    0                                   // default scale
};


static udt_VariableType vt_LongString = {
    (SetValueProc) StringVar_SetValue,
    (GetValueProc) StringVar_GetValue,
    (GetBufferSizeProc) StringVar_GetBufferSize,
    &g_LongStringVarType,               // Python type
    SQL_LONGVARCHAR,                    // SQL type
    SQL_C_CHAR,                         // C data type
    0,                                  // buffer size
    128 * 1024,                         // default size
    0                                   // default scale
};


//-----------------------------------------------------------------------------
// StringVar_GetBufferSize()
//   Returns the size to use for string buffers. ODBC requires the presence of
// a NULL terminator so one extra space is allocated for that purpose.
//-----------------------------------------------------------------------------
static SQLUINTEGER StringVar_GetBufferSize(
    udt_StringVar *var,                 // variable to determine value for
    SQLUINTEGER size)                   // size to allocate
{
    return size + 1;
}


//-----------------------------------------------------------------------------
// StringVar_GetValue()
//   Returns the value stored at the given array position.
//-----------------------------------------------------------------------------
static PyObject *StringVar_GetValue(
    udt_StringVar *var,                 // variable to determine value for
    unsigned pos)                       // array position
{
    return PyString_FromStringAndSize((char*) var->data +
            pos * var->bufferSize, var->lengthOrIndicator[pos]);
}


//-----------------------------------------------------------------------------
// StringVar_SetValue()
//   Set the value of the variable.
//-----------------------------------------------------------------------------
static int StringVar_SetValue(
    udt_StringVar *var,                 // variable to set value for
    unsigned pos,                       // array position to set
    PyObject *value)                    // value to set
{
    const void *buffer;
    Py_ssize_t size;

    // get the buffer data and size for binding
    if (PyString_Check(value)) {
        buffer = PyString_AS_STRING(value);
        size = PyString_GET_SIZE(value);
    } else {
        PyErr_SetString(PyExc_TypeError, "expecting string data");
        return -1;
    }

    // resize the variable if necessary
    if (size > var->size) {
        if (Variable_Resize((udt_Variable*) var, size) < 0)
            return -1;
    }

    // keep a copy of the string
    var->lengthOrIndicator[pos] = (SQLINTEGER) size;
    if (size)
        memcpy(var->data + var->bufferSize * pos, buffer, size);

    return 0;
}

