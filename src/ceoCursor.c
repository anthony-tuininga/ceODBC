//-----------------------------------------------------------------------------
// Cursor.c
//   Definition of the Python type for cursors.
//-----------------------------------------------------------------------------

#include "ceoModule.h"

//-----------------------------------------------------------------------------
// ceoCursor_isOpen()
//   Determines if the cursor object is open and if so, if the connection is
// also open.
//-----------------------------------------------------------------------------
static int ceoCursor_isOpen(ceoCursor *cursor)
{
    if (!cursor->handle) {
        PyErr_SetString(ceoExceptionInterfaceError, "not open");
        return -1;
    }
    return ceoConnection_isConnected(cursor->connection);
}


//-----------------------------------------------------------------------------
// ceoCursor_new()
//   Create a new cursor object.
//-----------------------------------------------------------------------------
static PyObject *ceoCursor_new(PyTypeObject *type, PyObject *args,
        PyObject *keywordArgs)
{
    return type->tp_alloc(type, 0);
}


//-----------------------------------------------------------------------------
// ceoCursor_internalInit()
//   Internal method used for creating a new cursor.
//-----------------------------------------------------------------------------
static int ceoCursor_internalInit(ceoCursor *cursor,
        ceoConnection *connection)
{
    SQLRETURN rc;

    // initialize members
    cursor->handle = SQL_NULL_HANDLE;
    Py_INCREF(connection);
    cursor->resultSetVars = NULL;
    cursor->parameterVars = NULL;
    cursor->statement = NULL;
    cursor->rowFactory = NULL;
    cursor->connection = connection;
    cursor->arraySize = 1;
    cursor->bindArraySize = 1;
    cursor->logSql = connection->logSql;
    cursor->setInputSizes = 0;
    cursor->setOutputSize = 0;
    cursor->setOutputSizeColumn = 0;
    cursor->rowCount = 0;
    cursor->fetchBufferRowCount = 0;
    cursor->fetchBufferRowIndex = 0;
    cursor->moreRowsToFetch = 1;

    // allocate handle
    rc = SQLAllocHandle(SQL_HANDLE_STMT, cursor->connection->handle,
            &cursor->handle);
    if (CEO_CONN_CHECK_ERROR(cursor->connection, rc,
            "ceoCursor_init(): allocate statement handle") < 0)
        return -1;

    return 0;
}


//-----------------------------------------------------------------------------
// ceoCursor_internalNew()
//   Internal method of creating a new cursor.
//-----------------------------------------------------------------------------
ceoCursor *ceoCursor_internalNew(ceoConnection *connection)
{
    ceoCursor *cursor;

    cursor = PyObject_NEW(ceoCursor, &ceoPyTypeCursor);
    if (!cursor)
        return NULL;
    if (ceoCursor_internalInit(cursor, connection) < 0) {
        Py_DECREF(cursor);
        return NULL;
    }

    return cursor;
}


//-----------------------------------------------------------------------------
// ceoCursor_init()
//   Create a new cursor object.
//-----------------------------------------------------------------------------
static int ceoCursor_init(ceoCursor *cursor, PyObject *args,
        PyObject *keywordArgs)
{
    ceoConnection *connection;

    if (!PyArg_ParseTuple(args, "O!", &ceoPyTypeConnection, &connection))
        return -1;
    return ceoCursor_internalInit(cursor, connection);
}


//-----------------------------------------------------------------------------
// ceoCursor_repr()
//   Return a string representation of the cursor.
//-----------------------------------------------------------------------------
static PyObject *ceoCursor_repr(ceoCursor *cursor)
{
    PyObject *connectionRepr, *module, *name, *result;

    connectionRepr = PyObject_Repr((PyObject*) cursor->connection);
    if (!connectionRepr)
        return NULL;
    if (ceoUtils_getModuleAndName(Py_TYPE(cursor), &module, &name) < 0) {
        Py_DECREF(connectionRepr);
        return NULL;
    }
    result = ceoUtils_formatString("<%s.%s on %s>",
            PyTuple_Pack(3, module, name, connectionRepr));
    Py_DECREF(module);
    Py_DECREF(name);
    Py_DECREF(connectionRepr);
    return result;
}


//-----------------------------------------------------------------------------
// ceoCursor_free()
//   Deallocate the cursor.
//-----------------------------------------------------------------------------
static void ceoCursor_free(ceoCursor *cursor)
{
    if (cursor->handle && cursor->connection->isConnected)
        SQLFreeHandle(SQL_HANDLE_STMT, cursor->handle);
    Py_CLEAR(cursor->connection);
    Py_CLEAR(cursor->resultSetVars);
    Py_CLEAR(cursor->parameterVars);
    Py_CLEAR(cursor->statement);
    Py_CLEAR(cursor->rowFactory);
    Py_CLEAR(cursor->inputTypeHandler);
    Py_CLEAR(cursor->outputTypeHandler);
    Py_TYPE(cursor)->tp_free((PyObject*) cursor);
}


//-----------------------------------------------------------------------------
// ceoCursor_massageArgs()
//   Massage the arguments. If one argument is passed and that argument is a
// list or tuple, use that value instead of the arguments given.
//-----------------------------------------------------------------------------
static int ceoCursor_massageArgs(PyObject **args, int *argsOffset)
{
    PyObject *temp;

    if (PyTuple_GET_SIZE(*args) == *argsOffset + 1) {
        temp = PyTuple_GET_ITEM(*args, *argsOffset);
        if (PyTuple_Check(temp) || PyList_Check(temp)) {
            *args = PySequence_Tuple(temp);
            if (!*args)
                return -1;
            *argsOffset = 0;
            return 0;
        }
    }

    Py_INCREF(*args);
    return 0;
}


//-----------------------------------------------------------------------------
// ceoCursor_prepareResultSet()
//   Prepare the result set for use.
//-----------------------------------------------------------------------------
static int ceoCursor_prepareResultSet(ceoCursor *cursor)
{
    SQLSMALLINT numColumns;
    SQLUSMALLINT position;
    ceoVar *var;
    SQLRETURN rc;

    // determine the number of columns in the result set
    rc = SQLNumResultCols(cursor->handle, &numColumns);
    if (CEO_CURSOR_CHECK_ERROR(cursor, rc,
            "ceoCursor_prepareResultSet(): determine number of columns") < 0)
        return -1;

    // if the number returned is 0, that means this isn't a query
    if (numColumns == 0)
        return 0;

    // set up the fetch array size
    cursor->fetchArraySize = cursor->arraySize;
    rc = SQLSetStmtAttr(cursor->handle, SQL_ATTR_ROW_ARRAY_SIZE,
            (SQLPOINTER) cursor->fetchArraySize, SQL_IS_UINTEGER);
    if (CEO_CURSOR_CHECK_ERROR(cursor, rc,
            "ceoCursor_prepareResultSet(): set array size") < 0)
        return -1;

    // set up the rows fetched pointer
    rc = SQLSetStmtAttr(cursor->handle, SQL_ATTR_ROWS_FETCHED_PTR,
            &cursor->fetchBufferRowCount, SQL_IS_POINTER);
    if (CEO_CURSOR_CHECK_ERROR(cursor, rc,
            "ceoCursor_prepareResultSet(): set rows fetched pointer") < 0)
        return -1;

    // create a list corresponding to the number of items
    cursor->resultSetVars = PyList_New(numColumns);
    if (!cursor->resultSetVars)
        return -1;

    // create a variable for each column in the result set
    for (position = 0; position < numColumns; position++) {
        var = ceoVar_newForResultSet(cursor, position + 1);
        if (!var)
            return -1;
        PyList_SET_ITEM(cursor->resultSetVars, position, (PyObject *) var);
    }

    // set internal counters
    cursor->rowCount = 0;
    cursor->fetchBufferRowCount = 0;
    cursor->fetchBufferRowIndex = 0;
    cursor->moreRowsToFetch = 1;

    return 0;
}


//-----------------------------------------------------------------------------
// ceoCursor_bindParameterHelper()
//   Helper for setting a bind variable.
//-----------------------------------------------------------------------------
static int ceoCursor_bindParameterHelper(ceoCursor *cursor,
        unsigned numElements, unsigned arrayPos, PyObject *value,
        ceoVar *origVar, ceoVar **newVar, int deferTypeAssignment)
{
    int isValueVar;

    // initialization
    *newVar = NULL;
    isValueVar = (Py_TYPE(value) == &ceoPyTypeVar);

    // handle case where variable is already bound
    if (origVar) {

        // if the value is a variable object, rebind it if necessary
        if (isValueVar) {
            if ( (PyObject*) origVar != value) {
                Py_INCREF(value);
                *newVar = (ceoVar*) value;
                (*newVar)->position = -1;
            }

        // if the number of elements has changed, create a new variable
        // this is only necessary for executemany() since execute() always
        // passes a value of 1 for the number of elements
        } else if (numElements > origVar->numElements) {
            *newVar = ceoVar_internalNew(numElements, origVar->type,
                    origVar->size, origVar->scale);
            if (!*newVar)
                return -1;
            if (ceoVar_setValue(*newVar, arrayPos, value) < 0)
                return -1;

        // otherwise, attempt to set the value
        } else if (ceoVar_setValue(origVar, arrayPos, value) < 0) {

            // executemany() should simply fail after the first element
            if (arrayPos > 0)
                return -1;

            // anything other than index error or type error should fail
            if (!PyErr_ExceptionMatches(PyExc_IndexError) &&
                    !PyErr_ExceptionMatches(PyExc_TypeError))
                return -1;

            // clear the exception and try to create a new variable
            PyErr_Clear();
            origVar = NULL;
        }

    }

    // if no original variable used, create a new one
    if (!origVar) {

        // if the value is a variable object, bind it directly
        if (isValueVar) {
            Py_INCREF(value);
            *newVar = (ceoVar*) value;
            (*newVar)->position = -1;

        // otherwise, create a new variable
        } else if (value != Py_None || !deferTypeAssignment) {
            *newVar = ceoVar_newByValue(cursor, value, numElements);
            if (!*newVar)
                return -1;
            if (ceoVar_setValue(*newVar, arrayPos, value) < 0)
                return -1;
        }

    }

    return 0;
}


//-----------------------------------------------------------------------------
// ceoCursor_logBindParameter()
//   Log the bind parameter.
//-----------------------------------------------------------------------------
static void ceoCursor_logBindParameter(unsigned position, PyObject *value)
{
    PyObject *format, *formatArgs, *positionObj, *message;

    if (!IsLoggingAtLevelForPython(LOG_LEVEL_DEBUG))
        return;
    positionObj = PyLong_FromLong(position);
    if (!positionObj) {
        LogMessageV(LOG_LEVEL_DEBUG, "    %d => cannot build position obj",
                position);
        return;
    }
    formatArgs = PyTuple_Pack(2, positionObj, value);
    Py_DECREF(positionObj);
    if (!formatArgs) {
        LogMessageV(LOG_LEVEL_DEBUG, "    %d => cannot build format args",
                position);
        return;
    }
    format = CEO_STR_FROM_ASCII("    %s => %r");
    if (!format) {
        Py_DECREF(formatArgs);
        LogMessageV(LOG_LEVEL_DEBUG, "    %d => cannot build format",
                position);
        return;
    }
    message = PyUnicode_Format(format, formatArgs);
    Py_DECREF(format);
    Py_DECREF(formatArgs);
    if (!message) {
        LogMessageV(LOG_LEVEL_DEBUG, "    %d => cannot build repr",
                position);
        return;
    }

    WriteMessageForPython(LOG_LEVEL_DEBUG, message);
    Py_DECREF(message);
}


//-----------------------------------------------------------------------------
// ceoCursor_bindParameters()
//   Bind all parameters, creating new ones as needed.
//-----------------------------------------------------------------------------
static int ceoCursor_bindParameters(ceoCursor *cursor, PyObject *parameters,
        int parametersOffset, unsigned numElements, unsigned arrayPos,
        int deferTypeAssignment)
{
    SQLUSMALLINT i, numParams, origNumParams;
    ceoVar *newVar, *origVar;
    PyObject *value;

    // set up the list of parameters
    numParams =
            (SQLUSMALLINT) (PyTuple_GET_SIZE(parameters) - parametersOffset);
    if (cursor->parameterVars) {
        origNumParams = (SQLUSMALLINT) PyList_GET_SIZE(cursor->parameterVars);
    } else {
        origNumParams = 0;
        cursor->parameterVars = PyList_New(numParams);
        if (!cursor->parameterVars)
            return -1;
    }

    // bind parameters
    if (cursor->logSql)
        LogMessageV(LOG_LEVEL_DEBUG, "BIND VARIABLES (%u)", arrayPos);
    for (i = 0; i < numParams; i++) {
        value = PySequence_GetItem(parameters, i + parametersOffset);
        if (!value)
            return -1;
        if (i < origNumParams) {
            origVar = (ceoVar*) PyList_GET_ITEM(cursor->parameterVars, i);
            if ( (PyObject*) origVar == Py_None)
                origVar = NULL;
        } else origVar = NULL;
        if (ceoCursor_bindParameterHelper(cursor, numElements, arrayPos, value,
                origVar, &newVar, deferTypeAssignment) < 0) {
            Py_DECREF(value);
            return -1;
        }
        if (cursor->logSql)
            ceoCursor_logBindParameter(i + 1, value);
        Py_DECREF(value);
        if (newVar) {
            if (i < PyList_GET_SIZE(cursor->parameterVars)) {
                if (PyList_SetItem(cursor->parameterVars, i,
                        (PyObject*) newVar) < 0) {
                    Py_DECREF(newVar);
                    return -1;
                }
            } else {
                if (PyList_Append(cursor->parameterVars,
                        (PyObject*) newVar) < 0) {
                    Py_DECREF(newVar);
                    return -1;
                }
                Py_DECREF(newVar);
            }
            if (ceoVar_bindParameter(newVar, cursor, i + 1) < 0)
                return -1;
        }
        if (!newVar && origVar && origVar->position < 0) {
            if (ceoVar_bindParameter(origVar, cursor, i + 1) < 0)
                return -1;
        }
    }

    return 0;
}


//-----------------------------------------------------------------------------
// ceoCursor_internalCatalogHelper()
//   Internal method used by the catalog methods in order to set up the result
// set that is supposed to be returned.
//-----------------------------------------------------------------------------
PyObject *ceoCursor_internalCatalogHelper(ceoCursor *cursor)
{
    if (ceoCursor_prepareResultSet(cursor) < 0) {
        Py_DECREF(cursor);
        return NULL;
    }

    return (PyObject*) cursor;
}


//-----------------------------------------------------------------------------
// ceoCursor_internalExecuteHelper()
//   Perform the work of executing a cursor and set the rowcount appropriately
// regardless of whether an error takes place.
//-----------------------------------------------------------------------------
static int ceoCursor_internalExecuteHelper(ceoCursor *cursor, SQLRETURN rc)
{
    SQLLEN rowCount;

    // SQL_NO_DATA is returned from statements which do not affect any rows
    if (rc == SQL_NO_DATA) {
        cursor->rowCount = 0;
        return 0;
    }
    if (CEO_CURSOR_CHECK_ERROR(cursor, rc, "Cursor_InternalExecute()") < 0)
        return -1;

    // prepare result set, if necessary
    if (!cursor->resultSetVars && ceoCursor_prepareResultSet(cursor) < 0)
        return -1;

    // determine the value of the rowcount attribute
    if (!cursor->resultSetVars) {
        rc = SQLRowCount(cursor->handle, &rowCount);
        if (CEO_CURSOR_CHECK_ERROR(cursor, rc, "get row count") < 0)
            return -1;
        cursor->rowCount = (unsigned long) rowCount;
    }

    // reset input and output sizes
    cursor->setInputSizes = 0;
    cursor->setOutputSize = 0;
    cursor->setOutputSizeColumn = 0;

    return 0;
}


//-----------------------------------------------------------------------------
// ceoCursor_internalExecute()
//   Perform the work of executing a cursor and set the rowcount appropriately
// regardless of whether an error takes place.
//-----------------------------------------------------------------------------
static int ceoCursor_internalExecute(ceoCursor *cursor)
{
    SQLRETURN rc;

    Py_BEGIN_ALLOW_THREADS
    rc = SQLExecute(cursor->handle);
    Py_END_ALLOW_THREADS
    return ceoCursor_internalExecuteHelper(cursor, rc);
}


//-----------------------------------------------------------------------------
// ceoCursor_itemDescription()
//   Returns a tuple for the column as defined by the DB API.
//-----------------------------------------------------------------------------
static PyObject *ceoCursor_itemDescription(ceoCursor *cursor,
        SQLUSMALLINT position)
{
    SQLSMALLINT dataType, nameLength, scale, nullable;
    SQLULEN precision, size, displaySize;
    SQLCHAR name[256];
    ceoDbType *dbType;
    PyObject *tuple;
    SQLRETURN rc;
    int i;

    // retrieve information about the column
    rc = SQLDescribeColA(cursor->handle, position, name, sizeof(name),
            &nameLength, &dataType, &precision, &scale, &nullable);
    if (nameLength > (SQLSMALLINT) sizeof(name) - 1)
        nameLength = (SQLSMALLINT) (sizeof(name) - 1);
    if (CEO_CURSOR_CHECK_ERROR(cursor, rc,
            "ceoCursor_itemDescription(): get column info") < 0)
        return NULL;

    // determine variable type
    dbType = ceoDbType_fromSqlDataType(dataType);
    if (!dbType)
        return NULL;

    // create the tuple and populate it
    tuple = PyTuple_New(7);
    if (!tuple)
        return NULL;

    // reset precision and scale for all but numbers
    size = precision;
    if (dbType != ceoDbTypeBigInt &&
            dbType != ceoDbTypeBit &&
            dbType != ceoDbTypeInt &&
            dbType != ceoDbTypeDouble &&
            dbType != ceoDbTypeDecimal) {
        precision = 0;
        scale = 0;
    }

    // set display size based on data type
    displaySize = size;
    if (dbType == ceoDbTypeBigInt || dbType == ceoDbTypeInt) {
        displaySize = size + 1;
    } else if (dbType == ceoDbTypeDouble || dbType == ceoDbTypeDecimal) {
        displaySize = size + 1;
        if (scale > 0)
            displaySize++;
    }

    // set each of the items in the tuple
    Py_INCREF((PyObject*) dbType);
    PyTuple_SET_ITEM(tuple, 0,
            PyUnicode_DecodeUTF8((const char*) name, nameLength, NULL));
    PyTuple_SET_ITEM(tuple, 1, (PyObject*) dbType);
    PyTuple_SET_ITEM(tuple, 2, PyLong_FromLong(displaySize));
    PyTuple_SET_ITEM(tuple, 3, PyLong_FromLong(size));
    PyTuple_SET_ITEM(tuple, 4, PyLong_FromLong(precision));
    PyTuple_SET_ITEM(tuple, 5, PyLong_FromLong(scale));
    PyTuple_SET_ITEM(tuple, 6, PyBool_FromLong(nullable != SQL_NO_NULLS));

    // make sure the tuple is ok
    for (i = 0; i < 7; i++) {
        if (!PyTuple_GET_ITEM(tuple, i)) {
            Py_DECREF(tuple);
            return NULL;
        }
    }

    return tuple;
}


//-----------------------------------------------------------------------------
// ceoCursor_getDescription()
//   Return a list of 7-tuples consisting of the description of the define
// variables.
//-----------------------------------------------------------------------------
static PyObject *ceoCursor_getDescription(ceoCursor *cursor, void *arg)
{
    PyObject *results, *tuple;
    int numItems, index;

    // make sure the cursor is open
    if (ceoCursor_isOpen(cursor) < 0)
        return NULL;

    // if no statement has been executed yet, return None
    if (!cursor->resultSetVars) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    // create a list of the required length
    numItems = PyList_GET_SIZE(cursor->resultSetVars);
    results = PyList_New(numItems);
    if (!results)
        return NULL;

    // create tuples corresponding to the select-items
    for (index = 0; index < numItems; index++) {
        tuple = ceoCursor_itemDescription(cursor, index + 1);
        if (!tuple) {
            Py_DECREF(results);
            return NULL;
        }
        PyList_SET_ITEM(results, index, tuple);
    }

    return results;
}


//-----------------------------------------------------------------------------
// ceoCursor_getName()
//   Return the name associated with the cursor.
//-----------------------------------------------------------------------------
static PyObject *ceoCursor_getName(ceoCursor *cursor, void *arg)
{
    SQLSMALLINT nameLength;
    SQLCHAR name[255];
    SQLRETURN rc;

    rc = SQLGetCursorNameA(cursor->handle, name, sizeof(name),
            &nameLength);
    if (nameLength > (SQLSMALLINT) sizeof(name) - 1)
        nameLength = (SQLSMALLINT) (sizeof(name) - 1);
    if (CEO_CURSOR_CHECK_ERROR(cursor, rc, "ceoCursor_getName()") < 0)
        return NULL;
    return PyUnicode_DecodeUTF8((const char*) name, nameLength, NULL);
}


//-----------------------------------------------------------------------------
// ceoCursor_setName()
//   Set the name associated with the cursor.
//-----------------------------------------------------------------------------
static int ceoCursor_setName(ceoCursor *cursor, PyObject *value, void *arg)
{
    Py_ssize_t nameLength;
    const char *name;
    SQLRETURN rc;

    name = PyUnicode_AsUTF8AndSize(value, &nameLength);
    if (!name)
        return -1;
    rc = SQLSetCursorNameA(cursor->handle, (SQLCHAR*) name, nameLength);
    if (CEO_CURSOR_CHECK_ERROR(cursor, rc, "ceoCursor_setName()") < 0)
        return -1;

    return 0;
}


//-----------------------------------------------------------------------------
// ceoCursor_close()
//   Close the cursor.
//-----------------------------------------------------------------------------
static PyObject *ceoCursor_close(ceoCursor *cursor, PyObject *args)
{
    SQLRETURN rc;

    // make sure cursor is actually open
    if (ceoCursor_isOpen(cursor) < 0)
        return NULL;

    // close the cursor
    rc = SQLFreeHandle(SQL_HANDLE_STMT, cursor->handle);
    if (CEO_CURSOR_CHECK_ERROR(cursor, rc, "ceoCursor_close()") < 0)
        return NULL;
    cursor->handle = NULL;

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// ceoCursor_nextSet()
//   Return the next result set for the cursor.
//-----------------------------------------------------------------------------
static PyObject *ceoCursor_nextSet(ceoCursor *cursor, PyObject *args)
{
    SQLRETURN rc;

    // make sure cursor is actually open
    if (ceoCursor_isOpen(cursor) < 0)
        return NULL;

    // get the next result set
    rc = SQLMoreResults(cursor->handle);
    if (rc == SQL_NO_DATA) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    if (CEO_CURSOR_CHECK_ERROR(cursor, rc, "ceoCursor_nextSet()") < 0)
        return NULL;

    // set up result set
    Py_CLEAR(cursor->resultSetVars);
    if (ceoCursor_prepareResultSet(cursor) < 0)
        return NULL;

    Py_INCREF(cursor);
    return (PyObject*) cursor;
}


//-----------------------------------------------------------------------------
// ceoCursor_createRow()
//   Create an object for the row. The object created is a tuple unless a row
// factory function has been defined in which case it is the result of the
// row factory function called with the argument tuple that would otherwise be
// returned.
//-----------------------------------------------------------------------------
static PyObject *ceoCursor_createRow(ceoCursor *cursor)
{
    PyObject *tuple, *item, *result;
    int numItems, pos;
    ceoVar *var;

    // create a new tuple
    numItems = PyList_GET_SIZE(cursor->resultSetVars);
    tuple = PyTuple_New(numItems);
    if (!tuple)
        return NULL;

    // acquire the value for each item
    for (pos = 0; pos < numItems; pos++) {
        var = (ceoVar*) PyList_GET_ITEM(cursor->resultSetVars, pos);
        item = ceoVar_getValue(var, cursor->fetchBufferRowIndex);
        if (!item) {
            Py_DECREF(tuple);
            return NULL;
        }
        PyTuple_SET_ITEM(tuple, pos, item);
    }

    // increment row counters
    cursor->fetchBufferRowIndex++;
    cursor->rowCount++;

    // if a row factory is defined, call it
    if (cursor->rowFactory && cursor->rowFactory != Py_None) {
        result = PyObject_CallObject(cursor->rowFactory, tuple);
        Py_DECREF(tuple);
        return result;
    }

    return tuple;
}


//-----------------------------------------------------------------------------
// ceoCursor_internalPrepare()
//   Internal method for preparing a statement for execution.
//-----------------------------------------------------------------------------
static int ceoCursor_internalPrepare(ceoCursor *cursor, PyObject *statement)
{
    PyObject *format, *formatArgs, *message;
    Py_ssize_t sqlLength;
    const char *sql;
    SQLRETURN rc;

    // make sure we don't get a situation where nothing is to be executed
    if (statement == Py_None && !cursor->statement) {
        PyErr_SetString(ceoExceptionProgrammingError,
                "no statement specified and no prior statement prepared");
        return -1;
    }

    // close original statement if necessary in order to discard results
    if (cursor->statement)
        SQLCloseCursor(cursor->handle);

    // keep track of statement
    if (statement != Py_None && statement != cursor->statement) {
        Py_INCREF(statement);
        Py_XDECREF(cursor->statement);
        cursor->statement = statement;
    }

    // log the statement, if applicable
    if (cursor->logSql) {
        format = CEO_STR_FROM_ASCII("SQL\n%s");
        if (!format)
            return -1;
        formatArgs = PyTuple_Pack(1, statement);
        if (!formatArgs) {
            Py_DECREF(format);
            return -1;
        }
        message = PyUnicode_Format(format, formatArgs);
        Py_DECREF(format);
        Py_DECREF(formatArgs);
        if (!message)
            return -1;
        WriteMessageForPython(LOG_LEVEL_DEBUG, message);
    }

    // clear previous result set parameters
    Py_XDECREF(cursor->resultSetVars);
    cursor->resultSetVars = NULL;
    Py_XDECREF(cursor->rowFactory);
    cursor->rowFactory = NULL;

    // prepare statement
    sql = PyUnicode_AsUTF8AndSize(cursor->statement, &sqlLength);
    if (!sql)
        return -1;
    Py_BEGIN_ALLOW_THREADS
    rc = SQLPrepareA(cursor->handle, (SQLCHAR*) sql, sqlLength);
    Py_END_ALLOW_THREADS
    if (CEO_CURSOR_CHECK_ERROR(cursor, rc, "ceoCursor_internalPrepare()") < 0)
        return -1;

    // clear parameters
    if (!cursor->setInputSizes) {
        Py_XDECREF(cursor->parameterVars);
        cursor->parameterVars = NULL;
    }

    return 0;
}


//-----------------------------------------------------------------------------
// ceoCursor_prepare()
//   Prepare the statement for execution.
//-----------------------------------------------------------------------------
static PyObject *ceoCursor_prepare(ceoCursor *cursor, PyObject *args)
{
    PyObject *statement;

    if (!PyArg_ParseTuple(args, "O", &statement))
        return NULL;
    if (ceoCursor_isOpen(cursor) < 0)
        return NULL;
    if (ceoCursor_internalPrepare(cursor, statement) < 0)
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// ceoCursor_execDirect()
//   Execute the statement.
//-----------------------------------------------------------------------------
static PyObject *ceoCursor_execDirect(ceoCursor *cursor, PyObject *args)
{
    PyObject *statement, *format, *formatArgs, *message;
    Py_ssize_t sqlTextLength;
    const char *sqlText;
    SQLRETURN rc;

    // parse arguments
    if (!PyArg_ParseTuple(args, "O", &statement))
        return NULL;
    if (ceoCursor_isOpen(cursor) < 0)
        return NULL;

    // log the statement, if applicable
    if (cursor->logSql) {
        format = CEO_STR_FROM_ASCII("SQL\n%s");
        if (!format)
            return NULL;
        formatArgs = PyTuple_Pack(1, statement);
        if (!formatArgs) {
            Py_DECREF(format);
            return NULL;
        }
        message = PyUnicode_Format(format, formatArgs);
        Py_DECREF(format);
        Py_DECREF(formatArgs);
        if (!message)
            return NULL;
        WriteMessageForPython(LOG_LEVEL_DEBUG, message);
    }

    // clear previous statement, if applicable
    if (cursor->statement)
        SQLCloseCursor(cursor->handle);
    Py_XDECREF(cursor->statement);
    cursor->statement = NULL;
    Py_XDECREF(cursor->resultSetVars);
    cursor->resultSetVars = NULL;
    Py_XDECREF(cursor->rowFactory);
    cursor->rowFactory = NULL;
    Py_XDECREF(cursor->parameterVars);
    cursor->parameterVars = NULL;

    // execute the statement
    sqlText = PyUnicode_AsUTF8AndSize(statement, &sqlTextLength);
    if (!sqlText)
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    rc = SQLExecDirectA(cursor->handle, (SQLCHAR*) sqlText, sqlTextLength);
    Py_END_ALLOW_THREADS
    if (ceoCursor_internalExecuteHelper(cursor, rc) < 0)
        return NULL;

    // for queries, return the cursor for convenience
    if (cursor->resultSetVars) {
        Py_INCREF(cursor);
        return (PyObject*) cursor;
    }

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// ceoCursor_execute()
//   Execute the statement.
//-----------------------------------------------------------------------------
static PyObject *ceoCursor_execute(ceoCursor *cursor, PyObject *args)
{
    int numArgs, argsOffset;
    PyObject *statement;

    // verify we have the right arguments
    numArgs = PyTuple_GET_SIZE(args);
    if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "expecting statement");
        return NULL;
    }
    statement = PyTuple_GET_ITEM(args, 0);

    // prepare the statement for execution
    if (ceoCursor_isOpen(cursor) < 0)
        return NULL;
    if (ceoCursor_internalPrepare(cursor, statement) < 0)
        return NULL;

    // bind the parameters
    argsOffset = 1;
    if (ceoCursor_massageArgs(&args, &argsOffset) < 0)
        return NULL;
    if (ceoCursor_bindParameters(cursor, args, argsOffset, 1, 0, 0) < 0) {
        Py_DECREF(args);
        return NULL;
    }
    Py_DECREF(args);

    // actually execute the statement
    if (ceoCursor_internalExecute(cursor) < 0)
        return NULL;

    // for queries, return the cursor for convenience
    if (cursor->resultSetVars) {
        Py_INCREF(cursor);
        return (PyObject*) cursor;
    }

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// ceoCursor_executeMany()
//   Execute the statement many times. The number of times is equivalent to the
// number of elements in the array of dictionaries.
//-----------------------------------------------------------------------------
static PyObject *ceoCursor_executeMany(ceoCursor *cursor, PyObject *args)
{
    PyObject *arguments, *listOfArguments, *statement;
    Py_ssize_t i, numRows;
    SQLULEN value;
    SQLRETURN rc;

    // expect statement text (optional) plus list of sequences
    if (!PyArg_ParseTuple(args, "OO!", &statement, &PyList_Type,
            &listOfArguments))
        return NULL;

    // make sure the cursor is open
    if (ceoCursor_isOpen(cursor) < 0)
        return NULL;

    // prepare the statement
    if (ceoCursor_internalPrepare(cursor, statement) < 0)
        return NULL;

    // perform binds
    numRows = PyList_GET_SIZE(listOfArguments);
    for (i = 0; i < numRows; i++) {
        arguments = PyList_GET_ITEM(listOfArguments, i);
        if (!PySequence_Check(arguments)) {
            PyErr_SetString(ceoExceptionInterfaceError,
                    "expecting a list of sequences");
            return NULL;
        }
        if (ceoCursor_bindParameters(cursor, arguments, 0, numRows, i,
                (i < numRows - 1)) < 0)
            return NULL;
    }

    // set the number of parameters bound
    value = (SQLULEN) numRows;
    rc = SQLSetStmtAttr(cursor->handle, SQL_ATTR_PARAMSET_SIZE,
            (SQLPOINTER) value, SQL_IS_UINTEGER);
    if (CEO_CURSOR_CHECK_ERROR(cursor, rc,
            "ceoCursor_executeMany(): set paramset size") < 0)
        return NULL;

    // execute the statement
    if (ceoCursor_internalExecute(cursor) < 0)
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// ceoCursor_callBuildStatement()
//   Call procedure or function.
//-----------------------------------------------------------------------------
static int ceoCursor_callBuildStatement(PyObject *name,
        ceoVar *returnValue, PyObject *args, PyObject **statementObj)
{
    PyObject *format, *formatArgs;
    int numArgs, statementSize, i;
    char *statement, *ptr;

    // allocate memory for statement
    numArgs = PyTuple_GET_SIZE(args);
    statementSize = numArgs * 2 + 16;
    statement = PyMem_Malloc(statementSize);
    if (!statement) {
        PyErr_NoMemory();
        return -1;
    }

    // build the statement
    strcpy(statement, "{");
    if (returnValue)
        strcat(statement, "? = ");
    strcat(statement, "CALL %s");
    ptr = statement + strlen(statement);
    if (numArgs > 0) {
        *ptr++ = '(';
        for (i = 0; i < numArgs; i++) {
            if (i > 0)
                *ptr++ = ',';
            *ptr++ = '?';
        }
        *ptr++ = ')';
    }
    *ptr++ = '}';
    *ptr = '\0';

    // create statement object
    format = CEO_STR_FROM_ASCII(statement);
    PyMem_Free(statement);
    if (!format)
        return -1;
    formatArgs = PyTuple_Pack(1, name);
    if (!formatArgs) {
        Py_DECREF(format);
        return -1;
    }
    *statementObj = PyUnicode_Format(format, formatArgs);
    Py_DECREF(format);
    Py_DECREF(formatArgs);
    if (!*statementObj)
        return -1;

    return 0;
}


//-----------------------------------------------------------------------------
// ceoCursor_call()
//   Call procedure or function.
//-----------------------------------------------------------------------------
static int ceoCursor_call(ceoCursor *cursor, ceoVar *returnValueVar,
        PyObject *procedureName, PyObject *args, int argsOffset)
{
    PyObject *statement, *temp;

    // determine the arguments to the procedure
    if (ceoCursor_massageArgs(&args, &argsOffset) < 0)
        return -1;
    if (argsOffset > 0) {
        temp = PyTuple_GetSlice(args, argsOffset, PyTuple_GET_SIZE(args));
        Py_DECREF(args);
        if (!temp)
            return -1;
        args = temp;
    }

    // build up the statement
    if (ceoCursor_callBuildStatement(procedureName, returnValueVar, args,
            &statement) < 0)
        return -1;

    // add the return value, if necessary, to the arguments
    if (returnValueVar) {
        temp = PySequence_List(args);
        Py_DECREF(args);
        if (!temp)
            return -1;
        if (PyList_Insert(temp, 0, (PyObject*) returnValueVar) < 0) {
            Py_DECREF(temp);
            return -1;
        }
        args = temp;
    }

    // execute the statement on the cursor
    temp = PyObject_CallMethod( (PyObject*) cursor, "execute", "OO", statement,
            args);
    Py_DECREF(args);
    if (!temp)
        return -1;
    Py_DECREF(temp);

    return 0;
}


//-----------------------------------------------------------------------------
// ceoCursor_callFunc()
//   Call function, returning the returned value.
//-----------------------------------------------------------------------------
static PyObject *ceoCursor_callFunc(ceoCursor *cursor, PyObject *args)
{
    PyObject *functionName, *returnType, *results;
    ceoVar *returnValueVar;
    int numArgs;

    // verify we have the right arguments
    numArgs = PyTuple_GET_SIZE(args);
    if (numArgs < 2) {
        PyErr_SetString(PyExc_TypeError,
                "expecting function name and return type");
        return NULL;
    }
    functionName = PyTuple_GET_ITEM(args, 0);
    returnType = PyTuple_GET_ITEM(args, 1);
    if (!PyUnicode_Check(functionName)) {
        PyErr_SetString(PyExc_TypeError, "expecting a string");
        return NULL;
    }

    // create the return value variable
    returnValueVar = ceoVar_newByType(cursor, returnType, 1);
    if (!returnValueVar)
        return NULL;
    returnValueVar->input = 0;
    returnValueVar->output = 1;

    // call the function
    if (ceoCursor_call(cursor, returnValueVar, functionName, args, 2) < 0)
        return NULL;

    // create the return value
    results = ceoVar_getValue(returnValueVar, 0);
    Py_DECREF(returnValueVar);
    return results;
}


//-----------------------------------------------------------------------------
// ceoCursor_callProc()
//   Call procedure, return (possibly) modified set of values.
//-----------------------------------------------------------------------------
static PyObject *ceoCursor_callProc(ceoCursor *cursor, PyObject *args)
{
    PyObject *procedureName, *results, *var, *temp;
    int numArgs, i;

    // verify we have the right arguments
    numArgs = PyTuple_GET_SIZE(args);
    if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "expecting procedure name");
        return NULL;
    }
    procedureName = PyTuple_GET_ITEM(args, 0);
    if (!PyUnicode_Check(procedureName)) {
        PyErr_SetString(PyExc_TypeError, "expecting a string");
        return NULL;
    }

    // call the procedure
    if (ceoCursor_call(cursor, NULL, procedureName, args, 1) < 0)
        return NULL;

    // create the return value
    numArgs = PyList_GET_SIZE(cursor->parameterVars);
    results = PyList_New(numArgs);
    if (!results)
        return NULL;
    for (i = 0; i < numArgs; i++) {
        var = PyList_GET_ITEM(cursor->parameterVars, i);
        temp = ceoVar_getValue((ceoVar*) var, 0);
        if (!temp) {
            Py_DECREF(results);
            return NULL;
        }
        PyList_SET_ITEM(results, i, temp);
    }

    return results;
}


//-----------------------------------------------------------------------------
// ceoCursor_verifyFetch()
//   Verify that fetching may happen from this cursor.
//-----------------------------------------------------------------------------
static int ceoCursor_verifyFetch(ceoCursor *cursor)
{
    // make sure the cursor is open
    if (ceoCursor_isOpen(cursor) < 0)
        return -1;

    // make sure the cursor is for a query
    if (!cursor->resultSetVars) {
        PyErr_SetString(ceoExceptionInterfaceError, "not a query");
        return -1;
    }

    return 0;
}


//-----------------------------------------------------------------------------
// ceoCursor_internalFetch()
//   Performs the actual fetch from the database.
//-----------------------------------------------------------------------------
static int ceoCursor_internalFetch(ceoCursor *cursor)
{
    SQLRETURN rc;

    if (!cursor->resultSetVars) {
        PyErr_SetString(ceoExceptionInterfaceError, "query not executed");
        return -1;
    }
    Py_BEGIN_ALLOW_THREADS
    rc = SQLFetch(cursor->handle);
    Py_END_ALLOW_THREADS
    if (rc == SQL_NO_DATA) {
        cursor->fetchBufferRowCount = 0;
        cursor->moreRowsToFetch = 0;
    } else if (CEO_CURSOR_CHECK_ERROR(cursor, rc,
            "ceoCursor_internalFetch(): fetch") < 0) {
        return -1;
    }
    cursor->fetchBufferRowIndex = 0;
    return 0;
}


//-----------------------------------------------------------------------------
// ceoCursor_moreRows()
//   Returns an integer indicating if more rows can be retrieved from the
// cursor.
//-----------------------------------------------------------------------------
static int ceoCursor_moreRows(ceoCursor *cursor)
{
    if (cursor->fetchBufferRowIndex >= cursor->fetchBufferRowCount) {
        if (cursor->moreRowsToFetch && ceoCursor_internalFetch(cursor) < 0)
            return -1;
    }
    return cursor->moreRowsToFetch;
}


//-----------------------------------------------------------------------------
// ceoCursor_multiFetch()
//   Return a list consisting of the remaining rows up to the given row limit
// (if specified).
//-----------------------------------------------------------------------------
static PyObject *ceoCursor_multiFetch(ceoCursor *cursor,
        unsigned long rowLimit)
{
    PyObject *results, *row;
    unsigned long rowNum;
    int rc;

    // create an empty list
    results = PyList_New(0);
    if (!results)
        return NULL;

    // fetch as many rows as possible
    for (rowNum = 0; rowLimit == 0 || rowNum < rowLimit; rowNum++) {
        rc = ceoCursor_moreRows(cursor);
        if (rc < 0) {
            Py_DECREF(results);
            return NULL;
        } else if (rc == 0) {
            break;
        } else {
            row = ceoCursor_createRow(cursor);
            if (!row) {
                Py_DECREF(results);
                return NULL;
            }
            if (PyList_Append(results, row) < 0) {
                Py_DECREF(row);
                Py_DECREF(results);
                return NULL;
            }
            Py_DECREF(row);
        }
    }

    return results;
}

//-----------------------------------------------------------------------------
// ceoCursor_fetchOne()
//   Fetch a single row from the cursor.
//-----------------------------------------------------------------------------
static PyObject *ceoCursor_fetchOne(ceoCursor *cursor, PyObject *args)
{
    int rc;

    // verify fetch can be performed
    if (ceoCursor_verifyFetch(cursor) < 0)
        return NULL;

    // setup return value
    rc = ceoCursor_moreRows(cursor);
    if (rc < 0)
        return NULL;
    else if (rc > 0)
        return ceoCursor_createRow(cursor);

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// ceoCursor_fetchMany()
//   Fetch multiple rows from the cursor based on the arraysize.
//-----------------------------------------------------------------------------
static PyObject *ceoCursor_fetchMany(ceoCursor *cursor, PyObject *args,
        PyObject *keywordArgs)
{
    static char *keywordList[] = { "numRows", NULL };
    unsigned long rowLimit;

    // parse arguments -- optional rowlimit expected
    rowLimit = cursor->arraySize;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|k", keywordList,
            &rowLimit))
        return NULL;

    // verify fetch can be performed
    if (ceoCursor_verifyFetch(cursor) < 0)
        return NULL;

    return ceoCursor_multiFetch(cursor, rowLimit);
}


//-----------------------------------------------------------------------------
// ceoCursor_fetchAll()
//   Fetch all remaining rows from the cursor.
//-----------------------------------------------------------------------------
static PyObject *ceoCursor_fetchAll(ceoCursor *cursor, PyObject *args)
{
    if (ceoCursor_verifyFetch(cursor) < 0)
        return NULL;
    return ceoCursor_multiFetch(cursor, 0);
}


//-----------------------------------------------------------------------------
// ceoCursor_setInputSizes()
//   Set the sizes of the bind variables.
//-----------------------------------------------------------------------------
static PyObject *ceoCursor_setInputSizes(ceoCursor *cursor, PyObject *args)
{
    PyObject *parameterVars, *value, *inputValue;
    int numArgs, i, argsOffset;

    // make sure the cursor is open
    if (ceoCursor_isOpen(cursor) < 0)
        return NULL;

    // massage the arguments
    argsOffset = 0;
    if (ceoCursor_massageArgs(&args, &argsOffset) < 0)
        return NULL;

    // initialization of parameter variables
    numArgs = PyTuple_GET_SIZE(args) - argsOffset;
    parameterVars = PyList_New(numArgs);

    // process each input
    for (i = 0; i < numArgs; i++) {
        inputValue = PyTuple_GET_ITEM(args, i + argsOffset);
        if (inputValue == Py_None) {
            Py_INCREF(Py_None);
            value = Py_None;
        } else {
            value = (PyObject*) ceoVar_newByType(cursor, inputValue,
                    cursor->bindArraySize);
            if (!value) {
                Py_DECREF(parameterVars);
                Py_DECREF(args);
                return NULL;
            }
        }
        PyList_SET_ITEM(parameterVars, i, value);
    }

    // overwrite existing parameter vars, if any
    Py_XDECREF(cursor->parameterVars);
    cursor->parameterVars = parameterVars;
    cursor->setInputSizes = 1;

    Py_DECREF(args);
    Py_INCREF(cursor->parameterVars);
    return cursor->parameterVars;
}


//-----------------------------------------------------------------------------
// ceoCursor_setOutputSize()
//   Set the size of all of the long columns or just one of them.
//-----------------------------------------------------------------------------
static PyObject *ceoCursor_setOutputSize(ceoCursor *cursor, PyObject *args)
{
    cursor->setOutputSizeColumn = 0;
    if (!PyArg_ParseTuple(args, "i|i", &cursor->setOutputSize,
            &cursor->setOutputSizeColumn))
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// ceoCursor_var()
//   Create a bind variable and return it.
//-----------------------------------------------------------------------------
static PyObject *ceoCursor_var(ceoCursor *cursor, PyObject *args,
        PyObject *keywordArgs)
{
    static char *keywordList[] = { "type", "size", "scale", "arraysize",
            "inconverter", "outconverter", "input", "output", NULL };
    int size, arraySize, scale, input, output;
    PyObject *inConverter, *outConverter;
    ceoDbType *dbType;
    PyObject *type;
    ceoVar *var;

    // parse arguments
    size = 0;
    input = 1;
    output = 0;
    arraySize = cursor->bindArraySize;
    inConverter = outConverter = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "O|iiiOOii",
            keywordList, &type, &size, &scale, &arraySize, &inConverter,
            &outConverter, &input, &output))
        return NULL;

    // determine the database type
    dbType = ceoDbType_fromType(type);
    if (!dbType)
        return NULL;

    // create the variable
    var = ceoVar_internalNew(arraySize, dbType, size, scale);
    if (!var)
        return NULL;

    // set associated variables
    Py_XINCREF(inConverter);
    var->inConverter = inConverter;
    Py_XINCREF(outConverter);
    var->outConverter = outConverter;
    var->input = input;
    var->output = output;

    return (PyObject*) var;
}


//-----------------------------------------------------------------------------
// ceoCursor_getIter()
//   Return a reference to the cursor which supports the iterator protocol.
//-----------------------------------------------------------------------------
static PyObject *ceoCursor_getIter(ceoCursor *cursor)
{
    if (ceoCursor_verifyFetch(cursor) < 0)
        return NULL;
    Py_INCREF(cursor);
    return (PyObject*) cursor;
}


//-----------------------------------------------------------------------------
// ceoCursor_getNext()
//   Return a reference to the cursor which supports the iterator protocol.
//-----------------------------------------------------------------------------
static PyObject *ceoCursor_getNext(ceoCursor *cursor)
{
    int rc;

    if (ceoCursor_verifyFetch(cursor) < 0)
        return NULL;
    rc = ceoCursor_moreRows(cursor);
    if (rc < 0)
        return NULL;
    else if (rc > 0)
        return ceoCursor_createRow(cursor);

    // no more rows, return NULL without setting an exception
    return NULL;
}


//-----------------------------------------------------------------------------
// declaration of methods for the Python type
//-----------------------------------------------------------------------------
static PyMethodDef ceoMethods[] = {
    { "execute", (PyCFunction) ceoCursor_execute, METH_VARARGS },
    { "executemany", (PyCFunction) ceoCursor_executeMany, METH_VARARGS },
    { "fetchall", (PyCFunction) ceoCursor_fetchAll, METH_NOARGS },
    { "fetchone", (PyCFunction) ceoCursor_fetchOne, METH_NOARGS },
    { "fetchmany", (PyCFunction) ceoCursor_fetchMany,
              METH_VARARGS | METH_KEYWORDS },
    { "prepare", (PyCFunction) ceoCursor_prepare, METH_VARARGS },
    { "setinputsizes", (PyCFunction) ceoCursor_setInputSizes, METH_VARARGS },
    { "setoutputsize", (PyCFunction) ceoCursor_setOutputSize, METH_VARARGS },
    { "callfunc", (PyCFunction) ceoCursor_callFunc, METH_VARARGS },
    { "callproc", (PyCFunction) ceoCursor_callProc, METH_VARARGS },
    { "close", (PyCFunction) ceoCursor_close, METH_NOARGS },
    { "nextset", (PyCFunction) ceoCursor_nextSet, METH_NOARGS },
    { "execdirect", (PyCFunction) ceoCursor_execDirect, METH_VARARGS },
    { "var", (PyCFunction) ceoCursor_var, METH_VARARGS | METH_KEYWORDS },
    { NULL, NULL }
};


//-----------------------------------------------------------------------------
// declaration of members for the Python type
//-----------------------------------------------------------------------------
static PyMemberDef ceoMembers[] = {
    { "arraysize", T_INT, offsetof(ceoCursor, arraySize), 0 },
    { "bindarraysize", T_INT, offsetof(ceoCursor, bindArraySize), 0 },
    { "logsql", T_INT, offsetof(ceoCursor, logSql), 0 },
    { "rowcount", T_INT, offsetof(ceoCursor, rowCount), READONLY },
    { "statement", T_OBJECT, offsetof(ceoCursor, statement), READONLY },
    { "connection", T_OBJECT_EX, offsetof(ceoCursor, connection), READONLY },
    { "rowfactory", T_OBJECT, offsetof(ceoCursor, rowFactory), 0 },
    { "inputtypehandler", T_OBJECT, offsetof(ceoCursor, inputTypeHandler),
            0 },
    { "outputtypehandler", T_OBJECT, offsetof(ceoCursor, outputTypeHandler),
            0 },
    { NULL }
};


//-----------------------------------------------------------------------------
// declaration of calculated members for the Python type
//-----------------------------------------------------------------------------
static PyGetSetDef ceoCalcMembers[] = {
    { "description", (getter) ceoCursor_getDescription, 0, 0, 0 },
    { "name", (getter) ceoCursor_getName, (setter) ceoCursor_setName, 0, 0 },
    { NULL }
};


//-----------------------------------------------------------------------------
// declaration of the Python type
//-----------------------------------------------------------------------------
PyTypeObject ceoPyTypeCursor = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ceODBC.Cursor",
    .tp_basicsize = sizeof(ceoCursor),
    .tp_dealloc = (destructor) ceoCursor_free,
    .tp_repr = (reprfunc) ceoCursor_repr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_iter = (getiterfunc) ceoCursor_getIter,
    .tp_iternext = (iternextfunc) ceoCursor_getNext,
    .tp_methods = ceoMethods,
    .tp_members = ceoMembers,
    .tp_getset = ceoCalcMembers,
    .tp_init = (initproc) ceoCursor_init,
    .tp_new = ceoCursor_new
};
