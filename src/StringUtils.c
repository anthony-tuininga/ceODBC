//-----------------------------------------------------------------------------
// StringUtils.c
//   Defines constants and routines specific to handling strings.
//-----------------------------------------------------------------------------

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
static int StringBuffer_Init(udt_StringBuffer *buf)
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
static void StringBuffer_Clear(udt_StringBuffer *buf)
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
static int StringBuffer_FromUnicode(udt_StringBuffer *buf, PyObject *obj)
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


//-----------------------------------------------------------------------------
// StringBuffer_FromString()
//   Populate the string buffer from a Unicode object.
//-----------------------------------------------------------------------------
static int StringBuffer_FromString(udt_StringBuffer *buf, PyObject *obj,
        const char *message)
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
static int StringBuffer_FromBinary(udt_StringBuffer *buf, PyObject *obj)
{
    if (!obj)
        return StringBuffer_Init(buf);
    buf->ptr = PyBytes_AS_STRING(obj);
    buf->size = buf->sizeInBytes = PyBytes_GET_SIZE(obj);
    buf->encodedString = NULL;
    return 0;
}
