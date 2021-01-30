#------------------------------------------------------------------------------
# string_buffer.pyx
#   Cython file defining the StringBuffer class (embedded in driver.pyx).
#------------------------------------------------------------------------------

@cython.freelist(20)
cdef class StringBuffer:
    cdef:
        bytes obj
        SQLCHAR *ptr
        SQLSMALLINT length

    cdef int set_value(self, value) except -1:
        if value is None:
            self.obj = None
            self.ptr = NULL
            self.length = 0
            return 0
        elif isinstance(value, str):
            self.obj = value.encode()
        elif isinstance(value, bytes):
            self.obj = value
        else:
            raise TypeError("expecting string or bytes")
        self.ptr = self.obj
        self.length = <SQLSMALLINT> len(self.obj)
