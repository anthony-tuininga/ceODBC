"""
Initialization of package ceODBC.
"""

from .version import __version__
from .constants import *
from .constructors import *
from .exceptions import *

from .driver import Connection

def connect(dsn, autocommit=False):
    """
    Creates a connection to the database and returns a Connection object.
    """
    conn = Connection.__new__(Connection)
    conn._connect(dsn, autocommit)
    return conn
