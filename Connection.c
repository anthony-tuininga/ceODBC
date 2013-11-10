//-----------------------------------------------------------------------------
// Connection.c
//   Definition of the Python type for connections.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// structure for the Python type "Connection"
//-----------------------------------------------------------------------------
typedef struct {
    ObjectWithHandle_HEAD
    udt_Environment *environment;
    int isConnected;
    PyObject *dsn;
    int logSql;
} udt_Connection;


//-----------------------------------------------------------------------------
// functions for the Python type "Connection"
//-----------------------------------------------------------------------------
static void Connection_Free(udt_Connection*);
static PyObject *Connection_New(PyTypeObject*, PyObject*, PyObject*);
static int Connection_Init(udt_Connection*, PyObject*, PyObject*);
static PyObject *Connection_Repr(udt_Connection*);
static PyObject *Connection_Close(udt_Connection*, PyObject*);
static PyObject *Connection_Commit(udt_Connection*, PyObject*);
static PyObject *Connection_Rollback(udt_Connection*, PyObject*);
static PyObject *Connection_NewCursor(udt_Connection*, PyObject*);
static PyObject *Connection_ContextManagerEnter(udt_Connection*, PyObject*);
static PyObject *Connection_ContextManagerExit(udt_Connection*, PyObject*);
static PyObject *Connection_Columns(udt_Connection*, PyObject*, PyObject*);
static PyObject *Connection_ColumnPrivileges(udt_Connection*, PyObject*,
        PyObject*);
static PyObject *Connection_ForeignKeys(udt_Connection*, PyObject*, PyObject*);
static PyObject *Connection_PrimaryKeys(udt_Connection*, PyObject*, PyObject*);
static PyObject *Connection_Procedures(udt_Connection*, PyObject*, PyObject*);
static PyObject *Connection_ProcedureColumns(udt_Connection*, PyObject*,
        PyObject*);
static PyObject *Connection_Tables(udt_Connection*, PyObject*, PyObject*);
static PyObject *Connection_TablePrivileges(udt_Connection*, PyObject*,
        PyObject*);
static PyObject *Connection_GetAutoCommit(udt_Connection*, void*);
static int Connection_SetAutoCommit(udt_Connection*, PyObject*, void*);


//-----------------------------------------------------------------------------
// declaration of methods for Python type "Connection"
//-----------------------------------------------------------------------------
static PyMethodDef g_ConnectionMethods[] = {
    { "cursor", (PyCFunction) Connection_NewCursor, METH_NOARGS },
    { "commit", (PyCFunction) Connection_Commit, METH_NOARGS },
    { "rollback", (PyCFunction) Connection_Rollback, METH_NOARGS },
    { "close", (PyCFunction) Connection_Close, METH_NOARGS },
    { "columns", (PyCFunction) Connection_Columns,
            METH_VARARGS | METH_KEYWORDS },
    { "columnprivileges", (PyCFunction) Connection_ColumnPrivileges,
            METH_VARARGS | METH_KEYWORDS },
    { "foreignkeys", (PyCFunction) Connection_ForeignKeys,
            METH_VARARGS | METH_KEYWORDS },
    { "primarykeys", (PyCFunction) Connection_PrimaryKeys,
            METH_VARARGS | METH_KEYWORDS },
    { "procedures", (PyCFunction) Connection_Procedures,
            METH_VARARGS | METH_KEYWORDS },
    { "procedurecolumns", (PyCFunction) Connection_ProcedureColumns,
            METH_VARARGS | METH_KEYWORDS },
    { "tables", (PyCFunction) Connection_Tables,
            METH_VARARGS | METH_KEYWORDS },
    { "tableprivileges", (PyCFunction) Connection_TablePrivileges,
            METH_VARARGS | METH_KEYWORDS },
    { "__enter__", (PyCFunction) Connection_ContextManagerEnter, METH_NOARGS },
    { "__exit__", (PyCFunction) Connection_ContextManagerExit, METH_VARARGS },
    { NULL }
};


//-----------------------------------------------------------------------------
// declaration of members for Python type "Connection"
//-----------------------------------------------------------------------------
static PyMemberDef g_ConnectionMembers[] = {
    { "dsn", T_OBJECT, offsetof(udt_Connection, dsn), READONLY },
    { "logsql", T_INT, offsetof(udt_Connection, logSql), 0 },
    { NULL }
};


//-----------------------------------------------------------------------------
// declaration of calculated members for Python type "Connection"
//-----------------------------------------------------------------------------
static PyGetSetDef g_ConnectionCalcMembers[] = {
    { "autocommit", (getter) Connection_GetAutoCommit,
            (setter) Connection_SetAutoCommit, 0, 0 },
    { NULL }
};


//-----------------------------------------------------------------------------
// declaration of Python type "Connection"
//-----------------------------------------------------------------------------
static PyTypeObject g_ConnectionType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "ceODBC.Connection",                // tp_name
    sizeof(udt_Connection),             // tp_basicsize
    0,                                  // tp_itemsize
    (destructor) Connection_Free,       // tp_dealloc
    0,                                  // tp_print
    0,                                  // tp_getattr
    0,                                  // tp_setattr
    0,                                  // tp_compare
    (reprfunc) Connection_Repr,         // tp_repr
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
    g_ConnectionMethods,                // tp_methods
    g_ConnectionMembers,                // tp_members
    g_ConnectionCalcMembers,            // tp_getset
    0,                                  // tp_base
    0,                                  // tp_dict
    0,                                  // tp_descr_get
    0,                                  // tp_descr_set
    0,                                  // tp_dictoffset
    (initproc) Connection_Init,         // tp_init
    0,                                  // tp_alloc
    (newfunc) Connection_New,           // tp_new
    0,                                  // tp_free
    0,                                  // tp_is_gc
    0                                   // tp_bases
};


//-----------------------------------------------------------------------------
// Connection_IsConnected()
//   Determines if the connection object is connected to the database. If not,
// a Python exception is raised.
//-----------------------------------------------------------------------------
static int Connection_IsConnected(
    udt_Connection *self)               // connection to check
{
    if (!self->isConnected) {
        PyErr_SetString(g_InterfaceErrorException, "not connected");
        return -1;
    }
    return 0;
}


#include "Cursor.c"


//-----------------------------------------------------------------------------
// Connection_New()
//   Create a new connection object and return it.
//-----------------------------------------------------------------------------
static PyObject* Connection_New(
    PyTypeObject *type,                 // type object
    PyObject *args,                     // arguments
    PyObject *keywordArgs)              // keyword arguments
{
    udt_Connection *self;

    // create the object
    self = (udt_Connection*) type->tp_alloc(type, 0);
    if (!self)
        return NULL;
    self->handleType = SQL_HANDLE_DBC;
    self->handle = SQL_NULL_HANDLE;
    self->environment = NULL;
    self->dsn = NULL;
    self->isConnected = 0;
#ifdef WITH_CX_LOGGING
    self->logSql = 1;
#else
    self->logSql = 0;
#endif

    return (PyObject*) self;
}


//-----------------------------------------------------------------------------
// FindInString()
//   Call the method "find" on the object and return the position in the
// string where it is found.
//-----------------------------------------------------------------------------
static int FindInString(
    PyObject *strObj,                   // string object to search
    char *stringToFind,                 // string to find
    int startPos,                       // starting position to search
    int *foundPos)                      // found position (OUT)
{
    PyObject *temp;

    temp = PyObject_CallMethod(strObj, "find", "si", stringToFind, startPos);
    if (!temp)
        return -1;
    *foundPos = PyInt_AsLong(temp);
    Py_DECREF(temp);
    if (PyErr_Occurred())
        return -1;
    return 0;
}


//-----------------------------------------------------------------------------
// Connection_RemovePasswordFromDsn()
//   Attempt to remove the password from the DSN for security reasons.
//-----------------------------------------------------------------------------
static PyObject *Connection_RemovePasswordFromDsn(
    PyObject *dsnObj,                   // input DSN object
    PyObject *upperDsnObj)              // input DSN object (uppercase)
{
    PyObject *firstPart, *lastPart, *result;
    int startPos, endPos, bracePos, length;

    // attempt to find PWD= in the DSN
    if (FindInString(upperDsnObj, "PWD=", 0, &startPos) < 0)
        return NULL;

    // if not found, simply return the string unchanged
    if (startPos < 0) {
        Py_INCREF(dsnObj);
        return dsnObj;
    }

    // otherwise search for the semicolon
    if (FindInString(upperDsnObj, ";", startPos, &endPos) < 0)
        return NULL;
    length = PySequence_Size(dsnObj);
    if (PyErr_Occurred())
        return NULL;
    if (endPos < 0)
        endPos = length;

    // search for a brace as well since that escapes the semicolon if present
    if (FindInString(upperDsnObj, "{", startPos, &bracePos) < 0)
        return NULL;
    if (bracePos >= startPos && bracePos < endPos) {
        if (FindInString(upperDsnObj, "}", bracePos, &bracePos) < 0)
            return NULL;
        if (bracePos < 0) {
            Py_INCREF(dsnObj);
            return dsnObj;
        }
        if (FindInString(upperDsnObj, ";", bracePos, &endPos) < 0)
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
// Connection_Init()
//   Initialize the connection members.
//-----------------------------------------------------------------------------
static int Connection_Init(
    udt_Connection *self,               // connection
    PyObject *args,                     // arguments
    PyObject *keywordArgs)              // keyword arguments
{
    PyObject *autocommitObj, *dsnObj, *upperDsnObj;
    CEODBC_CHAR actualDsnBuffer[1024];
    SQLSMALLINT actualDsnLength;
    udt_StringBuffer dsnBuffer;
    int autocommit;
    SQLRETURN rc;

    // define keyword arguments
    static char *keywordList[] = { "dsn", "autocommit", NULL };

    // parse arguments
    autocommitObj = Py_None;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "O|O", keywordList,
            &dsnObj, &autocommitObj))
        return -1;
    autocommit = PyObject_IsTrue(autocommitObj);
    if (autocommit < 0)
        return -1;

    // set up the environment
    self->environment = Environment_New();
    if (!self->environment)
        return -1;

    // allocate handle for the connection
    rc = SQLAllocHandle(SQL_HANDLE_DBC, self->environment->handle,
            &self->handle);
    if (CheckForError(self->environment, rc,
            "Connection_Init(): allocate DBC handle") < 0)
        return -1;

    // connecting to driver
    if (StringBuffer_FromString(&dsnBuffer, dsnObj,
                "DSN must be a string") < 0)
        return -1;
    rc = SQLDriverConnect(self->handle, NULL, (CEODBC_CHAR*) dsnBuffer.ptr,
            dsnBuffer.size, (CEODBC_CHAR*) actualDsnBuffer,
            sizeof(actualDsnBuffer), &actualDsnLength, SQL_DRIVER_NOPROMPT);
    StringBuffer_Clear(&dsnBuffer);
    if (CheckForError(self, rc,
            "Connection_Init(): connecting to driver") < 0)
        return -1;

    // turn off autocommit
    if (!autocommit) {
        rc = SQLSetConnectAttr(self->handle, SQL_ATTR_AUTOCOMMIT,
                (SQLPOINTER) SQL_AUTOCOMMIT_OFF, SQL_IS_UINTEGER);
        if (CheckForError(self, rc,
                "Connection_Init(): turning off autocommit") < 0)
            return -1;
    }

    // mark connection as connected
    self->isConnected = 1;

    // save copy of constructed DSN
    dsnObj = ceString_FromStringAndSize( (char*) actualDsnBuffer,
            actualDsnLength);
    if (!dsnObj) {
        Py_DECREF(self);
        return -1;
    }

    // attempt to remove password
    upperDsnObj = PyObject_CallMethod(dsnObj, "upper", "");
    if (!upperDsnObj) {
        Py_DECREF(dsnObj);
        Py_DECREF(self);
        return -1;
    }
    self->dsn = Connection_RemovePasswordFromDsn(dsnObj, upperDsnObj);
    Py_DECREF(dsnObj);
    Py_DECREF(upperDsnObj);
    if (!self->dsn) {
        Py_DECREF(self);
        return -1;
    }

    return 0;
}


//-----------------------------------------------------------------------------
// Connection_Free()
//   Deallocate the connection, disconnecting from the database if necessary.
//-----------------------------------------------------------------------------
static void Connection_Free(
    udt_Connection *self)               // connection object
{
    if (self->isConnected) {
        Py_BEGIN_ALLOW_THREADS
        SQLEndTran(self->handleType, self->handle, SQL_ROLLBACK);
        SQLDisconnect(self->handle);
        Py_END_ALLOW_THREADS
    }
    if (self->handle)
        SQLFreeHandle(SQL_HANDLE_DBC, self->handle);
    Py_CLEAR(self->environment);
    Py_CLEAR(self->dsn);
    Py_TYPE(self)->tp_free((PyObject*) self);
}


//-----------------------------------------------------------------------------
// Connection_Repr()
//   Return a string representation of the connection.
//-----------------------------------------------------------------------------
static PyObject *Connection_Repr(
    udt_Connection *connection)         // connection to return the string for
{
    PyObject *module, *name, *result, *format, *formatArgs;

    if (GetModuleAndName(Py_TYPE(connection), &module, &name) < 0)
        return NULL;
    if (connection->dsn)
        formatArgs = PyTuple_Pack(3, module, name, connection->dsn);
    else formatArgs = PyTuple_Pack(2, module, name);
    Py_DECREF(module);
    Py_DECREF(name);
    if (!formatArgs)
        return NULL;

    if (connection->dsn)
        format = ceString_FromAscii("<%s.%s to %s>");
    else format = ceString_FromAscii("<%s.%s to unknown DSN>");
    if (!format) {
        Py_DECREF(formatArgs);
        return NULL;
    }
    result = ceString_Format(format, formatArgs);
    Py_DECREF(format);
    Py_DECREF(formatArgs);
    return result;
}


//-----------------------------------------------------------------------------
// Connection_Close()
//   Close the connection, disconnecting from the database.
//-----------------------------------------------------------------------------
static PyObject *Connection_Close(
    udt_Connection *self,               // connection to close
    PyObject *args)                     // arguments
{
    SQLRETURN rc;

    // make sure we are actually connected
    if (Connection_IsConnected(self) < 0)
        return NULL;

    // perform a rollback first
    Py_BEGIN_ALLOW_THREADS
    rc = SQLEndTran(self->handleType, self->handle, SQL_ROLLBACK);
    Py_END_ALLOW_THREADS
    if (CheckForError(self, rc, "Connection_Close(): rollback") < 0)
        return NULL;

    // disconnect from the server
    Py_BEGIN_ALLOW_THREADS
    rc = SQLDisconnect(self->handle);
    Py_END_ALLOW_THREADS
    if (CheckForError(self, rc, "Connection_Close(): disconnect") < 0)
        return NULL;

    // mark connection as no longer connected
    self->isConnected = 0;

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// Connection_Commit()
//   Commit the transaction on the connection.
//-----------------------------------------------------------------------------
static PyObject *Connection_Commit(
    udt_Connection *self,               // connection to commit
    PyObject *args)                     // arguments
{
    SQLRETURN rc;

    // make sure we are actually connected
    if (Connection_IsConnected(self) < 0)
        return NULL;

    // perform the commit
    Py_BEGIN_ALLOW_THREADS
    rc = SQLEndTran(self->handleType, self->handle, SQL_COMMIT);
    Py_END_ALLOW_THREADS
    if (CheckForError(self, rc, "Connection_Commit()") < 0)
        return NULL;
    LogMessage(LOG_LEVEL_DEBUG, "transaction committed");

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// Connection_Rollback()
//   Rollback the transaction on the connection.
//-----------------------------------------------------------------------------
static PyObject *Connection_Rollback(
    udt_Connection *self,               // connection to rollback
    PyObject *args)                     // arguments
{
    SQLRETURN rc;

    // make sure we are actually connected
    if (Connection_IsConnected(self) < 0)
        return NULL;

    // perform the rollback
    Py_BEGIN_ALLOW_THREADS
    rc = SQLEndTran(self->handleType, self->handle, SQL_ROLLBACK);
    Py_END_ALLOW_THREADS
    if (CheckForError(self, rc, "Connection_Commit()") < 0)
        return NULL;
    LogMessage(LOG_LEVEL_DEBUG, "transaction rolled back");

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// Connection_NewCursor()
//   Create a new cursor (statement) referencing the connection.
//-----------------------------------------------------------------------------
static PyObject *Connection_NewCursor(
    udt_Connection *self,               // connection to create cursor on
    PyObject *args)                     // arguments
{
    PyObject *createArgs, *result;

    createArgs = PyTuple_New(1);
    if (!createArgs)
        return NULL;
    Py_INCREF(self);
    PyTuple_SET_ITEM(createArgs, 0, (PyObject*) self);
    result = PyObject_Call( (PyObject*) &g_CursorType, createArgs, NULL);
    Py_DECREF(createArgs);
    return result;
}


//-----------------------------------------------------------------------------
// Connection_ContextManagerEnter()
//   Called when the connection is used as a context manager and simply returns
// itself as a convenience to the caller.
//-----------------------------------------------------------------------------
static PyObject *Connection_ContextManagerEnter(
    udt_Connection *self,               // connection
    PyObject* args)                     // arguments
{
    Py_INCREF(self);
    return (PyObject*) self;
}


//-----------------------------------------------------------------------------
// Connection_ContextManagerExit()
//   Called when the connection is used as a context manager and if any
// exception a rollback takes place; otherwise, a commit takes place.
//-----------------------------------------------------------------------------
static PyObject *Connection_ContextManagerExit(
    udt_Connection *self,               // connection
    PyObject* args)                     // arguments
{
    PyObject *excType, *excValue, *excTraceback, *result;
    char *methodName;

    if (!PyArg_ParseTuple(args, "OOO", &excType, &excValue, &excTraceback))
        return NULL;
    if (excType == Py_None && excValue == Py_None && excTraceback == Py_None)
        methodName = "commit";
    else methodName = "rollback";
    result = PyObject_CallMethod((PyObject*) self, methodName, "");
    if (!result)
        return NULL;
    Py_DECREF(result);

    Py_INCREF(Py_False);
    return Py_False;
}


//-----------------------------------------------------------------------------
// Connection_Columns()
//   Return columns from the catalog for the data source.
//-----------------------------------------------------------------------------
static PyObject *Connection_Columns(
    udt_Connection *self,               // connection to fetch from
    PyObject *args,                     // arguments
    PyObject *keywordArgs)              // keyword arguments
{
    static char *keywordList[] =
            { "catalog", "schema", "table", "column", NULL };
    int catalogLength, schemaLength, tableLength, columnLength;
    SQLCHAR *catalog, *schema, *table, *column;
    udt_Cursor *cursor;
    SQLRETURN rc;

    // parse arguments
    catalog = schema = table = column = NULL;
    catalogLength = schemaLength = tableLength = columnLength = 0;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|z#z#z#z#",
            keywordList, &catalog, &catalogLength, &schema, &schemaLength,
            &table, &tableLength, &column, &columnLength))
        return NULL;

    // create cursor
    cursor = Cursor_InternalNew(self);
    if (!cursor)
        return NULL;

    // call catalog method
    rc = SQLColumns(cursor->handle, catalog, catalogLength, schema,
            schemaLength, table, tableLength, column, columnLength);
    if (CheckForError(cursor, rc, "Connection_Columns()") < 0) {
        Py_DECREF(cursor);
        return NULL;
    }

    return Cursor_InternalCatalogHelper(cursor);
}


//-----------------------------------------------------------------------------
// Connection_ColumnPrivileges()
//   Return column privileges from the catalog for the data source.
//-----------------------------------------------------------------------------
static PyObject *Connection_ColumnPrivileges(
    udt_Connection *self,               // connection to fetch from
    PyObject *args,                     // arguments
    PyObject *keywordArgs)              // keyword arguments
{
    static char *keywordList[] =
            { "catalog", "schema", "table", "column", NULL };
    int catalogLength, schemaLength, tableLength, columnLength;
    SQLCHAR *catalog, *schema, *table, *column;
    udt_Cursor *cursor;
    SQLRETURN rc;

    // parse arguments
    catalog = schema = table = column = NULL;
    catalogLength = schemaLength = tableLength = columnLength = 0;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|z#z#z#z#",
            keywordList, &catalog, &catalogLength, &schema, &schemaLength,
            &table, &tableLength, &column, &columnLength))
        return NULL;

    // create cursor
    cursor = Cursor_InternalNew(self);
    if (!cursor)
        return NULL;

    // call catalog method
    rc = SQLColumnPrivileges(cursor->handle, catalog, catalogLength, schema,
            schemaLength, table, tableLength, column, columnLength);
    if (CheckForError(cursor, rc, "Connection_ColumnPrivileges()") < 0) {
        Py_DECREF(cursor);
        return NULL;
    }

    return Cursor_InternalCatalogHelper(cursor);
}


//-----------------------------------------------------------------------------
// Connection_ForeignKeys()
//   Return foreign keys from the catalog for the data source.
//-----------------------------------------------------------------------------
static PyObject *Connection_ForeignKeys(
    udt_Connection *self,               // connection to fetch from
    PyObject *args,                     // arguments
    PyObject *keywordArgs)              // keyword arguments
{
    static char *keywordList[] = { "pkcatalog", "pkschema", "pktable",
            "fkcatalog", "fkschema", "fktable", NULL };
    SQLCHAR *pkCatalog, *pkSchema, *pkTable, *fkCatalog, *fkSchema, *fkTable;
    int pkCatalogLength, pkSchemaLength, pkTableLength, fkCatalogLength;
    int fkSchemaLength, fkTableLength;
    udt_Cursor *cursor;
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
    cursor = Cursor_InternalNew(self);
    if (!cursor)
        return NULL;

    // call catalog method
    rc = SQLForeignKeys(cursor->handle, pkCatalog, pkCatalogLength, pkSchema,
            pkSchemaLength, pkTable, pkTableLength, fkCatalog, fkCatalogLength,
            fkSchema, fkSchemaLength, fkTable, fkTableLength);
    if (CheckForError(cursor, rc, "Connection_ForeignKeys()") < 0) {
        Py_DECREF(cursor);
        return NULL;
    }

    return Cursor_InternalCatalogHelper(cursor);
}


//-----------------------------------------------------------------------------
// Connection_PrimaryKeys()
//   Return primary keys from the catalog for the data source.
//-----------------------------------------------------------------------------
static PyObject *Connection_PrimaryKeys(
    udt_Connection *self,               // connection to fetch from
    PyObject *args,                     // arguments
    PyObject *keywordArgs)              // keyword arguments
{
    static char *keywordList[] = { "catalog", "schema", "table", NULL };
    int catalogLength, schemaLength, tableLength;
    SQLCHAR *catalog, *schema, *table;
    udt_Cursor *cursor;
    SQLRETURN rc;

    // parse arguments
    catalog = schema = table = NULL;
    catalogLength = schemaLength = tableLength = 0;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|z#z#z#",
            keywordList, &catalog, &catalogLength, &schema, &schemaLength,
            &table, &tableLength))
        return NULL;

    // create cursor
    cursor = Cursor_InternalNew(self);
    if (!cursor)
        return NULL;

    // call catalog method
    rc = SQLPrimaryKeys(cursor->handle, catalog, catalogLength, schema,
            schemaLength, table, tableLength);
    if (CheckForError(cursor, rc, "Connection_PrimaryKeys()") < 0) {
        Py_DECREF(cursor);
        return NULL;
    }

    return Cursor_InternalCatalogHelper(cursor);
}


//-----------------------------------------------------------------------------
// Connection_Procedures()
//   Return procedures from the catalog for the data source.
//-----------------------------------------------------------------------------
static PyObject *Connection_Procedures(
    udt_Connection *self,               // connection to fetch from
    PyObject *args,                     // arguments
    PyObject *keywordArgs)              // keyword arguments
{
    static char *keywordList[] = { "catalog", "schema", "proc", NULL };
    int catalogLength, schemaLength, procLength;
    SQLCHAR *catalog, *schema, *proc;
    udt_Cursor *cursor;
    SQLRETURN rc;

    // parse arguments
    catalog = schema = proc = NULL;
    catalogLength = schemaLength = procLength = 0;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|z#z#z#",
            keywordList, &catalog, &catalogLength, &schema, &schemaLength,
            &proc, &procLength))
        return NULL;

    // create cursor
    cursor = Cursor_InternalNew(self);
    if (!cursor)
        return NULL;

    // call catalog method
    rc = SQLProcedures(cursor->handle, catalog, catalogLength, schema,
            schemaLength, proc, procLength);
    if (CheckForError(cursor, rc, "Connection_Procedures()") < 0) {
        Py_DECREF(cursor);
        return NULL;
    }

    return Cursor_InternalCatalogHelper(cursor);
}


//-----------------------------------------------------------------------------
// Connection_ProcedureColumns()
//   Return procedure columns from the catalog for the data source.
//-----------------------------------------------------------------------------
static PyObject *Connection_ProcedureColumns(
    udt_Connection *self,               // connection to fetch from
    PyObject *args,                     // arguments
    PyObject *keywordArgs)              // keyword arguments
{
    static char *keywordList[] =
            { "catalog", "schema", "proc", "column", NULL };
    int catalogLength, schemaLength, procLength, columnLength;
    SQLCHAR *catalog, *schema, *proc, *column;
    udt_Cursor *cursor;
    SQLRETURN rc;

    // parse arguments
    catalog = schema = proc = column = NULL;
    catalogLength = schemaLength = procLength = columnLength = 0;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|z#z#z#z#",
            keywordList, &catalog, &catalogLength, &schema, &schemaLength,
            &proc, &procLength, &column, &columnLength))
        return NULL;

    // create cursor
    cursor = Cursor_InternalNew(self);
    if (!cursor)
        return NULL;

    // call catalog method
    rc = SQLProcedureColumns(cursor->handle, catalog, catalogLength, schema,
            schemaLength, proc, procLength, column, columnLength);
    if (CheckForError(cursor, rc, "Connection_ProcedureColumns()") < 0) {
        Py_DECREF(cursor);
        return NULL;
    }

    return Cursor_InternalCatalogHelper(cursor);
}


//-----------------------------------------------------------------------------
// Connection_Tables()
//   Return tables from the catalog for the data source.
//-----------------------------------------------------------------------------
static PyObject *Connection_Tables(
    udt_Connection *self,               // connection to fetch from
    PyObject *args,                     // arguments
    PyObject *keywordArgs)              // keyword arguments
{
    static char *keywordList[] =
            { "catalog", "schema", "table", "type", NULL };
    int catalogLength, schemaLength, tableLength, typeLength;
    SQLCHAR *catalog, *schema, *table, *type;
    udt_Cursor *cursor;
    SQLRETURN rc;

    // parse arguments
    catalog = schema = table = type = NULL;
    catalogLength = schemaLength = tableLength = typeLength = 0;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|z#z#z#z#",
            keywordList, &catalog, &catalogLength, &schema, &schemaLength,
            &table, &tableLength, &type, &typeLength))
        return NULL;

    // create cursor
    cursor = Cursor_InternalNew(self);
    if (!cursor)
        return NULL;

    // call catalog method
    rc = SQLTables(cursor->handle, catalog, catalogLength, schema, schemaLength,
            table, tableLength, type, typeLength);
    if (CheckForError(cursor, rc, "Connection_Tables()") < 0) {
        Py_DECREF(cursor);
        return NULL;
    }

    return Cursor_InternalCatalogHelper(cursor);
}


//-----------------------------------------------------------------------------
// Connection_TablePrivileges()
//   Return table privileges from the catalog for the data source.
//-----------------------------------------------------------------------------
static PyObject *Connection_TablePrivileges(
    udt_Connection *self,               // connection to fetch from
    PyObject *args,                     // arguments
    PyObject *keywordArgs)              // keyword arguments
{
    static char *keywordList[] =
            { "catalog", "schema", "table", NULL };
    int catalogLength, schemaLength, tableLength;
    SQLCHAR *catalog, *schema, *table;
    udt_Cursor *cursor;
    SQLRETURN rc;

    // parse arguments
    catalog = schema = table = NULL;
    catalogLength = schemaLength = tableLength = 0;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|z#z#z#",
            keywordList, &catalog, &catalogLength, &schema, &schemaLength,
            &table, &tableLength))
        return NULL;

    // create cursor
    cursor = Cursor_InternalNew(self);
    if (!cursor)
        return NULL;

    // call catalog method
    rc = SQLTablePrivileges(cursor->handle, catalog, catalogLength, schema,
            schemaLength, table, tableLength);
    if (CheckForError(cursor, rc, "Connection_TablePrivileges()") < 0) {
        Py_DECREF(cursor);
        return NULL;
    }

    return Cursor_InternalCatalogHelper(cursor);
}


//-----------------------------------------------------------------------------
// Connection_GetAutoCommit()
//   Return the value of the autocommit flag.
//-----------------------------------------------------------------------------
static PyObject *Connection_GetAutoCommit(
    udt_Connection *self,               // connection to get value from
    void* arg)                          // argument (unused)
{
    SQLUINTEGER autocommit;
    PyObject *result;
    SQLRETURN rc;

    rc = SQLGetConnectAttr(self->handle, SQL_ATTR_AUTOCOMMIT,
            &autocommit, SQL_IS_UINTEGER, NULL);
    if (CheckForError(self, rc, "Connection_GetAutoCommit()") < 0)
        return NULL;
    if (autocommit)
        result = Py_True;
    else result = Py_False;
    Py_INCREF(result);
    return result;
}


//-----------------------------------------------------------------------------
// Connection_SetAutoCommit()
//   Set the value of the autocommit flag.
//-----------------------------------------------------------------------------
static int Connection_SetAutoCommit(
    udt_Connection *self,               // connection to set value on
    PyObject *value,                    // value to set
    void* arg)                          // argument (unused)
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

    rc = SQLSetConnectAttr(self->handle, SQL_ATTR_AUTOCOMMIT,
            (SQLPOINTER) sqlValue, SQL_IS_UINTEGER);
    if (CheckForError(self, rc,
            "Connection_Init(): turning off autocommit") < 0)
        return -1;
    return 0;
}

