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
                    const void *request /* UA_FindServersRequest */,
                    void *response /* UA_FindServersResponse */);

UA_Boolean
Service_GetEndpoints(UA_Server *server, UA_Session *session,
                     const void *request /* UA_GetEndpointsRequest */,
                     void *response /* UA_GetEndpointsResponse */);

#ifdef UA_ENABLE_DISCOVERY

UA_Boolean
Service_RegisterServer(UA_Server *server, UA_Session *session,
                       const void *request /* UA_RegisterServerRequest */,
                       void *response /* UA_RegisterServerResponse */);

UA_Boolean
Service_RegisterServer2(UA_Server *server, UA_Session *session,
                        const void *request /* UA_RegisterServer2Request */,
                        void *response /* UA_RegisterServer2Response */);

UA_Boolean
Service_FindServersOnNetwork(UA_Server *server, UA_Session *session,
                             const void *request /* UA_FindServersOnNetworkRequest */,
                             void *response /* UA_FindServersOnNetworkResponse */);

#endif /* UA_ENABLE_DISCOVERY */

/** SecureChannel Service Set **/
void
Service_OpenSecureChannel(UA_Server *server, UA_SecureChannel* channel,
                          void *request /* UA_OpenSecureChannelRequest */,
                          void *response /* UA_OpenSecureChannelResponse */);

void
Service_CloseSecureChannel(UA_Server *server, UA_SecureChannel *channel);

/** Session Service Set **/
void
Service_CreateSession(UA_Server *server, UA_SecureChannel *channel,
                      const void *request /* UA_CreateSessionRequest */,
                      void *response /* UA_CreateSessionResponse */);

void
Service_ActivateSession(UA_Server *server, UA_SecureChannel *channel,
                        const void *request /* UA_ActivateSessionRequest */,
                        void *response /* UA_ActivateSessionResponse */);

void
Service_CloseSession(UA_Server *server, UA_SecureChannel *channel,
                     const void *request /* UA_CloseSessionRequest */,
                     void *response /* UA_CloseSessionResponse */);

UA_Boolean
Service_Cancel(UA_Server *server, UA_Session *session,
               const void *request /* UA_CancelRequest */, void *response /* UA_CancelResponse */);

/** NodeManagement Service Set **/
UA_Boolean
Service_AddNodes(UA_Server *server, UA_Session *session,
                 const void *request /* UA_AddNodesRequest */,
                 void *response /* UA_AddNodesResponse */);

UA_Boolean
Service_AddReferences(UA_Server *server, UA_Session *session,
                      const void *request /* UA_AddReferencesRequest */,
                      void *response /* UA_AddReferencesResponse */);

UA_Boolean
Service_DeleteNodes(UA_Server *server, UA_Session *session,
                    const void *request /* UA_DeleteNodesRequest */,
                    void *response /* UA_DeleteNodesResponse */);

UA_Boolean
Service_DeleteReferences(UA_Server *server, UA_Session *session,
                         const void *request /* UA_DeleteReferencesRequest */,
                         void *response /* UA_DeleteReferencesResponse */);

/** View Service Set **/
UA_Boolean
Service_Browse(UA_Server *server, UA_Session *session,
               const void *request /* UA_BrowseRequest */, void *response /* UA_BrowseResponse */);

UA_Boolean
Service_BrowseNext(UA_Server *server, UA_Session *session,
                   const void *request /* UA_BrowseNextRequest */,
                   void *response /* UA_BrowseNextResponse */);

UA_Boolean
Service_TranslateBrowsePathsToNodeIds(UA_Server *server, UA_Session *session,
    const void *request /* UA_TranslateBrowsePathsToNodeIdsRequest */,
    void *response /* UA_TranslateBrowsePathsToNodeIdsResponse */);

UA_Boolean
Service_RegisterNodes(UA_Server *server, UA_Session *session,
                      const void *request /* UA_RegisterNodesRequest */,
                      void *response /* UA_RegisterNodesResponse */);

UA_Boolean
Service_UnregisterNodes(UA_Server *server, UA_Session *session,
                        const void *request /* UA_UnregisterNodesRequest */,
                        void *response /* UA_UnregisterNodesResponse */);

/** Query Service Set (not implemented) **/

/** Attribute Service Set **/
UA_Boolean
Service_Read(UA_Server *server, UA_Session *session,
             const void *request /* UA_ReadRequest */, void *response /* UA_ReadResponse */);

UA_Boolean
Operation_Read(UA_Server *server, UA_Session *session,
               UA_TimestampsToReturn ttr,
               const UA_ReadValueId *rvi, UA_DataValue *dv);

UA_Boolean
Service_Write(UA_Server *server, UA_Session *session,
              const void *request /* UA_WriteRequest */, void *response /* UA_WriteResponse */);

UA_Boolean
Operation_Write(UA_Server *server, UA_Session *session,
                const UA_WriteValue *wv, UA_StatusCode *result);

#ifdef UA_ENABLE_HISTORIZING
UA_Boolean
Service_HistoryRead(UA_Server *server, UA_Session *session,
                    const void *request /* UA_HistoryReadRequest */,
                    void *response /* UA_HistoryReadResponse */);

UA_Boolean
Service_HistoryUpdate(UA_Server *server, UA_Session *session,
                      const void *request /* UA_HistoryUpdateRequest */,
                      void *response /* UA_HistoryUpdateResponse */);
#endif

/** Method Service Set **/
#ifdef UA_ENABLE_METHODCALLS
UA_Boolean
Service_Call(UA_Server *server, UA_Session *session,
             const void *request /* UA_CallRequest */, void *response /* UA_CallResponse */);

UA_Boolean
Operation_CallMethod(UA_Server *server, UA_Session *session,
                     const UA_CallMethodRequest *request,
                     UA_CallMethodResult *result);
#endif

#ifdef UA_ENABLE_SUBSCRIPTIONS

/** MonitoredItem Service Set **/
UA_Boolean
Service_CreateMonitoredItems(UA_Server *server, UA_Session *session,
                             const void *request /* UA_CreateMonitoredItemsRequest */,
                             void *response /* UA_CreateMonitoredItemsResponse */);

UA_Boolean
Service_DeleteMonitoredItems(UA_Server *server, UA_Session *session,
                             const void *request /* UA_DeleteMonitoredItemsRequest */,
                             void *response /* UA_DeleteMonitoredItemsResponse */);

UA_Boolean
Service_ModifyMonitoredItems(UA_Server *server, UA_Session *session,
                             const void *request /* UA_ModifyMonitoredItemsRequest */,
                             void *response /* UA_ModifyMonitoredItemsResponse */);

UA_Boolean
Service_SetMonitoringMode(UA_Server *server, UA_Session *session,
                          const void *request /* UA_SetMonitoringModeRequest */,
                          void *response /* UA_SetMonitoringModeResponse */);

UA_Boolean
Service_SetTriggering(UA_Server *server, UA_Session *session,
                      const void *request /* UA_SetTriggeringRequest */,
                      void *response /* UA_SetTriggeringResponse */);

/** Subscription Service Set **/
UA_Boolean
Service_CreateSubscription(UA_Server *server, UA_Session *session,
                           const void *request /* UA_CreateSubscriptionRequest */,
                           void *response /* UA_CreateSubscriptionResponse */);

UA_Boolean
Service_ModifySubscription(UA_Server *server, UA_Session *session,
                           const void *request /* UA_ModifySubscriptionRequest */,
                           void *response /* UA_ModifySubscriptionResponse */);

UA_Boolean
Service_SetPublishingMode(UA_Server *server, UA_Session *session,
                          const void *request /* UA_SetPublishingModeRequest */,
                          void *response /* UA_SetPublishingModeResponse */);

UA_Boolean
Service_Publish(UA_Server *server, UA_Session *session,
                const void *request /* UA_PublishRequest */,
                void *response /* UA_PublishResponse */);

UA_Boolean
Service_Republish(UA_Server *server, UA_Session *session,
                  const void *request /* UA_RepublishRequest */,
                  void *response /* UA_RepublishResponse */);

UA_Boolean
Service_DeleteSubscriptions(UA_Server *server, UA_Session *session,
                            const void *request /* UA_DeleteSubscriptionsRequest */,
                            void *response /* UA_DeleteSubscriptionsResponse */);

UA_Boolean
Service_TransferSubscriptions(UA_Server *server, UA_Session *session,
                              const void *request /* UA_TransferSubscriptionsRequest */,
                              void *response /* UA_TransferSubscriptionsResponse */);

#endif /* UA_ENABLE_SUBSCRIPTIONS */

_UA_END_DECLS

#endif /* UA_SERVICES_H_ */
