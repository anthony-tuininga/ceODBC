//-----------------------------------------------------------------------------
// StringUtils.c
//   Defines constants and routines specific to handling strings.
//-----------------------------------------------------------------------------

// define structure for abstracting string buffers
typedef struct {
    const void *ptr;
    Py_ssize_t size;
#ifdef Py_UNICODE_WIDE
    PyObject *encodedString;
#endif
} udt_StringBuffer;


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
    #define ceBinary_Check              PyBytes_Check
#else
    #define ceBinary_Type               PyBuffer_Type
    #define ceBinary_Check              PyBuffer_Check
#endif


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
#ifdef Py_UNICODE_WIDE
    buf->encodedString = NULL;
#endif
    return 0;
}


//-----------------------------------------------------------------------------
// StringBuffer_FromUnicode()
//   Populate the string buffer from a unicode object.
//-----------------------------------------------------------------------------
static int StringBuffer_FromUnicode(
    udt_StringBuffer *buf,              // buffer to fill
    PyObject *obj)                      // unicode object expected
{
    if (!obj)
        return StringBuffer_Init(buf);
#ifdef Py_UNICODE_WIDE
    int one = 1;
    int byteOrder = (IS_LITTLE_ENDIAN) ? -1 : 1;
    buf->encodedString = PyUnicode_EncodeUTF16(PyUnicode_AS_UNICODE(obj),
            PyUnicode_GET_SIZE(obj), NULL, byteOrder);
    if (!buf->encodedString)
        return -1;
    buf->ptr = PyBytes_AS_STRING(buf->encodedString);
    buf->size = PyBytes_GET_SIZE(buf->encodedString);
#else
    buf->ptr = (char*) PyUnicode_AS_UNICODE(obj);
    buf->size = PyUnicode_GET_DATA_SIZE(obj);
#endif
    return 0;
}


//-----------------------------------------------------------------------------
// StringBuffer_FromBytes()
//   Populate the string buffer from a bytes object.
//-----------------------------------------------------------------------------
static int StringBuffer_FromBytes(
    udt_StringBuffer *buf,              // buffer to fill
    PyObject *obj)                      // bytes object expected
{
    if (!obj)
        return StringBuffer_Init(buf);
    buf->ptr = PyBytes_AS_STRING(obj);
    buf->size = PyBytes_GET_SIZE(obj);
#ifdef Py_UNICODE_WIDE
    buf->encodedString = NULL;
#endif
    return 0;
}


#if PY_MAJOR_VERSION < 3
//-----------------------------------------------------------------------------
// StringBuffer_FromBuffer()
//   Populate the string buffer from a buffer object.
//-----------------------------------------------------------------------------
static int StringBuffer_FromBuffer(
    udt_StringBuffer *buf,              // buffer to fill
    PyObject *obj)                      // bytes object expected
{
    if (!obj)
        return StringBuffer_Init(buf);
    if (PyObject_AsReadBuffer(obj, &buf->ptr, &buf->size) < 0)
        return -1;
#ifdef Py_UNICODE_WIDE
    buf->encodedString = NULL;
#endif
    return 0;
}
#endif


#if PY_MAJOR_VERSION >= 3
#define StringBuffer_FromBinary         StringBuffer_FromBytes
#else
#define StringBuffer_FromBinary         StringBuffer_FromBuffer
#endif


#if PY_MAJOR_VERSION >= 3
    #define ceString_Type               &PyUnicode_Type
    #define ceString_Format             PyUnicode_Format
    #define ceString_Check              PyUnicode_Check
    #define ceString_GetSize            PyUnicode_GET_SIZE
    #define ceString_FromObject         PyObject_Str
    #define ceString_FromAscii(str) \
        PyUnicode_DecodeASCII(str, strlen(str), NULL)
    #ifdef Py_UNICODE_WIDE
        #define StringBuffer_Clear(buffer) \
            Py_XDECREF((buffer)->encodedString)
        #define ceString_FromEncodedString(buffer, numBytes) \
            PyUnicode_DecodeUTF16(buffer, numBytes, NULL, NULL)
    #else
        #define StringBuffer_Clear(buffer)
        #define ceString_FromEncodedString(buffer, numBytes) \
            PyUnicode_FromUnicode((Py_UNICODE*) (buffer), (numBytes) / 2)
    #endif
    #define StringBuffer_Fill           StringBuffer_FromUnicode
#else
    #define ceString_Type               &PyBytes_Type
    #define ceString_Format             PyBytes_Format
    #define ceString_Check              PyBytes_Check
    #define ceString_GetSize            PyBytes_GET_SIZE
    #define ceString_FromObject         PyObject_Str
    #define StringBuffer_Clear(buffer)
    #define ceString_FromAscii(str) \
        PyBytes_FromString(str)
    #define ceString_FromEncodedString(buffer, numBytes) \
        PyBytes_FromStringAndSize(buffer, numBytes)
    #define StringBuffer_Fill           StringBuffer_FromBytes
#endif

