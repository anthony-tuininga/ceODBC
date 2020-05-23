//-----------------------------------------------------------------------------
// Connection.c
//   Definition of the Python type for connections.
//-----------------------------------------------------------------------------

#include "ceoModule.h"

//-----------------------------------------------------------------------------
// ceoConnection_isConnected()
//   Determines if the connection object is connected to the database. If not,
// a Python exception is raised.
//-----------------------------------------------------------------------------
int ceoConnection_isConnected(ceoConnection *conn)
{
    if (!conn->isConnected) {
        PyErr_SetString(g_InterfaceErrorException, "not connected");
        return -1;
    }
    return 0;
}


//-----------------------------------------------------------------------------
// ceoConnection_new()
//   Create a new connection object and return it.
//-----------------------------------------------------------------------------
static PyObject* ceoConnection_new(PyTypeObject *type, PyObject *args,
        PyObject *keywordArgs)
{
    ceoConnection *conn;

    // create the object
    conn = (ceoConnection*) type->tp_alloc(type, 0);
    if (!conn)
        return NULL;
    conn->handleType = SQL_HANDLE_DBC;
    conn->handle = SQL_NULL_HANDLE;
    conn->env = NULL;
    conn->dsn = NULL;
    conn->isConnected = 0;
#ifdef WITH_CX_LOGGING
    conn->logSql = 1;
#else
    conn->logSql = 0;
#endif

    return (PyObject*) conn;
}


//-----------------------------------------------------------------------------
// ceoConnection_removePasswordFromDsn()
//   Attempt to remove the password from the DSN for security reasons.
//-----------------------------------------------------------------------------
static PyObject *ceoConnection_removePasswordFromDsn(PyObject *dsnObj,
        PyObject *upperDsnObj)
{
    PyObject *firstPart, *lastPart, *result;
    int startPos, endPos, bracePos, length;

    // attempt to find PWD= in the DSN
    if (ceoUtils_findInString(upperDsnObj, "PWD=", 0, &startPos) < 0)
        return NULL;

    // if not found, simply return the string unchanged
    if (startPos < 0) {
        Py_INCREF(dsnObj);
        return dsnObj;
    }

    // otherwise search for the semicolon
    if (ceoUtils_findInString(upperDsnObj, ";", startPos, &endPos) < 0)
        return NULL;
    length = PySequence_Size(dsnObj);
    if (PyErr_Occurred())
        return NULL;
    if (endPos < 0)
        endPos = length;

    // search for a brace as well since that escapes the semicolon if present
    if (ceoUtils_findInString(upperDsnObj, "{", startPos, &bracePos) < 0)
        return NULL;
    if (bracePos >= startPos && bracePos < endPos) {
        if (ceoUtils_findInString(upperDsnObj, "}", bracePos, &bracePos) < 0)
            return NULL;
        if (bracePos < 0) {
            Py_INCREF(dsnObj);
            return dsnObj;
        }
        if (ceoUtils_findInString(upperDsnObj, ";", bracePos, &endPos) < 0)
            return NULL;
        if (endPos < 0)
            endPos = length;
    }

    // finally rip out the portion that doesn't need to be there
    firstPart = PySequence_GetSlice(dsnObj, 0, startPos + 4);
    if (!firstPart)
        return NULL;
    if (endPos == length)
        return firstPart;
    lastPart = PySequence_GetSlice(dsnObj, endPos, length);
    if (!lastPart) {
        Py_DECREF(firstPart);
        return NULL;
    }
    result = PySequence_Concat(firstPart, lastPart);
    Py_DECREF(firstPart);
    Py_DECREF(lastPart);
    return result;
}


//-----------------------------------------------------------------------------
// ceoConnection_init()
//   Initialize the connection members.
//-----------------------------------------------------------------------------
static int ceoConnection_init(ceoConnection *conn, PyObject *args,
        PyObject *keywordArgs)
{
    PyObject *dsnObj, *upperDsnObj;
    SQLCHAR actualDsnBuffer[1024];
    SQLSMALLINT actualDsnLength;
    Py_ssize_t dsnLength;
    const char *dsn;
    int autocommit;
    SQLRETURN rc;

    // define keyword arguments
    static char *keywordList[] = { "dsn", "autocommit", NULL };

    // parse arguments
    autocommit = 0;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "z#|p", keywordList,
            &dsn, &dsnLength, &autocommit))
        return -1;

    // set up the environment
    conn->env = ceoEnv_new();
    if (!conn->env)
        return -1;

    // allocate handle for the connection
    rc = SQLAllocHandle(SQL_HANDLE_DBC, conn->env->handle, &conn->handle);
    if (CheckForError(conn->env, rc,
            "ceoConnection_init(): allocate DBC handle") < 0)
        return -1;

    // connecting to driver
    rc = SQLDriverConnectA(conn->handle, NULL, dsn, dsnLength,
            actualDsnBuffer, CEO_ARRAYSIZE(actualDsnBuffer),
            &actualDsnLength, SQL_DRIVER_NOPROMPT);
    if ((size_t) actualDsnLength > CEO_ARRAYSIZE(actualDsnBuffer) - 1)
        actualDsnLength = CEO_ARRAYSIZE(actualDsnBuffer) - 1;
    if (CheckForError(conn, rc,
            "ceoConnection_init(): connecting to driver") < 0) {
        conn->handle = SQL_NULL_HANDLE;
        return -1;
    }

    // turn off autocommit
    if (!autocommit) {
        rc = SQLSetConnectAttr(conn->handle, SQL_ATTR_AUTOCOMMIT,
                (SQLPOINTER) SQL_AUTOCOMMIT_OFF, SQL_IS_UINTEGER);
        if (CheckForError(conn, rc,
                "ceoConnection_init(): turning off autocommit") < 0)
            return -1;
    }

    // mark connection as connected
    conn->isConnected = 1;

    // save copy of constructed DSN
    dsnObj = PyUnicode_FromStringAndSize(actualDsnBuffer, actualDsnLength);
    if (!dsnObj) {
        Py_DECREF(conn);
        return -1;
    }

    // attempt to remove password
    upperDsnObj = PyObject_CallMethod(dsnObj, "upper", "");
    if (!upperDsnObj) {
        Py_DECREF(dsnObj);
        Py_DECREF(conn);
        return -1;
    }
    conn->dsn = ceoConnection_removePasswordFromDsn(dsnObj, upperDsnObj);
    Py_DECREF(dsnObj);
    Py_DECREF(upperDsnObj);
    if (!conn->dsn) {
        Py_DECREF(conn);
        return -1;
    }

    return 0;
}


//-----------------------------------------------------------------------------
// ceoConnection_free()
//   Deallocate the connection, disconnecting from the database if necessary.
//-----------------------------------------------------------------------------
static void ceoConnection_free(ceoConnection *conn)
{
    if (conn->isConnected) {
        Py_BEGIN_ALLOW_THREADS
        SQLEndTran(conn->handleType, conn->handle, SQL_ROLLBACK);
        SQLDisconnect(conn->handle);
        SQLFreeHandle(SQL_HANDLE_DBC, conn->handle);
        Py_END_ALLOW_THREADS
        conn->handle = NULL;
    }
    if (conn->handle)
        SQLFreeHandle(SQL_HANDLE_DBC, conn->handle);
    Py_CLEAR(conn->env);
    Py_CLEAR(conn->dsn);
    Py_CLEAR(conn->inputTypeHandler);
    Py_CLEAR(conn->outputTypeHandler);
    Py_TYPE(conn)->tp_free((PyObject*) conn);
}


//-----------------------------------------------------------------------------
// ceoConnection_repr()
//   Return a string representation of the connection.
//-----------------------------------------------------------------------------
static PyObject *ceoConnection_repr(ceoConnection *connection)
{
    PyObject *module, *name, *result;

    if (ceoUtils_getModuleAndName(Py_TYPE(connection), &module, &name) < 0)
        return NULL;
    if (connection->dsn) {
        result = ceoUtils_formatString("<%s.%s to %s>",
                PyTuple_Pack(3, module, name, connection->dsn));
    } else {
        result = ceoUtils_formatString("<%s.%s to unknown DSN>",
                PyTuple_Pack(2, module, name));
    }
    Py_DECREF(module);
    Py_DECREF(name);
    return result;
}


//-----------------------------------------------------------------------------
// ceoConnection_close()
//   Close the connection, disconnecting from the database.
//-----------------------------------------------------------------------------
static PyObject *ceoConnection_close(ceoConnection *conn, PyObject *args)
{
    SQLRETURN rc;

    // make sure we are actually connected
    if (ceoConnection_isConnected(conn) < 0)
        return NULL;

    // perform a rollback first
    Py_BEGIN_ALLOW_THREADS
    rc = SQLEndTran(conn->handleType, conn->handle, SQL_ROLLBACK);
    Py_END_ALLOW_THREADS
    if (CheckForError(conn, rc, "ceoConnection_close(): rollback") < 0)
        return NULL;

    // disconnect from the server
    Py_BEGIN_ALLOW_THREADS
    rc = SQLDisconnect(conn->handle);
    Py_END_ALLOW_THREADS
    if (CheckForError(conn, rc, "ceoConnection_close(): disconnect") < 0)
        return NULL;

    // mark connection as no longer connected
    conn->isConnected = 0;

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// ceoConnection_commit()
//   Commit the transaction on the connection.
//-----------------------------------------------------------------------------
static PyObject *ceoConnection_commit(ceoConnection *conn, PyObject *args)
{
    SQLRETURN rc;

    // make sure we are actually connected
    if (ceoConnection_isConnected(conn) < 0)
        return NULL;

    // perform the commit
    Py_BEGIN_ALLOW_THREADS
    rc = SQLEndTran(conn->handleType, conn->handle, SQL_COMMIT);
    Py_END_ALLOW_THREADS
    if (CheckForError(conn, rc, "ceoConnection_commit()") < 0)
        return NULL;
    LogMessage(LOG_LEVEL_DEBUG, "transaction committed");

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// ceoConnection_rollback()
//   Rollback the transaction on the connection.
//-----------------------------------------------------------------------------
static PyObject *ceoConnection_rollback(ceoConnection *conn, PyObject *args)
{
    SQLRETURN rc;

    // make sure we are actually connected
    if (ceoConnection_isConnected(conn) < 0)
        return NULL;

    // perform the rollback
    Py_BEGIN_ALLOW_THREADS
    rc = SQLEndTran(conn->handleType, conn->handle, SQL_ROLLBACK);
    Py_END_ALLOW_THREADS
    if (CheckForError(conn, rc, "ceoConnection_commit()") < 0)
        return NULL;
    LogMessage(LOG_LEVEL_DEBUG, "transaction rolled back");

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// ceoConnection_newCursor()
//   Create a new cursor (statement) referencing the connection.
//-----------------------------------------------------------------------------
static PyObject *ceoConnection_newCursor(ceoConnection *conn, PyObject *args)
{
    PyObject *createArgs, *result;

    createArgs = PyTuple_New(1);
    if (!createArgs)
        return NULL;
    Py_INCREF(conn);
    PyTuple_SET_ITEM(createArgs, 0, (PyObject*) conn);
    result = PyObject_Call( (PyObject*) &ceoPyTypeCursor, createArgs, NULL);
    Py_DECREF(createArgs);
    return result;
}


//-----------------------------------------------------------------------------
// ceoConnection_contextManagerEnter()
//   Called when the connection is used as a context manager and simply returns
// itconn as a convenience to the caller.
//-----------------------------------------------------------------------------
static PyObject *ceoConnection_contextManagerEnter(ceoConnection *conn,
        PyObject* args)
{
    Py_INCREF(conn);
    return (PyObject*) conn;
}


//-----------------------------------------------------------------------------
// ceoConnection_contextManagerExit()
//   Called when the connection is used as a context manager and if any
// exception a rollback takes place; otherwise, a commit takes place.
//-----------------------------------------------------------------------------
static PyObject *ceoConnection_contextManagerExit(ceoConnection *conn,
        PyObject* args)
{
    PyObject *excType, *excValue, *excTraceback, *result;
    char *methodName;

    if (!PyArg_ParseTuple(args, "OOO", &excType, &excValue, &excTraceback))
        return NULL;
    if (excType == Py_None && excValue == Py_None && excTraceback == Py_None)
        methodName = "commit";
    else methodName = "rollback";
    result = PyObject_CallMethod((PyObject*) conn, methodName, "");
    if (!result)
        return NULL;
    Py_DECREF(result);

    Py_INCREF(Py_False);
    return Py_False;
}


//-----------------------------------------------------------------------------
// ceoConnection_columns()
//   Return columns from the catalog for the data source.
//-----------------------------------------------------------------------------
static PyObject *ceoConnection_columns(ceoConnection *conn, PyObject *args,
        PyObject *keywordArgs)
{
    static char *keywordList[] =
            { "catalog", "schema", "table", "column", NULL };
    int catalogLength, schemaLength, tableLength, columnLength;
    SQLCHAR *catalog, *schema, *table, *column;
    ceoCursor *cursor;
    SQLRETURN rc;

    // parse arguments
    catalog = schema = table = column = NULL;
    catalogLength = schemaLength = tableLength = columnLength = 0;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|z#z#z#z#",
            keywordList, &catalog, &catalogLength, &schema, &schemaLength,
            &table, &tableLength, &column, &columnLength))
        return NULL;

    // create cursor
    cursor = ceoCursor_internalNew(conn);
    if (!cursor)
        return NULL;

    // call catalog method
    rc = SQLColumns(cursor->handle, catalog, catalogLength, schema,
            schemaLength, table, tableLength, column, columnLength);
    if (CheckForError(cursor, rc, "ceoConnection_columns()") < 0) {
        Py_DECREF(cursor);
        return NULL;
    }

    return ceoCursor_internalCatalogHelper(cursor);
}


//-----------------------------------------------------------------------------
// ceoConnection_columnPrivileges()
//   Return column privileges from the catalog for the data source.
//-----------------------------------------------------------------------------
static PyObject *ceoConnection_columnPrivileges(ceoConnection *conn,
        PyObject *args, PyObject *keywordArgs)
{
    static char *keywordList[] =
            { "catalog", "schema", "table", "column", NULL };
    int catalogLength, schemaLength, tableLength, columnLength;
    SQLCHAR *catalog, *schema, *table, *column;
    ceoCursor *cursor;
    SQLRETURN rc;

    // parse arguments
    catalog = schema = table = column = NULL;
    catalogLength = schemaLength = tableLength = columnLength = 0;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|z#z#z#z#",
            keywordList, &catalog, &catalogLength, &schema, &schemaLength,
            &table, &tableLength, &column, &columnLength))
        return NULL;

    // create cursor
    cursor = ceoCursor_internalNew(conn);
    if (!cursor)
        return NULL;

    // call catalog method
    rc = SQLColumnPrivileges(cursor->handle, catalog, catalogLength, schema,
            schemaLength, table, tableLength, column, columnLength);
    if (CheckForError(cursor, rc, "ceoConnection_columnPrivileges()") < 0) {
        Py_DECREF(cursor);
        return NULL;
    }

    return ceoCursor_internalCatalogHelper(cursor);
}


//-----------------------------------------------------------------------------
// ceoConnection_foreignKeys()
//   Return foreign keys from the catalog for the data source.
//-----------------------------------------------------------------------------
static PyObject *ceoConnection_foreignKeys(ceoConnection *conn,
        PyObject *args, PyObject *keywordArgs)
{
    static char *keywordList[] = { "pkcatalog", "pkschema", "pktable",
            "fkcatalog", "fkschema", "fktable", NULL };
    SQLCHAR *pkCatalog, *pkSchema, *pkTable, *fkCatalog, *fkSchema, *fkTable;
    int pkCatalogLength, pkSchemaLength, pkTableLength, fkCatalogLength;
    int fkSchemaLength, fkTableLength;
    ceoCursor *cursor;
    SQLRETURN rc;

    // parse arguments
    pkCatalog = pkSchema = pkTable = fkCatalog = fkSchema = fkTable = NULL;
    pkCatalogLength = pkSchemaLength = pkTableLength = 0;
    fkCatalogLength = fkSchemaLength = fkTableLength = 0;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|z#z#z#z#z#z#",
            keywordList, &pkCatalog, &pkCatalogLength, &pkSchema,
            &pkSchemaLength, &pkTable, &pkTableLength, &fkCatalog,
            &fkCatalogLength, &fkSchema, &fkSchemaLength, &fkTable,
            &fkTableLength))
        return NULL;

    // create cursor
    cursor = ceoCursor_internalNew(conn);
    if (!cursor)
        return NULL;

    // call catalog method
    rc = SQLForeignKeys(cursor->handle, pkCatalog, pkCatalogLength, pkSchema,
            pkSchemaLength, pkTable, pkTableLength, fkCatalog, fkCatalogLength,
            fkSchema, fkSchemaLength, fkTable, fkTableLength);
    if (CheckForError(cursor, rc, "ceoConnection_foreignKeys()") < 0) {
        Py_DECREF(cursor);
        return NULL;
    }

    return ceoCursor_internalCatalogHelper(cursor);
}


//-----------------------------------------------------------------------------
// ceoConnection_primaryKeys()
//   Return primary keys from the catalog for the data source.
//-----------------------------------------------------------------------------
static PyObject *ceoConnection_primaryKeys(ceoConnection *conn,
        PyObject *args, PyObject *keywordArgs)
{
    static char *keywordList[] = { "catalog", "schema", "table", NULL };
    int catalogLength, schemaLength, tableLength;
    SQLCHAR *catalog, *schema, *table;
    ceoCursor *cursor;
    SQLRETURN rc;

    // parse arguments
    catalog = schema = table = NULL;
    catalogLength = schemaLength = tableLength = 0;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|z#z#z#",
            keywordList, &catalog, &catalogLength, &schema, &schemaLength,
            &table, &tableLength))
        return NULL;

    // create cursor
    cursor = ceoCursor_internalNew(conn);
    if (!cursor)
        return NULL;

    // call catalog method
    rc = SQLPrimaryKeys(cursor->handle, catalog, catalogLength, schema,
            schemaLength, table, tableLength);
    if (CheckForError(cursor, rc, "ceoConnection_primaryKeys()") < 0) {
        Py_DECREF(cursor);
        return NULL;
    }

    return ceoCursor_internalCatalogHelper(cursor);
}


//-----------------------------------------------------------------------------
// ceoConnection_procedures()
//   Return procedures from the catalog for the data source.
//-----------------------------------------------------------------------------
static PyObject *ceoConnection_procedures(ceoConnection *conn,
        PyObject *args, PyObject *keywordArgs)
{
    static char *keywordList[] = { "catalog", "schema", "proc", NULL };
    int catalogLength, schemaLength, procLength;
    SQLCHAR *catalog, *schema, *proc;
    ceoCursor *cursor;
    SQLRETURN rc;

    // parse arguments
    catalog = schema = proc = NULL;
    catalogLength = schemaLength = procLength = 0;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|z#z#z#",
            keywordList, &catalog, &catalogLength, &schema, &schemaLength,
            &proc, &procLength))
        return NULL;

    // create cursor
    cursor = ceoCursor_internalNew(conn);
    if (!cursor)
        return NULL;

    // call catalog method
    rc = SQLProcedures(cursor->handle, catalog, catalogLength, schema,
            schemaLength, proc, procLength);
    if (CheckForError(cursor, rc, "ceoConnection_procedures()") < 0) {
        Py_DECREF(cursor);
        return NULL;
    }

    return ceoCursor_internalCatalogHelper(cursor);
}


//-----------------------------------------------------------------------------
// ceoConnection_procedureColumns()
//   Return procedure columns from the catalog for the data source.
//-----------------------------------------------------------------------------
static PyObject *ceoConnection_procedureColumns(ceoConnection *conn,
        PyObject *args, PyObject *keywordArgs)
{
    static char *keywordList[] =
            { "catalog", "schema", "proc", "column", NULL };
    int catalogLength, schemaLength, procLength, columnLength;
    SQLCHAR *catalog, *schema, *proc, *column;
    ceoCursor *cursor;
    SQLRETURN rc;

    // parse arguments
    catalog = schema = proc = column = NULL;
    catalogLength = schemaLength = procLength = columnLength = 0;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|z#z#z#z#",
            keywordList, &catalog, &catalogLength, &schema, &schemaLength,
            &proc, &procLength, &column, &columnLength))
        return NULL;

    // create cursor
    cursor = ceoCursor_internalNew(conn);
    if (!cursor)
        return NULL;

    // call catalog method
    rc = SQLProcedureColumns(cursor->handle, catalog, catalogLength, schema,
            schemaLength, proc, procLength, column, columnLength);
    if (CheckForError(cursor, rc, "ceoConnection_procedureColumns()") < 0) {
        Py_DECREF(cursor);
        return NULL;
    }

    return ceoCursor_internalCatalogHelper(cursor);
}


//-----------------------------------------------------------------------------
// ceoConnection_tables()
//   Return tables from the catalog for the data source.
//-----------------------------------------------------------------------------
static PyObject *ceoConnection_tables(ceoConnection *conn, PyObject *args,
        PyObject *keywordArgs)
{
    static char *keywordList[] =
            { "catalog", "schema", "table", "type", NULL };
    int catalogLength, schemaLength, tableLength, typeLength;
    SQLCHAR *catalog, *schema, *table, *type;
    ceoCursor *cursor;
    SQLRETURN rc;

    // parse arguments
    catalog = schema = table = type = NULL;
    catalogLength = schemaLength = tableLength = typeLength = 0;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|z#z#z#z#",
            keywordList, &catalog, &catalogLength, &schema, &schemaLength,
            &table, &tableLength, &type, &typeLength))
        return NULL;

    // create cursor
    cursor = ceoCursor_internalNew(conn);
    if (!cursor)
        return NULL;

    // call catalog method
    rc = SQLTables(cursor->handle, catalog, catalogLength, schema, schemaLength,
            table, tableLength, type, typeLength);
    if (CheckForError(cursor, rc, "ceoConnection_tables()") < 0) {
        Py_DECREF(cursor);
        return NULL;
    }

    return ceoCursor_internalCatalogHelper(cursor);
}


//-----------------------------------------------------------------------------
// ceoConnection_tablePrivileges()
//   Return table privileges from the catalog for the data source.
//-----------------------------------------------------------------------------
static PyObject *ceoConnection_tablePrivileges(ceoConnection *conn,
        PyObject *args, PyObject *keywordArgs)
{
    static char *keywordList[] =
            { "catalog", "schema", "table", NULL };
    int catalogLength, schemaLength, tableLength;
    SQLCHAR *catalog, *schema, *table;
    ceoCursor *cursor;
    SQLRETURN rc;

    // parse arguments
    catalog = schema = table = NULL;
    catalogLength = schemaLength = tableLength = 0;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|z#z#z#",
            keywordList, &catalog, &catalogLength, &schema, &schemaLength,
            &table, &tableLength))
        return NULL;

    // create cursor
    cursor = ceoCursor_internalNew(conn);
    if (!cursor)
        return NULL;

    // call catalog method
    rc = SQLTablePrivileges(cursor->handle, catalog, catalogLength, schema,
            schemaLength, table, tableLength);
    if (CheckForError(cursor, rc, "ceoConnection_tablePrivileges()") < 0) {
        Py_DECREF(cursor);
        return NULL;
    }

    return ceoCursor_internalCatalogHelper(cursor);
}


//-----------------------------------------------------------------------------
// ceoConnection_getAutoCommit()
//   Return the value of the autocommit flag.
//-----------------------------------------------------------------------------
static PyObject *ceoConnection_getAutoCommit(ceoConnection *conn, void* arg)
{
    SQLUINTEGER autocommit;
    PyObject *result;
    SQLRETURN rc;

    rc = SQLGetConnectAttr(conn->handle, SQL_ATTR_AUTOCOMMIT,
            &autocommit, SQL_IS_UINTEGER, NULL);
    if (CheckForError(conn, rc, "ceoConnection_getAutoCommit()") < 0)
        return NULL;
    if (autocommit)
        result = Py_True;
    else result = Py_False;
    Py_INCREF(result);
    return result;
}


//-----------------------------------------------------------------------------
// ceoConnection_setAutoCommit()
//   Set the value of the autocommit flag.
//-----------------------------------------------------------------------------
static int ceoConnection_setAutoCommit(ceoConnection *conn, PyObject *value,
        void* arg)
{
    SQLUINTEGER sqlValue;
    SQLRETURN rc;

    if (!PyBool_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "expecting boolean");
        return -1;
    }
    if (value == Py_True)
        sqlValue = SQL_AUTOCOMMIT_ON;
    else sqlValue = SQL_AUTOCOMMIT_OFF;

    rc = SQLSetConnectAttr(conn->handle, SQL_ATTR_AUTOCOMMIT,
            (SQLPOINTER) sqlValue, SQL_IS_UINTEGER);
    if (CheckForError(conn, rc,
            "ceoConnection_init(): turning off autocommit") < 0)
        return -1;
    return 0;
}


//-----------------------------------------------------------------------------
// declaration of methods for the Python type
//-----------------------------------------------------------------------------
static PyMethodDef ceoMethods[] = {
    { "cursor", (PyCFunction) ceoConnection_newCursor, METH_NOARGS },
    { "commit", (PyCFunction) ceoConnection_commit, METH_NOARGS },
    { "rollback", (PyCFunction) ceoConnection_rollback, METH_NOARGS },
    { "close", (PyCFunction) ceoConnection_close, METH_NOARGS },
    { "columns", (PyCFunction) ceoConnection_columns,
            METH_VARARGS | METH_KEYWORDS },
    { "columnprivileges", (PyCFunction) ceoConnection_columnPrivileges,
            METH_VARARGS | METH_KEYWORDS },
    { "foreignkeys", (PyCFunction) ceoConnection_foreignKeys,
            METH_VARARGS | METH_KEYWORDS },
    { "primarykeys", (PyCFunction) ceoConnection_primaryKeys,
            METH_VARARGS | METH_KEYWORDS },
    { "procedures", (PyCFunction) ceoConnection_procedures,
            METH_VARARGS | METH_KEYWORDS },
    { "procedurecolumns", (PyCFunction) ceoConnection_procedureColumns,
            METH_VARARGS | METH_KEYWORDS },
    { "tables", (PyCFunction) ceoConnection_tables,
            METH_VARARGS | METH_KEYWORDS },
    { "tableprivileges", (PyCFunction) ceoConnection_tablePrivileges,
            METH_VARARGS | METH_KEYWORDS },
    { "__enter__", (PyCFunction) ceoConnection_contextManagerEnter,
            METH_NOARGS },
    { "__exit__", (PyCFunction) ceoConnection_contextManagerExit,
            METH_VARARGS },
    { NULL }
};


//-----------------------------------------------------------------------------
// declaration of members for the Python type
//-----------------------------------------------------------------------------
static PyMemberDef ceoMembers[] = {
    { "dsn", T_OBJECT, offsetof(ceoConnection, dsn), READONLY },
    { "logsql", T_INT, offsetof(ceoConnection, logSql), 0 },
    { "inputtypehandler", T_OBJECT,
            offsetof(ceoConnection, inputTypeHandler), 0 },
    { "outputtypehandler", T_OBJECT,
            offsetof(ceoConnection, outputTypeHandler), 0 },
    { NULL }
};


//-----------------------------------------------------------------------------
// declaration of calculated members for the Python type
//-----------------------------------------------------------------------------
static PyGetSetDef ceoCalcMembers[] = {
    { "autocommit", (getter) ceoConnection_getAutoCommit,
            (setter) ceoConnection_setAutoCommit, 0, 0 },
    { NULL }
};


//-----------------------------------------------------------------------------
// declaration of the Python type
//-----------------------------------------------------------------------------
PyTypeObject ceoPyTypeConnection = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ceODBC.Connection",
    .tp_basicsize = sizeof(ceoConnection),
    .tp_dealloc = (destructor) ceoConnection_free,
    .tp_repr = (reprfunc) ceoConnection_repr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_methods = ceoMethods,
    .tp_members = ceoMembers,
    .tp_getset = ceoCalcMembers,
    .tp_init = (initproc) ceoConnection_init,
    .tp_new = (newfunc) ceoConnection_new
};
