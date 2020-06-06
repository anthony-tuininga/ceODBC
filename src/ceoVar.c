//-----------------------------------------------------------------------------
// Variable.c
//   Defines Python types for variables.
//-----------------------------------------------------------------------------

#include "ceoModule.h"

//-----------------------------------------------------------------------------
// Declaration of common variable functions.
//-----------------------------------------------------------------------------
static PyObject *Variable_ExternalGetValue(ceoVar*, PyObject*,
        PyObject*);
static PyObject *Variable_ExternalSetValue(ceoVar*, PyObject*);


//-----------------------------------------------------------------------------
// Declaration of variable members
//-----------------------------------------------------------------------------
static PyMemberDef ceoMembers[] = {
    { "bufferSize", T_INT, offsetof(ceoVar, bufferSize), READONLY },
    { "inconverter", T_OBJECT, offsetof(ceoVar, inConverter), 0 },
    { "input", T_INT, offsetof(ceoVar, input), 0 },
    { "numElements", T_INT, offsetof(ceoVar, numElements), READONLY },
    { "outconverter", T_OBJECT, offsetof(ceoVar, outConverter), 0 },
    { "output", T_INT, offsetof(ceoVar, output), 0 },
    { "scale", T_INT, offsetof(ceoVar, scale), READONLY },
    { "size", T_INT, offsetof(ceoVar, size), READONLY },
    { "type", T_OBJECT, offsetof(ceoVar, type), READONLY },
    { NULL }
};


//-----------------------------------------------------------------------------
// Declaration of variable methods
//-----------------------------------------------------------------------------
static PyMethodDef ceoMethods[] = {
    { "getvalue", (PyCFunction) Variable_ExternalGetValue,
            METH_VARARGS  | METH_KEYWORDS },
    { "setvalue", (PyCFunction) Variable_ExternalSetValue, METH_VARARGS },
    { NULL, NULL }
};


//-----------------------------------------------------------------------------
// declaration of the Python type
//-----------------------------------------------------------------------------
PyTypeObject ceoPyTypeVar = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ceODBC.Var",
    .tp_basicsize = sizeof(ceoVar),
    .tp_dealloc = (destructor) Variable_Free,
    .tp_repr = (reprfunc) Variable_Repr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_methods = ceoMethods,
    .tp_members = ceoMembers,
    .tp_init = (initproc) Variable_DefaultInit,
    .tp_new = Variable_New
};


//-----------------------------------------------------------------------------
// Variable_InternalInit()
//   Internal method of initializing a new variable.
//-----------------------------------------------------------------------------
static int Variable_InternalInit(ceoVar *var, unsigned numElements,
        ceoDbType *type, SQLUINTEGER size, SQLSMALLINT scale,
        PyObject *value, int input, int output)
{
    unsigned PY_LONG_LONG dataLength;
    SQLUINTEGER i;

    // perform basic initialization
    var->position = -1;
    var->numElements = (numElements > 1) ? numElements : 1;
    var->size = size;
    var->bufferSize = (type->bufferSize > 0) ? type->bufferSize :
            var->size * type->bytesMultiplier;
    var->scale = scale;
    var->type = type;
    var->input = input;
    var->output = output;
    var->lengthOrIndicator = NULL;
    var->data.asRaw = NULL;
    var->inConverter = NULL;
    var->outConverter = NULL;

    // allocate the indicator and data arrays
    dataLength = (unsigned PY_LONG_LONG) numElements *
            (unsigned PY_LONG_LONG) var->bufferSize;
    if (dataLength > INT_MAX) {
        PyErr_SetString(PyExc_ValueError, "array size too large");
        return -1;
    }
    var->lengthOrIndicator = PyMem_Malloc(numElements * sizeof(SQLLEN));
    var->data.asRaw = PyMem_Malloc((size_t) dataLength);
    if (!var->lengthOrIndicator || !var->data.asRaw) {
        PyErr_NoMemory();
        return -1;
    }

    // ensure that all variable values start out NULL
    for (i = 0; i < numElements; i++)
        var->lengthOrIndicator[i] = SQL_NULL_DATA;

    // set value, if applicable
    if (value && Variable_SetValue(var, 0, value) < 0)
        return -1;

    return 0;
}


//-----------------------------------------------------------------------------
// Variable_DefaultInit()
//   Default constructor.
//-----------------------------------------------------------------------------
int Variable_DefaultInit(ceoVar *self, PyObject *args,
        PyObject *keywordArgs)
{
    ceoDbType *dbType;
    PyObject *value;
    int numElements;

    static char *keywordList[] = { "value", "numElements", NULL };

    value = NULL;
    numElements = 1;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|Oi", keywordList,
            &value, &numElements))
        return -1;
    dbType = ceoDbType_fromPythonType(Py_TYPE(self));
    if (!dbType)
        return -1;
    if (Variable_InternalInit(self, numElements, dbType, 0, 0, value, 1,
            1) < 0)
        return -1;

    return 0;
}


//-----------------------------------------------------------------------------
// Variable_InitWithScale()
//   Constructor which accepts scale as an argument as well.
//-----------------------------------------------------------------------------
int Variable_InitWithScale(ceoVar *self, PyObject *args,
        PyObject *keywordArgs)
{
    ceoDbType *dbType;
    int numElements, scale;
    PyObject *value;

    static char *keywordList[] = { "value", "scale", "numElements", NULL };

    dbType = ceoDbType_fromPythonType(Py_TYPE(self));
    if (!dbType)
        return -1;
    value = NULL;
    scale = 0;
    numElements = 1;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|Oii", keywordList,
            &value, &scale, &numElements))
        return -1;
    if (Variable_InternalInit(self, numElements, dbType, 0, scale, value, 1,
            1) < 0)
        return -1;

    return 0;
}


//-----------------------------------------------------------------------------
// Variable_InitWithSize()
//   Constructor which accepts scale as an argument as well.
//-----------------------------------------------------------------------------
int Variable_InitWithSize(ceoVar *self, PyObject *args,
        PyObject *keywordArgs)
{
    ceoDbType *dbType;
    int numElements, size;
    PyObject *value;

    static char *keywordList[] = { "value", "size", "numElements", NULL };

    dbType = ceoDbType_fromPythonType(Py_TYPE(self));
    if (!dbType)
        return -1;
    value = NULL;
    numElements = 1;
    size = 0;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|Oii", keywordList,
            &value, &size, &numElements))
        return -1;
    if (Variable_InternalInit(self, numElements, dbType, size, 0, value, 1,
            1) < 0)
        return -1;

    return 0;
}


//-----------------------------------------------------------------------------
// Variable_InternalNew()
//   Internal method of creating a new variable.
//-----------------------------------------------------------------------------
ceoVar *Variable_InternalNew(unsigned numElements,
        ceoDbType *type, SQLUINTEGER size, SQLSMALLINT scale)
{
    ceoVar *var;

    var = (ceoVar*) ceoPyTypeVar.tp_alloc(&ceoPyTypeVar, 0);
    if (!var)
        return NULL;
    if (Variable_InternalInit(var, numElements, type, size, scale,
            NULL, 1, 0) < 0) {
        Py_DECREF(var);
        return NULL;
    }

    return var;
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
void Variable_Free(ceoVar *self)
{
    if (self->lengthOrIndicator)
        PyMem_Free(self->lengthOrIndicator);
    if (self->data.asRaw)
        PyMem_Free(self->data.asRaw);
    Py_CLEAR(self->inConverter);
    Py_CLEAR(self->outConverter);
    Py_TYPE(self)->tp_free((PyObject*) self);
}


//-----------------------------------------------------------------------------
// Variable_DefaultNewByValue()
//   Default method for determining the type of variable to use for the data.
//-----------------------------------------------------------------------------
static ceoVar *Variable_DefaultNewByValue(ceoCursor *cursor,
        PyObject *value, unsigned numElements)
{
    ceoDbType *dbType;
    SQLUINTEGER size;

    dbType = ceoDbType_fromValue(value, &size);
    if (!dbType)
        return NULL;
    return Variable_InternalNew(numElements, dbType, size, 0);
}


//-----------------------------------------------------------------------------
// Variable_NewByInputTypeHandler()
//   Allocate a new variable by calling an input type handler. If the input
// type handler does not return anything, the default variable type is
// returned as usual.
//-----------------------------------------------------------------------------
static ceoVar *Variable_NewByInputTypeHandler(ceoCursor *cursor,
        PyObject *inputTypeHandler, PyObject *value, unsigned numElements)
{
    PyObject *result;

    result = PyObject_CallFunction(inputTypeHandler, "OOi", cursor, value,
            numElements);
    if (!result)
        return NULL;
    if (result != Py_None) {
        if (Py_TYPE(result) != &ceoPyTypeVar) {
            Py_DECREF(result);
            PyErr_SetString(PyExc_TypeError,
                    "expecting variable from input type handler");
            return NULL;
        }
        return (ceoVar*) result;
    }
    Py_DECREF(result);
    return Variable_DefaultNewByValue(cursor, value, numElements);
}


//-----------------------------------------------------------------------------
// Variable_NewByValue()
//   Allocate a new variable by looking at the type of the data.
//-----------------------------------------------------------------------------
ceoVar *Variable_NewByValue(ceoCursor *cursor, PyObject *value,
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
ceoVar *Variable_NewByType(ceoCursor *cursor, PyObject *value,
        unsigned numElements)
{
    ceoDbType *dbType;
    int size;

    // passing an integer is assumed to be a string
    if (PyLong_Check(value)) {
        size = PyLong_AsLong(value);
        if (PyErr_Occurred())
            return NULL;
        return Variable_InternalNew(numElements, ceoDbTypeString, size, 0);
    }

    // handle directly bound variables
    if (Py_TYPE(value) == &ceoPyTypeVar) {
        Py_INCREF(value);
        return (ceoVar*) value;
    }

    // everything else ought to be a Python type
    dbType = ceoDbType_fromType(value);
    if (!dbType)
        return NULL;
    return Variable_InternalNew(numElements, dbType, 0, 0);
}


//-----------------------------------------------------------------------------
// Variable_NewByOutputTypeHandler()
//   Create a new variable by calling the output type handler.
//-----------------------------------------------------------------------------
static ceoVar *Variable_NewByOutputTypeHandler(ceoCursor *cursor,
        PyObject *outputTypeHandler, ceoDbType *dbType,
        SQLUINTEGER size, SQLSMALLINT scale, unsigned numElements)
{
    ceoVar *var;
    PyObject *result;

    // call method, passing parameters
    result = PyObject_CallFunction(outputTypeHandler, "OOii", cursor,
            dbType, size, scale);
    if (!result)
        return NULL;

    // if result is None, assume default behavior
    if (result == Py_None) {
        Py_DECREF(result);
        return Variable_InternalNew(numElements, dbType, size, scale);
    }

    // otherwise, verify that the result is an actual variable
    if (Py_TYPE(result) != &ceoPyTypeVar) {
        Py_DECREF(result);
        PyErr_SetString(PyExc_TypeError,
                "expecting variable from output type handler");
        return NULL;
    }

    // verify that the array size is sufficient to handle the fetch
    var = (ceoVar*) result;
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
ceoVar *Variable_NewForResultSet(ceoCursor *cursor,
        SQLUSMALLINT position)
{
    SQLSMALLINT dataType, length, scale, nullable;
    ceoDbType *dbType;
    ceoVar *var;
    SQLULEN size;
    SQLRETURN rc;

    // retrieve information about the column
    rc = SQLDescribeColA(cursor->handle, position, NULL, 0, &length, &dataType,
            &size, &scale, &nullable);
    if (CEO_CURSOR_CHECK_ERROR(cursor, rc,
            "Variable_NewForResultSet(): get column info") < 0)
        return NULL;

    // determine data type
    dbType = ceoDbType_fromSqlDataType(dataType);
    if (!dbType)
        return NULL;

    // some ODBC drivers do not return a long string but instead return string
    // with a size of zero; provide a workaround
    if (size == 0) {
        if (dbType == ceoDbTypeString)
            dbType = ceoDbTypeLongString;
        else if (dbType == ceoDbTypeBinary)
            dbType = ceoDbTypeLongBinary;
    }

    // for long columns, set the size appropriately
    if (dbType == ceoDbTypeLongString || dbType == ceoDbTypeLongBinary) {
        if (cursor->setOutputSize > 0 &&
                (cursor->setOutputSizeColumn == 0 ||
                 position == cursor->setOutputSizeColumn)) {
            size = cursor->setOutputSize;
        } else {
            size = CEO_DEFAULT_LONG_VAR_SIZE;
        }
    }

    // create a variable of the correct type
    if (cursor->outputTypeHandler && cursor->outputTypeHandler != Py_None)
        var = Variable_NewByOutputTypeHandler(cursor, 
                cursor->outputTypeHandler, dbType, size, scale,
                cursor->fetchArraySize);
    else if (cursor->connection->outputTypeHandler &&
            cursor->connection->outputTypeHandler != Py_None)
        var = Variable_NewByOutputTypeHandler(cursor,
                cursor->connection->outputTypeHandler, dbType, size, scale,
                cursor->fetchArraySize);
    else var = Variable_InternalNew(cursor->fetchArraySize, dbType, size,
            scale);
    if (!var)
        return NULL;

    // bind the column
    var->position = position;
    rc = SQLBindCol(cursor->handle, position, var->type->cDataType,
            var->data.asRaw, var->bufferSize, var->lengthOrIndicator);
    if (CEO_CURSOR_CHECK_ERROR(cursor, rc,
            "Variable_NewForResultSet(): bind()") < 0) {
        Py_DECREF(var);
        return NULL;
    }

    return var;
}


//-----------------------------------------------------------------------------
// Variable_BindParameter()
//   Allocate a variable and bind it to the given statement.
//-----------------------------------------------------------------------------
int Variable_BindParameter(ceoVar *self, ceoCursor *cursor,
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
            self->scale, self->data.asRaw, self->bufferSize,
            self->lengthOrIndicator);
    if (CEO_CURSOR_CHECK_ERROR(cursor, rc, "Variable_BindParameter()") < 0)
        return -1;

    return 0;
}


//-----------------------------------------------------------------------------
// Variable_Resize()
//   Resize the variable.
//-----------------------------------------------------------------------------
int Variable_Resize(ceoVar *self, SQLUINTEGER newSize)
{
    char *newData, *oldData;
    SQLINTEGER i;

    // allocate new memory for the larger size
    newData = (char*) PyMem_Malloc(self->numElements * newSize);
    if (!newData) {
        PyErr_NoMemory();
        return -1;
    }

    // copy the data from the original array to the new array
    oldData = (char*) self->data.asRaw;
    for (i = 0; i < self->numElements; i++)
        memcpy(newData + newSize * i, oldData + self->bufferSize * i,
                self->bufferSize);
    PyMem_Free(self->data.asRaw);
    self->data.asRaw = newData;
    self->size = newSize;
    self->bufferSize = newSize;

    // force rebinding
    self->position = -1;

    return 0;
}


//-----------------------------------------------------------------------------
// ceoVar_getValueHelper()
//   Return the value of the variable at the given position.
//-----------------------------------------------------------------------------
PyObject *ceoVar_getValueHelper(ceoVar *var, unsigned pos)
{
    char message[250], *ptr;
    PyObject *obj, *result;

    switch (var->type->sqlDataType) {
        case SQL_VARBINARY:
        case SQL_LONGVARBINARY:
            ptr = (char*) var->data.asBinary + pos * var->bufferSize;
            return PyBytes_FromStringAndSize(ptr, var->lengthOrIndicator[pos]);
        case SQL_BIT:
            return PyBool_FromLong(var->data.asBit[pos]);
        case SQL_BIGINT:
            return PyLong_FromLongLong(var->data.asBigInt[pos]);
        case SQL_DOUBLE:
            return PyFloat_FromDouble(var->data.asDouble[pos]);
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR:
            ptr = (char*) var->data.asString + pos * var->bufferSize;
            return PyUnicode_DecodeUTF8(ptr, var->lengthOrIndicator[pos],
                    NULL);
        case SQL_CHAR:
            ptr = (char*) var->data.asString + pos * var->bufferSize;
            obj = PyUnicode_DecodeUTF8(ptr, var->lengthOrIndicator[pos],
                    NULL);
            if (!obj)
                return NULL;
            result = PyObject_CallFunctionObjArgs(ceoPyTypeDecimal, obj, NULL);
            Py_DECREF(obj);
            return result;
        case SQL_INTEGER:
            return PyLong_FromLong(var->data.asInt[pos]);
        case SQL_TYPE_DATE:
            return ceoTransform_dateFromSqlValue(&var->data.asDate[pos]);
        case SQL_TYPE_TIME:
            return ceoTransform_timeFromSqlValue(&var->data.asTime[pos]);
        case SQL_TYPE_TIMESTAMP:
            return ceoTransform_timestampFromSqlValue(
                    &var->data.asTimestamp[pos]);
    }
 
    snprintf(message, sizeof(message), "missing get support for DB type %s",
            var->type->name);
    ceoError_raiseFromString(ceoExceptionInternalError, message, __func__);
    return NULL;
}


//-----------------------------------------------------------------------------
// Variable_GetValue()
//   Return the value of the variable at the given position.
//-----------------------------------------------------------------------------
PyObject *Variable_GetValue(ceoVar *self, unsigned arrayPos)
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
        return PyErr_Format(ceoExceptionDatabaseError,
                "column %d (%d) truncated (need %ld, have %ld)",
                self->position, arrayPos, self->lengthOrIndicator[arrayPos],
                self->bufferSize);

    // calculate value to return
    value = ceoVar_getValueHelper(self, arrayPos);
    if (value && self->outConverter && self->outConverter != Py_None) {
        result = PyObject_CallFunctionObjArgs(self->outConverter, value, NULL);
        Py_DECREF(value);
        return result;
    }

    return value;
}


//-----------------------------------------------------------------------------
// ceoVar_setValueHelper()
//   Set the value of the variable at the given position.
//-----------------------------------------------------------------------------
int ceoVar_setValueHelper(ceoVar *var, unsigned pos, PyObject *value)
{
    Py_ssize_t tempLength;
    PyObject *textValue;
    const char *temp;
    char message[250];

    switch (var->type->sqlDataType) {
        case SQL_VARBINARY:
        case SQL_LONGVARBINARY:
            if (!PyBytes_Check(value)) {
                PyErr_SetString(PyExc_TypeError, "expecting bytes data");
                return -1;
            }
            temp = PyBytes_AS_STRING(value);
            tempLength = PyBytes_GET_SIZE(value);
            if (tempLength > var->size) {
                if (Variable_Resize(var, tempLength) < 0)
                    return -1;
            }
            var->lengthOrIndicator[pos] = (SQLINTEGER) tempLength;
            if (tempLength)
                memcpy(var->data.asBinary + var->bufferSize * pos, temp,
                        tempLength);
            return 0;
        case SQL_BIT:
            if (!PyBool_Check(value)) {
                PyErr_SetString(PyExc_TypeError, "expecting boolean data");
                return -1;
            }
            var->data.asBit[pos] = (unsigned char) PyLong_AsLong(value);
            if (PyErr_Occurred())
                return -1;
            return 0;
        case SQL_DOUBLE:
            if (PyFloat_Check(value)) {
                var->data.asDouble[pos] = PyFloat_AS_DOUBLE(value);
            } else if (PyLong_Check(value)) {
                var->data.asDouble[pos] = PyLong_AsLong(value);
                if (PyErr_Occurred())
                    return -1;
            } else {
                PyErr_Format(PyExc_TypeError, "expecting floating point data, "
                        "got value of type %s instead",
                        Py_TYPE(value)->tp_name);
                return -1;
            }
            return 0;
        case SQL_BIGINT:
            if (!PyLong_Check(value)) {
                PyErr_Format(PyExc_TypeError,
                        "expecting integer data, got value of type %s instead",
                        Py_TYPE(value)->tp_name);
                return -1;
            }
            var->data.asBigInt[pos] = PyLong_AsLongLong(value);
            if (PyErr_Occurred())
                return -1;
            return 0;
        case SQL_CHAR:
            if (Py_TYPE(value) != (PyTypeObject*) ceoPyTypeDecimal) {
                PyErr_SetString(PyExc_TypeError, "expecting decimal object");
                return -1;
            }
            textValue = PyObject_Str(value);
            if (!textValue)
                return -1;
            temp = PyUnicode_AsUTF8AndSize(textValue, &tempLength);
            Py_DECREF(textValue);
            if (!temp)
                return -1;
            var->lengthOrIndicator[pos] = (SQLINTEGER) tempLength;
            memcpy(var->data.asString + var->bufferSize * pos, temp,
                    tempLength);
            return 0;
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR:
            if (!PyUnicode_Check(value)) {
                PyErr_SetString(PyExc_TypeError, "expecting string");
                return -1;
            }
            temp = PyUnicode_AsUTF8AndSize(value, &tempLength);
            if (!temp)
                return -1;
            if (tempLength > var->size) {
                if (Variable_Resize((ceoVar*) var, tempLength) < 0)
                    return -1;
            }
            var->lengthOrIndicator[pos] = (SQLINTEGER) tempLength;
            if (tempLength)
                memcpy(var->data.asString + var->bufferSize * pos, temp,
                        tempLength);
            return 0;
        case SQL_INTEGER:
            if (!PyLong_Check(value)) {
                PyErr_Format(PyExc_TypeError,
                        "expecting integer data, got value of type %s instead",
                        Py_TYPE(value)->tp_name);
                return -1;
            }
            var->data.asInt[pos] = PyLong_AsLong(value);
            if (PyErr_Occurred())
                return -1;
            return 0;
        case SQL_TYPE_DATE:
            return ceoTransform_sqlValueFromDate(value,
                    &var->data.asDate[pos]);
        case SQL_TYPE_TIME:
            return ceoTransform_sqlValueFromTime(value,
                    &var->data.asTime[pos]);
        case SQL_TYPE_TIMESTAMP:
            return ceoTransform_sqlValueFromTimestamp(value,
                    &var->data.asTimestamp[pos]);
    }

    snprintf(message, sizeof(message), "missing set support for DB type %s",
            var->type->name);
    return ceoError_raiseFromString(ceoExceptionInternalError, message,
            __func__);
}


//-----------------------------------------------------------------------------
// Variable_SetValue()
//   Set the value of the variable at the given position.
//-----------------------------------------------------------------------------
int Variable_SetValue(ceoVar *self, unsigned arrayPos, PyObject *value)
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
    result = ceoVar_setValueHelper(self, arrayPos, value);
    Py_XDECREF(convertedValue);
    return result;
}


//-----------------------------------------------------------------------------
// Variable_ExternalGetValue()
//   Return the value of the variable at the given position.
//-----------------------------------------------------------------------------
static PyObject *Variable_ExternalGetValue(ceoVar *self, PyObject *args,
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
static PyObject *Variable_ExternalSetValue(ceoVar *self, PyObject *args)
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
PyObject *Variable_Repr(ceoVar *self)
{
    PyObject *valueRepr, *value, *module, *name, *result;

    value = Variable_GetValue(self, 0);
    if (!value)
        return NULL;
    valueRepr = PyObject_Repr(value);
    Py_DECREF(value);
    if (!valueRepr)
        return NULL;
    if (ceoUtils_getModuleAndName(Py_TYPE(self), &module, &name) < 0) {
        Py_DECREF(valueRepr);
        return NULL;
    }
    result = ceoUtils_formatString("<%s.%s with value %s>",
            PyTuple_Pack(3, module, name, valueRepr));
    Py_DECREF(module);
    Py_DECREF(name);
    Py_DECREF(valueRepr);
    return result;
}
