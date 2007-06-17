Open Source Python/ODBC Utility - ceODBC

ceODBC is a Python extension module that allows access to ODBC and 
conforms to the Python database API 2.0 specifications with a few exceptions.

See http://www.python.org/topics/database/DatabaseAPI-2.0.html for more
information on the Python database API specification.

For comments, contact Anthony Tuininga at anthony.tuininga@gmail.com or use the
mailing list at http://lists.sourceforge.net/lists/listinfo/ceODBC-users


BINARY INSTALL:
Place the file ceODBC.pyd or ceODBC.so anywhere on your Python path.


SOURCE INSTALL:
Use the provided setup.py to build and install the module which makes use of
the distutils module. Note that on Windows I have used mingw32
(http://www.mingw.org) and the module will not build with MSVC without
modification. The commands required to build and install the module are as
follows:

	python setup.py build
	python setup.py install


USAGE EXAMPLE:

import ceODBC

connection = ceODBC.connect("DSN")

cursor = connection.cursor()
cursor.execute("""
        select Col1, Col2, Col3
        from SomeTable
        where Col4 = ?
          and Col5 between ? and ?""",
        ["VALUE", 5, 10])
for column_1, column_2, column_3 in cursor:
    print "Values:", column_1, column_2, column_3


EXCEPTIONS:
Please see the included documentation for additional information.

