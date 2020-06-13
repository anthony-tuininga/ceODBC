//-----------------------------------------------------------------------------
// Variable.c
//   Defines Python types for variables.
//-----------------------------------------------------------------------------

#include "ceoModule.h"

//-----------------------------------------------------------------------------
// ceoVar_internalInit()
//   Internal method of initializing a new variable.
//-----------------------------------------------------------------------------
static int ceoVar_internalInit(ceoVar *var, unsigned numElements,
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
    if (value && ceoVar_setValue(var, 0, value) < 0)
        return -1;

    return 0;
}


//-----------------------------------------------------------------------------
// ceoVar_init()
//   Constructor.
//-----------------------------------------------------------------------------
static int ceoVar_init(ceoVar *var, PyObject *args, PyObject *keywordArgs)
{
    PyObject *type, *value;
    ceoDbType *dbType;
    int numElements;

    static char *keywordList[] = { "type", "value", "numElements", NULL };

    type = value = NULL;
    numElements = 1;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "O|Oi", keywordList,
            &type, &value, &numElements))
        return -1;
    dbType = ceoDbType_fromType(type);
    if (!dbType)
        return -1;
    return ceoVar_internalInit(var, numElements, dbType, 0, 0, value, 1, 1);
}


//-----------------------------------------------------------------------------
// ceoVar_internalNew()
//   Internal method of creating a new variable.
//-----------------------------------------------------------------------------
ceoVar *ceoVar_internalNew(unsigned numElements,
        ceoDbType *type, SQLUINTEGER size, SQLSMALLINT scale)
{
    ceoVar *var;

    var = (ceoVar*) ceoPyTypeVar.tp_alloc(&ceoPyTypeVar, 0);
    if (!var)
        return NULL;
    if (ceoVar_internalInit(var, numElements, type, size, scale,
            NULL, 1, 0) < 0) {
        Py_DECREF(var);
        return NULL;
    }

    return var;
}


//-----------------------------------------------------------------------------
// ceoVar_new()
//   Create a new cursor object.
//-----------------------------------------------------------------------------
static PyObject *ceoVar_new(PyTypeObject *type, PyObject *args,
        PyObject *keywordArgs)
{
    return type->tp_alloc(type, 0);
}


//-----------------------------------------------------------------------------
// ceoVar_free()
//   Free an existing variable.
//-----------------------------------------------------------------------------
static void ceoVar_free(ceoVar *var)
{
    if (var->lengthOrIndicator)
        PyMem_Free(var->lengthOrIndicator);
    if (var->data.asRaw)
        PyMem_Free(var->data.asRaw);
    Py_CLEAR(var->inConverter);
    Py_CLEAR(var->outConverter);
    Py_TYPE(var)->tp_free((PyObject*) var);
}


//-----------------------------------------------------------------------------
// ceoVar_defaultNewByValue()
//   Default method for determining the type of variable to use for the data.
//-----------------------------------------------------------------------------
static ceoVar *ceoVar_defaultNewByValue(ceoCursor *cursor,
        PyObject *value, unsigned numElements)
{
    ceoDbType *dbType;
    SQLUINTEGER size;

    dbType = ceoDbType_fromValue(value, &size);
    if (!dbType)
        return NULL;
    return ceoVar_internalNew(numElements, dbType, size, 0);
}


//-----------------------------------------------------------------------------
// ceoVar_newByInputTypeHandler()
//   Allocate a new variable by calling an input type handler. If the input
// type handler does not return anything, the default variable type is
// returned as usual.
//-----------------------------------------------------------------------------
static ceoVar *ceoVar_newByInputTypeHandler(ceoCursor *cursor,
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
    return ceoVar_defaultNewByValue(cursor, value, numElements);
}


//-----------------------------------------------------------------------------
// ceoVar_newByValue()
//   Allocate a new variable by looking at the type of the data.
//-----------------------------------------------------------------------------
ceoVar *ceoVar_newByValue(ceoCursor *cursor, PyObject *value,
        unsigned numElements)
{
    if (cursor->inputTypeHandler && cursor->inputTypeHandler != Py_None)
        return ceoVar_newByInputTypeHandler(cursor, cursor->inputTypeHandler,
                value, numElements);
    if (cursor->connection->inputTypeHandler &&
            cursor->connection->inputTypeHandler != Py_None)
        return ceoVar_newByInputTypeHandler(cursor,
                cursor->connection->inputTypeHandler, value, numElements);
    return ceoVar_defaultNewByValue(cursor, value, numElements);
}


//-----------------------------------------------------------------------------
// ceoVar_newByType()
//   Allocate a new variable by looking at the Python data type.
//-----------------------------------------------------------------------------
ceoVar *ceoVar_newByType(ceoCursor *cursor, PyObject *value,
        unsigned numElements)
{
    ceoDbType *dbType;
    int size;

    // passing an integer is assumed to be a string
    if (PyLong_Check(value)) {
        size = PyLong_AsLong(value);
        if (PyErr_Occurred())
            return NULL;
        return ceoVar_internalNew(numElements, ceoDbTypeString, size, 0);
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
    return ceoVar_internalNew(numElements, dbType, 0, 0);
}


//-----------------------------------------------------------------------------
// ceoVar_newByOutputTypeHandler()
//   Create a new variable by calling the output type handler.
//-----------------------------------------------------------------------------
static ceoVar *ceoVar_newByOutputTypeHandler(ceoCursor *cursor,
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
        return ceoVar_internalNew(numElements, dbType, size, scale);
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
// ceoVar_newForResultSet()
//   Create a new variable for the given position in the result set. The new
// variable is immediately bound to the statement as well.
//-----------------------------------------------------------------------------
ceoVar *ceoVar_newForResultSet(ceoCursor *cursor,
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
            "ceoVar_newForResultSet(): get column info") < 0)
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
        var = ceoVar_newByOutputTypeHandler(cursor, 
                cursor->outputTypeHandler, dbType, size, scale,
                cursor->fetchArraySize);
    else if (cursor->connection->outputTypeHandler &&
            cursor->connection->outputTypeHandler != Py_None)
        var = ceoVar_newByOutputTypeHandler(cursor,
                cursor->connection->outputTypeHandler, dbType, size, scale,
                cursor->fetchArraySize);
    else var = ceoVar_internalNew(cursor->fetchArraySize, dbType, size,
            scale);
    if (!var)
        return NULL;

    // bind the column
    var->position = position;
    rc = SQLBindCol(cursor->handle, position, var->type->cDataType,
            var->data.asRaw, var->bufferSize, var->lengthOrIndicator);
    if (CEO_CURSOR_CHECK_ERROR(cursor, rc,
            "ceoVar_newForResultSet(): bind()") < 0) {
        Py_DECREF(var);
        return NULL;
    }

    return var;
}


//-----------------------------------------------------------------------------
// ceoVar_bindParameter()
//   Allocate a variable and bind it to the given statement.
//-----------------------------------------------------------------------------
int ceoVar_bindParameter(ceoVar *var, ceoCursor *cursor,
        SQLUSMALLINT position)
{
    SQLSMALLINT inputOutputType;
    SQLRETURN rc;

    var->position = position;
    if (var->input && var->output) {
        inputOutputType = SQL_PARAM_INPUT_OUTPUT;
    } else if (var->output) {
        inputOutputType = SQL_PARAM_OUTPUT;
    } else {
        inputOutputType = SQL_PARAM_INPUT;
    }
    rc = SQLBindParameter(cursor->handle, position, inputOutputType,
            var->type->cDataType, var->type->sqlDataType, var->size,
            var->scale, var->data.asRaw, var->bufferSize,
            var->lengthOrIndicator);
    if (CEO_CURSOR_CHECK_ERROR(cursor, rc, "ceoVar_bindParameter()") < 0)
        return -1;

    return 0;
}


//-----------------------------------------------------------------------------
// ceoVar_resize()
//   Resize the variable.
//-----------------------------------------------------------------------------
static int ceoVar_resize(ceoVar *var, unsigned long newSize)
{
    char *newData, *oldData;
    unsigned long i;

    // allocate new memory for the larger size
    newData = (char*) PyMem_Malloc(var->numElements * newSize);
    if (!newData) {
        PyErr_NoMemory();
        return -1;
    }

    // copy the data from the original array to the new array
    oldData = (char*) var->data.asRaw;
    for (i = 0; i < var->numElements; i++)
        memcpy(newData + newSize * i, oldData + var->bufferSize * i,
                var->bufferSize);
    PyMem_Free(var->data.asRaw);
    var->data.asRaw = newData;
    var->size = newSize;
    var->bufferSize = newSize;

    // force rebinding
    var->position = -1;

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
            result = PyObject_CallFunctionObjArgs((PyObject*) ceoPyTypeDecimal,
                    obj, NULL);
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
// ceoVar_getValue()
//   Return the value of the variable at the given position.
//-----------------------------------------------------------------------------
PyObject *ceoVar_getValue(ceoVar *var, unsigned arrayPos)
{
    PyObject *value, *result;

    // ensure we do not exceed the number of allocated elements
    if (arrayPos >= var->numElements) {
        PyErr_SetString(PyExc_IndexError,
                "ceoVar_getValue(): array size exceeded");
        return NULL;
    }

    // check for a NULL value
    if (var->lengthOrIndicator[arrayPos] == SQL_NULL_DATA) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    // check for truncation
    if (var->lengthOrIndicator[arrayPos] > var->bufferSize)
        return PyErr_Format(ceoExceptionDatabaseError,
                "column %d (%d) truncated (need %ld, have %ld)",
                var->position, arrayPos, var->lengthOrIndicator[arrayPos],
                var->bufferSize);

    // calculate value to return
    value = ceoVar_getValueHelper(var, arrayPos);
    if (value && var->outConverter && var->outConverter != Py_None) {
        result = PyObject_CallFunctionObjArgs(var->outConverter, value, NULL);
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
                if (ceoVar_resize(var, (unsigned long) tempLength) < 0)
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
                if (ceoVar_resize((ceoVar*) var,
                        (unsigned long) tempLength) < 0)
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
// ceoVar_setValue()
//   Set the value of the variable at the given position.
//-----------------------------------------------------------------------------
int ceoVar_setValue(ceoVar *var, unsigned arrayPos, PyObject *value)
{
    PyObject *convertedValue = NULL;
    int result;

    // ensure we do not exceed the number of allocated elements
    if (arrayPos >= var->numElements) {
        PyErr_SetString(PyExc_IndexError,
                "ceoVar_setValue(): array size exceeded");
        return -1;
    }

    // convert value, if necessary
    if (var->inConverter && var->inConverter != Py_None) {
        convertedValue = PyObject_CallFunctionObjArgs(var->inConverter, value,
                NULL);
        if (!convertedValue)
            return -1;
        value = convertedValue;
    }

    // check for a NULL value
    if (value == Py_None) {
        var->lengthOrIndicator[arrayPos] = SQL_NULL_DATA;
        Py_XDECREF(convertedValue);
        return 0;
    }

    var->lengthOrIndicator[arrayPos] = 0;
    result = ceoVar_setValueHelper(var, arrayPos, value);
    Py_XDECREF(convertedValue);
    return result;
}


//-----------------------------------------------------------------------------
// ceoVar_externalGetValue()
//   Return the value of the variable at the given position.
//-----------------------------------------------------------------------------
static PyObject *ceoVar_externalGetValue(ceoVar *var, PyObject *args,
        PyObject *keywordArgs)
{
    static char *keywordList[] = { "pos", NULL };
    unsigned pos = 0;

    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|i", keywordList,
            &pos))
        return NULL;
    return ceoVar_getValue(var, pos);
}


//-----------------------------------------------------------------------------
// ceoVar_externalSetValue()
//   Set the value of the variable at the given position.
//-----------------------------------------------------------------------------
static PyObject *ceoVar_externalSetValue(ceoVar *var, PyObject *args)
{
    PyObject *value;
    unsigned pos;

    if (!PyArg_ParseTuple(args, "iO", &pos, &value))
      return NULL;
    if (ceoVar_setValue(var, pos, value) < 0)
      return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// ceoVar_repr()
//   Return a string representation of the variable.
//-----------------------------------------------------------------------------
static PyObject *ceoVar_repr(ceoVar *var)
{
    PyObject *valueRepr, *value, *module, *name, *result;

    value = ceoVar_getValue(var, 0);
    if (!value)
        return NULL;
    valueRepr = PyObject_Repr(value);
    Py_DECREF(value);
    if (!valueRepr)
        return NULL;
    if (ceoUtils_getModuleAndName(Py_TYPE(var), &module, &name) < 0) {
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
    { "getvalue", (PyCFunction) ceoVar_externalGetValue,
            METH_VARARGS  | METH_KEYWORDS },
    { "setvalue", (PyCFunction) ceoVar_externalSetValue, METH_VARARGS },
    { NULL, NULL }
};


//-----------------------------------------------------------------------------
// declaration of the Python type
//-----------------------------------------------------------------------------
PyTypeObject ceoPyTypeVar = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ceODBC.Var",
    .tp_basicsize = sizeof(ceoVar),
    .tp_dealloc = (destructor) ceoVar_free,
    .tp_repr = (reprfunc) ceoVar_repr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_methods = ceoMethods,
    .tp_members = ceoMembers,
    .tp_init = (initproc) ceoVar_init,
    .tp_new = ceoVar_new
};
