"""
Constructors mandated by the database API.
"""

import datetime

# pylint: disable=invalid-name
def Binary(value):
    """
    Constructor mandated by the database API for creating a binary value. This
    is merely a wrapper over the bytes class and that should be used instead.
    """
    return bytes(value)


def Date(year, month, day):
    """
    Constructor mandated by the database API for creating a date value. This is
    merely a wrapper over the datetime.date class and that should be used
    instead.
    """
    return datetime.date(year, month, day)


def DateFromTicks(ticks):
    """
    Constructor mandated by the database API for creating a date value given
    the number of seconds since the epoch (January 1, 1970). This is equivalent
    to using datetime.date.fromtimestamp() and that should be used instead.
    """
    return datetime.date.fromtimestamp(ticks)


def Time(hour, minute, second):
    """
    Constructor mandated by the database API for creating a time value. This is
    merely a wrapper over the datetime.time class and that should be used
    instead.
    """
    return datetime.time(hour, minute, second)


def TimeFromTicks(ticks):
    """
    Constructor mandated by the database API for creating a time value given
    the number of seconds since the epoch (January 1, 1970). This is equivalent
    to using datetime.datetime.fromtimestamp().time() and that should be used
    instead.
    """
    return datetime.datetime.fromtimestamp(ticks).time()


# pylint: disable=too-many-arguments
def Timestamp(year, month, day, hour=0, minute=0, second=0, fsecond=0):
    """
    Constructor mandated by the database API for creating a timestamp value.
    This is merely a wrapper over the datetime.datetime class and that should
    be used instead.
    """
    return datetime.datetime(year, month, day, hour, minute, second, fsecond)


def TimestampFromTicks(ticks):
    """
    Constructor mandated by the database API for creating a timestamp value
    given the number of seconds since the epoch (January 1, 1970). This is
    equivalent to using datetime.datetime.fromtimestamp() and that should be
    used instead.
    """
    return datetime.datetime.fromtimestamp(ticks)
