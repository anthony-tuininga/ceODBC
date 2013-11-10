//-----------------------------------------------------------------------------
// Cursor.c
//   Definition of the Python type for cursors.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// structure for the Python type "Cursor"
//-----------------------------------------------------------------------------
typedef struct {
    ObjectWithHandle_HEAD
    udt_Connection *connection;
    PyObject *statement;
    PyObject *resultSetVars;
    PyObject *parameterVars;
    PyObject *rowFactory;
    int arraySize;
    int bindArraySize;
    SQLULEN fetchArraySize;
    int setInputSizes;
    int setOutputSize;
    int setOutputSizeColumn;
    SQLLEN rowCount;
    int actualRows;
    int rowNum;
    int logSql;
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
static PyObject *Cursor_CallFunc(udt_Cursor*, PyObject*);
static PyObject *Cursor_CallProc(udt_Cursor*, PyObject*);
static PyObject *Cursor_Close(udt_Cursor*, PyObject*);
static PyObject *Cursor_NextSet(udt_Cursor*, PyObject*);
static PyObject *Cursor_ExecDirect(udt_Cursor*, PyObject*);
static PyObject *Cursor_Execute(udt_Cursor*, PyObject*);
static PyObject *Cursor_ExecuteMany(udt_Cursor*, PyObject*);
static PyObject *Cursor_FetchOne(udt_Cursor*, PyObject*);
static PyObject *Cursor_FetchMany(udt_Cursor*, PyObject*, PyObject*);
static PyObject *Cursor_FetchAll(udt_Cursor*, PyObject*);
static PyObject *Cursor_Prepare(udt_Cursor*, PyObject*);
static PyObject *Cursor_SetInputSizes(udt_Cursor*, PyObject*);
static PyObject *Cursor_SetOutputSize(udt_Cursor*, PyObject*);
static PyObject *Cursor_GetDescription(udt_Cursor*, void*);
static PyObject *Cursor_GetName(udt_Cursor*, void*);
static int Cursor_SetName(udt_Cursor*, PyObject*, void*);
static PyObject *Cursor_New(PyTypeObject*, PyObject*, PyObject*);
static int Cursor_Init(udt_Cursor*, PyObject*, PyObject*);
static PyObject *Cursor_Repr(udt_Cursor*);
static PyObject *Cursor_Var(udt_Cursor*, PyObject*, PyObject*);


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
    { "callfunc", (PyCFunction) Cursor_CallFunc, METH_VARARGS },
    { "callproc", (PyCFunction) Cursor_CallProc, METH_VARARGS },
    { "close", (PyCFunction) Cursor_Close, METH_NOARGS },
    { "nextset", (PyCFunction) Cursor_NextSet, METH_NOARGS },
    { "execdirect", (PyCFunction) Cursor_ExecDirect, METH_VARARGS },
    { "var", (PyCFunction) Cursor_Var, METH_VARARGS | METH_KEYWORDS },
    { NULL, NULL }
};


//-----------------------------------------------------------------------------
// declaration of members for Python type "Cursor"
//-----------------------------------------------------------------------------
static PyMemberDef g_CursorMembers[] = {
    { "arraysize", T_INT, offsetof(udt_Cursor, arraySize), 0 },
    { "bindarraysize", T_INT, offsetof(udt_Cursor, bindArraySize), 0 },
    { "logsql", T_INT, offsetof(udt_Cursor, logSql), 0 },
    { "rowcount", T_INT, offsetof(udt_Cursor, rowCount), READONLY },
    { "statement", T_OBJECT, offsetof(udt_Cursor, statement), READONLY },
    { "connection", T_OBJECT_EX, offsetof(udt_Cursor, connection), READONLY },
    { "rowfactory", T_OBJECT, offsetof(udt_Cursor, rowFactory), 0 },
    { NULL }
};


//-----------------------------------------------------------------------------
// declaration of calculated members for Python type "Cursor"
//-----------------------------------------------------------------------------
static PyGetSetDef g_CursorCalcMembers[] = {
    { "description", (getter) Cursor_GetDescription, 0, 0, 0 },
    { "name", (getter) Cursor_GetName, (setter) Cursor_SetName, 0, 0 },
    { NULL }
};


//-----------------------------------------------------------------------------
// declaration of Python type "Cursor"
//-----------------------------------------------------------------------------
static PyTypeObject g_CursorType = {
    PyVarObject_HEAD_INIT(NULL, 0)
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
// Cursor_InternalInit()
//   Internal method used for creating a new cursor.
//-----------------------------------------------------------------------------
static int Cursor_InternalInit(
    udt_Cursor *self,                   // cursor object
    udt_Connection *connection)         // connection object
{
    SQLRETURN rc;

    // initialize members
    self->handleType = SQL_HANDLE_STMT;
    self->handle = SQL_NULL_HANDLE;
    Py_INCREF(connection);
    self->resultSetVars = NULL;
    self->parameterVars = NULL;
    self->statement = NULL;
    self->rowFactory = NULL;
    self->connection = connection;
    self->arraySize = 1;
    self->bindArraySize = 1;
    self->logSql = connection->logSql;
    self->setInputSizes = 0;
    self->setOutputSize = 0;
    self->setOutputSizeColumn = 0;
    self->rowCount = 0;
    self->actualRows = 0;
    self->rowNum = 0;

    // allocate handle
    rc = SQLAllocHandle(self->handleType, self->connection->handle,
            &self->handle);
    if (CheckForError(self->connection, rc,
            "Cursor_Init(): allocate statement handle") < 0)
        return -1;

    return 0;
}


//-----------------------------------------------------------------------------
// Cursor_InternalNew()
//   Internal method of creating a new cursor.
//-----------------------------------------------------------------------------
static udt_Cursor *Cursor_InternalNew(
    udt_Connection *connection)         // connection object
{
    udt_Cursor *self;

    self = PyObject_NEW(udt_Cursor, &g_CursorType);
    if (!self)
        return NULL;
    if (Cursor_InternalInit(self, connection) < 0) {
        Py_DECREF(self);
        return NULL;
    }

    return self;
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

    if (!PyArg_ParseTuple(args, "O!", &g_ConnectionType, &connection))
        return -1;
    return Cursor_InternalInit(self, connection);
}


//-----------------------------------------------------------------------------
// Cursor_Repr()
//   Return a string representation of the cursor.
//-----------------------------------------------------------------------------
static PyObject *Cursor_Repr(
    udt_Cursor *cursor)                 // cursor to return the string for
{
    PyObject *connectionRepr, *module, *name, *result, *format, *formatArgs;

    format = ceString_FromAscii("<%s.%s on %s>");
    if (!format)
        return NULL;
    connectionRepr = PyObject_Repr((PyObject*) cursor->connection);
    if (!connectionRepr) {
        Py_DECREF(format);
        return NULL;
    }
    if (GetModuleAndName(Py_TYPE(cursor), &module, &name) < 0) {
        Py_DECREF(format);
        Py_DECREF(connectionRepr);
        return NULL;
    }
    formatArgs = PyTuple_Pack(3, module, name, connectionRepr);
    Py_DECREF(module);
    Py_DECREF(name);
    Py_DECREF(connectionRepr);
    if (!formatArgs) {
        Py_DECREF(format);
        return NULL;
    }
    result = ceString_Format(format, formatArgs);
    Py_DECREF(format);
    Py_DECREF(formatArgs);
    return result;
}


//-----------------------------------------------------------------------------
// Cursor_Free()
//   Deallocate the cursor.
//-----------------------------------------------------------------------------
static void Cursor_Free(
    udt_Cursor *self)                   // cursor object
{
    if (self->handle && self->connection->isConnected)
        SQLFreeHandle(self->handleType, self->handle);
    Py_CLEAR(self->connection);
    Py_CLEAR(self->resultSetVars);
    Py_CLEAR(self->parameterVars);
    Py_CLEAR(self->statement);
    Py_CLEAR(self->rowFactory);
    Py_TYPE(self)->tp_free((PyObject*) self);
}


//-----------------------------------------------------------------------------
// Cursor_MassageArgs()
//   Massage the arguments. If one argument is passed and that argument is a
// list or tuple, use that value instead of the arguments given.
//-----------------------------------------------------------------------------
static int Cursor_MassageArgs(
    PyObject **args,                    // incoming args
    int *argsOffset)                    // offset into the arguments (IN/OUT)
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

    // set internal counters
    self->rowCount = 0;
    self->actualRows = -1;
    self->rowNum = 0;

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
    udt_Variable **newVar,		// new variable to be bound
    int deferTypeAssignment)    // defer type assignment if null value?
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
            *newVar = Variable_InternalNew(numElements, origVar->type,
                    origVar->size, origVar->scale);
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
        } else if (value != Py_None || !deferTypeAssignment) {
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
// Cursor_LogBindParameter()
//   Log the bind parameter.
//-----------------------------------------------------------------------------
static void Cursor_LogBindParameter(
    unsigned position,                  // position being bound
    PyObject *value)                    // value being bound
{
    PyObject *format, *formatArgs, *positionObj, *message;

    if (!IsLoggingAtLevelForPython(LOG_LEVEL_DEBUG))
        return;
    positionObj = PyInt_FromLong(position);
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
    format = ceString_FromAscii("    %s => %r");
    if (!format) {
        Py_DECREF(formatArgs);
        LogMessageV(LOG_LEVEL_DEBUG, "    %d => cannot build format",
                position);
        return;
    }
    message = ceString_Format(format, formatArgs);
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
// Cursor_BindParameters()
//   Bind all parameters, creating new ones as needed.
//-----------------------------------------------------------------------------
static int Cursor_BindParameters(
    udt_Cursor *self,			// cursor to perform binds on
    PyObject *parameters,		// parameters to bind
    int parametersOffset,       // offset into parameters
    unsigned numElements,		// number of elements to create
    unsigned arrayPos,			// array position to set
    int deferTypeAssignment)    // defer type assignment if null value?
{
    int i, numParams, origNumParams;
    udt_Variable *newVar, *origVar;
    PyObject *value;

    // set up the list of parameters
    numParams = PyTuple_GET_SIZE(parameters) - parametersOffset;
    if (self->parameterVars) {
        origNumParams = PyList_GET_SIZE(self->parameterVars);
    } else {
        origNumParams = 0;
        self->parameterVars = PyList_New(numParams);
        if (!self->parameterVars)
            return -1;
    }

    // bind parameters
    if (self->logSql)
        LogMessageV(LOG_LEVEL_DEBUG, "BIND VARIABLES (%u)", arrayPos);
    for (i = 0; i < numParams; i++) {
        value = PySequence_GetItem(parameters, i + parametersOffset);
        if (!value)
            return -1;
        if (i < origNumParams) {
            origVar = (udt_Variable*) PyList_GET_ITEM(self->parameterVars, i);
            if ( (PyObject*) origVar == Py_None)
                origVar = NULL;
        } else origVar = NULL;
        if (Cursor_BindParameterHelper(self, numElements, arrayPos, value,
                origVar, &newVar, deferTypeAssignment) < 0) {
            Py_DECREF(value);
            return -1;
        }
        if (self->logSql)
            Cursor_LogBindParameter(i + 1, value);
        Py_DECREF(value);
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
        if (!newVar && origVar && origVar->position < 0) {
            if (Variable_BindParameter(origVar, self, i + 1) < 0)
                return -1;
        }
    }

    return 0;
}


//-----------------------------------------------------------------------------
// Cursor_InternalCatalogHelper()
//   Internal method used by the catalog methods in order to set up the result
// set that is supposed to be returned.
//-----------------------------------------------------------------------------
static PyObject *Cursor_InternalCatalogHelper(
    udt_Cursor *self)                   // cursor to fetch from
{
    if (!Cursor_PrepareResultSet(self) < 0) {
        Py_DECREF(self);
        return NULL;
    }

    return (PyObject*) self;
}


//-----------------------------------------------------------------------------
// Cursor_InternalExecuteHelper()
//   Perform the work of executing a cursor and set the rowcount appropriately
// regardless of whether an error takes place.
//-----------------------------------------------------------------------------
static int Cursor_InternalExecuteHelper(
    udt_Cursor *self,                   // cursor to perform the execute on
    SQLRETURN rc)                       // return code from ODBC routine
{
    // SQL_NO_DATA is returned from statements which do not affect any rows
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
    if (!self->resultSetVars) {
        rc = SQLRowCount(self->handle, &self->rowCount);
        if (CheckForError(self, rc, "Cursor_SetRowCount()") < 0)
            return -1;
    }

    // reset input and output sizes
    self->setInputSizes = 0;
    self->setOutputSize = 0;
    self->setOutputSizeColumn = 0;

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

    Py_BEGIN_ALLOW_THREADS
    rc = SQLExecute(self->handle);
    Py_END_ALLOW_THREADS
    return Cursor_InternalExecuteHelper(self, rc);
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
    SQLULEN precision, size, displaySize;
    udt_VariableType *varType;
    CEODBC_CHAR name[256];
    PyObject *tuple;
    SQLRETURN rc;
    int i;

    // retrieve information about the column
    rc = SQLDescribeCol(self->handle, position, name, sizeof(name),
            &nameLength, &dataType, &precision, &scale, &nullable);
    if (CheckForError(self, rc,
            "Cursor_ItemDescription(): get column info") < 0)
        return NULL;

    // determine variable type
    varType = Variable_TypeBySqlDataType(self, dataType);
    if (!varType)
        return NULL;

    // create the tuple and populate it
    tuple = PyTuple_New(7);
    if (!tuple)
        return NULL;

    // reset precision and scale for all but numbers
    size = precision;
    if (varType != &vt_BigInteger &&
            varType != &vt_Bit &&
            varType != &vt_Integer &&
            varType != &vt_Double &&
            varType != &vt_Decimal) {
        precision = 0;
        scale = 0;
    }

    // set display size based on data type
    displaySize = size;
    if (varType == &vt_BigInteger ||
            varType == &vt_Integer)
        displaySize = size + 1;
    else if (varType == &vt_Double ||
            varType == &vt_Decimal) {
        displaySize = size + 1;
        if (scale > 0)
            displaySize++;
    }

    // set each of the items in the tuple
    Py_INCREF(varType->pythonType);
    PyTuple_SET_ITEM(tuple, 0,
            ceString_FromStringAndSize( (char*) name, nameLength));
    PyTuple_SET_ITEM(tuple, 1, (PyObject*) varType->pythonType);
    PyTuple_SET_ITEM(tuple, 2, PyInt_FromLong(displaySize));
    PyTuple_SET_ITEM(tuple, 3, PyInt_FromLong(size));
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
// Cursor_GetName()
//   Return the name associated with the cursor.
//-----------------------------------------------------------------------------
static PyObject *Cursor_GetName(
    udt_Cursor *self,                   // cursor object
    void *arg)                          // optional argument (ignored)
{
    SQLSMALLINT nameLength;
    char name[255];
    SQLRETURN rc;

    rc = SQLGetCursorName(self->handle, (SQLCHAR*) name, sizeof(name),
            &nameLength);
    if (CheckForError(self, rc, "Cursor_GetName()") < 0)
        return NULL;
    return ceString_FromStringAndSize(name, nameLength);
}


//-----------------------------------------------------------------------------
// Cursor_SetName()
//   Set the name associated with the cursor.
//-----------------------------------------------------------------------------
static int Cursor_SetName(
    udt_Cursor *self,                   // cursor object
    PyObject *value,                    // value to set
    void *arg)                          // optional argument (ignored)
{
    udt_StringBuffer buffer;
    SQLRETURN rc;

    if (StringBuffer_FromString(&buffer, value,
                "cursor name must be a string") < 0)
        return -1;
    rc = SQLSetCursorName(self->handle, (CEODBC_CHAR*) buffer.ptr,
            buffer.size);
    StringBuffer_Clear(&buffer);
    if (CheckForError(self, rc, "Cursor_SetName()") < 0)
        return -1;

    return 0;
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
    rc = SQLFreeHandle(self->handleType, self->handle);
    if (CheckForError(self, rc, "Cursor_Close()") < 0)
        return NULL;
    self->handle = NULL;

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// Cursor_NextSet()
//   Return the next result set for the cursor.
//-----------------------------------------------------------------------------
static PyObject *Cursor_NextSet(
    udt_Cursor *self,                   // cursor to close
    PyObject *args)                     // arguments
{
    SQLRETURN rc;

    // make sure cursor is actually open
    if (Cursor_IsOpen(self) < 0)
        return NULL;

    // get the next result set
    rc = SQLMoreResults(self->handle);
    if (rc == SQL_NO_DATA) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    if (CheckForError(self, rc, "Cursor_NextSet()") < 0)
        return NULL;

    // set up result set
    Py_CLEAR(self->resultSetVars);
    if (Cursor_PrepareResultSet(self) < 0)
        return NULL;

    Py_INCREF(self);
    return (PyObject*) self;
}


//-----------------------------------------------------------------------------
// Cursor_CreateRow()
//   Create an object for the row. The object created is a tuple unless a row
// factory function has been defined in which case it is the result of the
// row factory function called with the argument tuple that would otherwise be
// returned.
//-----------------------------------------------------------------------------
static PyObject *Cursor_CreateRow(
    udt_Cursor *self)                   // cursor object
{
    PyObject *tuple, *item, *result;
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

    // increment row counters
    self->rowNum++;
    self->rowCount++;

    // if a row factory is defined, call it
    if (self->rowFactory && self->rowFactory != Py_None) {
        result = PyObject_CallObject(self->rowFactory, tuple);
        Py_DECREF(tuple);
        return result;
    }

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
    PyObject *format, *formatArgs, *message;
    udt_StringBuffer buffer;
    SQLRETURN rc;

    // make sure we don't get a situation where nothing is to be executed
    if (statement == Py_None && !self->statement) {
        PyErr_SetString(g_ProgrammingErrorException,
                "no statement specified and no prior statement prepared");
        return -1;
    }

    // close original statement if necessary in order to discard results
    if (self->statement)
        SQLCloseCursor(self->handle);

    // keep track of statement
    if (statement != Py_None && statement != self->statement) {
        Py_INCREF(statement);
        Py_XDECREF(self->statement);
        self->statement = statement;
    }

    // log the statement, if applicable
    if (self->logSql) {
        format = ceString_FromAscii("SQL\n%s");
        if (!format)
            return -1;
        formatArgs = PyTuple_Pack(1, statement);
        if (!formatArgs) {
            Py_DECREF(format);
            return -1;
        }
        message = ceString_Format(format, formatArgs);
        Py_DECREF(format);
        Py_DECREF(formatArgs);
        if (!message)
            return -1;
        WriteMessageForPython(LOG_LEVEL_DEBUG, message);
    }

    // clear previous result set parameters
    Py_XDECREF(self->resultSetVars);
    self->resultSetVars = NULL;
    Py_XDECREF(self->rowFactory);
    self->rowFactory = NULL;

    // prepare statement
    if (StringBuffer_FromString(&buffer, self->statement,
            "statement must be a string or None") < 0)
        return -1;
    Py_BEGIN_ALLOW_THREADS
    rc = SQLPrepare(self->handle, (CEODBC_CHAR*) buffer.ptr, buffer.size);
    Py_END_ALLOW_THREADS
    StringBuffer_Clear(&buffer);
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

    if (!PyArg_ParseTuple(args, "O", &statement))
        return NULL;
    if (Cursor_IsOpen(self) < 0)
        return NULL;
    if (Cursor_InternalPrepare(self, statement) < 0)
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}


//-----------------------------------------------------------------------------
// Cursor_ExecDirect()
//   Execute the statement.
//-----------------------------------------------------------------------------
static PyObject *Cursor_ExecDirect(
    udt_Cursor *self,                   // cursor to execute
    PyObject *args)                     // arguments
{
    PyObject *statement, *format, *formatArgs, *message;
    udt_StringBuffer buffer;
    SQLRETURN rc;

    // parse arguments
    if (!PyArg_ParseTuple(args, "O", &statement))
        return NULL;
    if (Cursor_IsOpen(self) < 0)
        return NULL;

    // log the statement, if applicable
    if (self->logSql) {
        format = ceString_FromAscii("SQL\n%s");
        if (!format)
            return NULL;
        formatArgs = PyTuple_Pack(1, statement);
        if (!formatArgs) {
            Py_DECREF(format);
            return NULL;
        }
        message = ceString_Format(format, formatArgs);
        Py_DECREF(format);
        Py_DECREF(formatArgs);
        if (!message)
            return NULL;
        WriteMessageForPython(LOG_LEVEL_DEBUG, message);
    }

    // clear previous statement, if applicable
    if (self->statement)
        SQLCloseCursor(self->handle);
    Py_XDECREF(self->statement);
    self->statement = NULL;
    Py_XDECREF(self->resultSetVars);
    self->resultSetVars = NULL;
    Py_XDECREF(self->rowFactory);
    self->rowFactory = NULL;
    Py_XDECREF(self->parameterVars);
    self->parameterVars = NULL;

    // execute the statement
    if (StringBuffer_FromString(&buffer, statement,
            "statement must be a string") < 0)
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    rc = SQLExecDirect(self->handle, (SQLCHAR*) buffer.ptr, buffer.size);
    Py_END_ALLOW_THREADS
    StringBuffer_Clear(&buffer);
    if (Cursor_InternalExecuteHelper(self, rc) < 0)
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
// Cursor_Execute()
//   Execute the statement.
//-----------------------------------------------------------------------------
static PyObject *Cursor_Execute(
    udt_Cursor *self,                   // cursor to execute
    PyObject *args)                     // arguments
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
    if (Cursor_IsOpen(self) < 0)
        return NULL;
    if (Cursor_InternalPrepare(self, statement) < 0)
        return NULL;

    // bind the parameters
    argsOffset = 1;
    if (Cursor_MassageArgs(&args, &argsOffset) < 0)
        return NULL;
    if (Cursor_BindParameters(self, args, argsOffset, 1, 0, 0) < 0) {
        Py_DECREF(args);
        return NULL;
    }
    Py_DECREF(args);

    // actually execute the statement
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
        if (Cursor_BindParameters(self, arguments, 0, numRows, i,
                (i < numRows - 1)) < 0)
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
// Cursor_CallBuildStatement()
//   Call procedure or function.
//-----------------------------------------------------------------------------
static int Cursor_CallBuildStatement(
    PyObject *name,                     // name of procedure/function to call
    udt_Variable *returnValue,          // return value variable (optional)
    PyObject *args,                     // arguments to procedure/function
    PyObject **statementObj)            // statement object (OUT)
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
    format = ceString_FromAscii(statement);
    PyMem_Free(statement);
    if (!format)
        return -1;
    formatArgs = PyTuple_Pack(1, name);
    if (!formatArgs) {
        Py_DECREF(format);
        return -1;
    }
    *statementObj = ceString_Format(format, formatArgs);
    Py_DECREF(format);
    Py_DECREF(formatArgs);
    if (!*statementObj)
        return -1;

    return 0;
}


//-----------------------------------------------------------------------------
// Cursor_Call()
//   Call procedure or function.
//-----------------------------------------------------------------------------
static int Cursor_Call(
    udt_Cursor *self,                   // cursor to call procedure/function on
    udt_Variable *returnValueVar,       // return value (optional)
    PyObject *procedureName,            // name of procedure/function
    PyObject *args,                     // method args
    int argsOffset)                     // offset into method args
{
    PyObject *statement, *temp;

    // determine the arguments to the procedure
    if (Cursor_MassageArgs(&args, &argsOffset) < 0)
        return -1;
    if (argsOffset > 0) {
        temp = PyTuple_GetSlice(args, argsOffset, PyTuple_GET_SIZE(args));
        Py_DECREF(args);
        if (!temp)
            return -1;
        args = temp;
    }

    // build up the statement
    if (Cursor_CallBuildStatement(procedureName, returnValueVar, args,
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
    temp = PyObject_CallMethod( (PyObject*) self, "execute", "OO", statement,
            args);
    Py_DECREF(args);
    if (!temp)
        return -1;
    Py_DECREF(temp);

    return 0;
}


//-----------------------------------------------------------------------------
// Cursor_CallFunc()
//   Call function, returning the returned value.
//-----------------------------------------------------------------------------
static PyObject *Cursor_CallFunc(
    udt_Cursor *self,                   // cursor to call procedure on
    PyObject *args)                     // arguments
{
    PyObject *functionName, *returnType, *results;
    udt_Variable *returnValueVar;
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
    if (!ceString_Check(functionName)) {
        PyErr_SetString(PyExc_TypeError, "expecting a string");
        return NULL;
    }

    // create the return value variable
    returnValueVar = Variable_NewByType(self, returnType, 1);
    if (!returnValueVar)
        return NULL;
    returnValueVar->input = 0;
    returnValueVar->output = 1;

    // call the function
    if (Cursor_Call(self, returnValueVar, functionName, args, 2) < 0)
        return NULL;

    // create the return value
    results = Variable_GetValue(returnValueVar, 0);
    Py_DECREF(returnValueVar);
    return results;
}


//-----------------------------------------------------------------------------
// Cursor_CallProc()
//   Call procedure, return (possibly) modified set of values.
//-----------------------------------------------------------------------------
static PyObject *Cursor_CallProc(
    udt_Cursor *self,                   // cursor to call procedure on
    PyObject *args)                     // arguments
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
    if (!ceString_Check(procedureName)) {
        PyErr_SetString(PyExc_TypeError, "expecting a string");
        return NULL;
    }

    // call the procedure
    if (Cursor_Call(self, NULL, procedureName, args, 1) < 0)
        return NULL;

    // create the return value
    numArgs = PyList_GET_SIZE(self->parameterVars);
    results = PyList_New(numArgs);
    if (!results)
        return NULL;
    for (i = 0; i < numArgs; i++) {
        var = PyList_GET_ITEM(self->parameterVars, i);
        temp = Variable_GetValue((udt_Variable*) var, 0);
        if (!temp) {
            Py_DECREF(results);
            return NULL;
        }
        PyList_SET_ITEM(results, i, temp);
    }

    return results;
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
    if (rc == SQL_NO_DATA)
        self->actualRows = 0;
    else if (CheckForError(self, rc, "Cursor_InternalFetch(): fetch") < 0)
        return -1;
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
    PyObject *results, *row;
    int rowNum, rc;

    // create an empty list
    results = PyList_New(0);
    if (!results)
        return NULL;

    // fetch as many rows as possible
    for (rowNum = 0; rowLimit == 0 || rowNum < rowLimit; rowNum++) {
        rc = Cursor_MoreRows(self);
        if (rc < 0) {
            Py_DECREF(results);
            return NULL;
        } else if (rc == 0) {
            break;
        } else {
            row = Cursor_CreateRow(self);
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
        return Cursor_CreateRow(self);

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
    int numArgs, i, argsOffset;

    // make sure the cursor is open
    if (Cursor_IsOpen(self) < 0)
        return NULL;

    // massage the arguments
    argsOffset = 0;
    if (Cursor_MassageArgs(&args, &argsOffset) < 0)
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
            value = (PyObject*) Variable_NewByType(self, inputValue,
                    self->bindArraySize);
            if (!value) {
                Py_DECREF(parameterVars);
                Py_DECREF(args);
                return NULL;
            }
        }
        PyList_SET_ITEM(parameterVars, i, value);
    }

    // overwrite existing parameter vars, if any
    Py_XDECREF(self->parameterVars);
    self->parameterVars = parameterVars;
    self->setInputSizes = 1;

    Py_DECREF(args);
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
// Cursor_Var()
//   Create a bind variable and return it.
//-----------------------------------------------------------------------------
static PyObject *Cursor_Var(
    udt_Cursor *self,                   // cursor to fetch from
    PyObject *args,                     // arguments
    PyObject *keywordArgs)              // keyword arguments
{
    static char *keywordList[] = { "type", "size", "scale", "arraysize",
            "inconverter", "outconverter", "input", "output", NULL };
    int size, arraySize, scale, input, output;
    PyObject *inConverter, *outConverter;
    udt_VariableType *varType;
    udt_Variable *var;
    PyObject *type;

    // parse arguments
    size = 0;
    input = 1;
    output = 0;
    arraySize = self->bindArraySize;
    inConverter = outConverter = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, keywordArgs, "O|iiiOOii",
            keywordList, &type, &size, &scale, &arraySize, &inConverter,
            &outConverter, &input, &output))
        return NULL;

    // determine the type of variable
    varType = Variable_TypeByPythonType(type);
    if (!varType)
        return NULL;

    // create the variable
    var = Variable_InternalNew(arraySize, varType, size, scale);
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
        return Cursor_CreateRow(self);

    // no more rows, return NULL without setting an exception
    return NULL;
}

