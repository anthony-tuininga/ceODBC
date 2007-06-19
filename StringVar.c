//-----------------------------------------------------------------------------
// StringVar.c
//   Defines the routines specific to all string like types.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Declaration of variable type structures
//-----------------------------------------------------------------------------
typedef struct {
    Variable_HEAD
    SQLCHAR *data;
} udt_VarcharVar;


//-----------------------------------------------------------------------------
// Declaration of variable functions
//-----------------------------------------------------------------------------
static int VarcharVar_SetValue(udt_VarcharVar*, unsigned, PyObject*);
static PyObject *VarcharVar_GetValue(udt_VarcharVar*, unsigned);
static SQLUINTEGER VarcharVar_GetBufferSize(udt_VarcharVar*, SQLUINTEGER);


//-----------------------------------------------------------------------------
// Declaration of Python types
//-----------------------------------------------------------------------------
static PyTypeObject g_VarcharVarType = {
    PyObject_HEAD_INIT(NULL)
    0,                                  // ob_size
    "ceODBC.VarcharVar",                // tp_name
    sizeof(udt_VarcharVar),             // tp_basicsize
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
static udt_VariableType vt_Varchar = {
    (SetValueProc) VarcharVar_SetValue,
    (GetValueProc) VarcharVar_GetValue,
    (GetBufferSizeProc) VarcharVar_GetBufferSize,
    &g_VarcharVarType,                  // Python type
    SQL_VARCHAR,                        // SQL type
    SQL_C_CHAR                          // C data type
};


//-----------------------------------------------------------------------------
// VarcharVar_SetValue()
//   Set the value of the variable.
//-----------------------------------------------------------------------------
static int VarcharVar_SetValue(
    udt_VarcharVar *var,                // variable to set value for
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


//-----------------------------------------------------------------------------
// VarcharVar_GetValue()
//   Returns the value stored at the given array position.
//-----------------------------------------------------------------------------
static PyObject *VarcharVar_GetValue(
    udt_VarcharVar *var,                // variable to determine value for
    unsigned pos)                       // array position
{
    return PyString_FromStringAndSize(var->data + pos * var->bufferSize,
            var->lengthOrIndicator[pos]);
}


//-----------------------------------------------------------------------------
// VarcharVar_GetBufferSize()
//   Returns the size to use for string buffers. ODBC requires the presence of
// a NULL terminator so one extra space is allocated for that purpose.
//-----------------------------------------------------------------------------
static SQLUINTEGER VarcharVar_GetBufferSize(
    udt_VarcharVar *var,                // variable to determine value for
    SQLUINTEGER size)                   // size to allocate
{
    return size + 1;
}

