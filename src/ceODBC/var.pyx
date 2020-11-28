#------------------------------------------------------------------------------
# var.pyx
#   Cython file defining the variable class (embedded in driver.pyx).
#------------------------------------------------------------------------------

ctypedef union VarData:
    void *as_raw
    long long int *as_bigint
    SQLCHAR *as_bytes
    unsigned char *as_bit
    DATE_STRUCT *as_date
    double *as_double
    SQLINTEGER *as_int
    TIME_STRUCT *as_time
    TIMESTAMP_STRUCT *as_timestamp


cdef class Var:
    cdef:
        readonly DbType type
        readonly unsigned num_elements
        readonly SQLUINTEGER size
        readonly SQLLEN buffer_size
        readonly SQLSMALLINT scale
        readonly str name
        readonly bint nulls_allowed
        public bint input
        public bint output
        public object inconverter
        public object outconverter
        SQLSMALLINT _position
        SQLLEN *_length_or_indicator
        VarData _data

    def __dealloc__(self):
        cpython.PyMem_Free(self._length_or_indicator)
        cpython.PyMem_Free(self._data.as_raw)

    cdef object _get_value(self, unsigned pos):
        cdef object value
        if pos >= self.num_elements:
            raise IndexError("array size exceeded")
        if self._length_or_indicator[pos] == SQL_NULL_DATA:
            return None
        if self._length_or_indicator[pos] > self.buffer_size:
            message = f"column {self._position} ({pos}) truncated " \
                      f"(need {self._length_or_indicator[pos]}, have " \
                      f"{self.buffer_size})"
            _raise_from_string(exceptions.DatabaseError, message)
        value = self._get_value_helper(pos)
        if self.outconverter:
            value = self.outconverter(value)
        return value

    cdef object _get_value_helper(self, unsigned pos):
        cdef:
            TIMESTAMP_STRUCT *as_timestamp
            DATE_STRUCT *as_date
            SQLSMALLINT c_data_type
            char *ptr
        c_data_type = self.type._c_data_type
        if c_data_type == SQL_C_CHAR:
            ptr = <char*> self._data.as_bytes + pos * self.buffer_size
            value = ptr[:self._length_or_indicator[pos]].decode()
            if self.type is DB_TYPE_DECIMAL:
                return decimal.Decimal(value)
            return value
        elif c_data_type == SQL_C_SBIGINT:
            return self._data.as_bigint[pos]
        elif c_data_type == SQL_C_LONG:
            return self._data.as_int[pos]
        elif c_data_type == SQL_C_DOUBLE:
            return self._data.as_double[pos]
        elif c_data_type == SQL_C_TYPE_DATE:
            as_date = &self._data.as_date[pos]
            return cydatetime.date_new(as_date.year, as_date.month,
                                       as_date.day)
        elif c_data_type == SQL_C_TYPE_TIMESTAMP:
            as_timestamp = &self._data.as_timestamp[pos]
            return cydatetime.datetime_new(as_timestamp.year,
                                           as_timestamp.month,
                                           as_timestamp.day,
                                           as_timestamp.hour,
                                           as_timestamp.minute,
                                           as_timestamp.second, 0, None)
        message = f"missing get support for DB type {self.type}"
        _raise_from_string(exceptions.NotSupportedError, message)

    cdef int _resize(self, SQLUINTEGER new_size,
                     SQLLEN new_buffer_size) except -1:
        cdef:
            SQLCHAR *new_data
            SQLCHAR *old_data
            unsigned i
        new_data = <SQLCHAR*> \
                cpython.PyMem_Malloc(self.num_elements * new_buffer_size)
        old_data = self._data.as_bytes
        for i in range(self.num_elements):
            memcpy(new_data + new_buffer_size * i,
                   old_data + self.buffer_size * i, self.buffer_size)
        cpython.PyMem_Free(old_data)
        self._data.as_raw = new_data
        self.size = new_size
        self.buffer_size = new_buffer_size
        self.position = -1

    cdef int _set_value(self, unsigned pos, object value) except -1:
        if pos >= self.num_elements:
            raise IndexError("array size exceeded")
        if self.inconverter is not None:
            value = self.inconverter(value)
        if value is None:
            self._length_or_indicator[pos] = SQL_NULL_DATA
        else:
            self._length_or_indicator[pos] = 0
            self._set_value_helper(pos, value)

    cdef int _set_value_helper(self, unsigned pos, object value) except -1:
        cdef:
            TIMESTAMP_STRUCT *as_timestamp
            SQLSMALLINT c_data_type
            SQLLEN temp_bytes_size
            SQLUINTEGER temp_size
            DATE_STRUCT *as_date
            bytes temp_bytes
        c_data_type = self.type._c_data_type
        if c_data_type == SQL_C_CHAR:
            if self.type is DB_TYPE_DECIMAL:
                if not isinstance(value, decimal.Decimal):
                    raise TypeError("expecting decimal.Decimal value")
                value = str(value)
            elif self.type is DB_TYPE_STRING \
                    and not isinstance(value, str):
                raise TypeError("expecting string")
            temp_bytes = value.encode()
            temp_bytes_size = <SQLLEN> len(temp_bytes)
            if temp_bytes_size > self.buffer_size:
                temp_size = <SQLUINTEGER> len(value)
                self._resize(temp_size, temp_bytes_size)
            self._length_or_indicator[pos] = <SQLINTEGER> temp_bytes_size
            if temp_bytes_size > 0:
                memcpy(self._data.as_bytes + self.buffer_size * pos,
                       <SQLCHAR*> temp_bytes, temp_bytes_size)
        elif c_data_type == SQL_C_DOUBLE:
            if isinstance(value, float):
                self._data.as_double[pos] = <double> value
            elif isinstance(value, int):
                self._data.as_double[pos] = \
                        <double> cpython.PyLong_AsLong(value)
            else:
                raise TypeError("expecting a number")
        elif c_data_type == SQL_C_SBIGINT:
            if not isinstance(value, int):
                raise TypeError("expecting integer")
            self._data.as_bigint[pos] = cpython.PyLong_AsLongLong(value)
        elif c_data_type == SQL_C_TYPE_DATE:
            as_date = &self._data.as_date[pos]
            if isinstance(value, datetime.datetime):
                as_date.year = cydatetime.datetime_year(value)
                as_date.month = cydatetime.datetime_month(value)
                as_date.day = cydatetime.datetime_day(value)
            elif isinstance(value, datetime.date):
                as_date.year = cydatetime.date_year(value)
                as_date.month = cydatetime.date_month(value)
                as_date.day = cydatetime.date_day(value)
            else:
                raise TypeError("expecting datetime.datetime or datetime.date")
        elif c_data_type == SQL_C_TYPE_TIMESTAMP:
            as_timestamp = &self._data.as_timestamp[pos]
            as_timestamp.fraction = 0
            if isinstance(value, datetime.datetime):
                as_timestamp.year = cydatetime.datetime_year(value)
                as_timestamp.month = cydatetime.datetime_month(value)
                as_timestamp.day = cydatetime.datetime_day(value)
                as_timestamp.hour = cydatetime.datetime_hour(value)
                as_timestamp.minute = cydatetime.datetime_minute(value)
                as_timestamp.second = cydatetime.datetime_second(value)
            elif isinstance(value, datetime.date):
                as_timestamp.year = cydatetime.date_year(value)
                as_timestamp.month = cydatetime.date_month(value)
                as_timestamp.day = cydatetime.date_day(value)
                as_timestamp.hour = 0
                as_timestamp.minute = 0
                as_timestamp.second = 0
            else:
                raise TypeError("expecting datetime.datetime or datetime.date")
        else:
            message = f"missing set support for DB type {self.type}"
            _raise_from_string(exceptions.NotSupportedError, message)

    @property
    def _description(self):
        precision = self.size
        scale = self.scale
        if self.type is not DB_TYPE_BIGINT and self.type is not DB_TYPE_BIT \
                and self.type is not DB_TYPE_INT \
                and self.type is not DB_TYPE_DOUBLE \
                and self.type is not DB_TYPE_DECIMAL:
            precision = scale = 0
        if self.type is DB_TYPE_BIGINT or self.type is DB_TYPE_INT:
            display_size = self.size + 1
        elif self.type is DB_TYPE_DOUBLE or self.type is DB_TYPE_DECIMAL:
            display_size = self.size + 1
            if scale > 0:
                display_size += 1
        else:
            display_size = self.size
        return (self.name, self.type, display_size, self.size, precision,
                scale, self.nulls_allowed)

    def getvalue(self, pos=0):
        return self._get_value(pos)

    def setvalue(self, pos, value):
        self._set_value(pos, value)
