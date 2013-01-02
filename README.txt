Open Source Python/ODBC Utility - ceODBC

ceODBC is a Python extension module that enables access to databases using the
ODBC API and conforms to the Python database API 2.0 specifications with a few
exceptions.

See http://www.python.org/topics/database/DatabaseAPI-2.0.html for more
information on the Python database API specification.

For comments, contact Anthony Tuininga at anthony.tuininga@gmail.com or use the
mailing list at http://lists.sourceforge.net/lists/listinfo/ceODBC-users


BINARY INSTALL:
Place the file ceODBC.pyd or ceODBC.so anywhere on your Python path.


SOURCE INSTALL:
Use the provided setup.py to build and install the module which makes use of
the distutils module. The commands required to build and install the module are
as follows:

	python setup.py build
	python setup.py install

If you wish to use cx_Logging for logging, run these commands instead:

    python setup.py build_ext --with-cx-logging --cx-logging <SourceDir>
    python setup.py install

The following packages are required on Linux before compilation is possible:

    python-devel
    unixODBC
    unixODBC-devel


USAGE EXAMPLE:

Note that the "<DSN>" in the example below should be replaced with an
appropriate string that ODBC understands. For example, if a system DSN called
"MYDATA" has been created in the ODBC manager, use "DSN=MYDATA" to connect.
For what is termed DSN-less connections, search the Internet for appropriate
strings.

import ceODBC

connection = ceODBC.connect("<DSN>")

cursor = connection.cursor()
cursor.execute("""
        select Col1, Col2, Col3
        from SomeTable
        where Col4 = ?
          and Col5 between ? and ?""",
        ["VALUE", 5, 10])
for column_1, column_2, column_3 in cursor:
    print "Values:", column_1, column_2, column_3


NOTES:

Please see the included documentation for additional information.

If your driver is not capable of transactions (often indicated by the exception
"driver not capable" when connecting) then use the following statement to
connect instead:

connection = ceODBC.connect("<DSN>", autocommit = True)

