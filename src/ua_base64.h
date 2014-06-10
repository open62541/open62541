/*
 * ua_base64.h
 *
 *  Created on: Jun 10, 2014
 *      Author: sten
 */

#ifndef UA_BASE64_H_
#define UA_BASE64_H_

#include "ua_basictypes.h"

/** @brief calculates the exact size for the binary data that is encoded in base64 encoded string */
UA_Int32 UA_base64_getDecodedSize(UA_String* const base64EncodedData);

/** @brief decodes base64 encoded string into target, target should be at least of the size returned by UA_base64_getDecodedSizeUB */
UA_Int32 UA_base64_decode(UA_String* const base64EncodedData, UA_Byte* target);

#endif /* UA_BASE64_H_ */
