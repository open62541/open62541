/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2014, 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014 (c) Florian Palm
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#include <open62541/types_generated_handling.h>
#include <open62541/util.h>

#include "ua_util_internal.h"
#include "base64.h"

size_t
UA_readNumberWithBase(const UA_Byte *buf, size_t buflen, UA_UInt32 *number, UA_Byte base) {
    UA_assert(buf);
    UA_assert(number);
    u32 n = 0;
    size_t progress = 0;
    /* read numbers until the end or a non-number character appears */
    while(progress < buflen) {
        u8 c = buf[progress];
        if(c >= '0' && c <= '9' && c <= '0' + (base-1))
           n = (n * base) + c - '0';
        else if(base > 9 && c >= 'a' && c <= 'z' && c <= 'a' + (base-11))
           n = (n * base) + c-'a' + 10;
        else if(base > 9 && c >= 'A' && c <= 'Z' && c <= 'A' + (base-11))
           n = (n * base) + c-'A' + 10;
        else
           break;
        ++progress;
    }
    *number = n;
    return progress;
}

size_t
UA_readNumber(const UA_Byte *buf, size_t buflen, UA_UInt32 *number) {
    return UA_readNumberWithBase(buf, buflen, number, 10);
}

UA_StatusCode
UA_parseEndpointUrl(const UA_String *endpointUrl, UA_String *outHostname,
                u16 *outPort, UA_String *outPath) {
UA_Boolean ipv6 = false;
UA_Boolean is_udp = false;
UA_Boolean is_tcp = false;
UA_Boolean is_mqtt = false;
UA_Boolean is_ws = false;
UA_Boolean is_wss = false;

//printf("UA_parseEndpointUrl() : endpointUrl = %s, length = %d \n", endpointUrl->data, endpointUrl->length);

/* Url must begin with "opc.tcp://" or opc.udp:// (if pubsub enabled) */
//if(endpointUrl->length < 11) {
//    return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
//}

if (strncmp((char*)endpointUrl->data, "opc.tcp://", 10) != 0) {
#ifdef UA_ENABLE_PUBSUB
  if (strncmp((char*)endpointUrl->data, "opc.udp://", 10) != 0) {
 if (strncmp((char*)endpointUrl->data, "opc.mqtt://", 11) != 0) {
    #ifdef UA_ENABLE_WEBSOCKET_SERVER
        if (strncmp((char*)endpointUrl->data, "ws://", 5) != 0) {
	  if (strncmp((char*)endpointUrl->data, "wss://", 6) != 0 ) {
			return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
	  }
	  else {
		 is_wss = true;
		 //printf("url prefix is a <wss://> \n");
	  }
	}
	else {
	       is_ws = true;
	       //printf("url prefix is a <ws://> \n");
	}
    #endif
        	//return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
 }
 else {
	is_mqtt=true;
	//printf("url prefix is a <opc.mqtt://> \n");
 }
  }
  else {
     is_udp = true;
     //printf("url prefix is a <opc.udp://> or <opc.mqtt://> \n");
  }
#else
    return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
#endif
}
else {
   is_tcp = true;
   //printf("strncmp endpointUrl->data with <opc.tcp://> is a match \n");
}

/* Where does the hostname end? */

size_t curr;
if (is_udp || is_tcp)
    curr = 10;
else if (is_ws)
    curr = 5;
else if (is_wss)
    curr = 6;
else if (is_mqtt)
    curr = 11;

if(endpointUrl->data[curr] == '[') {
    /* IPv6: opc.tcp://[2001:0db8:85a3::8a2e:0370:7334]:1234/path */
    for(; curr < endpointUrl->length; ++curr) {
        if(endpointUrl->data[curr] == ']')
            break;
    }
    if(curr == endpointUrl->length)
        return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
    curr++;
    ipv6 = true;
} else {
    /* IPv4 or hostname: opc.tcp://something.something:1234/path */
    for(; curr < endpointUrl->length; ++curr) {
        if(endpointUrl->data[curr] == ':' || endpointUrl->data[curr] == '/'){
	//printf("curr position is %d \n", curr);
            break;
    }
    }
}

/* Set the hostname */
if(ipv6) {
    /* Skip the ipv6 '[]' container for getaddrinfo() later */
    outHostname->data = &endpointUrl->data[11];
    outHostname->length = curr - 12;
} else {
	//printf("curr is %d \n", curr);
         switch (curr)
     {
	case 21: // for addMdnsRecordForNetworkLayer() : discoveryUrl ws://OPCServerUAT-112:7681/,
		outHostname->data = &endpointUrl->data[5];
		outHostname->length = curr-5;	// 26-5 = 21
		break;
	case 22: // -- opc.tcp://  or opc.udp://
	case 26: // for addMdnsRecordForNetworkLayer() : discoveryUrl opc.tcp://OPCServerUAT-112:4840/,
    		outHostname->data = &endpointUrl->data[10];
    		outHostname->length = curr-10;
		break;
	case 23: // -- opc.mqtt://
		outHostname->data = &endpointUrl->data[11];
		outHostname->data[17]='\0';	// remove extra character 'A'
		outHostname->length = curr-11;	// 23-11 = 12

     }
//printf("outHostname->data = %s, length = %d \n", outHostname->data, outHostname->length);
}

/* Empty string? */
if(outHostname->length == 0)
    outHostname->data = NULL;

if(curr == endpointUrl->length)
    return UA_STATUSCODE_GOOD;

//printf("setting the port \n");
/* Set the port */
if(endpointUrl->data[curr] == ':') {
//printf("here with ':' \n");
    if(++curr == endpointUrl->length)	// curr = 24
        return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
    u32 largeNum;
    size_t progress = UA_readNumber(&endpointUrl->data[curr],
                                    endpointUrl->length - curr, &largeNum);
//printf("progress = %d, largenum = %d \n", progress, largeNum);
    if(progress == 0 || largeNum > 65535)
        return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
    /* Test if the end of a valid port was reached */
    curr += progress;
//printf("curr += progress = %d \n", curr);
//printf("endpointUrl->length = %d \n", endpointUrl->length);
//printf("endpointUrl->data[%d] is %s \n", curr, (char*)endpointUrl->data);

    if(curr == endpointUrl->length || endpointUrl->data[curr] == '/')
        *outPort = (u16)largeNum;

    if (is_tcp || is_udp) {
        if(curr == endpointUrl->length) {
	    //printf("successfully executed UA_parseEndpointUrl():set the port, returning to calling function \n");
        	return UA_STATUSCODE_GOOD;
	}
    }
}

//printf("setting the path \n");
/* Set the path */
  if (is_tcp || is_udp) {
    UA_assert(curr < endpointUrl->length);	// curr = 27, length = 28
//printf("curr = %d, endpointUrl->length = %d \n", curr, endpointUrl->length);

if(endpointUrl->data[curr] != '/')
    return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
if(++curr == endpointUrl->length) {
//printf("just about to exit UA_parseEndpointUrl() \n");
    return UA_STATUSCODE_GOOD;
}
outPath->data = &endpointUrl->data[curr];
outPath->length = endpointUrl->length - curr;

/* Remove trailing slash from the path */
if(endpointUrl->data[endpointUrl->length - 1] == '/')
    outPath->length--;

/* Empty string? */
if(outPath->length == 0)
    outPath->data = NULL;

return UA_STATUSCODE_GOOD;
  }
else if (is_mqtt) {
//printf("curr = %d, endpointUrl->length = %d \n", curr, endpointUrl->length);
//printf("outHostname->length = %d \n", outHostname->length);
//printf("outHostname->data = %s \n", outHostname->data);
UA_assert(curr == endpointUrl->length);      // curr = 28, length = 28

endpointUrl->data[curr] = '/';	// added by Jacky
outPath->length = endpointUrl->length;
outPath->data = outHostname->data;

return UA_STATUSCODE_GOOD;

/*
if(endpointUrl->data[curr] != '/')
    return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
if(++curr == endpointUrl->length) {
    //printf("just about to exit UA_parseEndpointUrl() \n");
    return UA_STATUSCODE_GOOD;
}

outPath->data = &endpointUrl->data[curr];	// curr
outPath->length = endpointUrl->length - curr;

// Remove trailing slash from the path
if(endpointUrl->data[endpointUrl->length - 1] == '/')
    outPath->length--;

// Empty string?
if(outPath->length == 0)
    outPath->data = NULL;

return UA_STATUSCODE_GOOD;
*/
  }
else return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_ByteString_toBase64(const UA_ByteString *byteString,
                       UA_String *str) {
    UA_String_init(str);
    if(!byteString || !byteString->data)
        return UA_STATUSCODE_GOOD;

    str->data = (UA_Byte*)
        UA_base64(byteString->data, byteString->length, &str->length);
    if(!str->data)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_ByteString_fromBase64(UA_ByteString *bs,
                         const UA_String *input) {
    UA_ByteString_init(bs);
    if(input->length == 0)
        return UA_STATUSCODE_GOOD;
    bs->data = UA_unbase64((const unsigned char*)input->data,
                           input->length, &bs->length);
    /* TODO: Differentiate between encoding and memory errors */
    if(!bs->data)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

/* Key Value Map */

UA_StatusCode
UA_KeyValueMap_set(UA_KeyValuePair **map, size_t *mapSize,
                   const UA_QualifiedName key,
                   const UA_Variant *value) {
    /* Parameter exists already */
    const UA_Variant *v = UA_KeyValueMap_get(*map, *mapSize, key);
    if(v) {
        UA_Variant copyV;
        UA_StatusCode res = UA_Variant_copy(v, &copyV);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        UA_Variant *target = (UA_Variant*)(uintptr_t)v;
        UA_Variant_clear(target);
        *target = copyV;
        return UA_STATUSCODE_GOOD;
    }

    /* Append to the array */
    UA_KeyValuePair pair;
    pair.key = key;
    pair.value = *value;
    return UA_Array_appendCopy((void**)map, mapSize, &pair,
                               &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
}

const UA_Variant *
UA_KeyValueMap_get(const UA_KeyValuePair *map, size_t mapSize,
                   const UA_QualifiedName key) {
    for(size_t i = 0; i < mapSize; i++) {
        if(map[i].key.namespaceIndex == key.namespaceIndex &&
           UA_String_equal(&map[i].key.name, &key.name))
            return &map[i].value;

    }
    return NULL;
}

/* Returns NULL if the parameter is not defined or not of the right datatype */
const void *
UA_KeyValueMap_getScalar(const UA_KeyValuePair *map, size_t mapSize,
                         const UA_QualifiedName key,
                         const UA_DataType *type) {
    const UA_Variant *v = UA_KeyValueMap_get(map, mapSize, key);
    if(!v || !UA_Variant_hasScalarType(v, type))
        return NULL;
    return v->data;
}

void
UA_KeyValueMap_delete(UA_KeyValuePair **map, size_t *mapSize,
                      const UA_QualifiedName key) {
    UA_KeyValuePair *m = *map;
    size_t s = *mapSize;
    for(size_t i = 0; i < s; i++) {
        if(m[i].key.namespaceIndex != key.namespaceIndex ||
           !UA_String_equal(&m[i].key.name, &key.name))
            continue;

        /* Clean the pair */
        UA_KeyValuePair_clear(&m[i]);

        /* Move the last pair to fill the empty slot */
        if(s > 1 && i < s - 1) {
            m[i] = m[s-1];
            UA_KeyValuePair_init(&m[s-1]);
        }

        UA_StatusCode res = UA_Array_resize((void**)map, mapSize, *mapSize-1,
                                            &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
        (void)res;
        *mapSize = s - 1; /* In case resize fails, keep the longer original
                           * array around. Resize never fails when reducing
                           * the size to zero. Reduce the size integer in
                           * any case. */
        break;
    }
}
