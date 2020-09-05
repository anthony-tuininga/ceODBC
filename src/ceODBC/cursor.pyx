#------------------------------------------------------------------------------
# cursor.pyx
#   Cython file defining the cursor class (embedded in driver.pyx).
#------------------------------------------------------------------------------

cdef class Cursor:
    cdef:
        readonly Connection connection

    def __init__(self, connection):
        self.connection = connection
