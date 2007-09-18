//-----------------------------------------------------------------------------
// Variable.c
//   Defines Python types for variables.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// define structure common to all variables
//-----------------------------------------------------------------------------
struct _udt_VariableType;
#define Variable_HEAD \
    PyObject_HEAD \
    SQLSMALLINT position; \
    SQLINTEGER numElements; \
    SQLINTEGER *lengthOrIndicator; \
    struct _udt_VariableType *type; \
    SQLUINTEGER size; \
    SQLUINTEGER bufferSize; \
    SQLSMALLINT scale; \
    int input; \
    int output;
typedef struct {
    Variable_HEAD
    void *data;
} udt_Variable;


//-----------------------------------------------------------------------------
// define function types for the common actions that take place on a variable
//-----------------------------------------------------------------------------
typedef int (*SetValueProc)(udt_Variable*, unsigned, PyObject*);
typedef PyObject * (*GetValueProc)(udt_Variable*, unsigned);
typedef SQLUINTEGER (*GetBufferSizeProc)(udt_Variable*, SQLUINTEGER);


//-----------------------------------------------------------------------------
// define structure for the common actions that take place on a variable
//-----------------------------------------------------------------------------
typedef struct _udt_VariableType {
    SetValueProc setValueProc;
    GetValueProc getValueProc;
    GetBufferSizeProc getBufferSizeProc;
    PyTypeObject *pythonType;
    SQLSMALLINT sqlDataType;
    SQLSMALLINT cDataType;
    SQLUINTEGER bufferSize;
    SQLUINTEGER defaultSize;
    SQLSMALLINT defaultScale;
} udt_VariableType;


//-----------------------------------------------------------------------------
// Declaration of common variable functions.
//-----------------------------------------------------------------------------
static PyObject *Variable_New(PyTypeObject*, PyObject*, PyObject*);
static void Variable_Free(udt_Variable *);
static PyObject *Variable_Repr(udt_Variable *);
static int Variable_Resize(udt_Variable*, SQLUINTEGER);
static int Variable_SetValue(udt_Variable*, unsigned, PyObject*);
static udt_VariableType *Variable_TypeByPythonType(PyObject*);
static PyObject *Variable_ExternalGetValue(udt_Variable*, PyObject*,
        PyObject*);
static PyObject *Variable_ExternalSetValue(udt_Variable*, PyObject*);


//-----------------------------------------------------------------------------
// Declaration of variable members
//-----------------------------------------------------------------------------
static PyMemberDef g_VariableMembers[] = {
    { "bufferSize", T_INT, offsetof(udt_Variable, bufferSize), READONLY },
    { "input", T_INT, offsetof(udt_Variable, input), 0 },
    { "numElements", T_INT, offsetof(udt_Variable, numElements), READONLY },
    { "output", T_INT, offsetof(udt_Variable, output), 0 },
    { "scale", T_INT, offsetof(udt_Variable, scale), READONLY },
    { "size", T_INT, offsetof(udt_Variable, size), READONLY },
    { NULL }
};


//-----------------------------------------------------------------------------
// Declaration of variable methods
//-----------------------------------------------------------------------------
static PyMethodDef g_VariableMethods[] = {
    { "getvalue", (PyCFunction) Variable_ExternalGetValue,
            METH_VARARGS  | METH_KEYWORDS },
    { "setvalue", (PyCFunction) Variable_ExternalSetValue, METH_VARARGS },
    { NULL, NULL }
};


//-----------------------------------------------------------------------------
// Variable_InternalInit()
//   Internal method of initializing a new variable.
//-----------------------------------------------------------------------------
static int Variable_InternalInit(
    udt_Variable *self,                 // variable to initialize
    unsigned numElements,               // number of elements to allocate
    udt_VariableType *type,             // variable type
    SQLUINTEGER size,                   // size of variable
    SQLSMALLINT scale,                  // scale of variable
    PyObject *value,                    // value to set (optional)
    int input,                          // input variable?
    int output)                         // output variable?
{
    unsigned PY_LONG_LONG dataLength;
    SQLUINTEGER i;

    // perform basic initialization
    self->position = -1;
    if (numElements < 1)
        self->numElements = 1;
    else self->numElements = numElements;
    self->size = size;
    if (type->getBufferSizeProc)
        self->bufferSize = (*type->getBufferSizeProc)(self, size);
    else self->bufferSize = type->bufferSize;
    self->scale = scale;
    self->type = type;
    self->input = input;
    self->output = output;
    self->lengthOrIndicator = NULL;
    self->data = NULL;

    // allocate the indicator and data arrays
    dataLength = (unsigned PY_LONG_LONG) numElements *
            (unsigned PY_LONG_LONG) self->bufferSize;
    if (dataLength > INT_MAX) {
        PyErr_SetString(PyExc_ValueError, "array size too large");
        return -1;
    }
    self->lengthOrIndicator = PyMem_Malloc(numElements * sizeof(SQLINTEGER));
    self->data = PyMem_Malloc((size_t) dataLength);
    if (!self->lengthOrIndicator || !self->data) {
        PyErr_NoMemory();
        return -1;
    }

    // ensure that all variable values start out NULL
    for (i = 0; i < numElements; i++)
        self->lengthOrIndicator[i] = SQL_NULL_DATA;

    // set value, if applicable
    if (value && Variable_SetValue(self, 0, value) < 0)
        return -1;

    return 0;
}


//-----------------------------------------------------------------------------
// Variable_DefaultInit()
//   Default constructor.
//-----------------------------------------------------------------------------
static int Variable_DefaultInit(
    udt_Variable *self,                 // variable being constructed
    PyObject *args,                     // arguments
    PyObject *keywordArgs)              // keyword arguments
{
    udt_VariableType *varType;
    PyObject *value;
    int numElements;

    static char *keywordList[] = { "value", "numElements", NULL };

    value = NULL;
    numElements = 1;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|Oi", keywordList,
            &value, &numElements))
        return -1;
    varType = Variable_TypeByPythonType((PyObject*) self->ob_type);
    if (!varType)
        return -1;
    if (!Variable_InternalInit(self, numElements, varType,
            varType->defaultSize, varType->defaultScale, value, 1, 1) < 0)
        return -1;

    return 0;
}


//-----------------------------------------------------------------------------
// Variable_InitWithScale()
//   Constructor which accepts scale as an argument as well.
//-----------------------------------------------------------------------------
static int Variable_InitWithScale(
    udt_Variable *self,                 // variable being constructed
    PyObject *args,                     // arguments
    PyObject *keywordArgs)              // keyword arguments
{
    udt_VariableType *varType;
    int numElements, scale;
    PyObject *value;

    static char *keywordList[] = { "value", "scale", "numElements", NULL };

    varType = Variable_TypeByPythonType((PyObject*) self->ob_type);
    if (!varType)
        return -1;
    value = NULL;
    numElements = 1;
    scale = varType->defaultScale;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|Oii", keywordList,
            &value, &scale, &numElements))
        return -1;
    if (!Variable_InternalInit(self, numElements, varType,
            varType->defaultSize, scale, value, 1, 1) < 0)
        return -1;

    return 0;
}


//-----------------------------------------------------------------------------
// Variable_InitWithSize()
//   Constructor which accepts scale as an argument as well.
//-----------------------------------------------------------------------------
static int Variable_InitWithSize(
    udt_Variable *self,                 // variable being constructed
    PyObject *args,                     // arguments
    PyObject *keywordArgs)              // keyword arguments
{
    udt_VariableType *varType;
    int numElements, size;
    PyObject *value;

    static char *keywordList[] = { "value", "size", "numElements", NULL };

    varType = Variable_TypeByPythonType((PyObject*) self->ob_type);
    if (!varType)
        return -1;
    value = NULL;
    numElements = 1;
    size = varType->defaultSize;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|Oii", keywordList,
            &value, &size, &numElements))
        return -1;
    if (!Variable_InternalInit(self, numElements, varType, size,
            varType->defaultScale, value, 1, 1) < 0)
        return -1;

    return 0;
}


#include "BinaryVar.c"
#include "BitVar.c"
#include "NumberVar.c"
#include "StringVar.c"
#include "DateTimeVar.c"


//-----------------------------------------------------------------------------
// Variable_InternalNew()
//   Internal method of creating a new variable.
//-----------------------------------------------------------------------------
static udt_Variable *Variable_InternalNew(
    unsigned numElements,               // number of elements to allocate
    udt_VariableType *type,             // variable type
    SQLUINTEGER size,                   // size of variable
    SQLSMALLINT scale)                  // scale of variable
{
    udt_Variable *self;

    self = PyObject_NEW(udt_Variable, type->pythonType);
    if (!self)
        return NULL;
    if (Variable_InternalInit(self, numElements, type, size, scale,
            NULL, 1, 0) < 0) {
        Py_DECREF(self);
        return NULL;
    }

    return self;
}


//-----------------------------------------------------------------------------
// Variable_TypeByValue()
//   Return a variable type given a Python object or NULL if the Python
// object does not have a corresponding variable type.
//-----------------------------------------------------------------------------
static udt_VariableType *Variable_TypeByValue(
    PyObject* value)                    // Python type
{
    if (value == Py_None)
        return &vt_String;
    if (PyString_Check(value))
        return &vt_String;
    if (PyBuffer_Check(value))
        return &vt_Binary;
    if (PyBool_Check(value))
        return &vt_Bit;
    if (PyInt_Check(value))
        return &vt_Integer;
    if (PyLong_Check(value))
        return &vt_BigInteger;
    if (PyFloat_Check(value))
        return &vt_Double;
    if (value->ob_type == (PyTypeObject*) g_DecimalType)
        return &vt_Decimal;
    if (PyTime_Check(value))
        return &vt_Time;
    if (PyDateTime_Check(value))
        return &vt_Timestamp;
    if (PyDate_Check(value))
        return &vt_Timestamp;

    PyErr_Format(g_NotSupportedErrorException,
            "Variable_TypeByValue(): unhandled data type %s",
            value->ob_type->tp_name);
    return NULL;
}


//-----------------------------------------------------------------------------
// Variable_TypeByPythonType()
//   Return a variable type given a Python type object or NULL if the Python
// type does not have a corresponding variable type.
//-----------------------------------------------------------------------------
static udt_VariableType *Variable_TypeByPythonType(
    PyObject* type)                     // Python type
{
    if (type == (PyObject*) &g_StringVarType)
        return &vt_String;
    if (type == (PyObject*) &PyString_Type)
        return &vt_String;
    if (type == (PyObject*) g_StringApiType)
        return &vt_String;
    if (type == (PyObject*) &g_LongStringVarType)
        return &vt_LongString;
    if (type == (PyObject*) &g_BinaryVarType)
        return &vt_Binary;
    if (type == (PyObject*) &PyBuffer_Type)
        return &vt_Binary;
    if (type == (PyObject*) g_BinaryApiType)
        return &vt_Binary;
    if (type == (PyObject*) &g_LongBinaryVarType)
        return &vt_LongBinary;
    if (type == (PyObject*) &g_BitVarType)
        return &vt_Bit;
    if (type == (PyObject*) &PyBool_Type)
        return &vt_Bit;
    if (type == (PyObject*) &g_BigIntegerVarType)
        return &vt_BigInteger;
    if (type == (PyObject*) &PyLong_Type)
        return &vt_BigInteger;
    if (type == (PyObject*) &g_IntegerVarType)
        return &vt_Integer;
    if (type == (PyObject*) &PyInt_Type)
        return &vt_Integer;
    if (type == (PyObject*) &g_DoubleVarType)
        return &vt_Double;
    if (type == (PyObject*) &PyFloat_Type)
        return &vt_Double;
    if (type == (PyObject*) g_NumberApiType)
        return &vt_Double;
    if (type == (PyObject*) &g_DecimalVarType)
        return &vt_Decimal;
    if (type == g_DecimalType)
        return &vt_Decimal;
    if (type == (PyObject*) &g_DateVarType)
        return &vt_Date;
    if (type == (PyObject*) PyDateTimeAPI->DateType)
        return &vt_Date;
    if (type == (PyObject*) &g_TimeVarType)
        return &vt_Time;
    if (type == (PyObject*) PyDateTimeAPI->TimeType)
        return &vt_Time;
    if (type == (PyObject*) &g_TimestampVarType)
        return &vt_Timestamp;
    if (type == (PyObject*) PyDateTimeAPI->DateTimeType)
        return &vt_Timestamp;
    if (type == (PyObject*) g_DateTimeApiType)
        return &vt_Timestamp;

    PyErr_SetString(g_NotSupportedErrorException,
            "Variable_TypeByPythonType(): unhandled data type");
    return NULL;
}


//-----------------------------------------------------------------------------
// Variable_TypeBySqlDataType()
//   Return a variable type given a SQL data type or NULL if the SQL data type
// does not have a corresponding variable type.
//-----------------------------------------------------------------------------
static udt_VariableType *Variable_TypeBySqlDataType (
    SQLSMALLINT sqlDataType)            // SQL data type
{
    char buffer[100];

    switch(sqlDataType) {
        case SQL_BIGINT:
            return &vt_BigInteger;
        case SQL_BIT:
            return &vt_Bit;
        case SQL_SMALLINT:
        case SQL_TINYINT:
        case SQL_INTEGER:
            return &vt_Integer;
        case SQL_REAL:
        case SQL_FLOAT:
        case SQL_DOUBLE:
            return &vt_Double;
        case SQL_DECIMAL:
        case SQL_NUMERIC:
            return &vt_Decimal;
        case SQL_TYPE_DATE:
            return &vt_Date;
        case SQL_TYPE_TIME:
            return &vt_Time;
        case SQL_TYPE_TIMESTAMP:
            return &vt_Timestamp;
        case SQL_CHAR:
        case SQL_WCHAR:
        case SQL_VARCHAR:
        case SQL_WVARCHAR:
        case SQL_GUID:
            return &vt_String;
        case SQL_LONGVARCHAR:
        case SQL_WLONGVARCHAR:
            return &vt_LongString;
        case SQL_BINARY:
        case SQL_VARBINARY:
            return &vt_Binary;
        case SQL_LONGVARBINARY:
            return &vt_LongBinary;
    }

    sprintf(buffer, "Variable_TypeBySqlDataType: unhandled data type %d",
            sqlDataType);
    PyErr_SetString(g_NotSupportedErrorException, buffer);
    return NULL;
}


//-----------------------------------------------------------------------------
// Variable_New()
//   Create a new cursor object.
//-----------------------------------------------------------------------------
static PyObject *Variable_New(
    PyTypeObject *type,                 // type object
    PyObject *args,                     // arguments
    PyObject *keywordArgs)              // keyword arguments
{
    return type->tp_alloc(type, 0);
}


//-----------------------------------------------------------------------------
// Variable_Free()
//   Free an existing variable.
//-----------------------------------------------------------------------------
static void Variable_Free(
    udt_Variable *self)                 // variable to free
{
    if (self->lengthOrIndicator)
        PyMem_Free(self->lengthOrIndicator);
    if (self->data)
        PyMem_Free(self->data);
    self->ob_type->tp_free((PyObject*) self);
}


//-----------------------------------------------------------------------------
// Variable_Check()
//   Returns a boolean indicating if the object is a variable.
//-----------------------------------------------------------------------------
static int Variable_Check(
    PyObject *object)                   // Python object to check
{
    return (object->ob_type == &g_BigIntegerVarType ||
            object->ob_type == &g_BinaryVarType ||
            object->ob_type == &g_BitVarType ||
            object->ob_type == &g_DateVarType ||
            object->ob_type == &g_DecimalVarType ||
            object->ob_type == &g_DoubleVarType ||
            object->ob_type == &g_IntegerVarType ||
            object->ob_type == &g_LongBinaryVarType ||
            object->ob_type == &g_LongStringVarType ||
            object->ob_type == &g_TimestampVarType ||
            object->ob_type == &g_StringVarType);
}


//-----------------------------------------------------------------------------
// Variable_NewByValue()
//   Allocate a new variable by looking at the type of the data.
//-----------------------------------------------------------------------------
static udt_Variable *Variable_NewByValue(
    udt_Cursor *cursor,                 // cursor to associate variable with
    PyObject *value,                    // Python value to associate
    unsigned numElements)               // number of elements to allocate
{
    udt_VariableType *varType;
    udt_Variable *var;
    SQLUINTEGER size;

    varType = Variable_TypeByValue(value);
    if (!varType)
        return NULL;
    if (value == Py_None)
        size = 1;
    else if (PyString_Check(value))
        size = PyString_GET_SIZE(value);
    else size = varType->defaultSize;
    var = Variable_InternalNew(numElements, varType, size,
            varType->defaultScale);
    if (!var)
        return NULL;

    return var;
}


//-----------------------------------------------------------------------------
// Variable_NewByType()
//   Allocate a new variable by looking at the Python data type.
//-----------------------------------------------------------------------------
static udt_Variable *Variable_NewByType(
    udt_Cursor *cursor,                 // cursor to associate variable with
    PyObject *value,                    // Python data type to associate
    unsigned numElements)               // number of elements to allocate
{
    udt_VariableType *varType;
    int size;

    // passing an integer is assumed to be a string
    if (PyInt_Check(value)) {
        size = PyInt_AS_LONG(value);
        return Variable_InternalNew(numElements, &vt_String, size, 0);
    }

    // handle directly bound variables
    if (Variable_Check(value)) {
        Py_INCREF(value);
        return (udt_Variable*) value;
    }

    // everything else ought to be a Python type
    varType = Variable_TypeByPythonType(value);
    if (!varType)
        return NULL;
    return Variable_InternalNew(numElements, varType, varType->defaultSize,
            varType->defaultScale);
}


//-----------------------------------------------------------------------------
// Variable_NewForResultSet()
//   Create a new variable for the given position in the result set. The new
// variable is immediately bound to the statement as well.
//-----------------------------------------------------------------------------
static udt_Variable *Variable_NewForResultSet(
    udt_Cursor *cursor,                 // cursor in use
    SQLUSMALLINT position)              // position in define list
{
    SQLSMALLINT dataType, length, scale, nullable;
    udt_VariableType *varType;
    udt_Variable *var;
    SQLUINTEGER size;
    SQLRETURN rc;
    char name[1];

    // retrieve information about the column
    rc = SQLDescribeCol(cursor->handle, position, (SQLCHAR*) name,
            sizeof(name), &length, &dataType, &size, &scale,
            &nullable);
    if (CheckForError(cursor, rc,
                "Variable_NewForResultSet(): get column info") < 0)
        return NULL;

    // determine data type
    varType = Variable_TypeBySqlDataType(dataType);
    if (!varType)
        return NULL;

    // for long columns, set the size appropriately
    if (varType == &vt_LongString || varType == &vt_LongBinary) {
        if (cursor->setOutputSize > 0 &&
                (cursor->setOutputSizeColumn == 0 ||
                 position == cursor->setOutputSizeColumn))
            size = cursor->setOutputSize;
        else size = varType->defaultSize;
    }

    // create a variable of the correct type
    var = Variable_InternalNew(cursor->fetchArraySize, varType, size, scale);
    if (!var)
        return NULL;

    // bind the column
    var->position = position;
    rc = SQLBindCol(cursor->handle, position, var->type->cDataType,
            var->data, var->bufferSize, var->lengthOrIndicator);
    if (CheckForError(cursor, rc, "Variable_NewForResultSet(): bind()") < 0) {
        Py_DECREF(var);
        return NULL;
    }

    return var;
}


//-----------------------------------------------------------------------------
// Variable_BindParameter()
//   Allocate a variable and bind it to the given statement.
//-----------------------------------------------------------------------------
static int Variable_BindParameter(
    udt_Variable *self,                 // variable to bind
    udt_Cursor *cursor,                 // cursor to bind to
    SQLUSMALLINT position)              // position to bind to
{
    SQLSMALLINT inputOutputType;
    SQLRETURN rc;

    self->position = position;
    if (self->input && self->output)
        inputOutputType = SQL_PARAM_INPUT_OUTPUT;
    else if (self->output)
        inputOutputType = SQL_PARAM_OUTPUT;
    else inputOutputType = SQL_PARAM_INPUT;
    rc = SQLBindParameter(cursor->handle, position, inputOutputType,
            self->type->cDataType, self->type->sqlDataType, self->size,
            self->scale, self->data, self->bufferSize,
            self->lengthOrIndicator);
    if (CheckForError(cursor, rc, "Variable_BindParameter()") < 0)
        return -1;

    return 0;
}


//-----------------------------------------------------------------------------
// Variable_Resize()
//   Resize the variable.
//-----------------------------------------------------------------------------
static int Variable_Resize(
    udt_Variable *self,                 // variable to resize
    SQLUINTEGER newSize)                // new length to use
{
    SQLUINTEGER newBufferSize;
    char *newData;
    SQLINTEGER i;

    // allocate new memory for the larger size
    newBufferSize = (*self->type->getBufferSizeProc)(self, newSize);
    newData = (char*) PyMem_Malloc(self->numElements * newBufferSize);
    if (!newData) {
        PyErr_NoMemory();
        return -1;
    }

    // copy the data from the original array to the new array
    for (i = 0; i < self->numElements; i++)
        memcpy(newData + newBufferSize * i,
                (void*) ( (char*) self->data + self->bufferSize * i ),
                self->bufferSize);
    PyMem_Free(self->data);
    self->data = newData;
    self->size = newSize;
    self->bufferSize = newBufferSize;

    // force rebinding
    self->position = -1;

    return 0;
}


//-----------------------------------------------------------------------------
// Variable_GetValue()
//   Return the value of the variable at the given position.
//-----------------------------------------------------------------------------
static PyObject *Variable_GetValue(
    udt_Variable *self,                 // variable to get the value for
    unsigned arrayPos)                  // array position
{
    // ensure we do not exceed the number of allocated elements
    if (arrayPos >= self->numElements) {
        PyErr_SetString(PyExc_IndexError,
                "Variable_GetSingleValue: array size exceeded");
        return NULL;
    }

    // check for a NULL value
    if (self->lengthOrIndicator[arrayPos] == SQL_NULL_DATA) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    // check for truncation
    if (self->lengthOrIndicator[arrayPos] > self->bufferSize)
        return PyErr_Format(g_DatabaseErrorException,
                "column %d (%d) truncated (need %lu, have %lu)",
                self->position, arrayPos, self->lengthOrIndicator[arrayPos],
                self->bufferSize);

    return (*self->type->getValueProc)(self, arrayPos);
}


//-----------------------------------------------------------------------------
// Variable_SetValue()
//   Set the value of the variable at the given position.
//-----------------------------------------------------------------------------
static int Variable_SetValue(
    udt_Variable *self,                 // variable to set value for
    unsigned arrayPos,                  // array position
    PyObject *value)                    // value to set
{
    // ensure we do not exceed the number of allocated elements
    if (arrayPos >= self->numElements) {
        PyErr_SetString(PyExc_IndexError,
                "Variable_SetSingleValue: array size exceeded");
        return -1;
    }

    // check for a NULL value
    if (value == Py_None) {
        self->lengthOrIndicator[arrayPos] = SQL_NULL_DATA;
        return 0;
    }

    self->lengthOrIndicator[arrayPos] = 0;
    return (*self->type->setValueProc)(self, arrayPos, value);
}


//-----------------------------------------------------------------------------
// Variable_ExternalGetValue()
//   Return the value of the variable at the given position.
//-----------------------------------------------------------------------------
static PyObject *Variable_ExternalGetValue(
    udt_Variable *self,                 // variable to get value for
    PyObject *args,                     // arguments
    PyObject *keywordArgs)              // keyword arguments
{
    static char *keywordList[] = { "pos", NULL };
    unsigned pos = 0;

    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|i", keywordList,
            &pos))
        return NULL;
    return Variable_GetValue(self, pos);
}


//-----------------------------------------------------------------------------
// Variable_ExternalSetValue()
//   Set the value of the variable at the given position.
//-----------------------------------------------------------------------------
static PyObject *Variable_ExternalSetValue(
    udt_Variable *self,                 // variable to set value for
    PyObject *args)                     // arguments
{
    PyObject *value;
    unsigned pos;

    if (!PyArg_ParseTuple(args, "iO", &pos, &value))
      return NULL;
    if (Variable_SetValue(self, pos, value) < 0)
      return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// Variable_Repr()
//   Return a string representation of the variable.
//-----------------------------------------------------------------------------
static PyObject *Variable_Repr(
    udt_Variable *self)                 // variable to return the string for
{
    PyObject *valueRepr, *value, *module, *name, *result;

    value = Variable_GetValue(self, 0);
    if (!value)
        return NULL;
    valueRepr = PyObject_Repr(value);
    Py_DECREF(value);
    if (!valueRepr)
        return NULL;
    if (GetModuleAndName(self->ob_type, &module, &name) < 0) {
        Py_DECREF(valueRepr);
        return NULL;
    }
    result = PyString_FromFormat("<%s.%s with value %s>",
            PyString_AS_STRING(module), PyString_AS_STRING(name),
            PyString_AS_STRING(valueRepr));
    Py_DECREF(valueRepr);
    Py_DECREF(module);
    Py_DECREF(name);
    return result;
}

