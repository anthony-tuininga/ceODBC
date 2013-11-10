//-----------------------------------------------------------------------------
// DateTimeVar.c
//   Defines the routines for handling date/time variables.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Declaration of variable structure.
//-----------------------------------------------------------------------------
typedef struct {
    Variable_HEAD
    DATE_STRUCT *data;
} udt_DateVar;


typedef struct {
    Variable_HEAD
    TIME_STRUCT *data;
} udt_TimeVar;


typedef struct {
    Variable_HEAD
    TIMESTAMP_STRUCT *data;
} udt_TimestampVar;


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
static PyTypeObject g_DateVarType = {
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


static PyTypeObject g_TimeVarType = {
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


static PyTypeObject g_TimestampVarType = {
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
static udt_VariableType vt_Date = {
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


static udt_VariableType vt_Time = {
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


static udt_VariableType vt_Timestamp = {
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
static PyObject *DateVar_GetValue(
    udt_DateVar *var,                   // variable to determine value for
    unsigned pos)                       // array position
{
    DATE_STRUCT *sqlValue;

    sqlValue = &var->data[pos];
    return PyDate_FromDate(sqlValue->year, sqlValue->month, sqlValue->day);
}


//-----------------------------------------------------------------------------
// DateVar_SetValue()
//   Set the value of the variable.
//-----------------------------------------------------------------------------
static int DateVar_SetValue(
    udt_DateVar *var,                   // variable to set value for
    unsigned pos,                       // array position to set
    PyObject *value)                    // value to set
{
    DATE_STRUCT *sqlValue;

    sqlValue = &var->data[pos];
    if (PyDateTime_Check(value) || PyDate_Check(value)) {
        sqlValue->year = PyDateTime_GET_YEAR(value);
        sqlValue->month = PyDateTime_GET_MONTH(value);
        sqlValue->day = PyDateTime_GET_DAY(value);
    } else {
        PyErr_SetString(PyExc_TypeError, "expecting date or datetime data");
        return -1;
    }

    return 0;
}


//-----------------------------------------------------------------------------
// TimeVar_SetValue()
//   Set the value of the variable.
//-----------------------------------------------------------------------------
static int TimeVar_SetValue(
    udt_TimeVar *var,              // variable to set value for
    unsigned pos,                       // array position to set
    PyObject *value)                    // value to set
{
    TIME_STRUCT *sqlValue;

    sqlValue = &var->data[pos];
    if (PyDateTime_Check(value)) {
        sqlValue->hour = PyDateTime_DATE_GET_HOUR(value);
        sqlValue->minute = PyDateTime_DATE_GET_MINUTE(value);
        sqlValue->second = PyDateTime_DATE_GET_SECOND(value);
    } else if (PyTime_Check(value)) {
        sqlValue->hour = PyDateTime_TIME_GET_HOUR(value);
        sqlValue->minute = PyDateTime_TIME_GET_MINUTE(value);
        sqlValue->second = PyDateTime_TIME_GET_SECOND(value);
    } else {
        PyErr_SetString(PyExc_TypeError, "expecting datetime or time data");
        return -1;
    }

    return 0;
}


//-----------------------------------------------------------------------------
// TimeVar_GetValue()
//   Returns the value stored at the given array position.
//-----------------------------------------------------------------------------
static PyObject *TimeVar_GetValue(
    udt_TimeVar *var,              // variable to determine value for
    unsigned pos)                       // array position
{
    TIME_STRUCT *sqlValue;

    sqlValue = &var->data[pos];
    return PyTime_FromTime(sqlValue->hour, sqlValue->minute, sqlValue->second,
            0);
}


//-----------------------------------------------------------------------------
// TimestampVar_SetValue()
//   Set the value of the variable.
//-----------------------------------------------------------------------------
static int TimestampVar_SetValue(
    udt_TimestampVar *var,              // variable to set value for
    unsigned pos,                       // array position to set
    PyObject *value)                    // value to set
{
    TIMESTAMP_STRUCT *sqlValue;

    sqlValue = &var->data[pos];
    sqlValue->fraction = 0;
    if (PyDateTime_Check(value)) {
        sqlValue->year = PyDateTime_GET_YEAR(value);
        sqlValue->month = PyDateTime_GET_MONTH(value);
        sqlValue->day = PyDateTime_GET_DAY(value);
        sqlValue->hour = PyDateTime_DATE_GET_HOUR(value);
        sqlValue->minute = PyDateTime_DATE_GET_MINUTE(value);
        sqlValue->second = PyDateTime_DATE_GET_SECOND(value);
    } else if (PyDate_Check(value)) {
        sqlValue->year = PyDateTime_GET_YEAR(value);
        sqlValue->month = PyDateTime_GET_MONTH(value);
        sqlValue->day = PyDateTime_GET_DAY(value);
        sqlValue->hour = 0;
        sqlValue->minute = 0;
        sqlValue->second = 0;
    } else {
        PyErr_SetString(PyExc_TypeError, "expecting date or datetime data");
        return -1;
    }

    return 0;
}


//-----------------------------------------------------------------------------
// TimestampVar_GetValue()
//   Returns the value stored at the given array position.
//-----------------------------------------------------------------------------
static PyObject *TimestampVar_GetValue(
    udt_TimestampVar *var,              // variable to determine value for
    unsigned pos)                       // array position
{
    TIMESTAMP_STRUCT *sqlValue;

    sqlValue = &var->data[pos];
    return PyDateTime_FromDateAndTime(sqlValue->year, sqlValue->month,
            sqlValue->day, sqlValue->hour, sqlValue->minute, sqlValue->second,
            sqlValue->fraction / 1000);
}

