//-----------------------------------------------------------------------------
// StringUtils.c
//   Defines constants and routines specific to handling strings.
//-----------------------------------------------------------------------------

#if PY_MAJOR_VERSION >= 3
    #define CEODBC_CHAR                 SQLWCHAR
    #define ceString_Type               &PyUnicode_Type
    #define ceString_VariableType       vt_Unicode
    #define ceLongString_VariableType   vt_LongUnicode
    #define ceString_Format             PyUnicode_Format
    #define ceString_Join               PyUnicode_Join
    #define ceString_Check              PyUnicode_Check
    #define ceString_GetSize            PyUnicode_GET_SIZE
    #define ceString_FromObject         PyObject_Str
    #define ceString_FromAscii(str) \
        PyUnicode_DecodeASCII(str, strlen(str), NULL)
    #ifdef Py_UNICODE_WIDE
        #define ceString_FromStringAndSize(buffer, size) \
            PyUnicode_DecodeUTF16(buffer, (size) * 2, NULL, NULL)
        #define ceString_FromStringAndSizeInBytes(buffer, size) \
            PyUnicode_DecodeUTF16(buffer, (size), NULL, NULL)
    #else
        #define ceString_FromStringAndSize(buffer, size) \
            PyUnicode_FromUnicode((Py_UNICODE*) (buffer), size)
        #define ceString_FromStringAndSizeInBytes(buffer, size) \
            PyUnicode_FromUnicode((Py_UNICODE*) (buffer), (size) / 2)
    #endif
#else
    #define CEODBC_CHAR                 SQLCHAR
    #define ceString_VariableType       vt_String
    #define ceLongString_VariableType   vt_LongString
    #define ceString_Type               &PyBytes_Type
    #define ceString_Format             PyBytes_Format
    #define ceString_Check              PyBytes_Check
    #define ceString_GetSize            PyBytes_GET_SIZE
    #define ceString_FromObject         PyObject_Str
    #define ceString_FromAscii(str) \
        PyBytes_FromString(str)
    #define ceString_FromStringAndSize(buffer, size) \
        PyBytes_FromStringAndSize( (char*) (buffer), size)
    #define ceString_FromStringAndSizeInBytes(buffer, size) \
        PyBytes_FromStringAndSize( (char*) (buffer), size)
#endif


// use the bytes methods in ceODBC and define them as the equivalent string
// type methods as is done in Python 2.6
#ifndef PyBytes_Check
    #define PyBytes_Type                PyString_Type
    #define PyBytes_AS_STRING           PyString_AS_STRING
    #define PyBytes_GET_SIZE            PyString_GET_SIZE
    #define PyBytes_Check               PyString_Check
    #define PyBytes_Format              PyString_Format
    #define PyBytes_FromString          PyString_FromString
    #define PyBytes_FromStringAndSize   PyString_FromStringAndSize
#endif


// define binary type and methods
#if PY_MAJOR_VERSION >= 3
    #define ceBinary_Type               PyBytes_Type
    #define ceBinary_Check(obj)         PyBytes_Check(obj)
#else
    #define ceBinary_Type               PyBuffer_Type
    #define ceBinary_Check(obj) \
        PyBuffer_Check(obj) || PyBytes_Check(obj)
#endif


// define structure for abstracting string buffers
typedef struct {
    const void *ptr;
    Py_ssize_t size;
    Py_ssize_t sizeInBytes;
    PyObject *encodedString;
} udt_StringBuffer;


//-----------------------------------------------------------------------------
// StringBuffer_Init()
//   Initialize the string buffer with an empty string. Returns 0 as a
// convenience to the caller.
//-----------------------------------------------------------------------------
static int StringBuffer_Init(
    udt_StringBuffer *buf)              // buffer to fill
{
    buf->ptr = NULL;
    buf->size = 0;
    buf->sizeInBytes = 0;
    buf->encodedString = NULL;
    return 0;
}


//-----------------------------------------------------------------------------
// StringBuffer_Clear()
//   Clear the string buffer, if applicable.
//-----------------------------------------------------------------------------
static void StringBuffer_Clear(
    udt_StringBuffer *buf)              // buffer to fill
{
    Py_CLEAR(buf->encodedString);
    buf->ptr = NULL;
    buf->size = 0;
    buf->sizeInBytes = 0;
}


//-----------------------------------------------------------------------------
// StringBuffer_FromUnicode()
//   Populate the string buffer from a unicode object.
//-----------------------------------------------------------------------------
static int StringBuffer_FromUnicode(
    udt_StringBuffer *buf,              // buffer to fill
    PyObject *obj)                      // unicode object expected
{
#ifdef Py_UNICODE_WIDE
    int one = 1;
    int byteOrder = (IS_LITTLE_ENDIAN) ? -1 : 1;
    buf->encodedString = PyUnicode_EncodeUTF16(PyUnicode_AS_UNICODE(obj),
            PyUnicode_GET_SIZE(obj), NULL, byteOrder);
    if (!buf->encodedString)
        return -1;
    buf->ptr = PyBytes_AS_STRING(buf->encodedString);
    buf->sizeInBytes = PyBytes_GET_SIZE(buf->encodedString);
    buf->size = buf->sizeInBytes / 2;
#else
    buf->ptr = (char*) PyUnicode_AS_UNICODE(obj);
    buf->sizeInBytes = PyUnicode_GET_DATA_SIZE(obj);
    buf->size = PyUnicode_GET_SIZE(obj);
    buf->encodedString = NULL;
#endif
    return 0;
}


#if PY_MAJOR_VERSION >= 3

//-----------------------------------------------------------------------------
// StringBuffer_FromString()
//   Populate the string buffer from a Unicode object.
//-----------------------------------------------------------------------------
static int StringBuffer_FromString(
    udt_StringBuffer *buf,              // buffer to fill
    PyObject *obj,                      // bytes object expected
    const char *message)                // message to raise on error
{
    if (PyUnicode_Check(obj))
        return StringBuffer_FromUnicode(buf, obj);
    PyErr_SetString(PyExc_TypeError, message);
    return -1;
}


//-----------------------------------------------------------------------------
// StringBuffer_FromBinary()
//   Populate the string buffer from a bytes object.
//-----------------------------------------------------------------------------
static int StringBuffer_FromBinary(
    udt_StringBuffer *buf,              // buffer to fill
    PyObject *obj)                      // bytes object expected
{
    if (!obj)
        return StringBuffer_Init(buf);
    buf->ptr = PyBytes_AS_STRING(obj);
    buf->size = buf->sizeInBytes = PyBytes_GET_SIZE(obj);
    buf->encodedString = NULL;
    return 0;
}

#else

//-----------------------------------------------------------------------------
// StringBuffer_FromString()
//   Populate the string buffer from a bytes object.
//-----------------------------------------------------------------------------
static int StringBuffer_FromString(
    udt_StringBuffer *buf,              // buffer to fill
    PyObject *obj,                      // bytes object expected
    const char *message)                // message to raise on error
{
    if (PyUnicode_Check(obj)) {
        buf->encodedString = PyUnicode_AsEncodedString(obj, NULL, NULL);
        if (!buf->encodedString)
            return -1;
        obj = buf->encodedString;
    } else if (!PyString_Check(obj)) {
        PyErr_SetString(PyExc_TypeError, message);
        return -1;
    }
    buf->ptr = PyBytes_AS_STRING(obj);
    buf->size = buf->sizeInBytes = PyBytes_GET_SIZE(obj);
    buf->encodedString = NULL;
    return 0;
}


//-----------------------------------------------------------------------------
// StringBuffer_FromBinary()
//   Populate the string buffer from a buffer object.
//-----------------------------------------------------------------------------
static int StringBuffer_FromBinary(
    udt_StringBuffer *buf,              // buffer to fill
    PyObject *obj)                      // bytes object expected
{
    if (!obj)
        return StringBuffer_Init(buf);
    if (PyObject_AsReadBuffer(obj, &buf->ptr, &buf->size) < 0)
        return -1;
    buf->sizeInBytes = buf->size;
    buf->encodedString = NULL;
    return 0;
}

//-----------------------------------------------------------------------------
// ceString_Join()
//   Populate the string buffer from a buffer object.
//-----------------------------------------------------------------------------
static PyObject* ceString_Join(
    PyObject *separator,                // the separator
    PyObject *list)                     // list of strings to join together
{
    PyObject *methodName, *result;

    methodName = PyString_FromString("join");
    if (!methodName)
        return NULL;
    result = PyObject_CallMethodObjArgs(separator, methodName, list, NULL);
    Py_DECREF(methodName);
    return result;
}

#endif

