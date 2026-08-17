/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#ifndef EVENTLOOP_POSIX_HTTP_PARSER_H_
#define EVENTLOOP_POSIX_HTTP_PARSER_H_

#include <open62541/types.h>

#include "picohttpparser.h"

#define UA_HTTP_MAX_HEADER_SIZE 65536
#define UA_HTTP_MAX_HEADERS 128

typedef enum {
    UA_HTTP_PARSE_HEADERS,
    UA_HTTP_PARSE_FIXED_BODY,
    UA_HTTP_PARSE_CHUNKED_BODY
} UA_HTTPParseState;

typedef enum {
    UA_HTTP_PARSE_DIAGNOSTIC_NONE,
    UA_HTTP_PARSE_DIAGNOSTIC_HEADER_TOO_LARGE,
    UA_HTTP_PARSE_DIAGNOSTIC_INVALID_HEADERS,
    UA_HTTP_PARSE_DIAGNOSTIC_TOO_MANY_HEADERS,
    UA_HTTP_PARSE_DIAGNOSTIC_OBSOLETE_LINE_FOLDING,
    UA_HTTP_PARSE_DIAGNOSTIC_AMBIGUOUS_FRAMING,
    UA_HTTP_PARSE_DIAGNOSTIC_MISSING_HOST,
    UA_HTTP_PARSE_DIAGNOSTIC_UNSUPPORTED_TRANSFER_ENCODING,
    UA_HTTP_PARSE_DIAGNOSTIC_INVALID_CONTENT_LENGTH,
    UA_HTTP_PARSE_DIAGNOSTIC_MESSAGE_TOO_LARGE,
    UA_HTTP_PARSE_DIAGNOSTIC_INVALID_CHUNKED_BODY,
    UA_HTTP_PARSE_DIAGNOSTIC_CHUNK_OVERHEAD_TOO_LARGE,
    UA_HTTP_PARSE_DIAGNOSTIC_PIPELINING_NOT_SUPPORTED,
    UA_HTTP_PARSE_DIAGNOSTIC_CALLBACK_FAILED,
    UA_HTTP_PARSE_DIAGNOSTIC_OUT_OF_MEMORY
} UA_HTTPParseDiagnostic;

typedef struct {
    UA_ByteString headerBuffer;
    size_t headerSize;
    UA_ByteString chunkBuffer;
    size_t bodyRemaining;
    size_t bodySize;
    size_t chunkedInputSize;
    struct phr_chunked_decoder chunkedDecoder;
    UA_HTTPParseState state;
    UA_HTTPParseDiagnostic diagnostic;
    UA_Boolean complete;
} UA_HTTPParser;

typedef struct {
    void *context;
    UA_StatusCode (*headers)(void *context, const char *method,
                             size_t methodLength, const char *path,
                             size_t pathLength,
                             const struct phr_header *headers,
                             size_t headersSize,
                             UA_Boolean closeAfterResponse);
    UA_StatusCode (*body)(void *context, const UA_Byte *data, size_t length);
    void (*complete)(void *context);
} UA_HTTPParserCallbacks;

void UA_HTTPParser_reset(UA_HTTPParser *parser);
void UA_HTTPParser_clear(UA_HTTPParser *parser);
UA_StatusCode UA_HTTPParser_process(UA_HTTPParser *parser, const void *input,
                                    size_t inputLength,
                                    UA_UInt32 maxMessageSize,
                                    const UA_HTTPParserCallbacks *callbacks);
const char *UA_HTTPParser_diagnosticName(UA_HTTPParseDiagnostic diagnostic);

#endif /* EVENTLOOP_POSIX_HTTP_PARSER_H_ */
