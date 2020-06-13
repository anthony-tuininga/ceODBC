//-----------------------------------------------------------------------------
// ceoDbType.c
//   Defines the objects used for identifying all database types.
//-----------------------------------------------------------------------------

#include "ceoModule.h"

//-----------------------------------------------------------------------------
// ceoDbType_free()
//   Free the database type object.
//-----------------------------------------------------------------------------
static void ceoDbType_free(ceoDbType *dbType)
{
    Py_TYPE(dbType)->tp_free((PyObject*) dbType);
}


//-----------------------------------------------------------------------------
// ceoDbType_fromSqlDataType()
//   Return the database type corresponding to the SQL data type.
//-----------------------------------------------------------------------------
ceoDbType *ceoDbType_fromSqlDataType(SQLSMALLINT sqlDataType)
{
    char buffer[100];

    switch(sqlDataType) {
        case SQL_BIGINT:
            return ceoDbTypeBigInt;
        case SQL_BIT:
            return ceoDbTypeBit;
        case SQL_SMALLINT:
        case SQL_TINYINT:
        case SQL_INTEGER:
            return ceoDbTypeInt;
        case SQL_REAL:
        case SQL_FLOAT:
        case SQL_DOUBLE:
            return ceoDbTypeDouble;
        case SQL_DECIMAL:
        case SQL_NUMERIC:
            return ceoDbTypeDecimal;
        case SQL_TYPE_DATE:
            return ceoDbTypeDate;
        case SQL_TYPE_TIME:
            return ceoDbTypeTime;
        case SQL_TYPE_TIMESTAMP:
            return ceoDbTypeTimestamp;
        case SQL_CHAR:
        case SQL_VARCHAR:
        case SQL_GUID:
        case SQL_WCHAR:
        case SQL_WVARCHAR:
            return ceoDbTypeString;
        case SQL_LONGVARCHAR:
        case SQL_WLONGVARCHAR:
            return ceoDbTypeLongString;
        case SQL_BINARY:
        case SQL_VARBINARY:
            return ceoDbTypeBinary;
        case SQL_LONGVARBINARY:
            return ceoDbTypeLongBinary;
    }

    sprintf(buffer, "unsupported SQL data type %d", sqlDataType);
    PyErr_SetString(ceoExceptionNotSupportedError, buffer);
    return NULL;
}


//-----------------------------------------------------------------------------
// ceoDbType_fromPythonType()
//   Return the database type corresponding to the Python type. An exception is
// raised if the Python type is not supported.
//-----------------------------------------------------------------------------
ceoDbType *ceoDbType_fromPythonType(PyTypeObject *type)
{
    char message[250];

    if (type == &PyUnicode_Type)
        return ceoDbTypeString;
    if (type == &PyBytes_Type)
        return ceoDbTypeBinary;
    if (type == &PyFloat_Type)
        return ceoDbTypeDouble;
    if (type == &PyLong_Type)
        return ceoDbTypeInt;
    if (type == ceoPyTypeDecimal)
        return ceoDbTypeDecimal;
    if (type == &PyBool_Type)
        return ceoDbTypeBit;
    if (type == ceoPyTypeDate)
        return ceoDbTypeDate;
    if (type == ceoPyTypeDateTime)
        return ceoDbTypeTimestamp;
    if (type == ceoPyTypeTime)
        return ceoDbTypeTime;

    // no valid type specified
    snprintf(message, sizeof(message), "Python type %s not supported.",
            type->tp_name);
    ceoError_raiseFromString(ceoExceptionNotSupportedError, message, __func__);
    return NULL;
}


//-----------------------------------------------------------------------------
// ceoDbType_fromType()
//   Return the database type corresponding to the type value supplied by the
// user. This could be either a database type, an API type or a base Python
// type.
//-----------------------------------------------------------------------------
ceoDbType *ceoDbType_fromType(PyObject *type)
{
    ceoApiType *apiType;
    int status;

    // check to see if a database type constant was specified
    status = PyObject_IsInstance(type, (PyObject*) &ceoPyTypeDbType);
    if (status < 0)
        return NULL;
    if (status == 1)
        return (ceoDbType*) type;

    // check to see if an API type constant was specified
    status = PyObject_IsInstance(type, (PyObject*) &ceoPyTypeApiType);
    if (status < 0)
        return NULL;
    if (status == 1) {
        apiType = (ceoApiType*) type;
        return (ceoDbType*) PyList_GET_ITEM(apiType->types, 0);
    }

    // check to see if a Python type has been specified
    if (Py_TYPE(type) != &PyType_Type) {
        PyErr_SetString(PyExc_TypeError, "expecting type");
        return NULL;
    }

    return ceoDbType_fromPythonType((PyTypeObject*) type);
}


//-----------------------------------------------------------------------------
// ceoDbType_fromValue()
//   Return the database type corresponding to the value supplied by the user.
//-----------------------------------------------------------------------------
ceoDbType *ceoDbType_fromValue(PyObject *value, SQLUINTEGER *size)
{
    char message[250];

    *size = 0;
    if (value == Py_None) {
        *size = 1;
        return ceoDbTypeString;
    }
    if (PyUnicode_Check(value)) {
        *size = PyUnicode_GetLength(value);
        return ceoDbTypeString;
    }
    if (PyBytes_Check(value)) {
        *size = PyBytes_Size(value);
        return ceoDbTypeBinary;
    }
    if (PyBool_Check(value))
        return ceoDbTypeBit;
    if (PyLong_Check(value))
        return ceoDbTypeBigInt;
    if (PyFloat_Check(value))
        return ceoDbTypeDouble;
    if (Py_TYPE(value) == ceoPyTypeDecimal)
        return ceoDbTypeDecimal;
    if (Py_TYPE(value) == ceoPyTypeTime)
        return ceoDbTypeTime;
    if (Py_TYPE(value) == ceoPyTypeDateTime)
        return ceoDbTypeTimestamp;
    if (Py_TYPE(value) == ceoPyTypeDate)
        return ceoDbTypeDate;

    snprintf(message, sizeof(message), "Python value of type %s not supported",
            Py_TYPE(value)->tp_name);
    ceoError_raiseFromString(ceoExceptionNotSupportedError, message, __func__);
    return NULL;
}


//-----------------------------------------------------------------------------
// ceoDbType_repr()
//   Return a string representation of a queue.
//-----------------------------------------------------------------------------
static PyObject *ceoDbType_repr(ceoDbType *dbType)
{
    PyObject *module, *name, *dbTypeName, *result;

    dbTypeName = PyUnicode_DecodeASCII(dbType->name, strlen(dbType->name),
            NULL);
    if (!dbTypeName)
        return NULL;
    if (ceoUtils_getModuleAndName(Py_TYPE(dbType), &module, &name) < 0) {
        Py_DECREF(dbTypeName);
        return NULL;
    }
    result = ceoUtils_formatString("<%s.%s %s>",
            PyTuple_Pack(3, module, name, dbTypeName));
    Py_DECREF(module);
    Py_DECREF(name);
    Py_DECREF(dbTypeName);
    return result;
}


//-----------------------------------------------------------------------------
// ceoDbType_richCompare()
//   Peforms a comparison between the database type and another Python object.
// Equality (and inequality) are used to match database API types with their
// associated database types.
//-----------------------------------------------------------------------------
static PyObject *ceoDbType_richCompare(ceoDbType* dbType, PyObject* obj,
        int op)
{
    ceoApiType *apiType;
    int status, equal;

    // only equality and inequality can be checked
    if (op != Py_EQ && op != Py_NE) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    // check for exact object
    equal = 0;
    if (obj == (PyObject*) dbType) {
        equal = 1;

    // check for API type
    } else {
        status = PyObject_IsInstance(obj, (PyObject*) &ceoPyTypeApiType);
        if (status < 0)
            return NULL;
        if (status == 1) {
            apiType = (ceoApiType*) obj;
            status = PySequence_Contains(apiType->types, (PyObject*) dbType);
            if (status < 0)
                return NULL;
            equal = (status == 1) ? 1 : 0;
        }
    }

    // determine return value
    if ((equal && op == Py_EQ) || (!equal && op == Py_NE)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}


//-----------------------------------------------------------------------------
// ceoDbType_hash()
//   Return a hash value for the instance.
//-----------------------------------------------------------------------------
static Py_hash_t ceoDbType_hash(ceoDbType *dbType)
{
    return (Py_hash_t) dbType->sqlDataType;
}


//-----------------------------------------------------------------------------
// declaration of members
//-----------------------------------------------------------------------------
static PyMemberDef ceoMembers[] = {
    { "name", T_STRING, offsetof(ceoDbType, name), READONLY },
    { NULL }
};


//-----------------------------------------------------------------------------
// Python type declaration
//-----------------------------------------------------------------------------
PyTypeObject ceoPyTypeDbType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ceODBC._DbType",
    .tp_basicsize = sizeof(ceoDbType),
    .tp_dealloc = (destructor) ceoDbType_free,
    .tp_repr = (reprfunc) ceoDbType_repr,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_members = ceoMembers,
    .tp_richcompare = (richcmpfunc) ceoDbType_richCompare,
    .tp_hash = (hashfunc) ceoDbType_hash
};
