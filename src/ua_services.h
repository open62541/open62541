#ifndef UA_SERVICES_H_
#define UA_SERVICES_H_

#include "opcua.h"
#include "ua_application.h"
#include "ua_statuscodes.h"
#include "ua_transport_binary_secure.h"

/* Part 4: 5.4 Discovery Service Set */
// Service_FindServers
UA_Int32 Service_GetEndpoints(SL_Channel *channel, const UA_GetEndpointsRequest* request, UA_GetEndpointsResponse *response);
// Service_RegisterServer

/* Part 4: 5.5 SecureChannel Service Set */
UA_Int32 Service_OpenSecureChannel(SL_Channel *channel, const UA_OpenSecureChannelRequest* request, UA_OpenSecureChannelResponse* response);
UA_Int32 Service_CloseSecureChannel(SL_Channel *channel, const UA_CloseSecureChannelRequest *request, UA_CloseSecureChannelResponse *response);

/* Part 4: 5.6 Session Service Set */
UA_Int32 Service_CreateSession(SL_Channel *channel, const UA_CreateSessionRequest *request, UA_CreateSessionResponse *response);
UA_Int32 Service_ActivateSession(SL_Channel *channel, const UA_ActivateSessionRequest *request, UA_ActivateSessionResponse *response);
UA_Int32 Service_CloseSession(SL_Channel *channel, const UA_CloseSessionRequest *request, UA_CloseSessionResponse *response);
// Service_Cancel

/* Part 4: 5.7 NodeManagement Service Set */
// Service_AddNodes
// Service_AddReferences
// Service_DeleteNodes
// Service_DeleteReferences

/* Part 4: 5.8 View Service Set */
UA_Int32 Service_Browse(SL_Channel *channel, const UA_BrowseRequest *request, UA_BrowseResponse *response);
// Service_BrowseNext
// Service_TranslateBrowsePathsRoNodeIds
// Service_RegisterNodes
// Service_UnregisterNodes

/* Part 4: 5.9 Query Service Set */
// Service_QueryFirst
// Service_QueryNext

/* Part 4: 5.10 Attribute Service Set */
UA_Int32 Service_Read(SL_Channel *channel, const UA_ReadRequest *request, UA_ReadResponse *response);
// Service_HistoryRead;
// Service_Write;
// Service_HistoryUpdate;

/* Part 4: 5.11 Method Service Set */
// Service_Call

/* Part 4: 5.12 MonitoredItem Service Set */
// Service_CreateMonitoredItems
// Service_ModifyMonitoredItems
// Service_SetMonitoringMode
// Service_SetTriggering
// Service_DeleteMonitoredItems

/* Part 4: 5.13 Subscription Service Set */
// Service_CreateSubscription
// Service_ModifySubscription
// Service_SetPublishingMode
// Service_Publish
// Service_Republish
// Service_TransferSubscription
// Service_DeleteSubscription

#endif
