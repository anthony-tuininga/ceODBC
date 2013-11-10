.. _cursorobj:

*************
Cursor Object
*************

.. attribute:: Cursor.arraysize

   This read-write attribute specifies the number of rows to fetch at a time
   internally and is the default number of rows to fetch with the
   :meth:`~Cursor.fetchmany()` call.  It defaults to 1 meaning to fetch a
   single row at a time. Note that this attribute can drastically affect the
   performance of a query since it directly affects the number of network round
   trips that need to be performed.


.. attribute:: Cursor.bindarraysize

   This read-write attribute specifies the number of rows to bind at a time and
   is used when creating variables via :meth:`~Cursor.setinputsizes()`. It
   defaults to 1 meaning to bind a single row at a time.

   .. note::

      The DB API definition does not define this attribute.


.. method:: Cursor.callfunc(name, returnType, \*args)

   Call a function with the given name. Parameters may also be passed as a
   single list or tuple to conform to the DB API. The return type is specified
   in the same notation as is required by :meth:`~Cursor.setinputsizes()`. The
   result of the call is the return value of the function.

   .. note::

      The DB API definition does not define this method.


.. method:: Cursor.callproc(name, \*args)

   Call a procedure with the given name. Parameters may also be passed as a
   single list or tuple to conform to the DB API. The result of the call is a
   modified copy of the input sequence. Input parameters are left untouched;
   output and input/output parameters are replaced with possibly new values.


.. method:: Cursor.close()

   Close the cursor now, rather than whenever __del__ is called. The cursor
   will be unusable from this point forward; an Error exception will be raised
   if any operation is attempted with the cursor.


.. attribute:: Cursor.connection

   This read-only attribute returns a reference to the connection object on
   which the cursor was created.

   .. note::

      This attribute is an extension to the DB API definition but it is
      mentioned in PEP 249 as an optional extension.


.. data:: Cursor.description

  This read-only attribute is a sequence of 7-item sequences. Each of these
  sequences contains information describing one result column: (name, type,
  display_size, internal_size, precision, scale, null_ok). This attribute will
  be None for operations that do not return rows or if the cursor has not had
  an operation invoked via the execute() method yet.

  The type will be one of the variable type objects (:ref:`vartypes`) and is
  comparable to the type objects defined by the DB API.


.. method:: Cursor.execdirect(statement)

   Execute a statement against the database using SQLExecDirect instead of
   SQLExecute. This is necessary in some situations due to bugs in ODBC drivers
   such as exhibited by the SQL Server ODBC driver when calling certain stored
   procedures.

   If the statement is a query, the cursor is returned as a convenience since
   cursors implement the iterator protocol and there is thus no need to call
   one of the appropriate fetch methods; otherwise None is returned.

   .. note::

      The DB API definition does not define this method.


.. method:: Cursor.execute(statement, \*args)

   Execute a statement against the database. Paramters may also be passed as a
   single list or tuple to conform to the DB API.

   A reference to the statement will be retained by the cursor. If None or the
   same string object is passed in again, the cursor will execute that
   statement again without performing a prepare or rebinding and redefining.
   This is most effective for algorithms where the same statement is used, but
   different parameters are bound to it (many times).

   For maximum efficiency when reusing an statement, it is best to use the
   :meth:`~Cursor.setinputsizes()` method to specify the parameter types and
   sizes ahead of time; in particular, None is assumed to be a string of length
   1 so any values that are later bound as numbers or dates will raise a
   TypeError exception.

   If the statement is a query, the cursor is returned as a convenience since
   cursors implement the iterator protocol and there is thus no need to call
   one of the appropriate fetch methods; otherwise None is returned.

   .. note::

      The DB API definition does not define the return value of this method.


.. method:: Cursor.executemany(statement, parameters)

   Prepare a statement for execution against a database and then execute it
   against all parameter sequences found in the sequence parameters. The
   statement is managed in the same way as the :meth:`~Cursor.execute()` method
   manages it.


.. method:: Cursor.fetchall()

   Fetch all (remaining) rows of a query result, returning them as a list of
   tuples. An empty list is returned if no more rows are available. Note that
   the cursor's arraysize attribute can affect the performance of this
   operation, as internally reads from the database are done in batches
   corresponding to the arraysize.

   An exception is raised if the previous call to execute() did not produce any
   result set or no call was issued yet.


.. method:: Cursor.fetchmany([numRows=cursor.arraysize])

   Fetch the next set of rows of a query result, returning a list of tuples. An
   empty list is returned if no more rows are available. Note that the cursor's
   arraysize attribute can affect the performance of this operation.

   The number of rows to fetch is specified by the parameter. If it is not
   given, the cursor's arrysize attribute determines the number of rows to be
   fetched. If the number of rows available to be fetched is fewer than the
   amount requested, fewer rows will be returned.

   An exception is raised if the previous call to execute() did not produce any
   result set or no call was issued yet.


.. method:: Cursor.fetchone()

   Fetch the next row of a query result set, returning a single tuple or None
   when no more data is available.

   An exception is raised if the previous call to execute() did not produce any
   result set or no call was issued yet.


.. method:: Cursor.__iter__()

   Returns the cursor itself to be used as an iterator.

   .. note::

      This method is an extension to the DB API definition but it is
      mentioned in PEP 249 as an optional extension.


.. data:: Cursor.name

   This read-write attribute returns the name associated with the cursor. This
   name is used in positioned update or delete statements (as in delete from X
   where current of <NAME>).

   .. note::

      This attribute is an extension to the DB API definition.


.. method:: Cursor.next()

   Fetch the next row of a query result set, using the same semantics as the
   method fetchone().

   .. note::

      This method is an extension to the DB API definition but it is
      mentioned in PEP 249 as an optional extension.


.. method:: Cursor.nextset()

   Make the cursor skip to the next available set, discarding any remaining
   row from the current set. If there are no more sets, None is returned;
   otherwise, the cursor ifself is returned as a convenience for fetching data
   from it. Note that not all databases support the concept of multiple result
   sets.


.. method:: Cursor.prepare(statement)

   This can be used before a call to execute() to define the statement that
   will be executed. When this is done, the prepare phase will not be performed
   when the call to execute() is made with None or the same string object as
   the statement.

   .. note::

      The DB API definition does not define this method.


.. attribute:: Cursor.rowcount

   This read-only attribute specifies the number of rows that have currently
   been fetched from the cursor (for select statements) or that have been
   affected by the operation (for insert, update and delete statements).


.. attribute:: Cursor.rowfactory

   This read-write attribute specifies a method to call for each row that is
   retrieved from the database. Ordinarily a tuple is returned for each row but
   if this attribute is set, the method is called with the argument tuple that
   would normally be returned and the result of the method is returned instead.

   .. note::

      The DB API definition does not define this attribute.


.. method:: Cursor.setinputsizes(\*args)

   This can be used before a call to execute() to predefine memory areas for
   the operation's parameters. Each parameter should be a type object
   corresponding to the input that will be used or it should be an integer
   specifying the maximum length of a string parameter. The singleton None can
   be used as a parameter to indicate that no space should be reserved for that
   position.  Note that in order to conform to the DB API, passing a single
   argument which is a list or tuple will treat that list or tuple as the
   arguments sequence.


.. method:: Cursor.setoutputsize(size, [column])

   This can be used before a call to execute() to predefine memory areas for
   the long columns that will be fetched. The column is specified as an index
   into the result sequence. Not specifying the column will set the default
   size for all large columns in the cursor.


.. attribute:: Cursor.statement

   This read-only attribute provides the string object that was previously
   prepared with prepare() or executed with execute().

   .. note::

      The DB API definition does not define this attribute.


.. method:: Cursor.var(type, [size, scale, arraysize, inconverter, outconverter, input = True, output = False])

   Create a variable associated with the cursor of the given type and
   characteristics and return a variable object (:ref:`varobj`). If the
   arraysize is not specified, the bind array size (usually 1) is used. The
   inconverter and outconverter specify methods used for converting values
   to/from the database. More information can be found in the section on
   variable objects.
 
   This method was designed for use with in/out variables where the length or
   type cannot be determined automatically from the Python object passed in or
   for use in input and output type handlers defined on cursors or connections.

   .. note:: 

      The DB API definition does not define this method.

