/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2017, 2024 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014-2017 (c) Florian Palm
 *    Copyright 2015 (c) Sten Grüner
 *    Copyright 2014 (c) LEvertz
 *    Copyright 2015 (c) Chris Iatrou
 *    Copyright 2015 (c) Christian Fimmers
 *    Copyright 2015-2016 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_SERVICES_H_
#define UA_SERVICES_H_

#include <open62541/server.h>
#include "ua_session.h"

_UA_BEGIN_DECLS

/* Services return whether they are "done". Otherwise no response is sent.
 * This then needs to be done asynchronously at a later time. */
typedef UA_Boolean (*UA_Service)(UA_Server*, UA_Session*,
                                 const void *request, void *response);

typedef void (*UA_ChannelService)(UA_Server*, UA_SecureChannel*,
                                  const void *request, void *response);

typedef struct {
    UA_UInt32 requestTypeId;
#ifdef UA_ENABLE_DIAGNOSTICS
    UA_UInt16 counterOffset;
#endif
    UA_Boolean sessionRequired;
    UA_Service serviceCallback;
    const UA_DataType *requestType;
    const UA_DataType *responseType;
} UA_ServiceDescription;

/* Returns NULL if none found */
UA_ServiceDescription * getServiceDescription(UA_UInt32 requestTypeId);

/** Discovery Service Set **/
UA_Boolean
Service_FindServers(UA_Server *server, UA_Session *session,
                    const UA_FindServersRequest *request,
                    UA_FindServersResponse *response);

UA_Boolean
Service_GetEndpoints(UA_Server *server, UA_Session *session,
                     const UA_GetEndpointsRequest *request,
                     UA_GetEndpointsResponse *response);

#ifdef UA_ENABLE_DISCOVERY

UA_Boolean
Service_RegisterServer(UA_Server *server, UA_Session *session,
                       const UA_RegisterServerRequest *request,
                       UA_RegisterServerResponse *response);

UA_Boolean
Service_RegisterServer2(UA_Server *server, UA_Session *session,
                        const UA_RegisterServer2Request *request,
                        UA_RegisterServer2Response *response);

# ifdef UA_ENABLE_DISCOVERY_MULTICAST

UA_Boolean
Service_FindServersOnNetwork(UA_Server *server, UA_Session *session,
                             const UA_FindServersOnNetworkRequest *request,
                             UA_FindServersOnNetworkResponse *response);

# endif /* UA_ENABLE_DISCOVERY_MULTICAST */

#endif /* UA_ENABLE_DISCOVERY */

/** SecureChannel Service Set **/
void
Service_OpenSecureChannel(UA_Server *server, UA_SecureChannel* channel,
                          UA_OpenSecureChannelRequest *request,
                          UA_OpenSecureChannelResponse *response);

void
Service_CloseSecureChannel(UA_Server *server, UA_SecureChannel *channel);

/** Session Service Set **/
void
Service_CreateSession(UA_Server *server, UA_SecureChannel *channel,
                      const UA_CreateSessionRequest *request,
                      UA_CreateSessionResponse *response);

void
Service_ActivateSession(UA_Server *server, UA_SecureChannel *channel,
                        const UA_ActivateSessionRequest *request,
                        UA_ActivateSessionResponse *response);

void
Service_CloseSession(UA_Server *server, UA_SecureChannel *channel,
                     const UA_CloseSessionRequest *request,
                     UA_CloseSessionResponse *response);

UA_Boolean
Service_Cancel(UA_Server *server, UA_Session *session,
               const UA_CancelRequest *request,
               UA_CancelResponse *response);

/** NodeManagement Service Set **/
UA_Boolean
Service_AddNodes(UA_Server *server, UA_Session *session,
                 const UA_AddNodesRequest *request,
                 UA_AddNodesResponse *response);

UA_Boolean
Service_AddReferences(UA_Server *server, UA_Session *session,
                      const UA_AddReferencesRequest *request,
                      UA_AddReferencesResponse *response);

UA_Boolean
Service_DeleteNodes(UA_Server *server, UA_Session *session,
                    const UA_DeleteNodesRequest *request,
                    UA_DeleteNodesResponse *response);

UA_Boolean
Service_DeleteReferences(UA_Server *server, UA_Session *session,
                         const UA_DeleteReferencesRequest *request,
                         UA_DeleteReferencesResponse *response);

/** View Service Set **/
UA_Boolean
Service_Browse(UA_Server *server, UA_Session *session,
               const UA_BrowseRequest *request,
               UA_BrowseResponse *response);

UA_Boolean
Service_BrowseNext(UA_Server *server, UA_Session *session,
                   const UA_BrowseNextRequest *request,
                   UA_BrowseNextResponse *response);

UA_Boolean
Service_TranslateBrowsePathsToNodeIds(UA_Server *server, UA_Session *session,
    const UA_TranslateBrowsePathsToNodeIdsRequest *request,
    UA_TranslateBrowsePathsToNodeIdsResponse *response);

UA_Boolean
Service_RegisterNodes(UA_Server *server, UA_Session *session,
                      const UA_RegisterNodesRequest *request,
                      UA_RegisterNodesResponse *response);

UA_Boolean
Service_UnregisterNodes(UA_Server *server, UA_Session *session,
                        const UA_UnregisterNodesRequest *request,
                        UA_UnregisterNodesResponse *response);

/** Query Service Set **/
void Service_QueryFirst(UA_Server *server, UA_Session *session,
                        const UA_QueryFirstRequest *request,
                        UA_QueryFirstResponse *response);

void Service_QueryNext(UA_Server *server, UA_Session *session,
                       const UA_QueryNextRequest *request,
                       UA_QueryNextResponse *response);

/** Attribute Service Set **/
UA_Boolean
Service_Read(UA_Server *server, UA_Session *session,
             const UA_ReadRequest *request,
             UA_ReadResponse *response);

UA_Boolean
Service_Write(UA_Server *server, UA_Session *session,
              const UA_WriteRequest *request,
              UA_WriteResponse *response);

#ifdef UA_ENABLE_HISTORIZING
UA_Boolean
Service_HistoryRead(UA_Server *server, UA_Session *session,
                    const UA_HistoryReadRequest *request,
                    UA_HistoryReadResponse *response);

UA_Boolean
Service_HistoryUpdate(UA_Server *server, UA_Session *session,
                      const UA_HistoryUpdateRequest *request,
                      UA_HistoryUpdateResponse *response);
#endif

/** Method Service Set **/
#ifdef UA_ENABLE_METHODCALLS
UA_Boolean
Service_Call(UA_Server *server, UA_Session *session,
             const UA_CallRequest *request,
             UA_CallResponse *response);
#endif

#ifdef UA_ENABLE_SUBSCRIPTIONS

/** MonitoredItem Service Set **/
UA_Boolean
Service_CreateMonitoredItems(UA_Server *server, UA_Session *session,
                             const UA_CreateMonitoredItemsRequest *request,
                             UA_CreateMonitoredItemsResponse *response);

UA_Boolean
Service_DeleteMonitoredItems(UA_Server *server, UA_Session *session,
                             const UA_DeleteMonitoredItemsRequest *request,
                             UA_DeleteMonitoredItemsResponse *response);

UA_Boolean
Service_ModifyMonitoredItems(UA_Server *server, UA_Session *session,
                             const UA_ModifyMonitoredItemsRequest *request,
                             UA_ModifyMonitoredItemsResponse *response);

UA_Boolean
Service_SetMonitoringMode(UA_Server *server, UA_Session *session,
                          const UA_SetMonitoringModeRequest *request,
                          UA_SetMonitoringModeResponse *response);

UA_Boolean
Service_SetTriggering(UA_Server *server, UA_Session *session,
                      const UA_SetTriggeringRequest *request,
                      UA_SetTriggeringResponse *response);

/** Subscription Service Set **/
UA_Boolean
Service_CreateSubscription(UA_Server *server, UA_Session *session,
                           const UA_CreateSubscriptionRequest *request,
                           UA_CreateSubscriptionResponse *response);

UA_Boolean
Service_ModifySubscription(UA_Server *server, UA_Session *session,
                           const UA_ModifySubscriptionRequest *request,
                           UA_ModifySubscriptionResponse *response);

UA_Boolean
Service_SetPublishingMode(UA_Server *server, UA_Session *session,
                          const UA_SetPublishingModeRequest *request,
                          UA_SetPublishingModeResponse *response);

UA_Boolean
Service_Publish(UA_Server *server, UA_Session *session,
                const UA_PublishRequest *request,
                UA_PublishResponse *response);

UA_Boolean
Service_Republish(UA_Server *server, UA_Session *session,
                  const UA_RepublishRequest *request,
                  UA_RepublishResponse *response);

UA_Boolean
Service_DeleteSubscriptions(UA_Server *server, UA_Session *session,
                            const UA_DeleteSubscriptionsRequest *request,
                            UA_DeleteSubscriptionsResponse *response);

UA_Boolean
Service_TransferSubscriptions(UA_Server *server, UA_Session *session,
                              const UA_TransferSubscriptionsRequest *request,
                              UA_TransferSubscriptionsResponse *response);

#endif /* UA_ENABLE_SUBSCRIPTIONS */

_UA_END_DECLS

#endif /* UA_SERVICES_H_ */
