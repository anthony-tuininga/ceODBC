//-----------------------------------------------------------------------------
// DateTimeVar.c
//   Defines the routines for handling date/time variables.
//-----------------------------------------------------------------------------

#include "ceoModule.h"

//-----------------------------------------------------------------------------
// Declaration of variable functions.
//-----------------------------------------------------------------------------
static PyObject *DateVar_GetValue(udt_DateVar*, unsigned);
static int DateVar_SetValue(udt_DateVar*, unsigned, PyObject*);
static PyObject *TimeVar_GetValue(udt_TimeVar*, unsigned);
static int TimeVar_SetValue(udt_TimeVar*, unsigned, PyObject*);
static PyObject *TimestampVar_GetValue(udt_TimestampVar*, unsigned);
static int TimestampVar_SetValue(udt_TimestampVar*, unsigned, PyObject*);


//-----------------------------------------------------------------------------
// Python type declaration
//-----------------------------------------------------------------------------
PyTypeObject g_DateVarType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "ceODBC.DateVar",                   // tp_name
    sizeof(udt_DateVar),                // tp_basicsize
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


PyTypeObject g_TimeVarType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "ceODBC.TimeVar",                   // tp_name
    sizeof(udt_TimeVar),                // tp_basicsize
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


PyTypeObject g_TimestampVarType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "ceODBC.TimestampVar",              // tp_name
    sizeof(udt_TimestampVar),           // tp_basicsize
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
udt_VariableType vt_Date = {
    (SetValueProc) DateVar_SetValue,
    (GetValueProc) DateVar_GetValue,
    (GetBufferSizeProc) NULL,
    &g_DateVarType,                     // Python type
    SQL_TYPE_DATE,                      // SQL type
    SQL_C_TYPE_DATE,                    // C data type
    sizeof(DATE_STRUCT),                // buffer size
    23,                                 // default size
    3                                   // default scale
};


udt_VariableType vt_Time = {
    (SetValueProc) TimeVar_SetValue,
    (GetValueProc) TimeVar_GetValue,
    (GetBufferSizeProc) NULL,
    &g_TimeVarType,                     // Python type
    SQL_TYPE_TIME,                      // SQL type
    SQL_C_TYPE_TIME,                    // C data type
    sizeof(TIME_STRUCT),                // buffer size
    23,                                 // default size
    3                                   // default scale
};


udt_VariableType vt_Timestamp = {
    (SetValueProc) TimestampVar_SetValue,
    (GetValueProc) TimestampVar_GetValue,
    (GetBufferSizeProc) NULL,
    &g_TimestampVarType,                // Python type
    SQL_TYPE_TIMESTAMP,                 // SQL type
    SQL_C_TYPE_TIMESTAMP,               // C data type
    sizeof(TIMESTAMP_STRUCT),           // buffer size
    23,                                 // default size
    3                                   // default scale
};


//-----------------------------------------------------------------------------
// DateVar_GetValue()
//   Returns the value stored at the given array position.
//-----------------------------------------------------------------------------
static PyObject *DateVar_GetValue(udt_DateVar *var, unsigned pos)
{
    return ceoTransform_dateFromSqlValue(&var->data[pos]);
}


//-----------------------------------------------------------------------------
// DateVar_SetValue()
//   Set the value of the variable.
//-----------------------------------------------------------------------------
static int DateVar_SetValue(udt_DateVar *var, unsigned pos, PyObject *value)
{
    return ceoTransform_sqlValueFromDate(value, &var->data[pos]);
}


//-----------------------------------------------------------------------------
// TimeVar_SetValue()
//   Set the value of the variable.
//-----------------------------------------------------------------------------
static int TimeVar_SetValue(udt_TimeVar *var, unsigned pos, PyObject *value)
{
    return ceoTransform_sqlValueFromTime(value, &var->data[pos]);
}


//-----------------------------------------------------------------------------
// TimeVar_GetValue()
//   Returns the value stored at the given array position.
//-----------------------------------------------------------------------------
static PyObject *TimeVar_GetValue(udt_TimeVar *var, unsigned pos)
{
    return ceoTransform_timeFromSqlValue(&var->data[pos]);
}


//-----------------------------------------------------------------------------
// TimestampVar_SetValue()
//   Set the value of the variable.
//-----------------------------------------------------------------------------
static int TimestampVar_SetValue(udt_TimestampVar *var, unsigned pos,
        PyObject *value)
{
    return ceoTransform_sqlValueFromTimestamp(value, &var->data[pos]);
}


//-----------------------------------------------------------------------------
// TimestampVar_GetValue()
//   Returns the value stored at the given array position.
//-----------------------------------------------------------------------------
static PyObject *TimestampVar_GetValue(udt_TimestampVar *var, unsigned pos)
{
    return ceoTransform_timestampFromSqlValue(&var->data[pos]);
}
