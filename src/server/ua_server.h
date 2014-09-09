#ifndef UA_SERVER_H_
#define UA_SERVER_H_

#include "ua_types.h"
#include "ua_types_generated.h"
#include "ua_nodestore.h"
#include "ua_securechannel_manager.h"
#include "ua_session_manager.h"
#include "util/ua_log.h"

/**
   @defgroup server Server
 */

typedef struct UA_Server {
    UA_ApplicationDescription description;
    UA_SecureChannelManager *secureChannelManager;
    UA_SessionManager *sessionManager;
    UA_NodeStore *nodestore;
    UA_Logger logger;
} UA_Server;

void UA_Server_init(UA_Server *server, UA_String *endpointUrl);
UA_Int32 UA_Server_deleteMembers(UA_Server *server);
UA_Int32 UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection,
                                        const UA_ByteString *msg);

#endif /* UA_SERVER_H_ */
