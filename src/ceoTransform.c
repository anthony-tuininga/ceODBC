//-----------------------------------------------------------------------------
// ceoTransform.c
//   Routines for transforming data from the database to Python and from Python
// to the database.
//-----------------------------------------------------------------------------

#include "ceoModule.h"
#include <datetime.h>

// PyPy compatibility
#ifndef PyDateTime_DELTA_GET_DAYS
#define PyDateTime_DELTA_GET_DAYS(x) ((x)->days)
#endif

#ifndef PyDateTime_DELTA_GET_SECONDS
#define PyDateTime_DELTA_GET_SECONDS(x) ((x)->seconds)
#endif

#ifndef PyDateTime_DELTA_GET_MICROSECONDS
#define PyDateTime_DELTA_GET_MICROSECONDS(x) ((x)->microseconds)
#endif


//-----------------------------------------------------------------------------
// ceoTransform_dateFromSqlValue()
//   Returns a date given a SQL value.
//-----------------------------------------------------------------------------
PyObject *ceoTransform_dateFromSqlValue(DATE_STRUCT *sqlValue)
{
    return PyDate_FromDate(sqlValue->year, sqlValue->month, sqlValue->day);
}


//-----------------------------------------------------------------------------
// ceoTransform_dateFromTicks()
//   Creates a date from ticks (number of seconds since Unix epoch).
//-----------------------------------------------------------------------------
PyObject *ceoTransform_dateFromTicks(PyObject *args)
{
    return PyDate_FromTimestamp(args);
}


//-----------------------------------------------------------------------------
// ceoTransform_init()
//   Import the necessary modules for performing transformations.
//-----------------------------------------------------------------------------
int ceoTransform_init(void)
{
    PyObject *module;

    // import the datetime module for datetime support
    PyDateTime_IMPORT;
    if (PyErr_Occurred())
        return -1;
    ceoPyTypeDate = PyDateTimeAPI->DateType;
    ceoPyTypeTime = PyDateTimeAPI->TimeType;
    ceoPyTypeDateTime = PyDateTimeAPI->DateTimeType;

    // import the decimal module for decimal support
    module = PyImport_ImportModule("decimal");
    if (!module)
        return -1;
    ceoPyTypeDecimal =
            (PyTypeObject*) PyObject_GetAttrString(module, "Decimal");
    Py_DECREF(module);
    if (!ceoPyTypeDecimal)
        return -1;

    return 0;
}


//-----------------------------------------------------------------------------
// ceoTransform_sqlValueFromDate()
//   Returns a SQL value given a Python date.
//-----------------------------------------------------------------------------
int ceoTransform_sqlValueFromDate(PyObject *pyValue, DATE_STRUCT *sqlValue)
{
    if (!PyDateTime_Check(pyValue) && !PyDate_Check(pyValue)) {
        PyErr_SetString(PyExc_TypeError, "expecting date or datetime data");
        return -1;
    }
    sqlValue->year = PyDateTime_GET_YEAR(pyValue);
    sqlValue->month = PyDateTime_GET_MONTH(pyValue);
    sqlValue->day = PyDateTime_GET_DAY(pyValue);
    return 0;
}


//-----------------------------------------------------------------------------
// ceoTransform_sqlValueFromTime()
//   Returns a SQL value given a Python time.
//-----------------------------------------------------------------------------
int ceoTransform_sqlValueFromTime(PyObject *pyValue, TIME_STRUCT *sqlValue)
{
    if (PyDateTime_Check(pyValue)) {
        sqlValue->hour = PyDateTime_DATE_GET_HOUR(pyValue);
        sqlValue->minute = PyDateTime_DATE_GET_MINUTE(pyValue);
        sqlValue->second = PyDateTime_DATE_GET_SECOND(pyValue);
    } else if (PyTime_Check(pyValue)) {
        sqlValue->hour = PyDateTime_TIME_GET_HOUR(pyValue);
        sqlValue->minute = PyDateTime_TIME_GET_MINUTE(pyValue);
        sqlValue->second = PyDateTime_TIME_GET_SECOND(pyValue);
    } else {
        PyErr_SetString(PyExc_TypeError, "expecting datetime or time data");
        return -1;
    }
    return 0;
}


//-----------------------------------------------------------------------------
// ceoTransform_sqlValueFromTimestamp()
//   Returns a SQL value given a Python timestamp.
//-----------------------------------------------------------------------------
int ceoTransform_sqlValueFromTimestamp(PyObject *pyValue,
        TIMESTAMP_STRUCT *sqlValue)
{
    sqlValue->fraction = 0;
    if (PyDateTime_Check(pyValue)) {
        sqlValue->year = PyDateTime_GET_YEAR(pyValue);
        sqlValue->month = PyDateTime_GET_MONTH(pyValue);
        sqlValue->day = PyDateTime_GET_DAY(pyValue);
        sqlValue->hour = PyDateTime_DATE_GET_HOUR(pyValue);
        sqlValue->minute = PyDateTime_DATE_GET_MINUTE(pyValue);
        sqlValue->second = PyDateTime_DATE_GET_SECOND(pyValue);
    } else if (PyDate_Check(pyValue)) {
        sqlValue->year = PyDateTime_GET_YEAR(pyValue);
        sqlValue->month = PyDateTime_GET_MONTH(pyValue);
        sqlValue->day = PyDateTime_GET_DAY(pyValue);
        sqlValue->hour = 0;
        sqlValue->minute = 0;
        sqlValue->second = 0;
    } else {
        PyErr_SetString(PyExc_TypeError, "expecting date or datetime data");
        return -1;
    }
    return 0;
}


//-----------------------------------------------------------------------------
// ceoTransform_timeFromSqlValue()
//   Returns a time given a SQL value.
//-----------------------------------------------------------------------------
PyObject *ceoTransform_timeFromSqlValue(TIME_STRUCT *sqlValue)
{
    return PyTime_FromTime(sqlValue->hour, sqlValue->minute, sqlValue->second,
            0);
}


//-----------------------------------------------------------------------------
// ceoTransform_timeFromTicks()
//   Creates a time from ticks (number of seconds since Unix epoch).
//-----------------------------------------------------------------------------
PyObject* ceoTransform_timeFromTicks(PyObject* args)
{
    double inputTicks;
    struct tm *time;
#ifdef WIN32
    __time64_t ticks;
#else
    time_t ticks;
#endif

    if (!PyArg_ParseTuple(args, "d", &inputTicks))
        return NULL;
    ticks = (long) inputTicks;
#ifdef WIN32
    time = _localtime64(&ticks);
#else
    time = localtime(&ticks);
#endif
    return PyTime_FromTime(time->tm_hour, time->tm_min, time->tm_sec, 0);
}


//-----------------------------------------------------------------------------
// ceoTransform_timestampFromSqlValue()
//   Returns a timestamp given a SQL value.
//-----------------------------------------------------------------------------
PyObject *ceoTransform_timestampFromSqlValue(TIMESTAMP_STRUCT *sqlValue)
{
    return PyDateTime_FromDateAndTime(sqlValue->year, sqlValue->month,
            sqlValue->day, sqlValue->hour, sqlValue->minute, sqlValue->second,
            sqlValue->fraction / 1000);
}


//-----------------------------------------------------------------------------
// ceoTransform_timestampFromTicks()
//   Creates a timestamp from ticks (number of seconds since Unix epoch).
//-----------------------------------------------------------------------------
PyObject *ceoTransform_timestampFromTicks(PyObject *args)
{
    return PyDateTime_FromTimestamp(args);
}
