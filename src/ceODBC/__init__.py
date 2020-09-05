#------------------------------------------------------------------------------
# __init__.py
#   ceODBC package initialization.
#------------------------------------------------------------------------------

from .version import __version__
from .constants import *
from .exceptions import *

from .driver import Connection

def connect(dsn, autocommit=False):
    """
    Creates a connection to the database and returns a Connection object.
    """
    conn = Connection()
    conn._connect(dsn, autocommit)
    return conn
