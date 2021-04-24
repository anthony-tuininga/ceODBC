"""
Constants for ceODBC.
"""

from .version import __version__

# define constants required by the DB API
apilevel = "2.0"
threadsafety = 2
paramstyle = "qmark"

# database type constants from driver
from .driver import DB_TYPE_BIGINT, DB_TYPE_BINARY, DB_TYPE_BIT
from .driver import DB_TYPE_DATE, DB_TYPE_DECIMAL, DB_TYPE_DOUBLE, DB_TYPE_INT
from .driver import DB_TYPE_LONG_BINARY, DB_TYPE_LONG_STRING, DB_TYPE_STRING
from .driver import DB_TYPE_TIME, DB_TYPE_TIMESTAMP

# API type constants from driver
from .driver import BINARY, DATETIME, NUMBER, ROWID, STRING

# backwards compatible aliases (deprecated)
version = __version__
BigIntegerVar = DB_TYPE_BIGINT
BinaryVar = DB_TYPE_BINARY
BitVar = DB_TYPE_BIT
DateVar = DB_TYPE_DATE
DecimalVar = DB_TYPE_DECIMAL
DoubleVar = DB_TYPE_DOUBLE
IntegerVar = DB_TYPE_INT
LongBinaryVar = DB_TYPE_LONG_BINARY
LongStringVar = DB_TYPE_LONG_STRING
StringVar = DB_TYPE_STRING
TimeVar = DB_TYPE_TIME
TimestampVar = DB_TYPE_TIMESTAMP
