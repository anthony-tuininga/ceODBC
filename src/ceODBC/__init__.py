"""
Initialization of package ceODBC.
"""

from .version import __version__ as __version__

from .driver import (
    # DB API type constants
    BINARY as BINARY,
    DATETIME as DATETIME,
    NUMBER as NUMBER,
    ROWID as ROWID,
    STRING as STRING,
    # database type constants
    DB_TYPE_BIGINT as DB_TYPE_BIGINT,
    DB_TYPE_BINARY as DB_TYPE_BINARY,
    DB_TYPE_BIT as DB_TYPE_BIT,
    DB_TYPE_DATE as DB_TYPE_DATE,
    DB_TYPE_DECIMAL as DB_TYPE_DECIMAL,
    DB_TYPE_DOUBLE as DB_TYPE_DOUBLE,
    DB_TYPE_INT as DB_TYPE_INT,
    DB_TYPE_LONG_BINARY as DB_TYPE_LONG_BINARY,
    DB_TYPE_LONG_STRING as DB_TYPE_LONG_STRING,
    DB_TYPE_STRING as DB_TYPE_STRING,
    DB_TYPE_TIME as DB_TYPE_TIME,
    DB_TYPE_TIMESTAMP as DB_TYPE_TIMESTAMP,
    # classes and methods
    Connection as Connection,
    connect as connect,
    data_sources as data_sources,
    drivers as drivers,
)

from .constructors import (
    Binary as Binary,
    Date as Date,
    DateFromTicks as DateFromTicks,
    Time as Time,
    TimeFromTicks as TimeFromTicks,
    Timestamp as Timestamp,
    TimestampFromTicks as TimestampFromTicks,
)

from .exceptions import (
    Error as Error,
    DatabaseError as DatabaseError,
    DataError as DataError,
    IntegrityError as IntegrityError,
    InterfaceError as InterfaceError,
    InternalError as InternalError,
    NotSupportedError as NotSupportedError,
    OperationalError as OperationalError,
    ProgrammingError as ProgrammingError,
)

# general constants required by the DB API
apilevel = "2.0"
threadsafety = 2
paramstyle = "qmark"

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
