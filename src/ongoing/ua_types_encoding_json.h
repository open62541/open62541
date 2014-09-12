#ifndef UA_TYPES_ENCODING_JSON_H_
#define UA_TYPES_ENCODING_JSON_H_

#include "util/ua_util.h"
#include "ua_types.h"

/**
 * @ingroup encoding
 * @defgroup encoding_json JSON Encoding
 *
 * @brief Encoding of UA datatypes in JSON. This extends the IEC 62541 standard
 * and is only included in the "extended" profile. The extension is intended to
 * be used with a webserver that transmits JSON over HTTP.
 *
 * @{
 */

#define UA_TYPE_CALCSIZEJSON_AS(TYPE, TYPE_AS)       \
    UA_Int32 TYPE##_calcSizeJSON(TYPE const *p) {    \
		return TYPE_AS##_calcSizeJSON((TYPE_AS *)p); \
	}

#define UA_TYPE_ENCODEJSON_AS(TYPE, TYPE_AS)                                             \
    UA_Int32 TYPE##_encodeJSON(TYPE const *src, UA_ByteString *dst, UA_UInt32 *offset) { \
		return TYPE_AS##_encodeJSON((TYPE_AS *)src, dst, offset);                        \
	}

#define UA_TYPE_DECODEJSON_AS(TYPE, TYPE_AS)                                             \
    UA_Int32 TYPE##_decodeJSON(UA_ByteString const *src, UA_UInt32 *offset, TYPE *dst) { \
		return TYPE_AS##_decodeJSON(src, offset, (TYPE_AS *)dst);                        \
	}
#define UA_TYPE_JSON_ENCODING_AS(TYPE, TYPE_AS) \
    UA_TYPE_CALCSIZEJSON_AS(TYPE, TYPE_AS)      \
    UA_TYPE_ENCODEJSON_AS(TYPE, TYPE_AS)        \
    UA_TYPE_DECODEJSON_AS(TYPE, TYPE_AS)

#define UA_TYPE_JSON_ENCODING(TYPE)                                                     \
    UA_Int32 TYPE##_calcSizeJSON(TYPE const *p);                                        \
    UA_Int32 TYPE##_encodeJSON(TYPE const *src, UA_ByteString *dst, UA_UInt32 *offset); \
    UA_Int32 TYPE##_decodeJSON(UA_ByteString const *src, UA_UInt32 *offset, TYPE *dst);

UA_TYPE_JSON_ENCODING(UA_Boolean)
UA_TYPE_JSON_ENCODING(UA_SByte)
UA_TYPE_JSON_ENCODING(UA_Byte)
UA_TYPE_JSON_ENCODING(UA_Int16)
UA_TYPE_JSON_ENCODING(UA_UInt16)
UA_TYPE_JSON_ENCODING(UA_Int32)
UA_TYPE_JSON_ENCODING(UA_UInt32)
UA_TYPE_JSON_ENCODING(UA_Int64)
UA_TYPE_JSON_ENCODING(UA_UInt64)
UA_TYPE_JSON_ENCODING(UA_Float)
UA_TYPE_JSON_ENCODING(UA_Double)
UA_TYPE_JSON_ENCODING(UA_String)
UA_TYPE_JSON_ENCODING(UA_DateTime)
UA_TYPE_JSON_ENCODING(UA_Guid)
UA_TYPE_JSON_ENCODING(UA_ByteString)
UA_TYPE_JSON_ENCODING(UA_XmlElement)
UA_TYPE_JSON_ENCODING(UA_NodeId)
UA_TYPE_JSON_ENCODING(UA_ExpandedNodeId)
UA_TYPE_JSON_ENCODING(UA_StatusCode)
UA_TYPE_JSON_ENCODING(UA_QualifiedName)
UA_TYPE_JSON_ENCODING(UA_LocalizedText)
UA_TYPE_JSON_ENCODING(UA_ExtensionObject)
UA_TYPE_JSON_ENCODING(UA_DataValue)
UA_TYPE_JSON_ENCODING(UA_Variant)
UA_TYPE_JSON_ENCODING(UA_DiagnosticInfo)

/* Not built-in types */
UA_TYPE_JSON_ENCODING(UA_InvalidType)

/// @} /* end of group */

#endif /* UA_TYPES_ENCODING_JSON_H_ */
