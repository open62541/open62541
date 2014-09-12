#ifndef UA_SERVER_H_
#define UA_SERVER_H_

#include "ua_types.h"
#include "ua_types_generated.h"
#include "ua_connection.h"
#include "util/ua_log.h"

/**
   @defgroup server Server
 */

struct UA_SecureChannelManager;
typedef struct UA_SecureChannelManager UA_SecureChannelManager;

struct UA_SessionManager;
typedef struct UA_SessionManager UA_SessionManager;

struct UA_NodeStore;
typedef struct UA_NodeStore UA_NodeStore;

typedef struct UA_Server {
    UA_ApplicationDescription description;
    UA_SecureChannelManager *secureChannelManager;
    UA_SessionManager *sessionManager;
    UA_NodeStore *nodestore;
    UA_Logger logger;
    UA_ByteString serverCertificate;
} UA_Server;

void UA_LIBEXPORT UA_Server_init(UA_Server *server, UA_String *endpointUrl);
UA_Int32 UA_LIBEXPORT UA_Server_deleteMembers(UA_Server *server);
UA_Int32 UA_LIBEXPORT UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection,
                                                     const UA_ByteString *msg);

#endif /* UA_SERVER_H_ */
