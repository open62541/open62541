Data Types
==========

Introduction
------------

In open62541, all data types share the same basic API for creation, copying and
deletion. The following functions are present for all data types ``T``.

``void T_init(T *ptr)``
  Initialize the data type. This is synonymous with zeroing out the memory, i.e. *memset(dataptr, 0, sizeof(T))*.
``T* T_new()``
  Allocate and return the memory for the data type. The memory is already initialized.
``UA_StatusCode T_copy(const T *src, T *dst)``
  Copy the content of the data type. Returns *UA_STATUSCODE_GOOD* if it succeeded.
``void T_deleteMembers(T *ptr)``
  Delete the dynamically allocated content of the data type, but not the data type itself.
``void T_delete(T *ptr)``
  Delete the content of the data type and the memory for the data type itself.

The builtin data types
----------------------

OPC UA defines 25 builtin data types. All other data types are combinations of
the 25 builtin data types.

UA_Boolean
^^^^^^^^^^

A two-state logical value (true or false).

.. code-block:: c

  typedef bool UA_Boolean;
  #define UA_TRUE true
  #define UA_FALSE false

UA_SByte
^^^^^^^^

An integer value between -128 and 127.

.. code-block:: c

  typedef int8_t UA_SByte;

UA_Byte
^^^^^^^

An integer value between 0 and 256.

.. code-block:: c

  typedef uint8_t UA_Byte;

UA_Int16
^^^^^^^^

An integer value between -32,768 and 32,767.

.. code-block:: c

  typedef int16_t UA_Int16;

UA_UInt16
^^^^^^^^^

An integer value between 0 and 65,535.

.. code-block:: c

  typedef uint16_t UA_UInt16;

UA_Int32
^^^^^^^^

An integer value between -2,147,483,648 and 2,147,483,647.

.. code-block:: c

  typedef int32_t UA_Int32;

UA_UInt32
^^^^^^^^^

An integer value between 0 and 4,294,967,295.

.. code-block:: c

  typedef uint32_t UA_UInt32;

UA_Int64
^^^^^^^^

An integer value between -10,223,372,036,854,775,808 and 9,223,372,036,854,775,807.

.. code-block:: c

  typedef int64_t UA_Int64;

UA_UInt64
^^^^^^^^^

An integer value between 0 and 18,446,744,073,709,551,615.

.. code-block:: c

  typedef uint64_t UA_UInt64;

UA_Float
^^^^^^^^

An IEEE single precision (32 bit) floating point value.

.. code-block:: c

  typedef float UA_Float;

UA_Double
^^^^^^^^^

An IEEE double precision (64 bit) floating point value.

.. code-block:: c

  typedef double UA_Double;

UA_DateTime
^^^^^^^^^^^

An instance in time. A DateTime value is encoded as a 64-bit signed integer
which represents the number of 100 nanosecond intervals since January 1, 1601
(UTC).

.. code-block:: c

  typedef UA_Int64 UA_DateTime;

The following functions and definitions are used with UA_DateTime.

.. code-block:: c

  UA_DateTime UA_DateTime_now(void);

  typedef struct UA_DateTimeStruct {
      UA_UInt16 nanoSec;
      UA_UInt16 microSec;
      UA_UInt16 milliSec;
      UA_UInt16 sec;
      UA_UInt16 min;
      UA_UInt16 hour;
      UA_UInt16 day;
      UA_UInt16 month;
      UA_UInt16 year;
  } UA_DateTimeStruct;

  UA_DateTimeStruct UA_EXPORT UA_DateTime_toStruct(UA_DateTime time);

  UA_String UA_EXPORT UA_DateTime_toString(UA_DateTime time);

UA_Guid
^^^^^^^

A 16 byte value that can be used as a globally unique identifier.

.. code-block:: c

  typedef struct {
      UA_UInt32 data1;
      UA_UInt16 data2;
      UA_UInt16 data3;
      UA_Byte   data4[8];
  } UA_Guid;

The following functions and definitions are used with UA_Guid.

.. code-block:: c

  UA_Boolean UA_Guid_equal(const UA_Guid *g1, const UA_Guid *g2);

  UA_Guid UA_Guid_random(UA_UInt32 *seed);

UA_String
^^^^^^^^^

A sequence of Unicode characters. See also the section :ref:`array-handling` for
the usage of arrays in open62541.

.. code-block:: c

  typedef struct {
      size_t length; // The length of the string
      UA_Byte *data; // The string's content (not null-terminated)
  } UA_String;

The following functions and definitions are used with UA_String.

.. code-block:: c

  extern const UA_String UA_STRING_NULL;

  UA_String UA_STRING(char *chars);

  #define UA_STRING_ALLOC(CHARS) UA_String_fromChars(CHARS)
    
  /** Copies the content on the heap. Returns a null-string when alloc fails */
  UA_String UA_String_fromChars(char const src[]);

  UA_Boolean UA_String_equal(const UA_String *s1, const UA_String *s2);

Here's a small example for the usage of UA_String.

.. code-block:: c

  /* The definition of UA_String copied from ua_types.h */ 
  typedef struct {
      size_t length; ///< The length of the string
      UA_Byte *data; ///< The string's content (not null-terminated)
  } UA_String;

  UA_String s1 = UA_STRING("test1");       /* s1 points to the statically allocated string buffer */
  UA_String_init(&s1);                     /* Reset s1 (no memleak due to the statically allocated buffer) */
  
  UA_String s2 = UA_STRING_ALLOC("test2"); /* s2 points to a new copy of the string buffer (with malloc) */
  UA_String_deleteMembers(&s2);            /* Free the content of s2, but not s2 itself */
  
  UA_String *s3 = UA_String_new();         /* The string s3 is malloced and initialized */
  *s3 = UA_STRING_ALLOC("test3");          /* s3 points to a new copy of the string buffer */
  
  UA_String s4;
  UA_copy(s3, &s4);                        /* Copy the content of s3 to s4 */
  
  UA_String_delete(s3);                    /* Free the string buffer and the string itself */
  UA_String_deleteMembers(&s4);            /* Again, delete only the string buffer */

UA_ByteString
^^^^^^^^^^^^^

A sequence of octets.

.. code-block:: c

  typedef UA_String UA_ByteString;

UA_XmlEelement
^^^^^^^^^^^^^^

An XML element.

.. code-block:: c

  typedef UA_String UA_XmlElement;

UA_NodeId
^^^^^^^^^

An identifier for a node in the address space of an OPC UA Server.

.. code-block:: c

  enum UA_NodeIdType {
      UA_NODEIDTYPE_NUMERIC    = 0, // On the wire, this can be 0, 1 or 2 (shortened numeric nodeids)
      UA_NODEIDTYPE_STRING     = 3,
      UA_NODEIDTYPE_GUID       = 4,
      UA_NODEIDTYPE_BYTESTRING = 5
  };

  typedef struct {
      UA_UInt16 namespaceIndex;
      enum UA_NodeIdType identifierType;
      union {
          UA_UInt32     numeric;
          UA_String     string;
          UA_Guid       guid;
          UA_ByteString byteString;
      } identifier;
  } UA_NodeId;

The following functions and definitions are used with UA_NodeId.

.. code-block:: c

  UA_Boolean UA_NodeId_isNull(const UA_NodeId *p);

  UA_Boolean UA_NodeId_equal(const UA_NodeId *n1, const UA_NodeId *n2);

  extern const UA_NodeId UA_NODEID_NULL;

  UA_NodeId UA_NODEID_NUMERIC(UA_UInt16 nsIndex, UA_Int32 identifier);

  UA_NodeId UA_NODEID_STRING(UA_UInt16 nsIndex, char *chars) {

  UA_NodeId UA_NODEID_STRING_ALLOC(UA_UInt16 nsIndex, const char *chars);

  UA_NodeId UA_NODEID_GUID(UA_UInt16 nsIndex, UA_Guid guid);

  UA_NodeId UA_NODEID_BYTESTRING(UA_UInt16 nsIndex, char *chars);

  UA_NodeId UA_NODEID_BYTESTRING_ALLOC(UA_UInt16 nsIndex, const char *chars);

UA_ExpandedNodeId
^^^^^^^^^^^^^^^^^

A NodeId that allows the namespace URI to be specified instead of an index.

.. code-block:: c

  typedef struct {
      UA_NodeId nodeId;
      UA_String namespaceUri;
      UA_UInt32 serverIndex;
  } UA_ExpandedNodeId;

The following functions and definitions are used with UA_ExpandedNodeId.

.. code-block:: c

  UA_ExpandedNodeId UA_EXPANDEDNODEID_NUMERIC(UA_UInt16 nsIndex, UA_Int32 identifier);

  UA_ExpandedNodeId UA_EXPANDEDNODEID_STRING(UA_UInt16 nsIndex, char *chars);

  UA_ExpandedNodeId UA_EXPANDEDNODEID_STRING_ALLOC(UA_UInt16 nsIndex, const char *chars);

  UA_ExpandedNodeId UA_EXPANDEDNODEID_STRING_GUID(UA_UInt16 nsIndex, UA_Guid guid);

  UA_ExpandedNodeId UA_EXPANDEDNODEID_BYTESTRING(UA_UInt16 nsIndex, char *chars);

  UA_ExpandedNodeId UA_EXPANDEDNODEID_BYTESTRING_ALLOC(UA_UInt16 nsIndex, const char *chars);

UA_QualifiedName
^^^^^^^^^^^^^^^^

A name qualified by a namespace.

.. code-block:: c

  typedef struct {
      UA_UInt16 namespaceIndex;
      UA_String name;
  } UA_QualifiedName;

The following functions and definitions are used with UA_QualifiedName.

.. code-block:: c

  UA_QualifiedName UA_QUALIFIEDNAME(UA_UInt16 nsIndex, char *chars);

  UA_QualifiedName UA_QUALIFIEDNAME_ALLOC(UA_UInt16 nsIndex, const char *chars);

UA_LocalizedText
^^^^^^^^^^^^^^^^

Human readable text with an optional locale identifier.

.. code-block:: c

  typedef struct {
      UA_String locale;
      UA_String text;
  } UA_LocalizedText;
                
The following functions and definitions are used with UA_LocalizedText.

.. code-block:: c

  UA_LocalizedText UA_LOCALIZEDTEXT(char *locale, char *text);

  UA_LocalizedText UA_LOCALIZEDTEXT_ALLOC(const char *locale, const char *text);

UA_ExtensionObject
^^^^^^^^^^^^^^^^^^

A structure that contains an application specific data type that may not be
recognized by the receiver.

.. code-block:: c

  typedef struct {
      enum {
          UA_EXTENSIONOBJECT_ENCODED_NOBODY     = 0,
          UA_EXTENSIONOBJECT_ENCODED_BYTESTRING = 1,
          UA_EXTENSIONOBJECT_ENCODED_XML        = 2,
          UA_EXTENSIONOBJECT_DECODED            = 3, ///< There is a pointer to the decoded data
          UA_EXTENSIONOBJECT_DECODED_NODELETE   = 4  ///< Don't delete the decoded data at the lifecycle end
      } encoding;
      union {
          struct {
              UA_NodeId typeId; ///< The nodeid of the datatype
              UA_ByteString body; ///< The bytestring of the encoded data
          } encoded;
          struct {
              const UA_DataType *type;
              void *data;
          } decoded;
      } content;
  } UA_ExtensionObject;

UA_Variant
^^^^^^^^^^

Stores (arrays of) any data type. Please see section :ref:`generic-handling` for
the usage of UA_DataType. The semantics of the arrayLength field is explained in
section :ref:`array-handling`.

.. code-block:: c

  typedef struct {
      const UA_DataType *type; // The data type description
      enum {
          UA_VARIANT_DATA,          /* The data has the same lifecycle as the variant */
          UA_VARIANT_DATA_NODELETE, /* The data is "borrowed" by the variant and shall not be
                                       deleted at the end of the variant's lifecycle. */
      } storageType;
      size_t arrayLength;  // The number of elements in the data array
      void *data; // Points to the scalar or array data
      size_t arrayDimensionsSize; // The number of dimensions the data-array has
      UA_UInt32 *arrayDimensions; // The length of each dimension of the data-array
  } UA_Variant;

  /* NumericRanges are used to indicate subsets of a (multidimensional) variant
  * array. NumericRange has no official type structure in the standard. On the
  * wire, it only exists as an encoded string, such as "1:2,0:3,5". The colon
  * separates min/max index and the comma separates dimensions. A single value
  * indicates a range with a single element (min==max). */
  typedef struct {
      size_t dimensionsSize;
      struct UA_NumericRangeDimension {
          UA_UInt32 min;
          UA_UInt32 max;
      } *dimensions;
  } UA_NumericRange;


The following functions and definitions are used with UA_Variant.

.. code-block:: c

  /**
   * Returns true if the variant contains a scalar value. Note that empty
   * variants contain an array of length -1 (undefined).
   *
   * @param v The variant
   * @return Does the variant contain a scalar value.
   */
  UA_Boolean UA_Variant_isScalar(const UA_Variant *v);

  /**
   * Set the variant to a scalar value that already resides in memory. The value
   * takes on the lifecycle of the variant and is deleted with it.
   *
   * @param v The variant
   * @param p A pointer to the value data
   * @param type The datatype of the value in question
   */
  UA_Variant_setScalar(UA_Variant *v, void * UA_RESTRICT p, const UA_DataType *type);

  /**
   * Set the variant to a scalar value that is copied from an existing variable.
   *
   * @param v The variant
   * @param p A pointer to the value data
   * @param type The datatype of the value
   * @return Indicates whether the operation succeeded or returns an error code
   */
  UA_StatusCode UA_Variant_setScalarCopy(UA_Variant *v, const void *p, const UA_DataType *type);

  /**
   * Set the variant to an array that already resides in memory. The array takes
   * on the lifecycle of the variant and is deleted with it.
   *
   * @param v The variant
   * @param array A pointer to the array data
   * @param arraySize The size of the array
   * @param type The datatype of the array
   */
  void UA_Variant_setArray(UA_Variant *v, void * UA_RESTRICT array,
                           size_t arraySize, const UA_DataType *type);

  /**
   * Set the variant to an array that is copied from an existing array.
   *
   * @param v The variant
   * @param array A pointer to the array data
   * @param arraySize The size of the array
   * @param type The datatype of the array
   * @return Indicates whether the operation succeeded or returns an error code
   */
  UA_StatusCode UA_Variant_setArrayCopy(UA_Variant *v, const void *array,
                                        size_t arraySize, const UA_DataType *type);

  /**
   * Copy the variant, but use only a subset of the (multidimensional) array
   * into a variant. Returns an error code if the variant is not an array or if
   * the indicated range does not fit.
   *
   * @param src The source variant
   * @param dst The target variant
   * @param range The range of the copied data
   * @return Returns UA_STATUSCODE_GOOD or an error code
   */
  UA_StatusCode UA_Variant_copyRange(const UA_Variant *src, UA_Variant *dst,
                                     const UA_NumericRange range);

  /**
   * Insert a range of data into an existing variant. The data array can't be
   * reused afterwards if it contains types without a fixed size (e.g. strings)
   * since the members are moved into the variant and take on its lifecycle.
   *
   * @param v The variant
   * @param dataArray The data array. The type must match the variant
   * @param dataArraySize The length of the data array. This is checked to match the range size.
   * @param range The range of where the new data is inserted
   * @return Returns UA_STATUSCODE_GOOD or an error code
   */
  UA_StatusCode UA_Variant_setRange(UA_Variant *v, void * UA_RESTRICT array,
                                    size_t arraySize, const UA_NumericRange range);

  /**
   * Deep-copy a range of data into an existing variant.
   *
   * @param v The variant
   * @param dataArray The data array. The type must match the variant
   * @param dataArraySize The length of the data array. This is checked to match the range size.
   * @param range The range of where the new data is inserted
   * @return Returns UA_STATUSCODE_GOOD or an error code
   */
  UA_StatusCode UA_Variant_setRangeCopy(UA_Variant *v, const void *array,
                                        size_t arraySize, const UA_NumericRange range);

UA_DataValue
^^^^^^^^^^^^

A data value with an associated status code and timestamps.

.. code-block:: c
   
  typedef struct {
      UA_Boolean    hasValue             : 1;
      UA_Boolean    hasStatus            : 1;
      UA_Boolean    hasSourceTimestamp   : 1;
      UA_Boolean    hasServerTimestamp   : 1;
      UA_Boolean    hasSourcePicoseconds : 1;
      UA_Boolean    hasServerPicoseconds : 1;
      UA_Variant    value;
      UA_StatusCode status;
      UA_DateTime   sourceTimestamp;
      UA_Int16      sourcePicoseconds;
      UA_DateTime   serverTimestamp;
      UA_Int16      serverPicoseconds;
  } UA_DataValue;

UA_DiagnosticInfo
^^^^^^^^^^^^^^^^^

A structure that contains detailed error and diagnostic information associated
with a StatusCode.

.. code-block:: c

  typedef struct UA_DiagnosticInfo {
      UA_Boolean    hasSymbolicId          : 1;
      UA_Boolean    hasNamespaceUri        : 1;
      UA_Boolean    hasLocalizedText       : 1;
      UA_Boolean    hasLocale              : 1;
      UA_Boolean    hasAdditionalInfo      : 1;
      UA_Boolean    hasInnerStatusCode     : 1;
      UA_Boolean    hasInnerDiagnosticInfo : 1;
      UA_Int32      symbolicId;
      UA_Int32      namespaceUri;
      UA_Int32      localizedText;
      UA_Int32      locale;
      UA_String     additionalInfo;
      UA_StatusCode innerStatusCode;
      struct UA_DiagnosticInfo *innerDiagnosticInfo;
  } UA_DiagnosticInfo;

.. _generic-handling:

Generic Data Type Handling
--------------------------

All standard-defined data types are described with an ``UA_DataType`` structure.
In addition to the 25 builtin data types, OPC UA defines many more. But they are
mere combinations of the builtin data types. We handle all types in a unified
way by storing their internal structure. So it is not necessary to define
specialized functions for all additional types.

The ``UA_TYPES`` array contains the description of every standard-defined data
type.

.. code-block:: c

  extern const UA_DataType UA_TYPES[UA_TYPES_COUNT];

The following is an excerpt from ``ua_types_generated.h`` with the definition of
OPC UA read requests. This file is auto-generated from the XML-description of
the OPC UA data types that is part of the ISO/IEC 62541 standard.

.. code-block:: c

  typedef struct {
      UA_RequestHeader requestHeader;
      UA_Double maxAge;
      UA_TimestampsToReturn timestampsToReturn;
      size_t nodesToReadSize;
      UA_ReadValueId *nodesToRead;
  } UA_ReadRequest;

  #define UA_TYPES_READREQUEST 118
  
  static UA_INLINE void UA_ReadRequest_init(UA_ReadRequest *p) {
      memset(p, 0, sizeof(UA_ReadRequest)); }

  static UA_INLINE void UA_ReadRequest_delete(UA_ReadRequest *p) {
      UA_delete(p, &UA_TYPES[UA_TYPES_READREQUEST]); }

  static UA_INLINE void UA_ReadRequest_deleteMembers(UA_ReadRequest *p) {
      UA_deleteMembers(p, &UA_TYPES[UA_TYPES_READREQUEST]); }

  static UA_INLINE UA_ReadRequest * UA_ReadRequest_new(void) {
      return (UA_ReadRequest*) UA_new(&UA_TYPES[UA_TYPES_READREQUEST]); }

  static UA_INLINE UA_StatusCode
  UA_ReadRequest_copy(const UA_ReadRequest *src, UA_ReadRequest *dst) {
      return UA_copy(src, dst, &UA_TYPES[UA_TYPES_READREQUEST]); }

.. _array-handling:

Array Handling
--------------

In OPC UA, all arrays can be undefined, have length 0 or a positive length. In
data structures, all arrays are represented by a ``size_t`` length field and an
appropriate pointer directly afterwards. In order to distinguish between
undefined and length-zero arrays, we define the following.

.. code-block:: c

  #define UA_EMPTY_ARRAY_SENTINEL ((void*)0x01)

- size == 0 and data == NULL: The array is undefined
- size == 0 and data == ``UA_EMPTY_ARRAY_SENTINEL``: The array has length 0
- size > 0: The array at the given memory address has the given size

The following functions are defined for array handling.

.. code-block:: c

  /**
   * Allocates and initializes an array of variables of a specific type
   *
   * @param size The requested array length
   * @param type The datatype description
   * @return Returns the memory location of the variable or (void*)0 if no memory could be allocated
   */
  void * UA_Array_new(size_t size, const UA_DataType *type);

  /**
   * Allocates and copies an array. dst is set to (void*)0 if not enough memory is available.
   *
   * @param src The memory location of the source array
   * @param src_size The size of the array
   * @param dst The location of the pointer to the new array
   * @param type The datatype of the array members
   * @return Returns whether copying succeeded
   */
  UA_StatusCode UA_Array_copy(const void *src, size_t src_size, void **dst,
                              const UA_DataType *type);

  /**
   * Deletes an array.
   *
   * @param p The memory location of the array
   * @param size The size of the array
   * @param type The datatype of the array members
   */
  void UA_Array_delete(void *p, size_t size, const UA_DataType *type); 
