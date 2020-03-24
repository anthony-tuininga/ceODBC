//-----------------------------------------------------------------------------
// Variable.c
//   Defines Python types for variables.
//-----------------------------------------------------------------------------

#include "ceoModule.h"

//-----------------------------------------------------------------------------
// Declaration of common variable functions.
//-----------------------------------------------------------------------------
static PyObject *Variable_ExternalGetValue(udt_Variable*, PyObject*,
        PyObject*);
static PyObject *Variable_ExternalSetValue(udt_Variable*, PyObject*);


//-----------------------------------------------------------------------------
// Declaration of variable members
//-----------------------------------------------------------------------------
PyMemberDef g_VariableMembers[] = {
    { "bufferSize", T_INT, offsetof(udt_Variable, bufferSize), READONLY },
    { "inconverter", T_OBJECT, offsetof(udt_Variable, inConverter), 0 },
    { "input", T_INT, offsetof(udt_Variable, input), 0 },
    { "numElements", T_INT, offsetof(udt_Variable, numElements), READONLY },
    { "outconverter", T_OBJECT, offsetof(udt_Variable, outConverter), 0 },
    { "output", T_INT, offsetof(udt_Variable, output), 0 },
    { "scale", T_INT, offsetof(udt_Variable, scale), READONLY },
    { "size", T_INT, offsetof(udt_Variable, size), READONLY },
    { NULL }
};


//-----------------------------------------------------------------------------
// Declaration of variable methods
//-----------------------------------------------------------------------------
PyMethodDef g_VariableMethods[] = {
    { "getvalue", (PyCFunction) Variable_ExternalGetValue,
            METH_VARARGS  | METH_KEYWORDS },
    { "setvalue", (PyCFunction) Variable_ExternalSetValue, METH_VARARGS },
    { NULL, NULL }
};


//-----------------------------------------------------------------------------
// Variable_InternalInit()
//   Internal method of initializing a new variable.
//-----------------------------------------------------------------------------
static int Variable_InternalInit(udt_Variable *self, unsigned numElements,
        udt_VariableType *type, SQLUINTEGER size, SQLSMALLINT scale,
        PyObject *value, int input, int output)
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
    self->inConverter = NULL;
    self->outConverter = NULL;

    // allocate the indicator and data arrays
    dataLength = (unsigned PY_LONG_LONG) numElements *
            (unsigned PY_LONG_LONG) self->bufferSize;
    if (dataLength > INT_MAX) {
        PyErr_SetString(PyExc_ValueError, "array size too large");
        return -1;
    }
    self->lengthOrIndicator = PyMem_Malloc(numElements * sizeof(SQLLEN));
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
int Variable_DefaultInit(udt_Variable *self, PyObject *args,
        PyObject *keywordArgs)
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
    varType = Variable_TypeByPythonType((PyObject*) Py_TYPE(self));
    if (!varType)
        return -1;
    if (Variable_InternalInit(self, numElements, varType,
            varType->defaultSize, varType->defaultScale, value, 1, 1) < 0)
        return -1;

    return 0;
}


//-----------------------------------------------------------------------------
// Variable_InitWithScale()
//   Constructor which accepts scale as an argument as well.
//-----------------------------------------------------------------------------
int Variable_InitWithScale(udt_Variable *self, PyObject *args,
        PyObject *keywordArgs)
{
    udt_VariableType *varType;
    int numElements, scale;
    PyObject *value;

    static char *keywordList[] = { "value", "scale", "numElements", NULL };

    varType = Variable_TypeByPythonType((PyObject*) Py_TYPE(self));
    if (!varType)
        return -1;
    value = NULL;
    numElements = 1;
    scale = varType->defaultScale;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|Oii", keywordList,
            &value, &scale, &numElements))
        return -1;
    if (Variable_InternalInit(self, numElements, varType,
            varType->defaultSize, scale, value, 1, 1) < 0)
        return -1;

    return 0;
}


//-----------------------------------------------------------------------------
// Variable_InitWithSize()
//   Constructor which accepts scale as an argument as well.
//-----------------------------------------------------------------------------
int Variable_InitWithSize(udt_Variable *self, PyObject *args,
        PyObject *keywordArgs)
{
    udt_VariableType *varType;
    int numElements, size;
    PyObject *value;

    static char *keywordList[] = { "value", "size", "numElements", NULL };

    varType = Variable_TypeByPythonType((PyObject*) Py_TYPE(self));
    if (!varType)
        return -1;
    value = NULL;
    numElements = 1;
    size = varType->defaultSize;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|Oii", keywordList,
            &value, &size, &numElements))
        return -1;
    if (Variable_InternalInit(self, numElements, varType, size,
            varType->defaultScale, value, 1, 1) < 0)
        return -1;

    return 0;
}


//-----------------------------------------------------------------------------
// Variable_InternalNew()
//   Internal method of creating a new variable.
//-----------------------------------------------------------------------------
udt_Variable *Variable_InternalNew(unsigned numElements,
        udt_VariableType *type, SQLUINTEGER size, SQLSMALLINT scale)
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
static udt_VariableType *Variable_TypeByValue(PyObject* value,
        SQLUINTEGER* size)
{
    if (value == Py_None) {
        *size = 1;
        return &vt_Unicode;
    }
    if (PyUnicode_Check(value)) {
        *size = PyUnicode_GET_SIZE(value);
        return &vt_Unicode;
    }
    if (PyBytes_Check(value)) {
        udt_StringBuffer temp;
        if (StringBuffer_FromBinary(&temp, value) < 0)
            return NULL;
        *size = temp.size;
        StringBuffer_Clear(&temp);
        return &vt_Binary;
    }
    if (PyBool_Check(value))
        return &vt_Bit;
    if (PyLong_Check(value))
        return &vt_Integer;
    if (PyLong_Check(value))
        return &vt_BigInteger;
    if (PyFloat_Check(value))
        return &vt_Double;
    if (Py_TYPE(value) == g_DecimalType)
        return &vt_Decimal;
    if (Py_TYPE(value) == g_TimeType)
        return &vt_Time;
    if (Py_TYPE(value) == g_DateTimeType)
        return &vt_Timestamp;
    if (Py_TYPE(value) == g_DateType)
        return &vt_Timestamp;

    PyErr_Format(g_NotSupportedErrorException,
            "Variable_TypeByValue(): unhandled data type %s",
            Py_TYPE(value)->tp_name);
    return NULL;
}


//-----------------------------------------------------------------------------
// Variable_TypeByPythonType()
//   Return a variable type given a Python type object or NULL if the Python
// type does not have a corresponding variable type.
//-----------------------------------------------------------------------------
udt_VariableType *Variable_TypeByPythonType(PyObject* type)
{
    if (type == (PyObject*) g_StringApiType)
        return &vt_Unicode;
    if (type == (PyObject*) &g_UnicodeVarType)
        return &vt_Unicode;
    if (type == (PyObject*) &PyUnicode_Type)
        return &vt_Unicode;
    if (type == (PyObject*) &g_LongUnicodeVarType)
        return &vt_LongUnicode;
    if (type == (PyObject*) &g_BinaryVarType)
        return &vt_Binary;
    if (type == (PyObject*) &PyBytes_Type)
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
    if (type == (PyObject*) &PyLong_Type)
        return &vt_Integer;
    if (type == (PyObject*) &g_DoubleVarType)
        return &vt_Double;
    if (type == (PyObject*) &PyFloat_Type)
        return &vt_Double;
    if (type == (PyObject*) g_NumberApiType)
        return &vt_Double;
    if (type == (PyObject*) &g_DecimalVarType)
        return &vt_Decimal;
    if (type == (PyObject*) g_DecimalType)
        return &vt_Decimal;
    if (type == (PyObject*) &g_DateVarType)
        return &vt_Date;
    if (type == (PyObject*) g_DateType)
        return &vt_Date;
    if (type == (PyObject*) &g_TimeVarType)
        return &vt_Time;
    if (type == (PyObject*) g_TimeType)
        return &vt_Time;
    if (type == (PyObject*) &g_TimestampVarType)
        return &vt_Timestamp;
    if (type == (PyObject*) g_DateTimeType)
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
udt_VariableType *Variable_TypeBySqlDataType(udt_Cursor *cursor,
        SQLSMALLINT sqlDataType)
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
        case SQL_VARCHAR:
        case SQL_GUID:
            return &vt_Unicode;
        case SQL_WCHAR:
        case SQL_WVARCHAR:
            return &vt_Unicode;
        case SQL_LONGVARCHAR:
            return &vt_LongUnicode;
        case SQL_WLONGVARCHAR:
            return &vt_LongUnicode;
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
PyObject *Variable_New(PyTypeObject *type, PyObject *args,
        PyObject *keywordArgs)
{
    return type->tp_alloc(type, 0);
}


//-----------------------------------------------------------------------------
// Variable_Free()
//   Free an existing variable.
//-----------------------------------------------------------------------------
void Variable_Free(udt_Variable *self)
{
    if (self->lengthOrIndicator)
        PyMem_Free(self->lengthOrIndicator);
    if (self->data)
        PyMem_Free(self->data);
    Py_CLEAR(self->inConverter);
    Py_CLEAR(self->outConverter);
    Py_TYPE(self)->tp_free((PyObject*) self);
}


//-----------------------------------------------------------------------------
// Variable_Check()
//   Returns a boolean indicating if the object is a variable.
//-----------------------------------------------------------------------------
int Variable_Check(PyObject *object)
{
    return (Py_TYPE(object) == &g_BigIntegerVarType ||
            Py_TYPE(object) == &g_BinaryVarType ||
            Py_TYPE(object) == &g_BitVarType ||
            Py_TYPE(object) == &g_DateVarType ||
            Py_TYPE(object) == &g_DecimalVarType ||
            Py_TYPE(object) == &g_DoubleVarType ||
            Py_TYPE(object) == &g_IntegerVarType ||
            Py_TYPE(object) == &g_LongBinaryVarType ||
            Py_TYPE(object) == &g_LongUnicodeVarType ||
            Py_TYPE(object) == &g_TimestampVarType ||
            Py_TYPE(object) == &g_UnicodeVarType);
}


//-----------------------------------------------------------------------------
// Variable_DefaultNewByValue()
//   Default method for determining the type of variable to use for the data.
//-----------------------------------------------------------------------------
static udt_Variable *Variable_DefaultNewByValue(udt_Cursor *cursor,
        PyObject *value, unsigned numElements)
{
    udt_VariableType *varType;
    SQLUINTEGER size = 0;

    varType = Variable_TypeByValue(value, &size);
    if (!varType)
        return NULL;
    if (!size)
        size = varType->defaultSize;
    return Variable_InternalNew(numElements, varType, size,
            varType->defaultScale);
}


//-----------------------------------------------------------------------------
// Variable_NewByInputTypeHandler()
//   Allocate a new variable by calling an input type handler. If the input
// type handler does not return anything, the default variable type is
// returned as usual.
//-----------------------------------------------------------------------------
static udt_Variable *Variable_NewByInputTypeHandler(udt_Cursor *cursor,
        PyObject *inputTypeHandler, PyObject *value, unsigned numElements)
{
    PyObject *result;

    result = PyObject_CallFunction(inputTypeHandler, "OOi", cursor, value,
            numElements);
    if (!result)
        return NULL;
    if (result != Py_None) {
        if (!Variable_Check(result)) {
            Py_DECREF(result);
            PyErr_SetString(PyExc_TypeError,
                    "expecting variable from input type handler");
            return NULL;
        }
        return (udt_Variable*) result;
    }
    Py_DECREF(result);
    return Variable_DefaultNewByValue(cursor, value, numElements);
}


//-----------------------------------------------------------------------------
// Variable_NewByValue()
//   Allocate a new variable by looking at the type of the data.
//-----------------------------------------------------------------------------
udt_Variable *Variable_NewByValue(udt_Cursor *cursor, PyObject *value,
        unsigned numElements)
{
    if (cursor->inputTypeHandler && cursor->inputTypeHandler != Py_None)
        return Variable_NewByInputTypeHandler(cursor, cursor->inputTypeHandler,
                value, numElements);
    if (cursor->connection->inputTypeHandler &&
            cursor->connection->inputTypeHandler != Py_None)
        return Variable_NewByInputTypeHandler(cursor,
                cursor->connection->inputTypeHandler, value, numElements);
    return Variable_DefaultNewByValue(cursor, value, numElements);
}


//-----------------------------------------------------------------------------
// Variable_NewByType()
//   Allocate a new variable by looking at the Python data type.
//-----------------------------------------------------------------------------
udt_Variable *Variable_NewByType(udt_Cursor *cursor, PyObject *value,
        unsigned numElements)
{
    udt_VariableType *varType;
    int size;

    // passing an integer is assumed to be a string
    if (PyLong_Check(value)) {
        size = PyLong_AsLong(value);
        if (PyErr_Occurred())
            return NULL;
        return Variable_InternalNew(numElements, &vt_Unicode, size, 0);
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
// Variable_NewByOutputTypeHandler()
//   Create a new variable by calling the output type handler.
//-----------------------------------------------------------------------------
static udt_Variable *Variable_NewByOutputTypeHandler(udt_Cursor *cursor,
        PyObject *outputTypeHandler, udt_VariableType *varType,
        SQLUINTEGER size, SQLSMALLINT scale, unsigned numElements)
{
    udt_Variable *var;
    PyObject *result;

    // call method, passing parameters
    result = PyObject_CallFunction(outputTypeHandler, "OOii", cursor,
            varType->pythonType, size, scale);
    if (!result)
        return NULL;

    // if result is None, assume default behavior
    if (result == Py_None) {
        Py_DECREF(result);
        return Variable_InternalNew(numElements, varType, size, scale);
    }

    // otherwise, verify that the result is an actual variable
    if (!Variable_Check(result)) {
        Py_DECREF(result);
        PyErr_SetString(PyExc_TypeError,
                "expecting variable from output type handler");
        return NULL;
    }

    // verify that the array size is sufficient to handle the fetch
    var = (udt_Variable*) result;
    if (var->numElements < cursor->fetchArraySize) {
        Py_DECREF(result);
        PyErr_SetString(PyExc_TypeError,
                "expecting variable with array size large enough for fetch");
        return NULL;
    }

    return var;
}


//-----------------------------------------------------------------------------
// Variable_NewForResultSet()
//   Create a new variable for the given position in the result set. The new
// variable is immediately bound to the statement as well.
//-----------------------------------------------------------------------------
udt_Variable *Variable_NewForResultSet(udt_Cursor *cursor,
        SQLUSMALLINT position)
{
    SQLSMALLINT dataType, length, scale, nullable;
    udt_VariableType *varType;
    SQLWCHAR name[1];
    udt_Variable *var;
    SQLULEN size;
    SQLRETURN rc;

    // retrieve information about the column
    rc = SQLDescribeColW(cursor->handle, position, name, CEO_ARRAYSIZE(name),
            &length, &dataType, &size, &scale, &nullable);
    if (CheckForError(cursor, rc,
                "Variable_NewForResultSet(): get column info") < 0)
        return NULL;

    // determine data type
    varType = Variable_TypeBySqlDataType(cursor, dataType);
    if (!varType)
        return NULL;

    // some ODBC drivers do not return a long string but instead return string
    // with a size of zero; provide a workaround
    if (size == 0) {
        if (varType == &vt_Unicode)
            varType = &vt_LongUnicode;
        else if (varType == &vt_Binary)
            varType = &vt_LongBinary;
    }

    // for long columns, set the size appropriately
    if (varType == &vt_LongUnicode || varType == &vt_LongBinary) {
        if (cursor->setOutputSize > 0 &&
                (cursor->setOutputSizeColumn == 0 ||
                 position == cursor->setOutputSizeColumn))
            size = cursor->setOutputSize;
        else size = varType->defaultSize;
    }

    // create a variable of the correct type
    if (cursor->outputTypeHandler && cursor->outputTypeHandler != Py_None)
        var = Variable_NewByOutputTypeHandler(cursor, 
                cursor->outputTypeHandler, varType, size, scale,
                cursor->fetchArraySize);
    else if (cursor->connection->outputTypeHandler &&
            cursor->connection->outputTypeHandler != Py_None)
        var = Variable_NewByOutputTypeHandler(cursor,
                cursor->connection->outputTypeHandler, varType, size, scale,
                cursor->fetchArraySize);
    else var = Variable_InternalNew(cursor->fetchArraySize, varType, size,
            scale);
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
int Variable_BindParameter(udt_Variable *self, udt_Cursor *cursor,
        SQLUSMALLINT position)
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
int Variable_Resize(udt_Variable *self, SQLUINTEGER newSize)
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
PyObject *Variable_GetValue(udt_Variable *self, unsigned arrayPos)
{
    PyObject *value, *result;

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
                "column %d (%d) truncated (need %ld, have %ld)",
                self->position, arrayPos, self->lengthOrIndicator[arrayPos],
                self->bufferSize);

    // calculate value to return
    value = (*self->type->getValueProc)(self, arrayPos);
    if (value && self->outConverter && self->outConverter != Py_None) {
        result = PyObject_CallFunctionObjArgs(self->outConverter, value, NULL);
        Py_DECREF(value);
        return result;
    }

    return value;
}


//-----------------------------------------------------------------------------
// Variable_SetValue()
//   Set the value of the variable at the given position.
//-----------------------------------------------------------------------------
int Variable_SetValue(udt_Variable *self, unsigned arrayPos, PyObject *value)
{
    PyObject *convertedValue = NULL;
    int result;

    // ensure we do not exceed the number of allocated elements
    if (arrayPos >= self->numElements) {
        PyErr_SetString(PyExc_IndexError,
                "Variable_SetSingleValue: array size exceeded");
        return -1;
    }

    // convert value, if necessary
    if (self->inConverter && self->inConverter != Py_None) {
        convertedValue = PyObject_CallFunctionObjArgs(self->inConverter, value,
                NULL);
        if (!convertedValue)
            return -1;
        value = convertedValue;
    }

    // check for a NULL value
    if (value == Py_None) {
        self->lengthOrIndicator[arrayPos] = SQL_NULL_DATA;
        Py_XDECREF(convertedValue);
        return 0;
    }

    self->lengthOrIndicator[arrayPos] = 0;
    result = (*self->type->setValueProc)(self, arrayPos, value);
    Py_XDECREF(convertedValue);
    return result;
}


//-----------------------------------------------------------------------------
// Variable_ExternalGetValue()
//   Return the value of the variable at the given position.
//-----------------------------------------------------------------------------
static PyObject *Variable_ExternalGetValue(udt_Variable *self, PyObject *args,
        PyObject *keywordArgs)
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
static PyObject *Variable_ExternalSetValue(udt_Variable *self, PyObject *args)
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
PyObject *Variable_Repr(udt_Variable *self)
{
    PyObject *valueRepr, *value, *module, *name, *result, *format, *formatArgs;

    value = Variable_GetValue(self, 0);
    if (!value)
        return NULL;
    valueRepr = PyObject_Repr(value);
    Py_DECREF(value);
    if (!valueRepr)
        return NULL;
    format = ceString_FromAscii("<%s.%s with value %s>");
    if (!format) {
        Py_DECREF(valueRepr);
        return NULL;
    }
    if (GetModuleAndName(Py_TYPE(self), &module, &name) < 0) {
        Py_DECREF(valueRepr);
        Py_DECREF(format);
        return NULL;
    }
    formatArgs = PyTuple_Pack(3, module, name, valueRepr);
    Py_DECREF(module);
    Py_DECREF(name);
    Py_DECREF(valueRepr);
    if (!formatArgs) {
        Py_DECREF(format);
        return NULL;
    }
    result = PyUnicode_Format(format, formatArgs);
    Py_DECREF(format);
    Py_DECREF(formatArgs);
    return result;
}
