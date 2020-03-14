//-----------------------------------------------------------------------------
// NumberVar.c
//   Defines the routines for all number like types.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Declaration of variable type structures
//-----------------------------------------------------------------------------
typedef struct {
    Variable_HEAD
    SQLBIGINT *data;
} udt_BigIntegerVar;


typedef struct {
    Variable_HEAD
    CEODBC_CHAR *data;
} udt_DecimalVar;


typedef struct {
    Variable_HEAD
    double *data;
} udt_DoubleVar;


typedef struct {
    Variable_HEAD
    SQLINTEGER *data;
} udt_IntegerVar;


//-----------------------------------------------------------------------------
// Declaration of variable functions
//-----------------------------------------------------------------------------
static PyObject *BigIntegerVar_GetValue(udt_BigIntegerVar*, unsigned);
static int BigIntegerVar_SetValue(udt_BigIntegerVar*, unsigned, PyObject*);

static SQLUINTEGER DecimalVar_GetBufferSize(udt_DecimalVar*, SQLUINTEGER);
static PyObject *DecimalVar_GetValue(udt_DecimalVar*, unsigned);
static int DecimalVar_SetValue(udt_DecimalVar*, unsigned, PyObject*);

static PyObject *DoubleVar_GetValue(udt_DoubleVar*, unsigned);
static int DoubleVar_SetValue(udt_DoubleVar*, unsigned, PyObject*);

static PyObject *IntegerVar_GetValue(udt_IntegerVar*, unsigned);
static int IntegerVar_SetValue(udt_IntegerVar*, unsigned, PyObject*);


//-----------------------------------------------------------------------------
// Declaration of Python types
//-----------------------------------------------------------------------------
static PyTypeObject g_BigIntegerVarType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "ceODBC.BigIntegerVar",             // tp_name
    sizeof(udt_BigIntegerVar),          // tp_basicsize
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


static PyTypeObject g_DecimalVarType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "ceODBC.DecimalVar",                // tp_name
    sizeof(udt_DecimalVar),             // tp_basicsize
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
    (initproc) Variable_InitWithScale,  // tp_init
    0,                                  // tp_alloc
    (newfunc) Variable_New,             // tp_new
    0,                                  // tp_free
    0,                                  // tp_is_gc
    0                                   // tp_bases
};


static PyTypeObject g_DoubleVarType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "ceODBC.DoubleVar",                 // tp_name
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


static PyTypeObject g_IntegerVarType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "ceODBC.IntegerVar",                // tp_name
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
// Declaration of variable types
//-----------------------------------------------------------------------------
static udt_VariableType vt_BigInteger = {
    (SetValueProc) BigIntegerVar_SetValue,
    (GetValueProc) BigIntegerVar_GetValue,
    (GetBufferSizeProc) NULL,
    &g_BigIntegerVarType,               // Python type
    SQL_BIGINT,                         // SQL type
    SQL_C_SBIGINT,                      // C data type
    sizeof(SQLBIGINT),                  // buffer size
    19,                                 // default size
    0                                   // default scale
};


static udt_VariableType vt_Decimal = {
    (SetValueProc) DecimalVar_SetValue,
    (GetValueProc) DecimalVar_GetValue,
    (GetBufferSizeProc) DecimalVar_GetBufferSize,
    &g_DecimalVarType,                  // Python type
#if PY_MAJOR_VERSION >= 3
    SQL_WCHAR,                          // SQL type
    SQL_C_WCHAR,                        // C data type
#else
    SQL_CHAR,                           // SQL type
    SQL_C_CHAR,                         // C data type
#endif
    0,                                  // buffer size
    18,                                 // default size
    0                                   // default scale
};


static udt_VariableType vt_Double = {
    (SetValueProc) DoubleVar_SetValue,
    (GetValueProc) DoubleVar_GetValue,
    (GetBufferSizeProc) NULL,
    &g_DoubleVarType,                   // Python type
    SQL_DOUBLE,                         // SQL type
    SQL_C_DOUBLE,                       // C data type
    sizeof(SQLDOUBLE),                  // buffer size
    53,                                 // default size
    0                                   // default scale
};


static udt_VariableType vt_Integer = {
    (SetValueProc) IntegerVar_SetValue,
    (GetValueProc) IntegerVar_GetValue,
    (GetBufferSizeProc) NULL,
    &g_IntegerVarType,                  // Python type
    SQL_INTEGER,                        // SQL type
    SQL_C_LONG,                         // C data type
    sizeof(SQLINTEGER),                 // buffer size
    10,                                 // default size
    0                                   // default scale
};


//-----------------------------------------------------------------------------
// BigIntegerVar_GetValue()
//   Returns the value stored at the given array position.
//-----------------------------------------------------------------------------
static PyObject *BigIntegerVar_GetValue(
    udt_BigIntegerVar *var,             // variable to determine value for
    unsigned pos)                       // array position
{
    return PyLong_FromLongLong(var->data[pos]);
}


//-----------------------------------------------------------------------------
// BigIntegerVar_SetValue()
//   Set the value of the variable.
//-----------------------------------------------------------------------------
static int BigIntegerVar_SetValue(
    udt_BigIntegerVar *var,             // variable to set value for
    unsigned pos,                       // array position to set
    PyObject *value)                    // value to set
{
    if (PyInt_Check(value)) {
#if PY_MAJOR_VERSION >= 3
        var->data[pos] = PyLong_AsLongLong(value);
#else
        var->data[pos] = PyInt_AsLong(value);
#endif
        if (PyErr_Occurred())
            return -1;
    } else if (PyLong_Check(value)) {
        var->data[pos] = PyLong_AsLongLong(value);
        if (PyErr_Occurred())
            return -1;
    } else {
        PyErr_Format(PyExc_TypeError,
                "expecting integer data, got value of type %s instead",
                Py_TYPE(value)->tp_name);
        return -1;
    }

    return 0;
}


//-----------------------------------------------------------------------------
// DecimalVar_GetStringRepOfDecimal()
//   Return the string representation of a decimal value given the tuple value.
//-----------------------------------------------------------------------------
static int DecimalVar_GetStringRepOfDecimal(
    udt_DecimalVar *var,                // variable to set value for
    unsigned pos,                       // array position to set
    PyObject *tupleValue)               // tuple value to parse
{
    long numDigits, scale, i, sign, size, digit;
    char *valuePtr, *value;
    PyObject *digits;
#if PY_MAJOR_VERSION >= 3
    udt_StringBuffer buffer;
    PyObject *temp;
#endif

    // acquire basic information from the value tuple
    sign = PyInt_AsLong(PyTuple_GET_ITEM(tupleValue, 0));
    if (PyErr_Occurred())
        return -1;
    digits = PyTuple_GET_ITEM(tupleValue, 1);
    scale = PyInt_AsLong(PyTuple_GET_ITEM(tupleValue, 2));
    if (PyErr_Occurred())
        return -1;
    numDigits = PyTuple_GET_SIZE(digits);

    // resize the variable if needed
    size = numDigits + abs(scale);
    if (size > var->size) {
        if (Variable_Resize((udt_Variable*) var, size) < 0)
            return -1;
    }

    // populate the string and format
    value = valuePtr = (char*) var->data + pos * var->bufferSize;
    if (sign)
        *valuePtr++ = '-';
    for (i = 0; i < numDigits + scale; i++) {
        if (i < numDigits) {
            digit = PyInt_AsLong(PyTuple_GetItem(digits, i));
            if (PyErr_Occurred())
                return -1;
        } else digit = 0;
        *valuePtr++ = '0' + (char) digit;
    }
    if (scale < 0) {
        *valuePtr++ = '.';
        for (i = scale; i < 0; i++) {
            if (numDigits + i < 0)
                digit = 0;
            else {
                digit = PyInt_AsLong(PyTuple_GetItem(digits, numDigits + i));
                if (PyErr_Occurred())
                    return -1;
            }
            *valuePtr++ = '0' + (char) digit;
        }
    }
    *valuePtr = '\0';
#if PY_MAJOR_VERSION >= 3
    temp = ceString_FromAscii(value);
    if (!temp)
        return -1;
    if (StringBuffer_FromString(&buffer, temp, "unexpected error!") < 0) {
        Py_DECREF(temp);
        return -1;
    }
    Py_DECREF(temp);
    var->lengthOrIndicator[pos] = (SQLINTEGER) buffer.sizeInBytes;
    memcpy(value, buffer.ptr, buffer.sizeInBytes);
    StringBuffer_Clear(&buffer);
#else
    var->lengthOrIndicator[pos] = strlen(value);
#endif

    return 0;
}


//-----------------------------------------------------------------------------
// DecimalVar_GetBufferSize()
//   Returns the size to use for buffers. Since we are using strings to go back
// and forth between the database and Python we need to include room for the
// NULL terminator, the + or - sign and the decimal point.
//-----------------------------------------------------------------------------
static SQLUINTEGER DecimalVar_GetBufferSize(
    udt_DecimalVar *var,                // variable to determine value for
    SQLUINTEGER size)                   // size to allocate
{
    return (size + 3) * sizeof(CEODBC_CHAR);
}


//-----------------------------------------------------------------------------
// DecimalVar_GetValue()
//   Returns the value stored at the given array position.
//-----------------------------------------------------------------------------
static PyObject *DecimalVar_GetValue(
    udt_DecimalVar *var,                // variable to determine value for
    unsigned pos)                       // array position
{
    PyObject *obj, *result;

    obj = ceString_FromStringAndSizeInBytes((char*) var->data + pos *
            var->bufferSize, var->lengthOrIndicator[pos]);
    if (!obj)
        return NULL;
    result = PyObject_CallFunctionObjArgs(g_DecimalType, obj, NULL);
    Py_DECREF(obj);
    return result;
}


//-----------------------------------------------------------------------------
// DecimalVar_SetValue()
//   Set the value of the variable.
//-----------------------------------------------------------------------------
static int DecimalVar_SetValue(
    udt_DecimalVar *var,                // variable to set value for
    unsigned pos,                       // array position to set
    PyObject *value)                    // value to set
{
    PyObject *tupleValue;

    if (Py_TYPE(value) != (PyTypeObject*) g_DecimalType) {
        PyErr_SetString(PyExc_TypeError, "expecting decimal object");
        return -1;
    }

    tupleValue = PyObject_CallMethod(value, "as_tuple", NULL);
    if (!tupleValue)
        return -1;
    if (DecimalVar_GetStringRepOfDecimal(var, pos, tupleValue) < 0) {
        Py_DECREF(tupleValue);
        return -1;
    }
    Py_DECREF(tupleValue);
    return 0;
}


//-----------------------------------------------------------------------------
// DoubleVar_GetValue()
//   Returns the value stored at the given array position.
//-----------------------------------------------------------------------------
static PyObject *DoubleVar_GetValue(
    udt_DoubleVar *var,                 // variable to determine value for
    unsigned pos)                       // array position
{
    return PyFloat_FromDouble(var->data[pos]);
}


//-----------------------------------------------------------------------------
// DoubleVar_SetValue()
//   Set the value of the variable.
//-----------------------------------------------------------------------------
static int DoubleVar_SetValue(
    udt_DoubleVar *var,                 // variable to set value for
    unsigned pos,                       // array position to set
    PyObject *value)                    // value to set
{
    if (PyFloat_Check(value))
        var->data[pos] = PyFloat_AS_DOUBLE(value);
    else if (PyInt_Check(value)) {
        var->data[pos] = PyInt_AsLong(value);
        if (PyErr_Occurred())
            return -1;
    } else {
        PyErr_Format(PyExc_TypeError,
                "expecting floating point data, got value of type %s instead",
                Py_TYPE(value)->tp_name);
        return -1;
    }

    return 0;
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
        var->data[pos] = PyInt_AsLong(value);
        if (PyErr_Occurred())
            return -1;
        return 0;
    }
    PyErr_Format(PyExc_TypeError,
            "expecting integer data, got value of type %s instead",
            Py_TYPE(value)->tp_name);
    return -1;
}

