//-----------------------------------------------------------------------------
// BitVar.c
//   Defines the routines for handling bits.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Declaration of variable structure.
//-----------------------------------------------------------------------------
typedef struct {
    Variable_HEAD
    unsigned char *data;
} udt_BitVar;


//-----------------------------------------------------------------------------
// Declaration of variable functions.
//-----------------------------------------------------------------------------
static PyObject *BitVar_GetValue(udt_BitVar*, unsigned);
static int BitVar_SetValue(udt_BitVar*, unsigned, PyObject*);


//-----------------------------------------------------------------------------
// Python type declaration
//-----------------------------------------------------------------------------
static PyTypeObject g_BitVarType = {
    PyObject_HEAD_INIT(NULL)
    0,                                  // ob_size
    "ceODBC.BitVar",                    // tp_name
    sizeof(udt_BitVar),                 // tp_basicsize
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
// variable type declarations
//-----------------------------------------------------------------------------
static udt_VariableType vt_Bit = {
    (SetValueProc) BitVar_SetValue,
    (GetValueProc) BitVar_GetValue,
    (GetBufferSizeProc) NULL,
    &g_BitVarType,                      // Python type
    SQL_BIT,                            // SQL type
    SQL_C_BIT,                          // C data type
    sizeof(unsigned char)               // buffer size
};


//-----------------------------------------------------------------------------
// BitVar_SetValue()
//   Set the value of the variable.
//-----------------------------------------------------------------------------
static int BitVar_SetValue(
    udt_BitVar *var,                    // variable to set value for
    unsigned pos,                       // array position to set
    PyObject *value)                    // value to set
{
    if (PyBool_Check(value)) {
        var->data[pos] = (unsigned char) PyInt_AS_LONG(value);
        return 0;
    }
    PyErr_SetString(PyExc_TypeError, "expecting boolean data");
    return -1;
}


//-----------------------------------------------------------------------------
// BitVar_GetValue()
//   Returns the value stored at the given array position.
//-----------------------------------------------------------------------------
static PyObject *BitVar_GetValue(
    udt_BitVar *var,                    // variable to determine value for
    unsigned pos)                       // array position
{
    return PyBool_FromLong(var->data[pos]);
}

