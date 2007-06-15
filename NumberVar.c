//-----------------------------------------------------------------------------
// NumberVar.c
//   Defines the routines for all number like types.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Declaration of variable type structures
//-----------------------------------------------------------------------------
typedef struct {
    Variable_HEAD
    double *data;
} udt_DoubleVar;


typedef struct {
    Variable_HEAD
    SQLINTEGER *data;
} udt_IntegerVar;


//-----------------------------------------------------------------------------
// Declaration of variable functions.
//-----------------------------------------------------------------------------
static PyObject *DoubleVar_GetValue(udt_DoubleVar*, unsigned);
static int DoubleVar_SetValue(udt_DoubleVar*, unsigned, PyObject*);
static PyObject *IntegerVar_GetValue(udt_IntegerVar*, unsigned);
static int IntegerVar_SetValue(udt_IntegerVar*, unsigned, PyObject*);


//-----------------------------------------------------------------------------
// Declaration of Python types
//-----------------------------------------------------------------------------
static PyTypeObject g_DoubleVarType = {
    PyObject_HEAD_INIT(NULL)
    0,                                  // ob_size
    "ceODBC.DOUBLE",                    // tp_name
    sizeof(udt_DoubleVar),              // tp_basicsize
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


static PyTypeObject g_IntegerVarType = {
    PyObject_HEAD_INIT(NULL)
    0,                                  // ob_size
    "ceODBC.INTEGER",                   // tp_name
    sizeof(udt_IntegerVar),             // tp_basicsize
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
static udt_VariableType vt_Double = {
    (SetValueProc) DoubleVar_SetValue,
    (GetValueProc) DoubleVar_GetValue,
    (GetBufferSizeProc) NULL,
    &g_DoubleVarType,                   // Python type
    SQL_DOUBLE,                         // SQL type
    SQL_C_DOUBLE,                       // C data type
    sizeof(SQLDOUBLE)                   // buffer size
};


static udt_VariableType vt_Integer = {
    (SetValueProc) IntegerVar_SetValue,
    (GetValueProc) IntegerVar_GetValue,
    (GetBufferSizeProc) NULL,
    &g_IntegerVarType,                  // Python type
    SQL_INTEGER,                        // SQL type
    SQL_C_LONG,                         // C data type
    sizeof(SQLINTEGER)                  // buffer size
};


//-----------------------------------------------------------------------------
// DoubleVar_SetValue()
//   Set the value of the variable.
//-----------------------------------------------------------------------------
static int DoubleVar_SetValue(
    udt_DoubleVar *var,                 // variable to set value for
    unsigned pos,                       // array position to set
    PyObject *value)                    // value to set
{
    if (PyFloat_Check(value)) {
        var->data[pos] = PyFloat_AS_DOUBLE(value);
        return 0;
    }
    PyErr_SetString(PyExc_TypeError, "expecting floating point data");
    return -1;
}


//-----------------------------------------------------------------------------
// DoubleVar_GetValue()
//   Returns the value stored at the given array position.
//-----------------------------------------------------------------------------
static PyObject *DoubleVar_GetValue(
    udt_DoubleVar *var,                // variable to determine value for
    unsigned pos)                       // array position
{
    return PyFloat_FromDouble(var->data[pos]);
}


//-----------------------------------------------------------------------------
// IntegerVar_SetValue()
//   Set the value of the variable.
//-----------------------------------------------------------------------------
static int IntegerVar_SetValue(
    udt_IntegerVar *var,                // variable to set value for
    unsigned pos,                       // array position to set
    PyObject *value)                    // value to set
{
    if (PyInt_Check(value)) {
        var->data[pos] = PyInt_AS_LONG(value);
        return 0;
    }
    PyErr_SetString(PyExc_TypeError, "expecting numeric data");
    return -1;
}


//-----------------------------------------------------------------------------
// IntegerVar_GetValue()
//   Returns the value stored at the given array position.
//-----------------------------------------------------------------------------
static PyObject *IntegerVar_GetValue(
    udt_IntegerVar *var,                // variable to determine value for
    unsigned pos)                       // array position
{
    return PyInt_FromLong(var->data[pos]);
}

