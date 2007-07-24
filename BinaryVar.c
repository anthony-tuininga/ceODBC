//-----------------------------------------------------------------------------
// BinaryVar.c
//   Defines the routines specific to all binary types.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Declaration of variable type structures
//-----------------------------------------------------------------------------
typedef struct {
    Variable_HEAD
    SQLCHAR *data;
} udt_BinaryVar;


//-----------------------------------------------------------------------------
// Declaration of variable functions
//-----------------------------------------------------------------------------
static SQLUINTEGER BinaryVar_GetBufferSize(udt_BinaryVar*, SQLUINTEGER);
static PyObject *BinaryVar_GetValue(udt_BinaryVar*, unsigned);
static int BinaryVar_SetValue(udt_BinaryVar*, unsigned, PyObject*);


//-----------------------------------------------------------------------------
// Declaration of Python types
//-----------------------------------------------------------------------------
static PyTypeObject g_BinaryVarType = {
    PyObject_HEAD_INIT(NULL)
    0,                                  // ob_size
    "ceODBC.BinaryVar",                 // tp_name
    sizeof(udt_BinaryVar),              // tp_basicsize
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


static PyTypeObject g_LongBinaryVarType = {
    PyObject_HEAD_INIT(NULL)
    0,                                  // ob_size
    "ceODBC.LongBinaryVar",             // tp_name
    sizeof(udt_BinaryVar),              // tp_basicsize
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
static udt_VariableType vt_Binary = {
    (SetValueProc) BinaryVar_SetValue,
    (GetValueProc) BinaryVar_GetValue,
    (GetBufferSizeProc) BinaryVar_GetBufferSize,
    &g_BinaryVarType,                   // Python type
    SQL_VARBINARY,                      // SQL type
    SQL_C_BINARY,                       // C data type
    0,                                  // buffer size
    255,                                // default size
    0                                   // default scale
};


static udt_VariableType vt_LongBinary = {
    (SetValueProc) BinaryVar_SetValue,
    (GetValueProc) BinaryVar_GetValue,
    (GetBufferSizeProc) BinaryVar_GetBufferSize,
    &g_LongBinaryVarType,               // Python type
    SQL_LONGVARBINARY,                  // SQL type
    SQL_C_BINARY,                       // C data type
    0,                                  // buffer size
    128 * 1024,                         // default size
    0                                   // default scale
};


//-----------------------------------------------------------------------------
// BinaryVar_GetBufferSize()
//   Returns the size to use for binary buffers.
//-----------------------------------------------------------------------------
static SQLUINTEGER BinaryVar_GetBufferSize(
    udt_BinaryVar *var,                 // variable to determine value for
    SQLUINTEGER size)                   // size to allocate
{
    return size;
}


//-----------------------------------------------------------------------------
// BinaryVar_GetValue()
//   Returns the value stored at the given array position.
//-----------------------------------------------------------------------------
static PyObject *BinaryVar_GetValue(
    udt_BinaryVar *var,                 // variable to determine value for
    unsigned pos)                       // array position
{
    PyObject *str, *buffer;

    str = PyString_FromStringAndSize((char*) var->data + pos * var->bufferSize,
            var->lengthOrIndicator[pos]);
    if (!str)
        return NULL;
    buffer = PyBuffer_FromObject(str, 0, Py_END_OF_BUFFER);
    Py_DECREF(str);
    return buffer;
}


//-----------------------------------------------------------------------------
// BinaryVar_SetValue()
//   Set the value of the variable.
//-----------------------------------------------------------------------------
static int BinaryVar_SetValue(
    udt_BinaryVar *var,                 // variable to set value for
    unsigned pos,                       // array position to set
    PyObject *value)                    // value to set
{
    const void *buffer;
    Py_ssize_t size;

    if (PyObject_AsReadBuffer(value, &buffer, &size) < 0)
        return -1;
    if (size > var->size) {
        if (Variable_Resize( (udt_Variable*) var, size) < 0)
            return -1;
    }
    var->lengthOrIndicator[pos] = (SQLINTEGER) size;
    if (size)
        memcpy(var->data + var->bufferSize * pos, buffer, size);

    return 0;
}

