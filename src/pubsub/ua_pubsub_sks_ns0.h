#ifndef UA_PUBSUB_SKS_NS0_H_
#define UA_PUBSUB_SKS_NS0_H_

#include "server/ua_server_internal.h"
#include "ua_pubsub.h"

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_PUBSUB_SECURITY /* conditional compilation */

UA_StatusCode
UA_Server_initPubSubSKSNS0(UA_Server *server);

#endif /* UA_ENABLE_PUBSUB_SECURITY */

_UA_END_DECLS

#endif /* UA_PUBSUB_SKS_NS0_H_ */
