/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef EVENTLOOP_POSIX_HTTP_COMPRESSION_H_
#define EVENTLOOP_POSIX_HTTP_COMPRESSION_H_

#include <open62541/types.h>

typedef enum {
    UA_HTTP_CONTENT_ENCODING_IDENTITY,
    UA_HTTP_CONTENT_ENCODING_GZIP,
    UA_HTTP_CONTENT_ENCODING_DEFLATE,
    UA_HTTP_CONTENT_ENCODING_UNSUPPORTED
} UA_HTTPContentEncoding;

typedef struct {
    UA_HTTPContentEncoding encoding;
    UA_Boolean identityAllowed;
    UA_Boolean gzipAllowed;
    UA_Boolean acceptable;
} UA_HTTPCompressionPreference;

UA_HTTPContentEncoding
UA_HTTP_parseContentEncoding(const UA_String *value);

UA_HTTPCompressionPreference
UA_HTTP_selectResponseEncoding(const UA_String *acceptEncoding);

const char *
UA_HTTP_contentEncodingName(UA_HTTPContentEncoding encoding);

UA_StatusCode
UA_HTTP_compress(UA_HTTPContentEncoding encoding, const UA_ByteString *input,
                 UA_ByteString *output);

UA_StatusCode
UA_HTTP_decompress(UA_HTTPContentEncoding encoding, const UA_ByteString *input,
                   size_t maxOutputSize, UA_ByteString *output);

#endif /* EVENTLOOP_POSIX_HTTP_COMPRESSION_H_ */
