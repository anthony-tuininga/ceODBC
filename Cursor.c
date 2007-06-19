//-----------------------------------------------------------------------------
// Cursor.c
//   Definition of the Python type for cursors.
//-----------------------------------------------------------------------------

#define DEFAULT_LONG_SIZE               (128 * 1024)

//-----------------------------------------------------------------------------
// structure for the Python type "Cursor"
//-----------------------------------------------------------------------------
typedef struct {
    ObjectWithHandle_HEAD
    udt_Connection *connection;
    PyObject *statement;
    PyObject *resultSetVars;
    PyObject *parameterVars;
    int arraySize;
    int bindArraySize;
    int fetchArraySize;
    int setInputSizes;
    int setOutputSize;
    int setOutputSizeColumn;
    SQLINTEGER rowCount;
    int actualRows;
    int rowNum;
} udt_Cursor;


//-----------------------------------------------------------------------------
// dependent function defintions
//-----------------------------------------------------------------------------
static void Cursor_Free(udt_Cursor*);


//-----------------------------------------------------------------------------
// functions for the Python type "Cursor"
//-----------------------------------------------------------------------------
static PyObject *Cursor_GetIter(udt_Cursor*);
static PyObject *Cursor_GetNext(udt_Cursor*);
static PyObject *Cursor_Close(udt_Cursor*, PyObject*);
static PyObject *Cursor_Execute(udt_Cursor*, PyObject*);
static PyObject *Cursor_ExecuteMany(udt_Cursor*, PyObject*);
static PyObject *Cursor_FetchOne(udt_Cursor*, PyObject*);
static PyObject *Cursor_FetchMany(udt_Cursor*, PyObject*, PyObject*);
static PyObject *Cursor_FetchAll(udt_Cursor*, PyObject*);
static PyObject *Cursor_Prepare(udt_Cursor*, PyObject*);
static PyObject *Cursor_SetInputSizes(udt_Cursor*, PyObject*);
static PyObject *Cursor_SetOutputSize(udt_Cursor*, PyObject*);
static PyObject *Cursor_GetDescription(udt_Cursor*, void*);
static PyObject *Cursor_New(PyTypeObject*, PyObject*, PyObject*);
static int Cursor_Init(udt_Cursor*, PyObject*, PyObject*);
static PyObject *Cursor_Repr(udt_Cursor*);


//-----------------------------------------------------------------------------
// declaration of methods for Python type "Cursor"
//-----------------------------------------------------------------------------
static PyMethodDef g_CursorMethods[] = {
    { "execute", (PyCFunction) Cursor_Execute, METH_VARARGS },
    { "executemany", (PyCFunction) Cursor_ExecuteMany, METH_VARARGS },
    { "fetchall", (PyCFunction) Cursor_FetchAll, METH_NOARGS },
    { "fetchone", (PyCFunction) Cursor_FetchOne, METH_NOARGS },
    { "fetchmany", (PyCFunction) Cursor_FetchMany,
              METH_VARARGS | METH_KEYWORDS },
    { "prepare", (PyCFunction) Cursor_Prepare, METH_VARARGS },
    { "setinputsizes", (PyCFunction) Cursor_SetInputSizes, METH_VARARGS },
    { "setoutputsize", (PyCFunction) Cursor_SetOutputSize, METH_VARARGS },
    { "close", (PyCFunction) Cursor_Close, METH_NOARGS },
    { NULL, NULL }
};


//-----------------------------------------------------------------------------
// declaration of members for Python type "Cursor"
//-----------------------------------------------------------------------------
static PyMemberDef g_CursorMembers[] = {
    { "arraysize", T_INT, offsetof(udt_Cursor, arraySize), 0 },
    { "bindarraysize", T_INT, offsetof(udt_Cursor, bindArraySize), 0 },
    { "rowcount", T_INT, offsetof(udt_Cursor, rowCount), READONLY },
    { "statement", T_OBJECT, offsetof(udt_Cursor, statement), READONLY },
    { "connection", T_OBJECT_EX, offsetof(udt_Cursor, connection), READONLY },
    { NULL }
};


//-----------------------------------------------------------------------------
// declaration of calculated members for Python type "Cursor"
//-----------------------------------------------------------------------------
static PyGetSetDef g_CursorCalcMembers[] = {
    { "description", (getter) Cursor_GetDescription, 0, 0, 0 },
    { NULL }
};


//-----------------------------------------------------------------------------
// declaration of Python type "Cursor"
//-----------------------------------------------------------------------------
static PyTypeObject g_CursorType = {
    PyObject_HEAD_INIT(NULL)
    0,                                  // ob_size
    "ceODBC.Cursor",                    // tp_name
    sizeof(udt_Cursor),                 // tp_basicsize
    0,                                  // tp_itemsize
    (destructor) Cursor_Free,           // tp_dealloc
    0,                                  // tp_print
    0,                                  // tp_getattr
    0,                                  // tp_setattr
    0,                                  // tp_compare
    (reprfunc) Cursor_Repr,             // tp_repr
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
    (getiterfunc) Cursor_GetIter,       // tp_iter
    (iternextfunc) Cursor_GetNext,      // tp_iternext
    g_CursorMethods,                    // tp_methods
    g_CursorMembers,                    // tp_members
    g_CursorCalcMembers,                // tp_getset
    0,                                  // tp_base
    0,                                  // tp_dict
    0,                                  // tp_descr_get
    0,                                  // tp_descr_set
    0,                                  // tp_dictoffset
    (initproc) Cursor_Init,             // tp_init
    0,                                  // tp_alloc
    Cursor_New,                         // tp_new
    0,                                  // tp_free
    0,                                  // tp_is_gc
    0                                   // tp_bases
};


#include "Variable.c"


//-----------------------------------------------------------------------------
// Cursor_IsOpen()
//   Determines if the cursor object is open and if so, if the connection is
// also open.
//-----------------------------------------------------------------------------
static int Cursor_IsOpen(
    udt_Cursor *self)                   // cursor to check
{
    if (!self->handle) {
        PyErr_SetString(g_InterfaceErrorException, "not open");
        return -1;
    }
    return Connection_IsConnected(self->connection);
}


//-----------------------------------------------------------------------------
// Cursor_New()
//   Create a new cursor object.
//-----------------------------------------------------------------------------
static PyObject *Cursor_New(
    PyTypeObject *type,                 // type object
    PyObject *args,                     // arguments
    PyObject *keywordArgs)              // keyword arguments
{
    return type->tp_alloc(type, 0);
}


//-----------------------------------------------------------------------------
// Cursor_Init()
//   Create a new cursor object.
//-----------------------------------------------------------------------------
static int Cursor_Init(
    udt_Cursor *self,                   // cursor object
    PyObject *args,                     // arguments
    PyObject *keywordArgs)              // keyword arguments
{
    udt_Connection *connection;
    SQLRETURN rc;

    // parse arguments
    if (!PyArg_ParseTuple(args, "O!", &g_ConnectionType, &connection))
        return -1;

    // initialize members
    self->handleType = SQL_HANDLE_STMT;
    self->handle = SQL_NULL_HANDLE;
    Py_INCREF(connection);
    self->resultSetVars = NULL;
    self->parameterVars = NULL;
    self->connection = connection;
    self->arraySize = 1;
    self->bindArraySize = 1;
    self->setInputSizes = 0;
    self->setOutputSize = DEFAULT_LONG_SIZE;
    self->setOutputSizeColumn = 0;

    // allocate handle
    rc = SQLAllocHandle(self->handleType, self->connection->handle,
            &self->handle);
    if (CheckForError(self->connection, rc,
            "Cursor_Init(): allocate statement handle") < 0)
        return -1;

    return 0;
}


//-----------------------------------------------------------------------------
// Cursor_Repr()
//   Return a string representation of the cursor.
//-----------------------------------------------------------------------------
static PyObject *Cursor_Repr(
    udt_Cursor *cursor)                 // cursor to return the string for
{
    PyObject *connectionRepr, *module, *name, *result;

    connectionRepr = PyObject_Repr((PyObject*) cursor->connection);
    if (!connectionRepr)
        return NULL;
    if (GetModuleAndName(cursor->ob_type, &module, &name) < 0) {
        Py_DECREF(connectionRepr);
        return NULL;
    }
    result = PyString_FromFormat("<%s.%s on %s>",
            PyString_AS_STRING(module), PyString_AS_STRING(name),
            PyString_AS_STRING(connectionRepr));
    Py_DECREF(connectionRepr);
    Py_DECREF(module);
    Py_DECREF(name);
    return result;
}


//-----------------------------------------------------------------------------
// Cursor_Free()
//   Deallocate the cursor.
//-----------------------------------------------------------------------------
static void Cursor_Free(
    udt_Cursor *self)                   // cursor object
{
    if (self->handle)
        SQLFreeHandle(self->handleType, self->handle);
    Py_XDECREF(self->connection);
    Py_XDECREF(self->resultSetVars);
    Py_XDECREF(self->parameterVars);
    Py_XDECREF(self->statement);
    self->ob_type->tp_free((PyObject*) self);
}


//-----------------------------------------------------------------------------
// Cursor_PrepareResultSet()
//   Prepare the result set for use.
//-----------------------------------------------------------------------------
static int Cursor_PrepareResultSet(
    udt_Cursor *self)                   // cursor to perform define on
{
    SQLSMALLINT numColumns;
    SQLUSMALLINT position;
    udt_Variable *var;
    SQLRETURN rc;

    // determine the number of columns in the result set
    rc = SQLNumResultCols(self->handle, &numColumns);
    if (CheckForError(self, rc,
            "Cursor_PrepareResultSet(): determine number of columns") < 0)
        return -1;

    // if the number returned is 0, that means this isn't a query
    if (numColumns == 0)
        return 0;

    // set up the fetch array size
    self->fetchArraySize = self->arraySize;
    rc = SQLSetStmtAttr(self->handle, SQL_ATTR_ROW_ARRAY_SIZE,
            (SQLPOINTER) self->fetchArraySize, SQL_IS_INTEGER);
    if (CheckForError(self, rc,
            "Cursor_PrepareResultSet(): set array size") < 0)
        return -1;

    // set up the rows fetched pointer
    rc = SQLSetStmtAttr(self->handle, SQL_ATTR_ROWS_FETCHED_PTR,
            &self->actualRows, SQL_IS_POINTER);
    if (CheckForError(self, rc,
            "Cursor_PrepareResultSet(): set rows fetched pointer") < 0)
        return -1;

    // create a list corresponding to the number of items
    self->resultSetVars = PyList_New(numColumns);
    if (!self->resultSetVars)
        return -1;

    // create a variable for each column in the result set
    for (position = 0; position < numColumns; position++) {
        var = Variable_NewForResultSet(self, position + 1);
        if (!var)
            return -1;
        PyList_SET_ITEM(self->resultSetVars, position, (PyObject *) var);
    }

    return 0;
}


//-----------------------------------------------------------------------------
// Cursor_BindParameterHelper()
//   Helper for setting a bind variable.
//-----------------------------------------------------------------------------
static int Cursor_BindParameterHelper(
    udt_Cursor *self,			// cursor to perform bind on
    unsigned numElements,		// number of elements to create
    unsigned arrayPos,			// array position to set
    PyObject *value,			// value to bind
    udt_Variable *origVar,		// original variable bound
    udt_Variable **newVar)		// new variable to be bound
{
    int isValueVar;

    // initialization
    *newVar = NULL;
    isValueVar = Variable_Check(value);

    // handle case where variable is already bound
    if (origVar) {

        // if the value is a variable object, rebind it if necessary
        if (isValueVar) {
            if ( (PyObject*) origVar != value) {
                Py_INCREF(value);
                *newVar = (udt_Variable*) value;
                (*newVar)->position = -1;
            }

        // if the number of elements has changed, create a new variable
        // this is only necessary for executemany() since execute() always
        // passes a value of 1 for the number of elements
        } else if (numElements > origVar->numElements) {
            *newVar = Variable_New(self, numElements, origVar->type,
                    origVar->size);
            if (!*newVar)
                return -1;
            if (Variable_SetValue(*newVar, arrayPos, value) < 0)
                return -1;

        // otherwise, attempt to set the value
        } else if (Variable_SetValue(origVar, arrayPos, value) < 0) {

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
            *newVar = (udt_Variable*) value;
            (*newVar)->position = -1;

        // otherwise, create a new variable
        } else {
            *newVar = Variable_NewByValue(self, value, numElements);
            if (!*newVar)
                return -1;
            if (Variable_SetValue(*newVar, arrayPos, value) < 0)
                return -1;
        }

    }

    return 0;
}


//-----------------------------------------------------------------------------
// Cursor_BindParameters()
//   Bind all parameters, creating new ones as needed.
//-----------------------------------------------------------------------------
static int Cursor_BindParameters(
    udt_Cursor *self,			// cursor to perform binds on
    PyObject *parameters,		// parameters to bind
    unsigned numElements,		// number of elements to create
    unsigned arrayPos)			// array position to set
{
    int i, numParams, origNumParams;
    udt_Variable *newVar, *origVar;
    PyObject *value;

    // set up the list of parameters
    numParams = PySequence_Size(parameters);
    if (numParams < 0)
        return -1;
    if (self->parameterVars) {
        origNumParams = PyList_GET_SIZE(self->parameterVars);
    } else {
        origNumParams = 0;
        self->parameterVars = PyList_New(numParams);
        if (!self->parameterVars)
            return -1;
    }

    // bind parameters
    for (i = 0; i < numParams; i++) {
        value = PySequence_GetItem(parameters, i);
        if (!value)
            return -1;
        Py_DECREF(value);
        if (i < origNumParams) {
            origVar = (udt_Variable*) PyList_GET_ITEM(self->parameterVars, i);
            if ( (PyObject*) origVar == Py_None)
                origVar = NULL;
        } else origVar = NULL;
        if (Cursor_BindParameterHelper(self, numElements, arrayPos, value,
                origVar, &newVar) < 0)
            return -1;
        if (newVar) {
            if (i < PyList_GET_SIZE(self->parameterVars)) {
                if (PyList_SetItem(self->parameterVars, i,
                        (PyObject*) newVar) < 0) {
                    Py_DECREF(newVar);
                    return -1;
                }
            } else {
                if (PyList_Append(self->parameterVars,
                        (PyObject*) newVar) < 0) {
                    Py_DECREF(newVar);
                    return -1;
                }
                Py_DECREF(newVar);
            }
            if (Variable_BindParameter(newVar, self, i + 1) < 0)
                return -1;
        }
        if (!newVar && origVar->position < 0) {
            if (Variable_BindParameter(origVar, self, i + 1) < 0)
                return -1;
        }
    }

    return 0;
}


//-----------------------------------------------------------------------------
// Cursor_InternalExecute()
//   Perform the work of executing a cursor and set the rowcount appropriately
// regardless of whether an error takes place.
//-----------------------------------------------------------------------------
static int Cursor_InternalExecute(
    udt_Cursor *self)                   // cursor to perform the execute on
{
    SQLRETURN rc;

    // execute the statement
    Py_BEGIN_ALLOW_THREADS
    rc = SQLExecute(self->handle);
    Py_END_ALLOW_THREADS

    // SQL_NO_DATA can be returned from (for example) a delete statement that
    // didn't actually delete any rows; ignore this foolishness
    if (rc == SQL_NO_DATA) {
        self->rowCount = 0;
        return 0;
    }

    if (CheckForError(self, rc, "Cursor_InternalExecute()") < 0)
        return -1;

    // prepare result set, if necessary
    if (!self->resultSetVars && Cursor_PrepareResultSet(self) < 0)
        return -1;

    // determine the value of the rowcount attribute
    if (self->resultSetVars) {
        self->rowCount = 0;
        self->actualRows = -1;
        self->rowNum = 0;
    } else {
        rc = SQLRowCount(self->handle, &self->rowCount);
        if (CheckForError(self, rc, "Cursor_SetRowCount()") < 0)
            return -1;
    }

    // reset input and output sizes
    self->setInputSizes = 0;
    self->setOutputSize = DEFAULT_LONG_SIZE;
    self->setOutputSizeColumn = 0;

    return 0;
}


//-----------------------------------------------------------------------------
// Cursor_ItemDescription()
//   Returns a tuple for the column as defined by the DB API.
//-----------------------------------------------------------------------------
static PyObject *Cursor_ItemDescription(
    udt_Cursor *self,                   // cursor object
    SQLUSMALLINT position)              // position in description
{
    SQLSMALLINT dataType, nameLength, scale, nullable;
    udt_VariableType *varType;
    SQLUINTEGER precision;
    PyObject *tuple;
    char name[256];
    SQLRETURN rc;
    int i;

    // retrieve information about the column
    rc = SQLDescribeCol(self->handle, position, (SQLCHAR*) name, sizeof(name),
            &nameLength, &dataType, &precision, &scale, &nullable);
    if (CheckForError(self, rc,
            "Cursor_ItemDescription(): get column info") < 0)
        return NULL;

    // determine variable type
    varType = Variable_TypeBySqlDataType(dataType);
    if (!varType)
        return NULL;

    // create the tuple and populate it
    tuple = PyTuple_New(7);
    if (!tuple)
        return NULL;

    // set each of the items in the tuple
    Py_INCREF(varType->pythonType);
    Py_INCREF(Py_None);
    Py_INCREF(Py_None);
    PyTuple_SET_ITEM(tuple, 0,
            PyString_FromStringAndSize( (char*) name, nameLength));
    PyTuple_SET_ITEM(tuple, 1, (PyObject*) varType->pythonType);
    PyTuple_SET_ITEM(tuple, 2, Py_None);
    PyTuple_SET_ITEM(tuple, 3, Py_None);
    PyTuple_SET_ITEM(tuple, 4, PyInt_FromLong(precision));
    PyTuple_SET_ITEM(tuple, 5, PyInt_FromLong(scale));
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
// Cursor_GetDescription()
//   Return a list of 7-tuples consisting of the description of the define
// variables.
//-----------------------------------------------------------------------------
static PyObject *Cursor_GetDescription(
    udt_Cursor *self,                   // cursor object
    void *arg)                          // optional argument (ignored)
{
    PyObject *results, *tuple;
    int numItems, index;

    // make sure the cursor is open
    if (Cursor_IsOpen(self) < 0)
        return NULL;

    // if no statement has been executed yet, return None
    if (!self->resultSetVars) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    // create a list of the required length
    numItems = PyList_GET_SIZE(self->resultSetVars);
    results = PyList_New(numItems);
    if (!results)
        return NULL;

    // create tuples corresponding to the select-items
    for (index = 0; index < numItems; index++) {
        tuple = Cursor_ItemDescription(self, index + 1);
        if (!tuple) {
            Py_DECREF(results);
            return NULL;
        }
        PyList_SET_ITEM(results, index, tuple);
    }

    return results;
}


//-----------------------------------------------------------------------------
// Cursor_Close()
//   Close the cursor.
//-----------------------------------------------------------------------------
static PyObject *Cursor_Close(
    udt_Cursor *self,                   // cursor to close
    PyObject *args)                     // arguments
{
    SQLRETURN rc;

    // make sure cursor is actually open
    if (Cursor_IsOpen(self) < 0)
        return NULL;

    // close the cursor
    rc = SQLCloseCursor(self->handle);
    if (CheckForError(self, rc, "Cursor_Close()") < 0)
        return NULL;
    self->handle = NULL;

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// Cursor_CreateTuple()
//   Create a tuple consisting of each of the items in the select-list.
//-----------------------------------------------------------------------------
static PyObject *Cursor_CreateTuple(
    udt_Cursor *self)                   // cursor object
{
    PyObject *tuple, *item;
    int numItems, pos;
    udt_Variable *var;

    // create a new tuple
    numItems = PyList_GET_SIZE(self->resultSetVars);
    tuple = PyTuple_New(numItems);
    if (!tuple)
        return NULL;

    // acquire the value for each item
    for (pos = 0; pos < numItems; pos++) {
        var = (udt_Variable*) PyList_GET_ITEM(self->resultSetVars, pos);
        item = Variable_GetValue(var, self->rowNum);
        if (!item) {
            Py_DECREF(tuple);
            return NULL;
        }
        PyTuple_SET_ITEM(tuple, pos, item);
    }

    self->rowNum++;
    self->rowCount++;
    return tuple;
}


//-----------------------------------------------------------------------------
// Cursor_InternalPrepare()
//   Internal method for preparing a statement for execution.
//-----------------------------------------------------------------------------
static int Cursor_InternalPrepare(
    udt_Cursor *self,                   // cursor to perform prepare on
    PyObject *statement)                // statement to prepare
{
    SQLRETURN rc;

    // make sure we don't get a situation where nothing is to be executed
    if (statement == Py_None && !self->statement) {
        PyErr_SetString(g_ProgrammingErrorException,
                "no statement specified and no prior statement prepared");
        return -1;
    }

    // nothing to do if the statement is identical to the one already stored
    if (statement == Py_None || statement == self->statement)
        return 0;

    // keep track of the statement; close original cursor if necessary
    if (self->statement) {
        SQLCloseCursor(self->handle);
        Py_DECREF(self->statement);
    }
    Py_INCREF(statement);
    self->statement = statement;

    // clear previous result set parameters
    Py_XDECREF(self->resultSetVars);
    self->resultSetVars = NULL;

    // prepare statement
    Py_BEGIN_ALLOW_THREADS
    rc = SQLPrepare(self->handle, (SQLCHAR*) PyString_AS_STRING(statement),
            PyString_GET_SIZE(statement));
    Py_END_ALLOW_THREADS
    if (CheckForError(self, rc, "Cursor_InternalPrepare()") < 0)
        return -1;

    // clear parameters
    if (!self->setInputSizes) {
        Py_XDECREF(self->parameterVars);
        self->parameterVars = NULL;
    }

    return 0;
}


//-----------------------------------------------------------------------------
// Cursor_Prepare()
//   Prepare the statement for execution.
//-----------------------------------------------------------------------------
static PyObject *Cursor_Prepare(
    udt_Cursor *self,                   // cursor to perform prepare on
    PyObject *args)                     // arguments
{
    PyObject *statement;

    if (!PyArg_ParseTuple(args, "S", &statement))
        return NULL;
    if (Cursor_IsOpen(self) < 0)
        return NULL;
    if (Cursor_InternalPrepare(self, statement) < 0)
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// Cursor_Execute()
//   Execute the statement.
//-----------------------------------------------------------------------------
static PyObject *Cursor_Execute(
    udt_Cursor *self,                   // cursor to execute
    PyObject *args)                     // arguments
{
    PyObject *statement, *executeArgs;

    // verify we have the right arguments
    executeArgs = NULL;
    if (!PyArg_ParseTuple(args, "O|O", &statement, &executeArgs))
        return NULL;
    if (statement != Py_None && !PyString_Check(statement)) {
        PyErr_SetString(PyExc_TypeError, "expecting None or a string");
        return NULL;
    }
    if (executeArgs && !PySequence_Check(executeArgs)) {
        PyErr_SetString(PyExc_TypeError, "expecting a sequence");
        return NULL;
    }

    // perform the work of executing the statement
    if (Cursor_IsOpen(self) < 0)
        return NULL;
    if (Cursor_InternalPrepare(self, statement) < 0)
        return NULL;
    if (executeArgs && Cursor_BindParameters(self, executeArgs, 1, 0) < 0)
        return NULL;
    if (Cursor_InternalExecute(self) < 0)
        return NULL;

    // for queries, return the cursor for convenience
    if (self->resultSetVars) {
        Py_INCREF(self);
        return (PyObject*) self;
    }

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// Cursor_ExecuteMany()
//   Execute the statement many times. The number of times is equivalent to the
// number of elements in the array of dictionaries.
//-----------------------------------------------------------------------------
static PyObject *Cursor_ExecuteMany(
    udt_Cursor *self,                   // cursor to execute
    PyObject *args)                     // arguments
{
    PyObject *arguments, *listOfArguments, *statement;
    int i, numRows;
    SQLRETURN rc;

    // expect statement text (optional) plus list of sequences
    if (!PyArg_ParseTuple(args, "OO!", &statement, &PyList_Type,
            &listOfArguments))
        return NULL;
    if (statement != Py_None && !PyString_Check(statement)) {
        PyErr_SetString(PyExc_TypeError, "expecting None or a string");
        return NULL;
    }

    // make sure the cursor is open
    if (Cursor_IsOpen(self) < 0)
        return NULL;

    // prepare the statement
    if (Cursor_InternalPrepare(self, statement) < 0)
        return NULL;

    // perform binds
    numRows = PyList_GET_SIZE(listOfArguments);
    for (i = 0; i < numRows; i++) {
        arguments = PyList_GET_ITEM(listOfArguments, i);
        if (!PySequence_Check(arguments)) {
            PyErr_SetString(g_InterfaceErrorException,
                    "expecting a list of sequences");
            return NULL;
        }
        if (Cursor_BindParameters(self, arguments, numRows, i) < 0)
            return NULL;
    }

    // set the number of parameters bound
    rc = SQLSetStmtAttr(self->handle, SQL_ATTR_PARAMSET_SIZE,
            (SQLPOINTER) numRows, SQL_IS_UINTEGER);
    if (CheckForError(self, rc, "Cursor_ExecuteMany(): set paramset size") < 0)
        return NULL;

    // execute the statement
    if (Cursor_InternalExecute(self) < 0)
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// Cursor_VerifyFetch()
//   Verify that fetching may happen from this cursor.
//-----------------------------------------------------------------------------
static int Cursor_VerifyFetch(
    udt_Cursor *self)                   // cursor to fetch from
{
    // make sure the cursor is open
    if (Cursor_IsOpen(self) < 0)
        return -1;

    // make sure the cursor is for a query
    if (!self->resultSetVars) {
        PyErr_SetString(g_InterfaceErrorException, "not a query");
        return -1;
    }

    return 0;
}


//-----------------------------------------------------------------------------
// Cursor_InternalFetch()
//   Performs the actual fetch from the database.
//-----------------------------------------------------------------------------
static int Cursor_InternalFetch(
    udt_Cursor *self)                   // cursor to fetch from
{
    SQLRETURN rc;

    if (!self->resultSetVars) {
        PyErr_SetString(g_InterfaceErrorException, "query not executed");
        return -1;
    }
    Py_BEGIN_ALLOW_THREADS
    rc = SQLFetch(self->handle);
    Py_END_ALLOW_THREADS
    if (rc != SQL_NO_DATA) {
        if (CheckForError(self, rc, "Cursor_InternalFetch(): fetch") < 0)
            return -1;
    }
    self->rowNum = 0;
    return 0;
}


//-----------------------------------------------------------------------------
// Cursor_MoreRows()
//   Returns an integer indicating if more rows can be retrieved from the
// cursor.
//-----------------------------------------------------------------------------
static int Cursor_MoreRows(
    udt_Cursor *self)                   // cursor to fetch from
{
    if (self->rowNum >= self->actualRows) {
        if (self->actualRows < 0 || self->actualRows == self->fetchArraySize) {
            if (Cursor_InternalFetch(self) < 0)
                return -1;
        }
        if (self->rowNum >= self->actualRows)
            return 0;
    }
    return 1;
}


//-----------------------------------------------------------------------------
// Cursor_MultiFetch()
//   Return a list consisting of the remaining rows up to the given row limit
// (if specified).
//-----------------------------------------------------------------------------
static PyObject *Cursor_MultiFetch(
    udt_Cursor *self,                   // cursor to fetch from
    int rowLimit)                       // row limit
{
    PyObject *results, *tuple;
    int row, rc;

    // create an empty list
    results = PyList_New(0);
    if (!results)
        return NULL;

    // fetch as many rows as possible
    for (row = 0; rowLimit == 0 || row < rowLimit; row++) {
        rc = Cursor_MoreRows(self);
        if (rc < 0) {
            Py_DECREF(results);
            return NULL;
        } else if (rc == 0) {
            break;
        } else {
            tuple = Cursor_CreateTuple(self);
            if (!tuple) {
                Py_DECREF(results);
                return NULL;
            }
            if (PyList_Append(results, tuple) < 0) {
                Py_DECREF(tuple);
                Py_DECREF(results);
                return NULL;
            }
            Py_DECREF(tuple);
        }
    }

    return results;
}

//-----------------------------------------------------------------------------
// Cursor_FetchOne()
//   Fetch a single row from the cursor.
//-----------------------------------------------------------------------------
static PyObject *Cursor_FetchOne(
    udt_Cursor *self,                   // cursor to fetch from
    PyObject *args)                     // arguments
{
    int rc;

    // verify fetch can be performed
    if (Cursor_VerifyFetch(self) < 0)
        return NULL;

    // setup return value
    rc = Cursor_MoreRows(self);
    if (rc < 0)
        return NULL;
    else if (rc > 0)
        return Cursor_CreateTuple(self);

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// Cursor_FetchMany()
//   Fetch multiple rows from the cursor based on the arraysize.
//-----------------------------------------------------------------------------
static PyObject *Cursor_FetchMany(
    udt_Cursor *self,                   // cursor to fetch from
    PyObject *args,                     // arguments
    PyObject *keywordArgs)              // keyword arguments
{
    static char *keywordList[] = { "numRows", NULL };
    int rowLimit;

    // parse arguments -- optional rowlimit expected
    rowLimit = self->arraySize;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "|i", keywordList,
            &rowLimit))
        return NULL;

    // verify fetch can be performed
    if (Cursor_VerifyFetch(self) < 0)
        return NULL;

    return Cursor_MultiFetch(self, rowLimit);
}


//-----------------------------------------------------------------------------
// Cursor_FetchAll()
//   Fetch all remaining rows from the cursor.
//-----------------------------------------------------------------------------
static PyObject *Cursor_FetchAll(
    udt_Cursor *self,                   // cursor to fetch from
    PyObject *args)                     // arguments
{
    if (Cursor_VerifyFetch(self) < 0)
        return NULL;
    return Cursor_MultiFetch(self, 0);
}


//-----------------------------------------------------------------------------
// Cursor_SetInputSizes()
//   Set the sizes of the bind variables.
//-----------------------------------------------------------------------------
static PyObject *Cursor_SetInputSizes(
    udt_Cursor *self,                   // cursor to fetch from
    PyObject *args)                     // arguments
{
    PyObject *parameterVars, *value, *inputValue;
    int numArgs, i;

    // make sure the cursor is open
    if (Cursor_IsOpen(self) < 0)
        return NULL;

    // initialization of parameter variables
    numArgs = PyTuple_Size(args);
    parameterVars = PyList_New(numArgs);

    // process each input
    for (i = 0; i < numArgs; i++) {
        inputValue = PyTuple_GET_ITEM(args, i);
        if (inputValue == Py_None) {
            Py_INCREF(Py_None);
            value = Py_None;
        } else {
            value = (PyObject*) Variable_NewByType(self, inputValue,
                    self->bindArraySize);
            if (!value) {
                Py_DECREF(parameterVars);
                return NULL;
            }
        }
        PyList_SET_ITEM(parameterVars, i, value);
    }

    // overwrite existing parameter vars, if any
    Py_XDECREF(self->parameterVars);
    self->parameterVars = parameterVars;
    self->setInputSizes = 1;

    Py_INCREF(self->parameterVars);
    return self->parameterVars;
}


//-----------------------------------------------------------------------------
// Cursor_SetOutputSize()
//   Set the size of all of the long columns or just one of them.
//-----------------------------------------------------------------------------
static PyObject *Cursor_SetOutputSize(
    udt_Cursor *self,                   // cursor to fetch from
    PyObject *args)                     // arguments
{
    self->setOutputSizeColumn = 0;
    if (!PyArg_ParseTuple(args, "i|i", &self->setOutputSize,
            &self->setOutputSizeColumn))
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// Cursor_GetIter()
//   Return a reference to the cursor which supports the iterator protocol.
//-----------------------------------------------------------------------------
static PyObject *Cursor_GetIter(
    udt_Cursor *self)                   // cursor
{
    if (Cursor_VerifyFetch(self) < 0)
        return NULL;
    Py_INCREF(self);
    return (PyObject*) self;
}


//-----------------------------------------------------------------------------
// Cursor_GetNext()
//   Return a reference to the cursor which supports the iterator protocol.
//-----------------------------------------------------------------------------
static PyObject *Cursor_GetNext(
    udt_Cursor *self)                   // cursor
{
    int rc;

    if (Cursor_VerifyFetch(self) < 0)
        return NULL;
    rc = Cursor_MoreRows(self);
    if (rc < 0)
        return NULL;
    else if (rc > 0)
        return Cursor_CreateTuple(self);

    // no more rows, return NULL without setting an exception
    return NULL;
}

