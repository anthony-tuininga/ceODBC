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


//-----------------------------------------------------------------------------
// declaration of methods for Python type "Connection"
//-----------------------------------------------------------------------------
static PyMethodDef g_ConnectionMethods[] = {
    { "cursor", (PyCFunction) Connection_NewCursor, METH_NOARGS },
    { "commit", (PyCFunction) Connection_Commit, METH_NOARGS },
    { "rollback", (PyCFunction) Connection_Rollback, METH_NOARGS },
    { "close", (PyCFunction) Connection_Close, METH_NOARGS },
    { NULL }
};


//-----------------------------------------------------------------------------
// declaration of members for Python type "Connection"
//-----------------------------------------------------------------------------
static PyMemberDef g_ConnectionMembers[] = {
    { "dsn", T_OBJECT, offsetof(udt_Connection, dsn), READONLY },
    { NULL }
};


//-----------------------------------------------------------------------------
// declaration of Python type "Connection"
//-----------------------------------------------------------------------------
static PyTypeObject g_ConnectionType = {
    PyObject_HEAD_INIT(NULL)
    0,                                  // ob_size
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
    0,                                  // tp_getset
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

    return (PyObject*) self;
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
    SQLSMALLINT actualDsnLength;
    char actualDsn[1024], *dsn;
    int dsnLength;
    SQLRETURN rc;

    // define keyword arguments
    static char *keywordList[] = { "dsn", NULL };

    // parse arguments
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "s#", keywordList,
            &dsn, &dsnLength))
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
    rc = SQLDriverConnect(self->handle, NULL, dsn, dsnLength, actualDsn,
            sizeof(actualDsn), &actualDsnLength, SQL_DRIVER_NOPROMPT);
    if (CheckForError(self, rc,
            "Connection_Init(): connecting to driver") < 0)
        return -1;

    // mark connection as connected
    self->isConnected = 1;

    // save copy of constructed DSN
    self->dsn = PyString_FromStringAndSize(actualDsn, actualDsnLength);
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
    Py_XDECREF(self->environment);
    Py_XDECREF(self->dsn);
    self->ob_type->tp_free((PyObject*) self);
}


//-----------------------------------------------------------------------------
// Connection_Repr()
//   Return a string representation of the connection.
//-----------------------------------------------------------------------------
static PyObject *Connection_Repr(
    udt_Connection *connection)         // connection to return the string for
{
    PyObject *module, *name, *result;

    if (GetModuleAndName(connection->ob_type, &module, &name) < 0)
        return NULL;
    if (connection->dsn)
        result = PyString_FromFormat("<%s.%s to %s>",
                PyString_AS_STRING(module), PyString_AS_STRING(name),
                PyString_AS_STRING(connection->dsn));
    else result = PyString_FromFormat("<%s.%s to unknown DSN>",
                PyString_AS_STRING(module), PyString_AS_STRING(name));
    Py_DECREF(module);
    Py_DECREF(name);
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

