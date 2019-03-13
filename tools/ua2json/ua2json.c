/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

/* Enable POSIX features */
#if !defined(_XOPEN_SOURCE)
# define _XOPEN_SOURCE 600
#endif
#ifndef _DEFAULT_SOURCE
# define _DEFAULT_SOURCE
#endif
/* On older systems we need to define _BSD_SOURCE.
 * _DEFAULT_SOURCE is an alias for that. */
#ifndef _BSD_SOURCE
# define _BSD_SOURCE
#endif

#include <open62541/types.h>

#include <stdio.h>

/* Internal headers */
#include <open62541/types_generated.h>
#include <open62541/types_generated_handling.h>

#include "ua_pubsub_networkmessage.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_encoding_json.h"

static UA_StatusCode
encode(const UA_ByteString *buf, UA_ByteString *out,
       const UA_DataType *type) {
    void *data = malloc(type->memSize);
    if(!data)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    size_t offset = 0;
    UA_StatusCode retval = UA_decodeBinary(buf, &offset, data, type, NULL);
    if(retval != UA_STATUSCODE_GOOD) {
        free(data);
        return retval;
    }
    if(offset != buf->length) {
        UA_delete(data, type);
        fprintf(stderr, "Input buffer not completely read\n");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    size_t jsonLength = UA_calcSizeJson(data, type, NULL, 0, NULL, 0, true);
    retval = UA_ByteString_allocBuffer(out, jsonLength);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_delete(data, type);
        return retval;
    }

    uint8_t *bufPos = &out->data[0];
    const uint8_t *bufEnd = &out->data[out->length];
    retval = UA_encodeJson(data, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, true);
    UA_delete(data, type);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_deleteMembers(out);
        return retval;
    }

    out->length = (size_t)((uintptr_t)bufPos - (uintptr_t)out->data);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
decode(const UA_ByteString *buf, UA_ByteString *out,
       const UA_DataType *type) {
    void *data = malloc(type->memSize);
    if(!data)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode retval = UA_decodeJson(buf, data, type);
    if(retval != UA_STATUSCODE_GOOD) {
        free(data);
        return retval;
    }

    size_t binLength = UA_calcSizeBinary(data, type);
    retval = UA_ByteString_allocBuffer(out, binLength);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_delete(data, type);
        return retval;
    }

    uint8_t *bufPos = &out->data[0];
    const uint8_t *bufEnd = &out->data[out->length];
    retval = UA_encodeBinary(data, type, &bufPos, &bufEnd, NULL, NULL);
    UA_delete(data, type);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_deleteMembers(out);
        return retval;
    }

    out->length = (size_t)((uintptr_t)bufPos - (uintptr_t)out->data);
    return UA_STATUSCODE_GOOD;
}

#ifdef UA_ENABLE_PUBSUB

static UA_StatusCode
encodeNetworkMessage(const UA_ByteString *buf, UA_ByteString *out) {
    size_t offset = 0;
    UA_NetworkMessage msg;
    UA_StatusCode retval = UA_NetworkMessage_decodeBinary(buf, &offset, &msg);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    if(offset != buf->length) {
        UA_NetworkMessage_deleteMembers(&msg);
        fprintf(stderr, "Input buffer not completely read\n");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    size_t jsonLength = UA_NetworkMessage_calcSizeJson(&msg, NULL, 0, NULL, 0, true);
    retval = UA_ByteString_allocBuffer(out, jsonLength);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_NetworkMessage_deleteMembers(&msg);
        return retval;
    }

    uint8_t *bufPos = &out->data[0];
    const uint8_t *bufEnd = &out->data[out->length];
    retval = UA_NetworkMessage_encodeJson(&msg, &bufPos, &bufEnd, NULL, 0, NULL, 0, true);
    UA_NetworkMessage_deleteMembers(&msg);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_deleteMembers(out);
        return retval;
    }

    out->length = (size_t)((uintptr_t)bufPos - (uintptr_t)out->data);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
decodeNetworkMessage(const UA_ByteString *buf, UA_ByteString *out) {
    UA_NetworkMessage msg;
    UA_StatusCode retval = UA_NetworkMessage_decodeJson(&msg, buf);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    size_t binLength = UA_NetworkMessage_calcSizeBinary(&msg);
    retval = UA_ByteString_allocBuffer(out, binLength);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_NetworkMessage_deleteMembers(&msg);
        return retval;
    }

    uint8_t *bufPos = &out->data[0];
    const uint8_t *bufEnd = &out->data[out->length];
    retval = UA_NetworkMessage_encodeBinary(&msg, &bufPos, bufEnd);
    UA_NetworkMessage_deleteMembers(&msg);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_deleteMembers(out);
        return retval;
    }

    out->length = (size_t)((uintptr_t)bufPos - (uintptr_t)out->data);
    return UA_STATUSCODE_GOOD;
}

#endif

static void
usage(void) {
    printf("Usage: ua2json [encode|decode] [-t dataType] [-o outputFile] [inputFile]\n"
           "- encode/decode: Translate UA binary input to UA JSON / "
             "Translate UA JSON input to UA binary (required)\n"
           "- dataType: UA DataType of the input (default: Variant)\n"
           "- outputFile: Output target (default: write to stdout)\n"
           "- inputFile: Input source (default: read from stdin)\n");
}

int main(int argc, char **argv) {
    UA_Boolean encode_option = true;
    UA_Boolean pubsub = false;
    const char *datatype_option = "Variant";
    const char *input_option = NULL;
    const char *output_option = NULL;
    UA_ByteString outbuf = UA_BYTESTRING_NULL;
    UA_ByteString buf = UA_BYTESTRING_NULL;
    FILE *in = stdin;
    FILE *out = stdout;
    int retcode = -1;

    /* Read the command line options */
    if(argc < 2) {
        usage();
        return 0;
    }

    if(strcmp(argv[1], "encode") == 0) {
        encode_option = true;
    } else if(strcmp(argv[1], "decode") == 0) {
        encode_option = false;
    } else {
        fprintf(stderr, "Error: The first argument must be \"encode\" or \"decode\"\n");
        return -1;
    }
        
    for(int argpos = 2; argpos < argc; argpos++) {
        if(strcmp(argv[argpos], "--help") == 0) {
            usage();
            return 0;
        }

        if(strcmp(argv[argpos], "-t") == 0) {
            if(argpos + 1 == argc) {
                usage();
                return -1;
            }
            argpos++;
            datatype_option = argv[argpos];
            continue;
        }

        if(strcmp(argv[argpos], "-o") == 0) {
            if(argpos + 1 == argc) {
                usage();
                return -1;
            }
            argpos++;
            output_option = argv[argpos];
            continue;
        }

        if(argpos + 1 == argc) {
            input_option = argv[argpos];
            continue;
        }

        usage();
        return -1;
    }

    /* Find the data type */
    const UA_DataType *type = NULL;
    if(strcmp(datatype_option, "PubSub") == 0) {
        pubsub = true;
    } else {
        for(size_t i = 0; i < UA_TYPES_COUNT; ++i) {
            if(strcmp(datatype_option, UA_TYPES[i].typeName) == 0) {
                type = &UA_TYPES[i];
                break;
            }
        }
        if(!type) {
            fprintf(stderr, "Error: Datatype not found\n");
            return -1;
        }
    }

    /* Open files */
    if(input_option) {
        in = fopen(input_option, "rb");
        if(!in) {
            fprintf(stderr, "Could not open input file %s\n", input_option);
            goto cleanup;
        }
    }
    if(output_option) {
        out = fopen(output_option, "wb");
        if(!out) {
            fprintf(stderr, "Could not open output file %s\n", output_option);
            goto cleanup;
        }
    }

    /* Read input until EOF */
    size_t pos = 0;
    size_t length = 128;
    while(true) {
        if(pos >= buf.length) {
            length = length * 8;
            UA_Byte *r = (UA_Byte*)realloc(buf.data, length);
            if(!r) {
                fprintf(stderr, "Out of memory\n");
                goto cleanup;
            }
            buf.length = length;
            buf.data = r;
        }

        ssize_t c = read(fileno(in), &buf.data[pos], length - pos);
        if(c == 0)
            break;
        if(c < 0) {
            fprintf(stderr, "Reading from input failed\n");
            goto cleanup;
        }

        pos += (size_t)c;
    }

    if(pos == 0) {
        fprintf(stderr, "No input\n");
        goto cleanup;
    }
    buf.length = pos;

    /* Convert */
    UA_StatusCode result = UA_STATUSCODE_BADNOTIMPLEMENTED;
#ifdef UA_ENABLE_PUBSUB
    if(pubsub && encode_option) {
        result = encodeNetworkMessage(&buf, &outbuf);
    } else if(pubsub) {
        result = decodeNetworkMessage(&buf, &outbuf);
    } else
#endif
    if(encode_option) {
        result = encode(&buf, &outbuf, type);
    } else {
        result = decode(&buf, &outbuf, type);
    }
    
    if(result != UA_STATUSCODE_GOOD) {
        fprintf(stderr, "Error: Parsing failed with code %s\n",
                UA_StatusCode_name(result));
        goto cleanup;
    }

    /* Print the output and quit */
    fwrite(outbuf.data, 1, outbuf.length, out);
    retcode = 0;

 cleanup:
    UA_ByteString_deleteMembers(&buf);
    UA_ByteString_deleteMembers(&outbuf);
    if(in != stdin && in)
        fclose(in);
    if(out != stdout && out)
        fclose(out);
    return retcode;
}
