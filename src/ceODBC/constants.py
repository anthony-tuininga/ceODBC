from .version import __version__

# define constants required by the DB API
apilevel = "2.0"
threadsafety = 2
paramstyle = "qmark"

# backwards compatible alias
version = __version__
