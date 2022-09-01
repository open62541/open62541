//
// Created by mark on 11.05.22.
//

#ifndef OPEN62541_ENDPOINT_H
#define OPEN62541_ENDPOINT_H

#include <open62541/types_generated.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/certstore.h>

typedef struct UA_Endpoint UA_Endpoint;

struct UA_Endpoint {
	UA_EndpointDescription* endpointDescription;
    UA_PKIStore* pkiStore;
    UA_SecurityPolicy *securityPolicy;
    UA_Boolean allowNone;
    UA_Boolean allowSign;
    UA_Boolean allowSignAndEncrypt;
    UA_String endpointUrl; /* const EndpointDescription* - Nicht verwendet: securityMode, serverCert*/
};

UA_StatusCode
UA_Endpoint_init(
	UA_Endpoint* endpoint
);

void
UA_Endpoint_clear(
	UA_Endpoint* endpoint
);

UA_StatusCode
UA_Endpoint_setValues(
	UA_Endpoint* endpoint,
    const UA_String* endpointUrl,
    UA_PKIStore* pkiStore,
    UA_SecurityPolicy* securityPolicy,
    UA_Boolean allowNone,
    UA_Boolean allowSign,
    UA_Boolean allowSignAndEncrypt,
    UA_ApplicationDescription applicationDescription,
    UA_UserTokenPolicy* userTokenPolicies,
    size_t userTokenPoliciesSize
);

UA_StatusCode
UA_Endpoint_updateApplicationDescription(
	UA_Endpoint* endpoint,
	const UA_ApplicationDescription* applicationDescription
);

UA_StatusCode
UA_Endpoint_toEndpointDescription(
	const UA_Endpoint* endpoint,
    UA_EndpointDescription *endpointDescription,
    UA_MessageSecurityMode securityMode
);

bool
UA_Endpoint_matchesUrl(
	UA_Endpoint* endpoint,
	UA_String* url
);

#endif //OPEN62541_ENDPOINT_H
