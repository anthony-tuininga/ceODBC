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
            SQLSMALLINT c_data_type
            char *ptr
        c_data_type = self.type._c_data_type
        if c_data_type == SQL_C_CHAR:
            ptr = <char*> self._data.as_bytes + pos * self.buffer_size
            return ptr[:self._length_or_indicator[pos]].decode()
        elif c_data_type == SQL_C_SBIGINT:
            return self._data.as_bigint[pos]
        message = f"missing get support for DB type {self.type}"
        _raise_from_string(exceptions.InternalError, message)
