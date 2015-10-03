
Release notes
=============

2.x releases
############

Development version
-------------------

Version 2.1
-----------
1) Added support for Python 3.4 and 3.5.
2) Added input and output type handlers on cursors and connections. This
   enables the default types to be overridden if desired. See the documentation
   for more details.
3) Added better support for 64-bit Python.
4) Eliminated compiler warnings; other minor tweaks to improve error handling.


Version 2.0.2
-------------

1) Added support for Python 3.3.
2) The document is now available on `ReadTheDocs.org
   <http://ceodbc.readthedocs.org/en/latest/index.html>`_.


Version 2.0.1
-------------

1) Removed memory leak that occurred when binding parameters to a cursor;
   thanks to Robert Ritchie and Don Reid for discovering this.
2) Remove the password from the DSN in order to eliminate potential security
   leaks.
3) Improve performance when logging is disabled or not at level DEBUG by
   avoiding the entire attempt to log bind variable values.
4) Use the size value rather than the length value when defining result set
   variables since the length value is for the length of the column name;
   thanks to Heran Quan for the patch.
5) Added support for Python 3.2.


Version 2.0.1
-------------

1) Added support for Python 3.x and Unicode.
2) Added support for 64-bit Python installations.
3) Added test suites for MySQL, PostgreSQL and SQL Server.
4) Added support for cursor nextset().
5) Added support for cursor execdirect() which calls SQLExecDirect() instead
   of SQLExecute() which can be necessary in order to work around bugs in
   various ODBC drivers.
6) Added support for creating variables and for specifying input and output
   converters as in cx_Oracle.
7) Added support for deferred type assignment for cursor executemany() as in
   cx_Oracle.
8) Fixed a number of bugs found by testing against various ODBC drivers.


Older versions
##############

Version 1.2
-----------

1) Added support for time data as requested by Dmitry Solitsky.
2) Added support for Python 2.4 as requested by Lukasz Szybalski.
3) Added support for setting the autocommit flag in the connection constructor
   since some drivers do not support transactions and raise a "driver not
   capable" exception if any attempt is made to turn autocommit off; thanks to
   Carl Karsten for working with me to resolve this problem.
4) Added support for calculating the size and display size of columns in the
   description attribute of cursors as requested by Carl Karsten.
5) Use SQLFreeHandle() rather than SQLCloseCursor() since closing a cursor in
   the ODBC sense is not the same as closing a cursor in the DB API sense and
   caused strange exceptions to occur if no query was executed before calling
   cursor.close().
6) Added additional documentation to README.txt as requested by Lukasz
   Szybalski.
7) Tweaked setup script and associated configuration files to make it easier
   to build and distribute; better support for building with cx_Logging if
   desired.


Version 1.1
-----------

1) Added support for determining the columns, column privileges, foreign keys,
   primary keys, procedures, procedure columns, tables and table privileges
   available in the catalog as requested by Dmitry Selitsky.
2) Added support for getting/setting the autocommit flag for connections.
3) Added support for getting/setting the cursor name which is useful for
   performing positioned updates and deletes (as in delete from X where
   current of cursorname).
4) Explicitly set end of rows when SQL_NO_DATA is returned from SQLFetch() as
   some drivers do not properly set the number of rows fetched.

