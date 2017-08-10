/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_PLUGIN_SECURITYPOLICY_H_
#define UA_PLUGIN_SECURITYPOLICY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_types.h"
#include "ua_plugin_log.h"
#include "ua_types_generated.h"

extern const UA_ByteString UA_SECURITY_POLICY_NONE_URI;

struct UA_SecurityPolicy;
typedef struct UA_SecurityPolicy UA_SecurityPolicy;

/* Holds an endpoint description and the corresponding security policy
 * Also holds the context for the endpoint. */
typedef struct {
    UA_SecurityPolicy *securityPolicy;
    void *securityContext;
    UA_EndpointDescription endpointDescription;
} UA_Endpoint;

typedef struct {
    size_t count;
    UA_Endpoint *endpoints;
} UA_Endpoints;

#ifdef __cplusplus
}
#endif

#endif /* UA_PLUGIN_SECURITYPOLICY_H_ */
