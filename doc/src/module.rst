.. module:: ceODBC

.. _module:

****************
Module Interface
****************

.. function:: Binary(string)

   Construct an object holding a binary (long) string value.


.. function:: Connection(dsn, [autocommit = False])
              connect(dsn, [autocommit = False])

   Constructor for creating a connection to the database. Return a Connection
   object (\ref{connobj}). The only required argument is the DSN in the format
   that ODBC expects. The autocommit flag can be set in the constructor or it
   can be manipulated after the connection has been established. If you are
   using a driver that does not handle transactions, ensure that this value is
   set to true or you may get a "driver not capable" exception.


.. function:: Cursor(connection)

   Constructor for creating a cursor.  Return a new Cursor object
   (:ref:`cursorobj`) using the connection.

   .. note::

      This method is an extension to the DB API definition.


.. function:: Date(year, month, day)

   Construct an object holding a date value.


.. function:: DateFromTicks(ticks)

   Construct an object holding a date value from the given ticks value (number
   of seconds since the epoch; see the documentation of the standard Python
   time module for details).


.. function:: Time(hour, minute, second)

   Construct an object holding a time value.


.. function:: TimeFromTicks(ticks)

   Construct an object holding a time value from the given ticks value (number
   of seconds since the epoch; see the documentation of the standard Python
   time module for details).


.. function:: Timestamp(year, month, day, hour, minute, second)

   Construct an object holding a time stamp value.


.. function:: TimestampFromTicks(ticks)

   Construct an object holding a time stamp value from the given ticks value
   (number of seconds since the epoch; see the documentation of the standard
   Python time module for details).



.. _constants:

Constants
=========

.. data:: apilevel

   String constant stating the supported DB API level. Currently '2.0'.


.. data:: buildtime

   String constant stating the time when the binary was built.

   .. note::

      This attribute is an extension to the DB API definition.


.. data:: BINARY

   This type object is used to describe columns in a database that are binary.


.. data:: DATETIME

   This type object is used to describe columns in a database that are dates.


.. data:: NUMBER

   This type object is used to describe columns in a database that are numbers.


.. data:: paramstyle

   String constant stating the type of parameter marker formatting expected by
   the interface. Currently 'qmark' as in 'where name = ?'.


.. data:: ROWID

   This type object is used to describe the pseudo column "rowid".


.. data:: STRING

   This type object is used to describe columns in a database that are strings.


.. data:: threadsafety

   Integer constant stating the level of thread safety that the interface
   supports.  Currently 2, which means that threads may share the module and
   connections, but not cursors. Sharing means that a thread may use a
   resource without wrapping it using a mutex semaphore to implement resource
   locking.


.. data:: version

   String constant stating the version of the module. Currently '|release|'.

   .. note::

      This attribute is an extension to the DB API definition.



.. _exceptions:

Exceptions
==========

.. exception:: Warning

   Exception raised for important warnings and defined by the DB API but not
   actually used by ceODBC.


.. exception:: Error

   Exception that is the base class of all other exceptions defined by
   ceODBC and is a subclass of the Python StandardError exception (defined in
   the module exceptions).


.. exception:: InterfaceError

   Exception raised for errors that are related to the database interface
   rather than the database itself. It is a subclass of Error.


.. exception:: DatabaseError

   Exception raised for errors that are related to the database. It is a
   subclass of Error.


.. exception:: DataError

   Exception raised for errors that are due to problems with the processed
   data. It is a subclass of DatabaseError.


.. exception:: OperationalError

   Exception raised for errors that are related to the operation of the
   database but are not necessarily under the control of the progammer. It is a
   subclass of DatabaseError.


.. exception:: IntegrityError

   Exception raised when the relational integrity of the database is affected.
   It is a subclass of DatabaseError.


.. exception:: InternalError

   Exception raised when the database encounters an internal error. It is a
   subclass of DatabaseError.


.. exception:: ProgrammingError

   Exception raised for programming errors. It is a subclass of DatabaseError.


.. exception:: NotSupportedError

   Exception raised when a method or database API was used which is not
   supported by the database. It is a subclass of DatabaseError.



.. _vartypes:

Variable Types
==============

.. note::

   The DB API definition does not define these objects.

   These classes all create variable objects (:ref:`varobj`). They are created
   implicitly by cursor.execute() as needed and normally need not be created
   directly. These classes can also be passed in to cursor.setinputsizes() in
   favor of the types defined by the DB API in order to have finer control over
   the types of variables created.


.. data:: BigIntegerVar

   Variable used to bind and/or fetch big integers. Values are returned as
   Python longs and accept Python integers or longs.


.. data:: BinaryVar

   Variable used to bind and/or fetch binary data. Values are returned as
   Python buffer objects and accept Python objects that implement the buffer
   protocol.


.. data:: BitVar

   Variable used to bind and/or fetch bits. Values are returned as Python
   booleans and accept the same.


.. data:: DateVar

   Variable used to bind and/or fetch dates. Values are returned as Python
   datetime.date objects and accept Python datetime.date or datetime.datetime
   objects.


.. data:: DecimalVar

   Variable used to bind and/or fetch decimal numbers. Values are returned as
   Python decimal.Decimal objects and accept the same.


.. data:: DoubleVar

   Variable used to bind and/or fetch floating point numbers. Values are
   returned as Python floats and accept Python integers or floats.


.. data:: IntegerVar

   Variable used to bind and/or fetch integers. Values are returned as Python
   integers and accept the same.


.. data:: LongBinaryVar

   Variable used to bind and/or fetch long binary data. Values are returned as
   Python buffer objects and accept Python objects that implement the buffer
   protocol.


.. data:: LongStringVar

   Variable used to bind and/or fetch long string data. Values are returned as
   Python strings and accept the same.


.. data:: StringVar

   Variable used to bind and/or fetch string data. Values are returned as
   Python strings and accept the same.


.. data:: TimeVar

   Variable used to bind and/or fetch time data. Values are returned as Python
   datetime.time objects and accept Python datetime.time or datetime.datetime
   objects.


.. data:: TimestampVar

   Variable used to bind and/or fetch timestamps. Values are returned as Python
   datetime.datetime objects and accept Python datetime.date or
   datetime.datetime objects.

