from .version import __version__

# define constants required by the DB API
apilevel = "2.0"
threadsafety = 2
paramstyle = "qmark"

# backwards compatible alias
version = __version__

# database type constants from driver
from .driver import DB_TYPE_STRING

# API type constants from driver
from .driver import STRING
