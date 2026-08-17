/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "eventloop_posix_http_parser.h"

#include <ctype.h>
#include <limits.h>

static void
clearBuffer(UA_ByteString *buffer) {
    UA_free(buffer->data);
    buffer->data = NULL;
    buffer->length = 0;
}

static UA_StatusCode
fail(UA_HTTPParser *parser, UA_HTTPParseDiagnostic diagnostic,
     UA_StatusCode status) {
    parser->diagnostic = diagnostic;
    return status;
}

static UA_Boolean
equalCaseInsensitive(const char *value, size_t valueLength, const char *text) {
    size_t length = strlen(text);
    if(valueLength != length)
        return false;
    for(size_t i = 0; i < length; i++) {
        if(tolower((unsigned char)value[i]) !=
           tolower((unsigned char)text[i]))
            return false;
    }
    return true;
}

static UA_Boolean
hasToken(const char *value, size_t valueLength, const char *token) {
    size_t offset = 0;
    while(offset < valueLength) {
        while(offset < valueLength &&
              (value[offset] == ' ' || value[offset] == '\t' ||
               value[offset] == ','))
            offset++;
        size_t end = offset;
        while(end < valueLength && value[end] != ',')
            end++;
        size_t trimmed = end;
        while(trimmed > offset &&
              (value[trimmed - 1] == ' ' || value[trimmed - 1] == '\t'))
            trimmed--;
        if(equalCaseInsensitive(value + offset, trimmed - offset, token))
            return true;
        offset = end + 1;
    }
    return false;
}

static const struct phr_header *
findHeader(const struct phr_header *headers, size_t headersSize,
           const char *name, size_t *matches) {
    const struct phr_header *result = NULL;
    *matches = 0;
    for(size_t i = 0; i < headersSize; i++) {
        if(!headers[i].name ||
           !equalCaseInsensitive(headers[i].name, headers[i].name_len, name))
            continue;
        (*matches)++;
        result = &headers[i];
    }
    return result;
}

static UA_StatusCode
parseContentLength(UA_HTTPParser *parser, const struct phr_header *header,
                   size_t *length) {
    if(!header || header->value_len == 0)
        return fail(parser, UA_HTTP_PARSE_DIAGNOSTIC_INVALID_CONTENT_LENGTH,
                    UA_STATUSCODE_BADDECODINGERROR);
    size_t result = 0;
    for(size_t i = 0; i < header->value_len; i++) {
        unsigned char c = (unsigned char)header->value[i];
        if(c < '0' || c > '9' || result > (SIZE_MAX - (size_t)(c - '0')) / 10)
            return fail(parser, UA_HTTP_PARSE_DIAGNOSTIC_INVALID_CONTENT_LENGTH,
                        UA_STATUSCODE_BADDECODINGERROR);
        result = result * 10 + (size_t)(c - '0');
    }
    *length = result;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
invokeBody(UA_HTTPParser *parser, const UA_HTTPParserCallbacks *callbacks,
           const UA_Byte *data, size_t length, UA_UInt32 maxMessageSize) {
    if(length > SIZE_MAX - parser->bodySize ||
       (maxMessageSize > 0 &&
        (length > maxMessageSize || parser->bodySize > maxMessageSize - length)))
        return fail(parser, UA_HTTP_PARSE_DIAGNOSTIC_MESSAGE_TOO_LARGE,
                    UA_STATUSCODE_BADREQUESTTOOLARGE);
    UA_StatusCode res = callbacks->body(callbacks->context, data, length);
    if(res != UA_STATUSCODE_GOOD)
        return fail(parser, res == UA_STATUSCODE_BADOUTOFMEMORY ?
                    UA_HTTP_PARSE_DIAGNOSTIC_OUT_OF_MEMORY :
                    UA_HTTP_PARSE_DIAGNOSTIC_CALLBACK_FAILED, res);
    parser->bodySize += length;
    return UA_STATUSCODE_GOOD;
}

static void
complete(UA_HTTPParser *parser, const UA_HTTPParserCallbacks *callbacks) {
    parser->complete = true;
    callbacks->complete(callbacks->context);
}

static UA_StatusCode
parseHeaders(UA_HTTPParser *parser, const UA_HTTPParserCallbacks *callbacks,
             UA_UInt32 maxMessageSize) {
    const char *method = NULL;
    const char *path = NULL;
    size_t methodLength = 0;
    size_t pathLength = 0;
    int minorVersion = -1;
    struct phr_header headers[UA_HTTP_MAX_HEADERS];
    size_t headersSize = UA_HTTP_MAX_HEADERS;
    int parsed = phr_parse_request((const char*)parser->headerBuffer.data,
                                   parser->headerSize, &method, &methodLength,
                                   &path, &pathLength, &minorVersion, headers,
                                   &headersSize, 0);
    if(parsed == -1 && headersSize == UA_HTTP_MAX_HEADERS)
        return fail(parser, UA_HTTP_PARSE_DIAGNOSTIC_TOO_MANY_HEADERS,
                    UA_STATUSCODE_BADDECODINGERROR);
    if(parsed < 0 || (size_t)parsed != parser->headerSize ||
       (minorVersion != 0 && minorVersion != 1))
        return fail(parser, UA_HTTP_PARSE_DIAGNOSTIC_INVALID_HEADERS,
                    UA_STATUSCODE_BADDECODINGERROR);
    for(size_t i = 0; i < headersSize; i++) {
        if(!headers[i].name)
            return fail(parser, UA_HTTP_PARSE_DIAGNOSTIC_OBSOLETE_LINE_FOLDING,
                        UA_STATUSCODE_BADDECODINGERROR);
    }

    size_t contentLengthCount, transferEncodingCount, connectionCount, hostCount;
    const struct phr_header *contentLength =
        findHeader(headers, headersSize, "content-length", &contentLengthCount);
    const struct phr_header *transferEncoding =
        findHeader(headers, headersSize, "transfer-encoding", &transferEncodingCount);
    const struct phr_header *connection =
        findHeader(headers, headersSize, "connection", &connectionCount);
    const struct phr_header *host = findHeader(headers, headersSize, "host", &hostCount);
    (void)connectionCount;
    UA_Boolean closeAfterResponse = (minorVersion == 0);
    if(connection && hasToken(connection->value, connection->value_len, "close"))
        closeAfterResponse = true;
    else if(connection && hasToken(connection->value, connection->value_len,
                                   "keep-alive"))
        closeAfterResponse = false;

    /* Headers represent the start of the current request. Report them before
     * applying body-framing policy so the lifecycle is balanced even when a
     * syntactically valid request must subsequently be rejected. */
    UA_StatusCode res = callbacks->headers(
        callbacks->context, method, methodLength, path, pathLength, headers,
        headersSize, closeAfterResponse);
    if(res != UA_STATUSCODE_GOOD)
        return fail(parser, res == UA_STATUSCODE_BADOUTOFMEMORY ?
                    UA_HTTP_PARSE_DIAGNOSTIC_OUT_OF_MEMORY :
                    UA_HTTP_PARSE_DIAGNOSTIC_CALLBACK_FAILED, res);

    if(contentLengthCount > 1 || transferEncodingCount > 1 ||
       (contentLength && transferEncoding))
        return fail(parser, UA_HTTP_PARSE_DIAGNOSTIC_AMBIGUOUS_FRAMING,
                    UA_STATUSCODE_BADDECODINGERROR);
    if(minorVersion == 1 &&
       (hostCount != 1 || !host || host->value_len == 0))
        return fail(parser, UA_HTTP_PARSE_DIAGNOSTIC_MISSING_HOST,
                    UA_STATUSCODE_BADDECODINGERROR);
    if(transferEncoding &&
       !equalCaseInsensitive(transferEncoding->value,
                             transferEncoding->value_len, "chunked"))
        return fail(parser,
                    UA_HTTP_PARSE_DIAGNOSTIC_UNSUPPORTED_TRANSFER_ENCODING,
                    UA_STATUSCODE_BADDECODINGERROR);

    if(contentLength) {
        res = parseContentLength(parser, contentLength, &parser->bodyRemaining);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        if(maxMessageSize > 0 && parser->bodyRemaining > maxMessageSize)
            return fail(parser, UA_HTTP_PARSE_DIAGNOSTIC_MESSAGE_TOO_LARGE,
                        UA_STATUSCODE_BADREQUESTTOOLARGE);
    }

    if(transferEncoding) {
        memset(&parser->chunkedDecoder, 0, sizeof(parser->chunkedDecoder));
        parser->chunkedDecoder.consume_trailer = 1;
        parser->state = UA_HTTP_PARSE_CHUNKED_BODY;
    } else if(parser->bodyRemaining > 0) {
        parser->state = UA_HTTP_PARSE_FIXED_BODY;
    } else {
        complete(parser, callbacks);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
appendHeaderBytes(UA_HTTPParser *parser, const UA_Byte **data, size_t *length,
                  UA_Boolean *completeHeader) {
    *completeHeader = false;
    while(*length > 0) {
        if(parser->headerSize >= UA_HTTP_MAX_HEADER_SIZE)
            return fail(parser, UA_HTTP_PARSE_DIAGNOSTIC_HEADER_TOO_LARGE,
                        UA_STATUSCODE_BADREQUESTHEADERINVALID);
        size_t required = parser->headerSize + 1;
        if(required > parser->headerBuffer.length) {
            size_t capacity = parser->headerBuffer.length > 0 ?
                parser->headerBuffer.length * 2 : 1024;
            if(capacity > UA_HTTP_MAX_HEADER_SIZE)
                capacity = UA_HTTP_MAX_HEADER_SIZE;
            UA_Byte *buffer = (UA_Byte*)UA_realloc(parser->headerBuffer.data,
                                                   capacity);
            if(!buffer)
                return fail(parser, UA_HTTP_PARSE_DIAGNOSTIC_OUT_OF_MEMORY,
                            UA_STATUSCODE_BADOUTOFMEMORY);
            parser->headerBuffer.data = buffer;
            parser->headerBuffer.length = capacity;
        }
        parser->headerBuffer.data[parser->headerSize++] = **data;
        (*data)++;
        (*length)--;
        if(parser->headerSize >= 4) {
            UA_Byte *tail = &parser->headerBuffer.data[parser->headerSize - 4];
            if(tail[0] == '\r' && tail[1] == '\n' &&
               tail[2] == '\r' && tail[3] == '\n') {
                *completeHeader = true;
                return UA_STATUSCODE_GOOD;
            }
        }
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
processChunked(UA_HTTPParser *parser, const UA_Byte *input, size_t inputLength,
               UA_UInt32 maxMessageSize,
               const UA_HTTPParserCallbacks *callbacks) {
    if(inputLength > SIZE_MAX - parser->chunkedInputSize)
        return fail(parser, UA_HTTP_PARSE_DIAGNOSTIC_MESSAGE_TOO_LARGE,
                    UA_STATUSCODE_BADOUTOFRANGE);
    parser->chunkedInputSize += inputLength;
    if(parser->chunkBuffer.length < inputLength) {
        UA_Byte *buffer = (UA_Byte*)UA_realloc(parser->chunkBuffer.data,
                                               inputLength);
        if(!buffer)
            return fail(parser, UA_HTTP_PARSE_DIAGNOSTIC_OUT_OF_MEMORY,
                        UA_STATUSCODE_BADOUTOFMEMORY);
        parser->chunkBuffer.data = buffer;
        parser->chunkBuffer.length = inputLength;
    }
    memcpy(parser->chunkBuffer.data, input, inputLength);
    size_t decodedLength = inputLength;
    ssize_t remaining = phr_decode_chunked(&parser->chunkedDecoder,
        (char*)parser->chunkBuffer.data, &decodedLength);
    if(remaining < -2 || remaining == -1)
        return fail(parser, UA_HTTP_PARSE_DIAGNOSTIC_INVALID_CHUNKED_BODY,
                    UA_STATUSCODE_BADDECODINGERROR);
    UA_StatusCode res = invokeBody(parser, callbacks, parser->chunkBuffer.data,
                                   decodedLength, maxMessageSize);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(parser->chunkedInputSize - parser->bodySize > UA_HTTP_MAX_HEADER_SIZE)
        return fail(parser,
                    UA_HTTP_PARSE_DIAGNOSTIC_CHUNK_OVERHEAD_TOO_LARGE,
                    UA_STATUSCODE_BADREQUESTHEADERINVALID);
    if(remaining == -2)
        return UA_STATUSCODE_GOOD;
    if(remaining > 0)
        return fail(parser,
                    UA_HTTP_PARSE_DIAGNOSTIC_PIPELINING_NOT_SUPPORTED,
                    UA_STATUSCODE_BADINVALIDSTATE);
    complete(parser, callbacks);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_HTTPParser_process(UA_HTTPParser *parser, const void *input,
                      size_t inputLength, UA_UInt32 maxMessageSize,
                      const UA_HTTPParserCallbacks *callbacks) {
    if(!parser || (!input && inputLength > 0) || !callbacks ||
       !callbacks->headers || !callbacks->body || !callbacks->complete)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    const UA_Byte *data = (const UA_Byte*)input;
    size_t length = inputLength;
    while(length > 0) {
        if(parser->complete)
            return fail(parser,
                        UA_HTTP_PARSE_DIAGNOSTIC_PIPELINING_NOT_SUPPORTED,
                        UA_STATUSCODE_BADINVALIDSTATE);
        switch(parser->state) {
        case UA_HTTP_PARSE_HEADERS: {
            UA_Boolean completeHeader;
            UA_StatusCode res = appendHeaderBytes(parser, &data, &length,
                                                   &completeHeader);
            if(res != UA_STATUSCODE_GOOD || !completeHeader)
                return res;
            res = parseHeaders(parser, callbacks, maxMessageSize);
            clearBuffer(&parser->headerBuffer);
            parser->headerSize = 0;
            if(res != UA_STATUSCODE_GOOD)
                return res;
            break;
        }
        case UA_HTTP_PARSE_FIXED_BODY: {
            size_t amount = length < parser->bodyRemaining ? length :
                parser->bodyRemaining;
            UA_StatusCode res = invokeBody(parser, callbacks, data, amount,
                                           maxMessageSize);
            if(res != UA_STATUSCODE_GOOD)
                return res;
            data += amount;
            length -= amount;
            parser->bodyRemaining -= amount;
            if(parser->bodyRemaining == 0)
                complete(parser, callbacks);
            break;
        }
        case UA_HTTP_PARSE_CHUNKED_BODY:
            return processChunked(parser, data, length, maxMessageSize,
                                  callbacks);
        }
    }
    return UA_STATUSCODE_GOOD;
}

void
UA_HTTPParser_reset(UA_HTTPParser *parser) {
    clearBuffer(&parser->headerBuffer);
    parser->headerSize = 0;
    parser->bodyRemaining = 0;
    parser->bodySize = 0;
    parser->chunkedInputSize = 0;
    memset(&parser->chunkedDecoder, 0, sizeof(parser->chunkedDecoder));
    parser->state = UA_HTTP_PARSE_HEADERS;
    parser->diagnostic = UA_HTTP_PARSE_DIAGNOSTIC_NONE;
    parser->complete = false;
}

void
UA_HTTPParser_clear(UA_HTTPParser *parser) {
    UA_HTTPParser_reset(parser);
    clearBuffer(&parser->chunkBuffer);
}

const char *
UA_HTTPParser_diagnosticName(UA_HTTPParseDiagnostic diagnostic) {
    switch(diagnostic) {
    case UA_HTTP_PARSE_DIAGNOSTIC_NONE: return "none";
    case UA_HTTP_PARSE_DIAGNOSTIC_HEADER_TOO_LARGE: return "header too large";
    case UA_HTTP_PARSE_DIAGNOSTIC_INVALID_HEADERS: return "invalid request headers";
    case UA_HTTP_PARSE_DIAGNOSTIC_TOO_MANY_HEADERS: return "too many headers";
    case UA_HTTP_PARSE_DIAGNOSTIC_OBSOLETE_LINE_FOLDING: return "obsolete header folding";
    case UA_HTTP_PARSE_DIAGNOSTIC_AMBIGUOUS_FRAMING: return "ambiguous message framing";
    case UA_HTTP_PARSE_DIAGNOSTIC_MISSING_HOST: return "missing HTTP/1.1 Host header";
    case UA_HTTP_PARSE_DIAGNOSTIC_UNSUPPORTED_TRANSFER_ENCODING: return "unsupported transfer encoding";
    case UA_HTTP_PARSE_DIAGNOSTIC_INVALID_CONTENT_LENGTH: return "invalid Content-Length";
    case UA_HTTP_PARSE_DIAGNOSTIC_MESSAGE_TOO_LARGE: return "message too large";
    case UA_HTTP_PARSE_DIAGNOSTIC_INVALID_CHUNKED_BODY: return "invalid chunked body";
    case UA_HTTP_PARSE_DIAGNOSTIC_CHUNK_OVERHEAD_TOO_LARGE: return "chunk metadata too large";
    case UA_HTTP_PARSE_DIAGNOSTIC_PIPELINING_NOT_SUPPORTED: return "HTTP pipelining is not supported";
    case UA_HTTP_PARSE_DIAGNOSTIC_CALLBACK_FAILED: return "request callback failed";
    case UA_HTTP_PARSE_DIAGNOSTIC_OUT_OF_MEMORY: return "out of memory";
    default: return "unknown parser error";
    }
}
