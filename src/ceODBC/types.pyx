#------------------------------------------------------------------------------
# connection.pyx
#   Cython file defining the API and database type classes (embedded in
# driver.pyx).
#------------------------------------------------------------------------------

cdef class ApiType:
    cdef:
        readonly str name
        readonly tuple dbtypes

    def __init__(self, name, *dbtypes):
        self.name = name
        self.dbtypes = dbtypes

    def __eq__(self, other):
        if isinstance(other, DbType):
            return other in self.dbtypes
        return NotImplemented

    def __hash__(self):
        return hash(self.name)

    def __reduce__(self):
        return self.name

    def __repr__(self):
        return f"<ApiType {self.name}>"


cdef class DbType:
    cdef:
        readonly str name
        SQLSMALLINT _sql_data_type
        SQLSMALLINT _c_data_type
        SQLUINTEGER _buffer_size
        SQLUINTEGER _bytes_multiplier

    def __init__(self, name, sql_data_type, c_data_type, buffer_size,
                 bytes_multiplier=0):
        self.name = name
        self._sql_data_type = sql_data_type
        self._c_data_type = c_data_type
        self._buffer_size = buffer_size
        self._bytes_multiplier = bytes_multiplier

    def __reduce__(self):
        return self.name

    def __repr__(self):
        return f"<DbType {self.name}>"

    @staticmethod
    cdef DbType _from_sql_data_type(SQLSMALLINT data_type):
        if data_type == SQL_VARCHAR \
                or data_type == SQL_WVARCHAR \
                or data_type == SQL_LONGVARCHAR \
                or data_type == SQL_WLONGVARCHAR:
            return DB_TYPE_STRING
        elif data_type == SQL_BIGINT:
            return DB_TYPE_BIGINT
        elif data_type == SQL_INTEGER \
                or data_type == SQL_SMALLINT \
                or data_type == SQL_TINYINT:
            return DB_TYPE_INT
        elif data_type == SQL_FLOAT \
                or data_type == SQL_DOUBLE \
                or data_type == SQL_REAL:
            return DB_TYPE_DOUBLE
        elif data_type == SQL_TYPE_DATE:
            return DB_TYPE_DATE
        elif data_type == SQL_NUMERIC or data_type == SQL_DECIMAL:
            return DB_TYPE_DECIMAL
        elif data_type == SQL_TYPE_TIMESTAMP:
            return DB_TYPE_TIMESTAMP
        _raise_from_string(exceptions.NotSupportedError,
                           f"SQL data type {data_type} not supported")

    @staticmethod
    cdef DbType _from_type(object typ):
        cdef ApiType apitype
        if isinstance(typ, DbType):
            return typ
        elif isinstance(typ, ApiType):
            apitype = typ
            return apitype.dbtypes[0]
        elif not isinstance(typ, type):
            raise TypeError("expecting type")
        elif typ is int:
            return DB_TYPE_INT
        elif typ is float:
            return DB_TYPE_DOUBLE
        elif typ is str:
            return DB_TYPE_STRING
        elif typ is bytes:
            return DB_TYPE_BINARY
        elif typ is decimal.Decimal:
            return DB_TYPE_DECIMAL
        elif typ is bool:
            return DB_TYPE_BIT
        elif typ is datetime.date:
            return DB_TYPE_DATE
        elif typ is datetime.datetime:
            return DB_TYPE_TIMESTAMP
        else:
            _raise_from_string(exceptions.NotSupportedError,
                               f"Python type {typ} not supported")

    @staticmethod
    cdef DbType _from_value(object value, SQLUINTEGER *size):
        size[0] = 0
        if value is None:
            size[0] = 1
            return DB_TYPE_STRING
        elif isinstance(value, str):
            size[0] = len(value)
            return DB_TYPE_STRING
        elif isinstance(value, bytes):
            size[0] = len(value)
            return DB_TYPE_BINARY
        elif isinstance(value, bool):
            return DB_TYPE_BIT
        elif isinstance(value, int):
            return DB_TYPE_BIGINT
        elif isinstance(value, decimal.Decimal):
            return DB_TYPE_DECIMAL
        elif isinstance(value, float):
            return DB_TYPE_DOUBLE
        elif isinstance(value, datetime.time):
            return DB_TYPE_TIME
        elif isinstance(value, datetime.datetime):
            return DB_TYPE_TIMESTAMP
        elif isinstance(value, datetime.date):
            return DB_TYPE_DATE
        message = f"Python value of type {type(value)} not supported"
        _raise_from_string(exceptions.NotSupportedError, message)


# database types
DB_TYPE_BIGINT = DbType("DB_TYPE_BIGINT", SQL_BIGINT, SQL_C_SBIGINT,
                        sizeof(SQLBIGINT))
DB_TYPE_BINARY = DbType("DB_TYPE_BINARY", SQL_VARBINARY, SQL_C_BINARY, 0, 1)
DB_TYPE_BIT = DbType("DB_TYPE_BIT", SQL_BIT, SQL_C_BIT, sizeof(unsigned char))
DB_TYPE_DATE = DbType("DB_TYPE_DATE", SQL_TYPE_DATE, SQL_C_TYPE_DATE,
                      sizeof(DATE_STRUCT))
DB_TYPE_DECIMAL = DbType("DB_TYPE_DECIMAL", SQL_CHAR, SQL_C_CHAR, 40)
DB_TYPE_DOUBLE = DbType("DB_TYPE_DOUBLE", SQL_DOUBLE, SQL_C_DOUBLE,
                        sizeof(double))
DB_TYPE_INT = DbType("DB_TYPE_INT", SQL_INTEGER, SQL_C_LONG,
                     sizeof(SQLINTEGER))
DB_TYPE_LONG_BINARY = DbType("DB_TYPE_LONG_BINARY", SQL_LONGVARBINARY,
                             SQL_C_BINARY, 0, 1)
DB_TYPE_LONG_STRING = DbType("DB_TYPE_LONG_STRING", SQL_LONGVARCHAR,
                             SQL_C_CHAR, 0, 4)
DB_TYPE_STRING = DbType("DB_TYPE_STRING", SQL_VARCHAR, SQL_C_CHAR, 0, 4)
DB_TYPE_TIME = DbType("DB_TYPE_TIME", SQL_TYPE_TIME, SQL_C_TYPE_TIME,
                      sizeof(TIME_STRUCT))
DB_TYPE_TIMESTAMP = DbType("DB_TYPE_TIMESTAMP", SQL_TYPE_TIMESTAMP,
                           SQL_C_TYPE_TIMESTAMP, sizeof(TIMESTAMP_STRUCT))

# DB API types
BINARY = ApiType("BINARY", DB_TYPE_BINARY, DB_TYPE_LONG_BINARY)
DATETIME = ApiType("DATETIME", DB_TYPE_DATE, DB_TYPE_TIMESTAMP)
NUMBER = ApiType("NUMBER", DB_TYPE_BIGINT, DB_TYPE_DECIMAL,
                 DB_TYPE_DOUBLE, DB_TYPE_INT)
ROWID = ApiType("ROWID")
STRING = ApiType("STRING", DB_TYPE_STRING, DB_TYPE_LONG_STRING)
