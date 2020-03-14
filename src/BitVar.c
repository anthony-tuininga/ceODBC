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
    PyVarObject_HEAD_INIT(NULL, 0)
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
    (initproc) Variable_DefaultInit,    // tp_init
    0,                                  // tp_alloc
    (newfunc) Variable_New,             // tp_new
    0,                                  // tp_free
    0,                                  // tp_is_gc
    0                                   // tp_bases
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
    sizeof(unsigned char),              // buffer size
    1,                                  // default size
    0                                   // default scale
};


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
        var->data[pos] = (unsigned char) PyInt_AsLong(value);
        if (PyErr_Occurred())
            return -1;
        return 0;
    }
    PyErr_SetString(PyExc_TypeError, "expecting boolean data");
    return -1;
}

