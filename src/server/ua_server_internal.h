#ifndef UA_SERVER_INTERNAL_H_
#define UA_SERVER_INTERNAL_H_

#include "ua_server.h"
#include "ua_session_manager.h"
#include "ua_securechannel_manager.h"

struct UA_Server {
    UA_ApplicationDescription description;
    UA_Int32 endpointDescriptionsSize;
    UA_EndpointDescription *endpointDescriptions;
    UA_ByteString serverCertificate;
    
    UA_SecureChannelManager secureChannelManager;
    UA_SessionManager sessionManager;
    UA_NodeStore *nodestore;
    UA_Logger logger;
};

#endif /* UA_SERVER_INTERNAL_H_ */
