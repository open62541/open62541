#ifndef UA_TYPES_ENCODING_BINARY_H_
#define UA_TYPES_ENCODING_BINARY_H_

#include "util/ua_util.h"
#include "ua_types.h"

/**
 * @ingroup types
 * @defgroup encoding Datatype Encoding
 *
 * @brief Datatypes can have several encodings. The methods defined for
 * encodings and their signature are fixed.
 *
 * - CalcSize: Returns the size of the (encoded) variable in bytes. This
 *    function is mainly used to allocate the bytestring into which the encoding
 *    is done.
 *
 * - Encode: Encodes a variable into a bytestring.
 *
 * - Decode: Decodes a variable stored in a bytestring. It is assumed that the
 *   input variable (dst) is allocated in memory, but not its dynamically sized
 *   members.
 */

/**
 * @ingroup encoding
 * @defgroup encoding_binary Binary Encoding
 *
 * @brief Functions for binary en- and decoding of built-in datatypes as defined
 * in the standard.
 *
 * @{
 */

/* Stop decoding at the first failure. Free members that were already allocated.
   It is assumed that retval is already defined. */
#define CHECKED_DECODE(DECODE, CLEAN_UP) do { \
		retval |= DECODE;                     \
		if(retval != UA_SUCCESS) {            \
			CLEAN_UP;                         \
			return retval;                    \
		} } while(0)

#define UA_TYPE_CALCSIZEBINARY_AS(TYPE, TYPE_AS)       \
    UA_Int32 TYPE##_calcSizeBinary(TYPE const *p) {    \
		return TYPE_AS##_calcSizeBinary((TYPE_AS *)p); \
	}

#define UA_TYPE_ENCODEBINARY_AS(TYPE, TYPE_AS)                                             \
    UA_Int32 TYPE##_encodeBinary(TYPE const *src, UA_ByteString *dst, UA_UInt32 *offset) { \
		return TYPE_AS##_encodeBinary((TYPE_AS *)src, dst, offset);                        \
	}

#define UA_TYPE_DECODEBINARY_AS(TYPE, TYPE_AS)                                             \
    UA_Int32 TYPE##_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, TYPE *dst) { \
		return TYPE_AS##_decodeBinary(src, offset, (TYPE_AS *)dst);                        \
	}
#define UA_TYPE_BINARY_ENCODING_AS(TYPE, TYPE_AS) \
    UA_TYPE_CALCSIZEBINARY_AS(TYPE, TYPE_AS)      \
    UA_TYPE_ENCODEBINARY_AS(TYPE, TYPE_AS)        \
    UA_TYPE_DECODEBINARY_AS(TYPE, TYPE_AS)

#define UA_TYPE_BINARY_ENCODING(TYPE)                                                     \
    UA_Int32 TYPE##_calcSizeBinary(TYPE const *p);                                        \
    UA_Int32 TYPE##_encodeBinary(TYPE const *src, UA_ByteString *dst, UA_UInt32 *offset); \
    UA_Int32 TYPE##_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, TYPE *dst);

UA_TYPE_BINARY_ENCODING(UA_Boolean)
UA_TYPE_BINARY_ENCODING(UA_SByte)
UA_TYPE_BINARY_ENCODING(UA_Byte)
UA_TYPE_BINARY_ENCODING(UA_Int16)
UA_TYPE_BINARY_ENCODING(UA_UInt16)
UA_TYPE_BINARY_ENCODING(UA_Int32)
UA_TYPE_BINARY_ENCODING(UA_UInt32)
UA_TYPE_BINARY_ENCODING(UA_Int64)
UA_TYPE_BINARY_ENCODING(UA_UInt64)
UA_TYPE_BINARY_ENCODING(UA_Float)
UA_TYPE_BINARY_ENCODING(UA_Double)
UA_TYPE_BINARY_ENCODING(UA_String)
UA_TYPE_BINARY_ENCODING(UA_DateTime)
UA_TYPE_BINARY_ENCODING(UA_Guid)
UA_TYPE_BINARY_ENCODING(UA_ByteString)
UA_TYPE_BINARY_ENCODING(UA_XmlElement)
UA_TYPE_BINARY_ENCODING(UA_NodeId)
UA_TYPE_BINARY_ENCODING(UA_ExpandedNodeId)
UA_TYPE_BINARY_ENCODING(UA_StatusCode)
UA_TYPE_BINARY_ENCODING(UA_QualifiedName)
UA_TYPE_BINARY_ENCODING(UA_LocalizedText)
UA_TYPE_BINARY_ENCODING(UA_ExtensionObject)
UA_TYPE_BINARY_ENCODING(UA_DataValue)
UA_TYPE_BINARY_ENCODING(UA_Variant)
UA_TYPE_BINARY_ENCODING(UA_DiagnosticInfo)

/* Not built-in types */
UA_TYPE_BINARY_ENCODING(UA_InvalidType)

/*********/
/* Array */
/*********/

/* Computes the size of an array (incl. length field) in a binary blob. */
UA_Int32 UA_Array_calcSizeBinary(UA_Int32 nElements, UA_VTable_Entry *vt, const void *data);

/* @brief Encodes an array into a binary blob. The array size is printed as well. */
UA_Int32 UA_Array_encodeBinary(const void *src, UA_Int32 noElements, UA_VTable_Entry *vt, UA_ByteString *dst, UA_UInt32 *offset);

/* @brief Decodes an array from a binary blob. The array is allocated automatically before decoding. */
UA_Int32 UA_Array_decodeBinary(const UA_ByteString *src, UA_UInt32 * offset, UA_Int32 noElements, UA_VTable_Entry *vt, void **dst);

/// @} /* end of group */

#endif /* UA_TYPES_ENCODING_BINARY_H_ */
