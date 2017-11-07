/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ua_types.h>
#include "ua_server_internal.h"
#include "ua_config_default.h"
#include "ua_log_stdout.h"
#include "ua_types_encoding_binary.h"

/*
** Main entry point.  The fuzzer invokes this function with each
** fuzzed input.
*/
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {

	if (size == 0)
		return 0;

	const uint8_t *ptr = data;
	size_t ptrSize = size;

	// get some random type
	uint16_t typeIndex = ptr[0];
	ptr++;
	ptrSize--;

	if (typeIndex >= UA_TYPES_COUNT)
		return 0;

	size_t offset = 0;
	if (ptrSize >= sizeof(size_t)) {
		offset = (*ptr);
		ptr += sizeof(size_t);
		ptrSize -= sizeof(size_t);
	}

	void *dst = UA_new(&UA_TYPES[typeIndex]);

	const UA_ByteString binary = {
			ptrSize, //length
			(UA_Byte *)(void *)ptr //data
	};

	UA_StatusCode ret = UA_decodeBinary(&binary, &offset, dst, &UA_TYPES[typeIndex], 0, nullptr);
	if (ret == UA_STATUSCODE_GOOD) {
		//do nothing
	}
	UA_delete(dst, &UA_TYPES[typeIndex]);

	return 0;
}
