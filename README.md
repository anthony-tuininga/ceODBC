Open Source Python/ODBC Utility - ceODBC
----------------------------------------

ceODBC is a Python extension module that enables access to databases using the
ODBC API and conforms to the Python database API 2.0 specifications with a
number of additions. Python 3.6 and higher is required as of version 3.0.

See https://www.python.org/dev/peps/pep-0249 for more information on the Python
database API specification.

For feedback or patches, please use GitHub issues:
https://github.com/anthony-tuininga/ceODBC/issues


Installation
------------

python -m pip install ceODBC --upgrade

Add the --user option if you do not have system access. Binaries are available
for Linux and Windows as a convenience.


Usage Example
-------------

Note that the `DSN` in the example below should be replaced with an
appropriate string that ODBC understands. For example, if a system DSN called
"MYDATA" has been created in the ODBC manager, use "DSN=MYDATA" to connect.
For what is termed DSN-less connections, search the Internet for appropriate
strings.

```python
import ceODBC

connection = ceODBC.connect(DSN)

cursor = connection.cursor()
cursor.execute("""
        select Col1, Col2, Col3
        from SomeTable
        where Col4 = ?
          and Col5 between ? and ?""",
        ["VALUE", 5, 10])
for column_1, column_2, column_3 in cursor:
    print("Values:", column_1, column_2, column_3)
```


Notes
-----

If your driver is not capable of transactions (often indicated by the exception
"driver not capable" when connecting) then use the following statement to
connect instead:

```python
connection = ceODBC.connect(DSN, autocommit=True)
```

For further information see

http://ceodbc.readthedocs.org
