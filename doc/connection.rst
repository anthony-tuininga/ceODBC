.. _connobj:

*****************
Connection Object
*****************

.. note::

   Any outstanding changes will be rolled back when the connection object
   is destroyed or closed.


.. method:: Connection.__enter__()

   The entry point for the connection as a context manager, a feature available
   in Python 2.5 and higher. It returns itself.

   .. note::

      This method is an extension to the DB API definition.


.. method:: Connection.__exit__()

   The exit point for the connection as a context manager, a feature available
   in Python 2.5 and higher. In the event of an exception, the transaction is
   rolled back; otherwise, the transaction is committed.

   .. note::

      This method is an extension to the DB API definition.

.. attribute:: Connection.autocommit

   This read-write attribute returns the setting of the autocommit flag for the
   connection. When set, any statements executed are automatically committed
   if successful; otherwise, a commit() or rollback() must be issued for the
   changes to be committed to (or rolled back from) the database.

   .. note::

      This attribute is an extension to the DB API definition.


.. method:: Connection.close()

   Close the connection now, rather than whenever __del__ is called. The
   connection will be unusable from this point forward; an Error exception will
   be raised if any operation is attempted with the connection. The same
   applies to any cursor objects trying to use the connection.


.. method:: Connection.columnprivileges(catalog=None, schema=None, table=None, column=None)

   Return a cursor containing the privileges for columns in the catalog
   filtered by the parameters catalog, schema, table and column as desired. See
   the ODBC API reference for SQLColumnPrivileges() for more information.

   .. note::

      This method is an extension to the DB API definition.


.. method:: Connection.columns(catalog=None, schema=None, table=None, column=None)

   Return a cursor containing the columns in the catalog filtered by the
   parameters catalog, schema, table and column as desired. See the ODBC API
   reference for SQLColumns() for more information.

   .. note::

      This method is an extension to the DB API definition.


.. method:: Connection.commit()

   Commit any pending transactions to the database.


.. method:: Connection.cursor()

   Return a new Cursor object (:ref:`cursorobj`) using the connection.


.. attribute:: Connection.dsn

   This read-only attribute returns the DSN of the database to which a
   connection has been established.

   .. note::

      This attribute is an extension to the DB API definition.


.. attribute:: Connection.inputtypehandler

   This read-write attribute specifies a method called for each value that is
   bound to a statement executed on any cursor associated with this connection,
   unless a different handler is specified for that cursor. The method
   signature is handler(cursor, value, arraysize) and the return value is
   expected to be a variable object or None in which case a default variable
   object will be created. If this attribute is None, the default behavior will
   take place for all values bound to statements.

   note::

   This attribute is an extension to the DB API definition.


.. method:: Connection.foreignkeys(pkcatalog=None, pkschema=None, pktable=None, fkcatalog=None, fkschema=None, fktable=None)

   Return a cursor containing the foreign keys in the catalog filtered by the
   parameters catalog, schema and table for both the primary and foreign key
   table as desired. See the ODBC API reference for SQLForeignKeys() for more
   information.

   .. note::

      This method is an extension to the DB API definition.


.. attribute:: Connection.outputtypehandler

   This read-write attribute specifies a method called for each value that is
   to be fetched from any cursor associated with this connection, unless a
   different handler is specified for that cursor. The method signature is
   handler(cursor, name, defaultType, length, scale) and the return value is
   expected to be a variable object or None in which case a default variable
   object will be created. If this attribute is None, the default behavior will
   take place for all values fetched from cursors.

   .. note::

      This attribute is an extension to the DB API definition.


.. method:: Connection.primarykeys(catalog=None, schema=None, table=None)

   Return a cursor containing the primary key columns in the catalog filtered
   by the parameters catalog, schema and table as desired. See the ODBC API
   reference for SQLPrimaryKeys() for more information.

   .. note::

      This method is an extension to the DB API definition.


.. method:: Connection.procedurecolumns(catalog=None, schema=None, proc=None, column=None)

   Return a cursor containing the columns for procedures in the catalog
   filtered by the parameters catalog, schema, proc and column as desired. See
   the ODBC API reference for SQLProcedureColumns() for more information.

   .. note::

      This method is an extension to the DB API definition.


.. method:: Connection.procedures(catalog=None, schema=None, proc=None)

   Return a cursor containing the procedures in the catalog filtered by the
   parameters catalog, schema and proc as desired. See the ODBC API reference
   for SQLProcedures() for more information.

   .. note::

      This method is an extension to the DB API definition.


.. method:: Connection.rollback()

   Rollback any pending transactions.


.. method:: Connection.tableprivileges(catalog=None, schema=None, table=None)

   Return a cursor containing the privileges for tables in the catalog filtered
   by the parameters catalog, schema and table as desired. See the ODBC API
   reference for SQLTablePrivileges() for more information.

   .. note::

      This method is an extension to the DB API definition.


.. method:: Connection.tables(catalog=None, schema=None, table=None)

   Return a cursor containing the tables in the catalog filtered by the
   parameters catalog, schema and table as desired. See the ODBC API reference
   for SQLTables() for more information.

   .. note::

      This method is an extension to the DB API definition.

