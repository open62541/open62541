//
// Copyright 2020 NanoMQ Team, Inc. <jaylin@emqx.io>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

#ifndef REST_API_H
#define REST_API_H

#include "web_server.h"
#include "mqtt_api.h"
#include <ctype.h>
#include <nng/nng.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REST_URI_ROOT "/api/v4"
#define REST_HOST "http://%s:%u"
#define REST_URL REST_HOST REST_URI_ROOT

#define getNumberValue(obj, item, key, value, rv)           \
	{                                                   \
		item = cJSON_GetObjectItem(obj, key);       \
		if (cJSON_IsNumber(item)) {                 \
			value = cJSON_GetNumberValue(item); \
			rv    = (0);                        \
		} else {                                    \
			rv = (-1);                          \
		}                                           \
	}

#define getBoolValue(obj, item, key, value, rv)       \
	{                                             \
		item = cJSON_GetObjectItem(obj, key); \
		if (cJSON_IsBool(item)) {             \
			value = cJSON_IsTrue(item);   \
			rv    = (0);                  \
		} else {                              \
			rv = (-1);                    \
		}                                     \
	}

#define getStringValue(obj, item, key, value, rv)           \
	{                                                   \
		item = cJSON_GetObjectItem(obj, key);       \
		if (cJSON_IsString(item)) {                 \
			value = cJSON_GetStringValue(item); \
			rv    = (0);                        \
		} else {                                    \
			rv = (-1);                          \
		}                                           \
	}

enum result_code {
	SUCCEED                        = 0,
	RPC_ERROR                      = 101,
	UNKNOWN_MISTAKE                = 102,
	WRONG_USERNAME_OR_PASSWORD     = 103,
	EMPTY_USERNAME_OR_PASSWORD     = 104,
	USER_DOES_NOT_EXIST            = 105,
	ADMIN_CANNOT_BE_DELETED        = 106,
	MISSING_KEY_REQUEST_PARAMES    = 107,
	REQ_PARAM_ERROR                = 108,
	REQ_PARAMS_JSON_FORMAT_ILLEGAL = 109,
	PLUGIN_IS_ENABLED              = 110,
	PLUGIN_IS_CLOSED               = 111,
	CLIENT_IS_OFFLINE              = 112,
	USER_ALREADY_EXISTS            = 113,
	OLD_PASSWORD_IS_WRONG          = 114,
	ILLEGAL_SUBJECT                = 115,
	TOKEN_EXPIRED                  = 116,
	PARAMS_HOCON_FORMAT_ILLEGAL    = 117,
	WRITE_CONFIG_FAILED            = 118,
};

typedef struct http_msg {
	uint16_t status;
	int      request;
	size_t   content_type_len;
	char *   content_type;
	size_t   method_len;
	char *   method;
	size_t   uri_len;
	char *   uri;
	size_t   token_len;
	char *   token;
	size_t   data_len;
	char *   data;
	bool     encrypt_data;
} http_msg;

extern void     put_http_msg(http_msg *msg, const char *content_type,
        const char *method, const char *uri, const char *token, const char *data,
        size_t data_sz);
extern void     destory_http_msg(http_msg *msg);
extern http_msg process_request(
    http_msg *msg, conf_http_server *config, nng_socket *sock);

#define GET_METHOD "GET"
#define POST_METHOD "POST"
#define PUT_METHOD "PUT"
#define DELETE_METHOD "DELETE"

#define REQ_ENDPOINTS 1
#define REQ_BROKERS 2
#define REQ_NODES 3
#define REQ_SUBSCRIPTIONS 4
#define REQ_CLIENTS 5
#define REQ_LOGIN 6
#define REQ_LOGOUT 7

#define REQ_CTRL 10
#define REQ_GET_CONFIG 11
#define REQ_SET_CONFIG 12
#define REQ_TREE 13

#define cJSON_AddStringOrNullToObject(obj, name, value)            \
	{                                                          \
		if (value) {                                       \
			cJSON_AddStringToObject(obj, name, value); \
		} else {                                           \
			cJSON_AddNullToObject(obj, name);          \
		}                                                  \
	}

#endif