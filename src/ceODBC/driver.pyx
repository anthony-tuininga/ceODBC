#------------------------------------------------------------------------------
# driver.pyx
#   Cythonized module that uses the ODBC C API to connect to databases.
#------------------------------------------------------------------------------

# cython: language_level=3

cimport cython
cimport cpython
cimport cpython.datetime as cydatetime

from libc.stdint cimport int8_t, int16_t, int32_t, int64_t
from libc.stdint cimport uint8_t, uint16_t, uint32_t, uint64_t
from libc.limits cimport INT_MAX
from libc.string cimport memcpy

import datetime
import decimal

cydatetime.import_datetime()

from . import exceptions

include "odbc.pxd"

include "errors.pyx"
include "string_buffer.pyx"
include "types.pyx"
include "connection.pyx"
include "cursor.pyx"
include "var.pyx"
