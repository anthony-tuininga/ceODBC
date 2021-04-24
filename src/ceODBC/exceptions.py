"""
Exception classes mandated by the database API.
"""

class Error(Exception):
    """
    Base class of all other error exceptions.
    """


class DatabaseError(Error):
    """
    Exception raised for errors that are related to the database.
    """


class DataError(DatabaseError):
    """
    Exception raised for errors that are due to problems with the processed
    data like division by zero, numeric value out of range, etc.
    """


class IntegrityError(DatabaseError):
    """
    Exception raised when the relational integrity of the database is affected
    (for example, when a foreign key check fails).
    """


class InterfaceError(Error):
    """
    Exception raised for errors that are related to the database interface
    rather than the database itself.
    """


class InternalError(DatabaseError):
    """
    Exception raised when the database encounters an internal error.
    """


class NotSupportedError(DatabaseError):
    """
    Exception raised when a method or database API ws used which is not
    supported by the database.
    """


class OperationalError(DatabaseError):
    """
    Exception raised for errors that are related to the database's operation
    and not necessarily under the control of the programmer.
    """


class ProgrammingError(DatabaseError):
    """
    Exception raised for programming errors (table not found or already exists,
    syntax errors in the SQL statement, wrong number of parameters specified,
    etc.
    """
