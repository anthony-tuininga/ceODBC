Release notes
=============

3.x releases
############

Version 3.0
-----------

#)  Dropped support for Python 2. Support is now for Python 3.6 and higher.
#)  Migrated module to a Python package with the use of Cython for speedups.
#)  Migrated test suite to using tox in order to automate testing of different
    environments.
#)  Added input and output type handlers on cursors and connections. This
    enables the default types to be overridden if desired. See the
    documentation for more details.
#)  Added better support for 64-bit Python.
#)  Eliminated compiler warnings; other minor tweaks to improve error handling.
#)  Dropped attribute `ceODBC.buildtime`.
#)  Dropped use of cx_Logging for logging.


Older versions
##############

Version 2.0.1
-------------

#)  Removed memory leak that occurred when binding parameters to a cursor;
    thanks to Robert Ritchie and Don Reid for discovering this.
#)  Remove the password from the DSN in order to eliminate potential security
    leaks.
#)  Improve performance when logging is disabled or not at level DEBUG by
    avoiding the entire attempt to log bind variable values.
#)  Use the size value rather than the length value when defining result set
    variables since the length value is for the length of the column name;
    thanks to Heran Quan for the patch.
#)  Added support for Python 3.2.


Version 2.0
-----------

#)  Added support for Python 3.x and Unicode.
#)  Added support for 64-bit Python installations.
#)  Added test suites for MySQL, PostgreSQL and SQL Server.
#)  Added support for cursor nextset().
#)  Added support for cursor execdirect() which calls SQLExecDirect() instead
    of SQLExecute() which can be necessary in order to work around bugs in
    various ODBC drivers.
#)  Added support for creating variables and for specifying input and output
    converters as in cx_Oracle.
#)  Added support for deferred type assignment for cursor executemany() as in
    cx_Oracle.
#)  Fixed a number of bugs found by testing against various ODBC drivers.


Version 1.2
-----------

#)  Added support for time data as requested by Dmitry Solitsky.
#)  Added support for Python 2.4 as requested by Lukasz Szybalski.
#)  Added support for setting the autocommit flag in the connection constructor
    since some drivers do not support transactions and raise a "driver not
    capable" exception if any attempt is made to turn autocommit off; thanks to
    Carl Karsten for working with me to resolve this problem.
#)  Added support for calculating the size and display size of columns in the
    description attribute of cursors as requested by Carl Karsten.
#)  Use SQLFreeHandle() rather than SQLCloseCursor() since closing a cursor in
    the ODBC sense is not the same as closing a cursor in the DB API sense and
    caused strange exceptions to occur if no query was executed before calling
    cursor.close().
#)  Added additional documentation to README.txt as requested by Lukasz
    Szybalski.
#)  Tweaked setup script and associated configuration files to make it easier
    to build and distribute; better support for building with cx_Logging if
    desired.


Version 1.1
-----------

#)  Added support for determining the columns, column privileges, foreign keys,
    primary keys, procedures, procedure columns, tables and table privileges
    available in the catalog as requested by Dmitry Selitsky.
#)  Added support for getting/setting the autocommit flag for connections.
#)  Added support for getting/setting the cursor name which is useful for
    performing positioned updates and deletes (as in delete from X where
    current of cursorname).
#)  Explicitly set end of rows when SQL_NO_DATA is returned from SQLFetch() as
    some drivers do not properly set the number of rows fetched.
