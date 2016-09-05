/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <signal.h>

#ifdef UA_NO_AMALGAMATION
#include "ua_types.h"
#include "ua_server.h"
#include "ua_config_standard.h"
#include "ua_network_tcp.h"
#else
#include "open62541.h"
#endif

UA_Boolean running = true;
static void stopHandler(int sig) {
    running = false;
}

int main(void) {
    signal(SIGINT,  stopHandler);
    signal(SIGTERM, stopHandler);

    UA_ServerConfig config = UA_ServerConfig_standard;
    UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664);
    config.networkLayers = &nl;
    config.networkLayersSize = 1;
    UA_Server *server = UA_Server_new(config);
    
    /* Create a rudimentary objectType
     * 
     * BaseObjectType
     * |
     * +- (OT) MamalType
     *    + (V) Age
     *    + (OT) DogType
     *      |
     *      + (V) Name
     */ 
    UA_StatusCode retval;
    UA_ObjectTypeAttributes otAttr;
    UA_ObjectTypeAttributes_init(&otAttr);
    
    otAttr.description = UA_LOCALIZEDTEXT("en_US", "A mamal");
    otAttr.displayName = UA_LOCALIZEDTEXT("en_US", "MamalType");
    UA_Server_addObjectTypeNode(server, UA_NODEID_NUMERIC(1, 10000), 
                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "MamalType"), otAttr, NULL, NULL);
  
    UA_VariableAttributes   vAttr;
    UA_VariableAttributes_init(&vAttr);
    vAttr.description =  UA_LOCALIZEDTEXT("en_US", "This mamals Age in months");
    vAttr.displayName =  UA_LOCALIZEDTEXT("en_US", "Age");
    UA_UInt32 ageVar = 0;
    UA_Variant_setScalarCopy(&vAttr.value, &ageVar, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 10001), 
                              UA_NODEID_NUMERIC(1, 10000), UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                              UA_QUALIFIEDNAME(1, "Age"), UA_NODEID_NULL, vAttr, NULL, NULL);
  
    UA_ObjectTypeAttributes_init(&otAttr);
    otAttr.description = UA_LOCALIZEDTEXT("en_US", "A dog, subtype of mamal");
    otAttr.displayName = UA_LOCALIZEDTEXT("en_US", "DogType");
    UA_Server_addObjectTypeNode(server, UA_NODEID_NUMERIC(1, 10002), 
                                UA_NODEID_NUMERIC(1, 10000), UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "DogType"), otAttr, NULL, NULL);
    
    UA_VariableAttributes_init(&vAttr);
    vAttr.description =  UA_LOCALIZEDTEXT("en_US", "This mamals Age in months");
    vAttr.displayName =  UA_LOCALIZEDTEXT("en_US", "Name");
    UA_String defaultName = UA_STRING("unnamed dog");
    UA_Variant_setScalarCopy(&vAttr.value, &defaultName, &UA_TYPES[UA_TYPES_STRING]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 10003), 
                              UA_NODEID_NUMERIC(1, 10002), UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                              UA_QUALIFIEDNAME(1, "Name"), UA_NODEID_NULL, vAttr, NULL, NULL);
    
    /* Instatiate a dog named bello:
     * (O) Objects
     *   + O Bello <DogType>
     *     + Age 
     *     + Name
     */
    
    UA_ObjectAttributes oAttr;
    UA_ObjectAttributes_init(&oAttr);
    oAttr.description = UA_LOCALIZEDTEXT("en_US", "A dog named Bello");
    oAttr.displayName = UA_LOCALIZEDTEXT("en_US", "Bello");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 0), 
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "Bello"), UA_NODEID_NUMERIC(1, 10002), oAttr, NULL, NULL);
    
    retval = UA_Server_run(server, &running);
    UA_Server_delete(server);
    nl.deleteMembers(&nl);
    return (int)retval;
}
