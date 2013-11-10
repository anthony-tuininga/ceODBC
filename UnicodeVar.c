//-----------------------------------------------------------------------------
// UnicodeVar.c
//   Defines the routines specific to unicode types.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Declaration of variable type structures
//-----------------------------------------------------------------------------
typedef struct {
    Variable_HEAD
    SQLWCHAR *data;
} udt_UnicodeVar;


//-----------------------------------------------------------------------------
// Declaration of variable functions
//-----------------------------------------------------------------------------
static PyObject *UnicodeVar_GetValue(udt_UnicodeVar*, unsigned);
static SQLUINTEGER UnicodeVar_GetBufferSize(udt_UnicodeVar*, SQLUINTEGER);
static int UnicodeVar_SetValue(udt_UnicodeVar*, unsigned, PyObject*);


//-----------------------------------------------------------------------------
// Declaration of Python types
//-----------------------------------------------------------------------------
static PyTypeObject g_UnicodeVarType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "ceODBC.UnicodeVar",                // tp_name
    sizeof(udt_UnicodeVar),             // tp_basicsize
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

 
static PyTypeObject g_LongUnicodeVarType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "ceODBC.LongUnicodeVar",            // tp_name
    sizeof(udt_UnicodeVar),             // tp_basicsize
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
static udt_VariableType vt_Unicode = {
    (SetValueProc) UnicodeVar_SetValue,
    (GetValueProc) UnicodeVar_GetValue,
    (GetBufferSizeProc) UnicodeVar_GetBufferSize,
    &g_UnicodeVarType,                  // Python type
    SQL_WVARCHAR,                       // SQL type
    SQL_C_WCHAR,                        // C data type
    0,                                  // buffer size
    255,                                // default size
    0                                   // default scale
};


static udt_VariableType vt_LongUnicode = {
    (SetValueProc) UnicodeVar_SetValue,
    (GetValueProc) UnicodeVar_GetValue,
    (GetBufferSizeProc) UnicodeVar_GetBufferSize,
    &g_LongUnicodeVarType,              // Python type
    SQL_WLONGVARCHAR,                   // SQL type
    SQL_C_WCHAR,                        // C data type
    0,                                  // buffer size
    128 * 1024,                         // default size
    0                                   // default scale
};


//-----------------------------------------------------------------------------
// UnicodeVar_GetBufferSize()
//   Returns the size to use for string buffers. ODBC requires the presence of
// a NULL terminator so one extra space is allocated for that purpose.
//-----------------------------------------------------------------------------
static SQLUINTEGER UnicodeVar_GetBufferSize(
    udt_UnicodeVar *var,                // variable to determine value for
    SQLUINTEGER size)                   // size to allocate
{
    return (size + 1) * 2;
}


//-----------------------------------------------------------------------------
// UnicodeVar_GetValue()
//   Returns the value stored at the given array position.
//-----------------------------------------------------------------------------
static PyObject *UnicodeVar_GetValue(
    udt_UnicodeVar *var,                // variable to determine value for
    unsigned pos)                       // array position
{
#ifdef Py_UNICODE_WIDE
    return PyUnicode_DecodeUTF16((char*) var->data + pos * var->bufferSize,
            var->lengthOrIndicator[pos], NULL, NULL);
#else
    return PyUnicode_FromUnicode(
            (Py_UNICODE*) ( (char*) var->data + pos * var->bufferSize),
            var->lengthOrIndicator[pos] / 2);
#endif
}


//-----------------------------------------------------------------------------
// UnicodeVar_SetValue()
//   Set the value of the variable.
//-----------------------------------------------------------------------------
static int UnicodeVar_SetValue(
    udt_UnicodeVar *var,                // variable to set value for
    unsigned pos,                       // array position to set
    PyObject *value)                    // value to set
{
    udt_StringBuffer buffer;

    // confirm that the value is Unicode
    if (!PyUnicode_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "expecting unicode data");
        return -1;
    }

    // resize the variable if necessary
    if (StringBuffer_FromUnicode(&buffer, value) < 0)
        return -1;
    if (buffer.size > var->size) {
        if (Variable_Resize((udt_Variable*) var, buffer.size) < 0) {
            StringBuffer_Clear(&buffer);
            return -1;
        }
    }

    // keep a copy of the string
    var->lengthOrIndicator[pos] = (SQLINTEGER) buffer.sizeInBytes;
    if (buffer.sizeInBytes)
        memcpy( ((char*) var->data) + var->bufferSize * pos, buffer.ptr,
                buffer.sizeInBytes);
    StringBuffer_Clear(&buffer);

    return 0;
}

