/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "eventloop_posix_http_compression.h"

#include <ctype.h>
#include <limits.h>
#include <string.h>

#ifdef UA_ENABLE_HTTP_COMPRESSION
# include <zlib.h>
#endif

static UA_Boolean
equalToken(const UA_Byte *data, size_t length, const char *literal) {
    size_t literalLength = strlen(literal);
    if(length != literalLength)
        return false;
    for(size_t i = 0; i < length; i++) {
        if(tolower((unsigned char)data[i]) !=
           tolower((unsigned char)literal[i]))
            return false;
    }
    return true;
}

static void
trim(const UA_Byte **data, size_t *length) {
    while(*length > 0 && isspace((unsigned char)(*data)[0])) {
        (*data)++;
        (*length)--;
    }
    while(*length > 0 && isspace((unsigned char)(*data)[*length - 1]))
        (*length)--;
}

UA_HTTPContentEncoding
UA_HTTP_parseContentEncoding(const UA_String *value) {
    if(!value || value->length == 0)
        return UA_HTTP_CONTENT_ENCODING_IDENTITY;
    const UA_Byte *data = value->data;
    size_t length = value->length;
    trim(&data, &length);
    if(equalToken(data, length, "identity"))
        return UA_HTTP_CONTENT_ENCODING_IDENTITY;
#ifdef UA_ENABLE_HTTP_COMPRESSION
    if(equalToken(data, length, "gzip") || equalToken(data, length, "x-gzip"))
        return UA_HTTP_CONTENT_ENCODING_GZIP;
    if(equalToken(data, length, "deflate"))
        return UA_HTTP_CONTENT_ENCODING_DEFLATE;
#endif
    return UA_HTTP_CONTENT_ENCODING_UNSUPPORTED;
}

/* Parse an RFC 9110 qvalue into thousandths. Invalid values are rejected. */
static int
parseQuality(const UA_Byte *data, size_t length) {
    trim(&data, &length);
    if(length == 0 || (data[0] != '0' && data[0] != '1'))
        return -1;
    int quality = data[0] == '1' ? 1000 : 0;
    if(length == 1)
        return quality;
    if(data[1] != '.' || length > 5)
        return -1;
    int factor = 100;
    for(size_t i = 2; i < length; i++, factor /= 10) {
        if(!isdigit((unsigned char)data[i]))
            return -1;
        if(data[0] == '1' && data[i] != '0')
            return -1;
        quality += (data[i] - '0') * factor;
    }
    return quality;
}

UA_HTTPCompressionPreference
UA_HTTP_selectResponseEncoding(const UA_String *acceptEncoding) {
    UA_HTTPCompressionPreference result = {
        UA_HTTP_CONTENT_ENCODING_IDENTITY, true, false, true
    };
    if(!acceptEncoding || acceptEncoding->length == 0)
        return result;

    int gzipQuality = -1;
    int deflateQuality = -1;
    int identityQuality = -1;
    int wildcardQuality = -1;
    size_t offset = 0;
    while(offset < acceptEncoding->length) {
        size_t end = offset;
        while(end < acceptEncoding->length && acceptEncoding->data[end] != ',')
            end++;
        const UA_Byte *item = &acceptEncoding->data[offset];
        size_t itemLength = end - offset;
        trim(&item, &itemLength);

        size_t semicolon = 0;
        while(semicolon < itemLength && item[semicolon] != ';')
            semicolon++;
        const UA_Byte *token = item;
        size_t tokenLength = semicolon;
        trim(&token, &tokenLength);
        int quality = 1000;
        if(semicolon < itemLength) {
            const UA_Byte *parameter = &item[semicolon + 1];
            size_t parameterLength = itemLength - semicolon - 1;
            trim(&parameter, &parameterLength);
            if(parameterLength < 2 ||
               tolower((unsigned char)parameter[0]) != 'q' ||
               parameter[1] != '=')
                quality = 0;
            else {
                quality = parseQuality(&parameter[2], parameterLength - 2);
                if(quality < 0)
                    quality = 0;
            }
        }

        if(equalToken(token, tokenLength, "gzip") ||
           equalToken(token, tokenLength, "x-gzip"))
            gzipQuality = quality;
        else if(equalToken(token, tokenLength, "deflate"))
            deflateQuality = quality;
        else if(equalToken(token, tokenLength, "identity"))
            identityQuality = quality;
        else if(equalToken(token, tokenLength, "*"))
            wildcardQuality = quality;
        offset = end + 1;
    }

    if(identityQuality < 0)
        identityQuality = wildcardQuality == 0 ? 0 : 1000;
#ifdef UA_ENABLE_HTTP_COMPRESSION
    if(gzipQuality < 0)
        gzipQuality = wildcardQuality;
    if(deflateQuality < 0)
        deflateQuality = wildcardQuality;
#else
    gzipQuality = -1;
    deflateQuality = -1;
#endif

    result.identityAllowed = identityQuality > 0;
    result.gzipAllowed = gzipQuality > 0;
    int selectedQuality = identityQuality;
    if(gzipQuality > 0 && gzipQuality >= selectedQuality) {
        result.encoding = UA_HTTP_CONTENT_ENCODING_GZIP;
        selectedQuality = gzipQuality;
    }
    if(deflateQuality > selectedQuality ||
       (deflateQuality == selectedQuality && deflateQuality > 0 &&
        result.encoding == UA_HTTP_CONTENT_ENCODING_IDENTITY)) {
        result.encoding = UA_HTTP_CONTENT_ENCODING_DEFLATE;
        selectedQuality = deflateQuality;
    }
    result.acceptable = selectedQuality > 0;
    return result;
}

const char *
UA_HTTP_contentEncodingName(UA_HTTPContentEncoding encoding) {
    switch(encoding) {
    case UA_HTTP_CONTENT_ENCODING_GZIP: return "gzip";
    case UA_HTTP_CONTENT_ENCODING_DEFLATE: return "deflate";
    case UA_HTTP_CONTENT_ENCODING_IDENTITY: return "identity";
    default: return NULL;
    }
}

UA_StatusCode
UA_HTTP_compress(UA_HTTPContentEncoding encoding, const UA_ByteString *input,
                 UA_ByteString *output) {
    if(!input || !output || encoding == UA_HTTP_CONTENT_ENCODING_IDENTITY)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
#ifndef UA_ENABLE_HTTP_COMPRESSION
    (void)encoding;
    return UA_STATUSCODE_BADNOTSUPPORTED;
#else
    if(input->length > UINT_MAX)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    int windowBits = encoding == UA_HTTP_CONTENT_ENCODING_GZIP ?
        MAX_WBITS + 16 : MAX_WBITS;
    if(encoding != UA_HTTP_CONTENT_ENCODING_GZIP &&
       encoding != UA_HTTP_CONTENT_ENCODING_DEFLATE)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    if(deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, windowBits,
                    8, Z_DEFAULT_STRATEGY) != Z_OK)
        return UA_STATUSCODE_BADINTERNALERROR;
    uLong bound = deflateBound(&stream, (uLong)input->length);
    if(bound > UINT_MAX) {
        deflateEnd(&stream);
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    }
    UA_StatusCode status = UA_ByteString_allocBuffer(output, (size_t)bound);
    if(status != UA_STATUSCODE_GOOD) {
        deflateEnd(&stream);
        return status;
    }
    stream.next_in = (Bytef*)(uintptr_t)input->data;
    stream.avail_in = (uInt)input->length;
    stream.next_out = output->data;
    stream.avail_out = (uInt)output->length;
    int result = deflate(&stream, Z_FINISH);
    if(result == Z_STREAM_END)
        output->length = (size_t)stream.total_out;
    else {
        UA_ByteString_clear(output);
        status = UA_STATUSCODE_BADENCODINGERROR;
    }
    deflateEnd(&stream);
    return status;
#endif
}

UA_StatusCode
UA_HTTP_decompress(UA_HTTPContentEncoding encoding, const UA_ByteString *input,
                   size_t maxOutputSize, UA_ByteString *output) {
    if(!input || !output || maxOutputSize == 0 ||
       encoding == UA_HTTP_CONTENT_ENCODING_IDENTITY)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
#ifndef UA_ENABLE_HTTP_COMPRESSION
    (void)encoding;
    return UA_STATUSCODE_BADNOTSUPPORTED;
#else
    if(input->length > UINT_MAX || maxOutputSize > UINT_MAX)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    int windowBits = encoding == UA_HTTP_CONTENT_ENCODING_GZIP ?
        MAX_WBITS + 16 : MAX_WBITS;
    if(encoding != UA_HTTP_CONTENT_ENCODING_GZIP &&
       encoding != UA_HTTP_CONTENT_ENCODING_DEFLATE)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    size_t capacity = input->length < 1024 ? 1024 : input->length * 2;
    if(capacity < input->length || capacity > maxOutputSize)
        capacity = maxOutputSize;
    UA_StatusCode status = UA_ByteString_allocBuffer(output, capacity);
    if(status != UA_STATUSCODE_GOOD)
        return status;

    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    if(inflateInit2(&stream, windowBits) != Z_OK) {
        UA_ByteString_clear(output);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    stream.next_in = (Bytef*)(uintptr_t)input->data;
    stream.avail_in = (uInt)input->length;
    int result = Z_OK;
    while(result == Z_OK) {
        if(stream.total_out == maxOutputSize) {
            UA_Byte overflow;
            stream.next_out = &overflow;
            stream.avail_out = 1;
            result = inflate(&stream, Z_NO_FLUSH);
            if(stream.total_out > maxOutputSize) {
                status = UA_STATUSCODE_BADREQUESTTOOLARGE;
                break;
            }
            continue;
        }
        if(stream.total_out == output->length) {
            if(output->length >= maxOutputSize) {
                status = UA_STATUSCODE_BADREQUESTTOOLARGE;
                break;
            }
            size_t newCapacity = output->length > maxOutputSize / 2 ?
                maxOutputSize : output->length * 2;
            UA_Byte *newData = (UA_Byte*)UA_realloc(output->data, newCapacity);
            if(!newData) {
                status = UA_STATUSCODE_BADOUTOFMEMORY;
                break;
            }
            output->data = newData;
            output->length = newCapacity;
        }
        stream.next_out = output->data + stream.total_out;
        stream.avail_out = (uInt)(output->length - stream.total_out);
        result = inflate(&stream, Z_NO_FLUSH);
    }
    if(status == UA_STATUSCODE_GOOD && result == Z_STREAM_END) {
        output->length = (size_t)stream.total_out;
        status = UA_STATUSCODE_GOOD;
    } else if(status == UA_STATUSCODE_GOOD) {
        status = UA_STATUSCODE_BADDECODINGERROR;
    }
    inflateEnd(&stream);
    if(status != UA_STATUSCODE_GOOD)
        UA_ByteString_clear(output);
    return status;
#endif
}
