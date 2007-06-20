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
static int BinaryVar_SetValue(udt_BinaryVar*, unsigned, PyObject*);
static PyObject *BinaryVar_GetValue(udt_BinaryVar*, unsigned);
static SQLUINTEGER BinaryVar_GetBufferSize(udt_BinaryVar*, SQLUINTEGER);


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
    Py_TPFLAGS_DEFAULT,                 // tp_flags
    0                                   // tp_doc
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
    Py_TPFLAGS_DEFAULT,                 // tp_flags
    0                                   // tp_doc
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
    SQL_C_BINARY                        // C data type
};


static udt_VariableType vt_LongBinary = {
    (SetValueProc) BinaryVar_SetValue,
    (GetValueProc) BinaryVar_GetValue,
    (GetBufferSizeProc) BinaryVar_GetBufferSize,
    &g_LongBinaryVarType,               // Python type
    SQL_LONGVARBINARY,                  // SQL type
    SQL_C_BINARY                        // C data type
};


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
// BinaryVar_GetBufferSize()
//   Returns the size to use for binary buffers.
//-----------------------------------------------------------------------------
static SQLUINTEGER BinaryVar_GetBufferSize(
    udt_BinaryVar *var,                 // variable to determine value for
    SQLUINTEGER size)                   // size to allocate
{
    return size;
}

