/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) basysKom GmbH <opensource@basyskom.com> (Author: Peter Rustler)
 */

#include <open62541/client_subscriptions.h>
#include <open62541/plugin/historydata/history_data_gathering_default.h>
#include <open62541/plugin/historydata/history_database_default.h>

#include <string.h>

typedef struct {
    UA_NodeId nodeId;
    UA_HistorizingNodeIdSettings setting;
    UA_MonitoredItemCreateResult monitoredResult;
} UA_NodeIdStoreContextItem_gathering_default;

typedef struct {
    UA_NodeIdStoreContextItem_gathering_default *dataStore;
    size_t storeEnd;
    size_t storeSize;
} UA_NodeIdStoreContext;

static void
dataChangeCallback_gathering_default(UA_Server *server,
                                     UA_UInt32 monitoredItemId,
                                     void *monitoredItemContext,
                                     const UA_NodeId *nodeId,
                                     void *nodeContext,
                                     UA_UInt32 attributeId,
                                     const UA_DataValue *value)
{
    UA_NodeIdStoreContextItem_gathering_default *context = (UA_NodeIdStoreContextItem_gathering_default*)monitoredItemContext;
    context->setting.historizingBackend.serverSetHistoryData(server,
                                                             context->setting.historizingBackend.context,
                                                             NULL,
                                                             NULL,
                                                             nodeId,
                                                             UA_TRUE,
                                                             value);
}

static UA_NodeIdStoreContextItem_gathering_default*
getNodeIdStoreContextItem_gathering_default(UA_NodeIdStoreContext *context,
                                            const UA_NodeId *nodeId)
{
    for (size_t i = 0; i < context->storeEnd; ++i) {
        if (UA_NodeId_equal(&context->dataStore[i].nodeId, nodeId)) {
            return &context->dataStore[i];
        }
    }
    return NULL;
}

static UA_StatusCode
startPoll(UA_Server *server, UA_NodeIdStoreContextItem_gathering_default *item)
{
    UA_MonitoredItemCreateRequest monitorRequest =
            UA_MonitoredItemCreateRequest_default(item->nodeId);
    monitorRequest.requestedParameters.samplingInterval = (double)item->setting.pollingInterval;
    monitorRequest.monitoringMode = UA_MONITORINGMODE_REPORTING;
    item->monitoredResult =
            UA_Server_createDataChangeMonitoredItem(server,
                                                    UA_TIMESTAMPSTORETURN_BOTH,
                                                    monitorRequest,
                                                    item,
                                                    &dataChangeCallback_gathering_default);
    return item->monitoredResult.statusCode;
}

static UA_StatusCode
stopPoll(UA_Server *server, UA_NodeIdStoreContextItem_gathering_default *item)
{
    UA_StatusCode retval = UA_Server_deleteMonitoredItem(server, item->monitoredResult.monitoredItemId);
    UA_MonitoredItemCreateResult_init(&item->monitoredResult);
    return retval;
}

static UA_StatusCode
stopPoll_gathering_default(UA_Server *server,
                           void *context,
                           const UA_NodeId *nodeId)
{
    UA_NodeIdStoreContext *ctx = (UA_NodeIdStoreContext *)context;
    UA_NodeIdStoreContextItem_gathering_default *item = getNodeIdStoreContextItem_gathering_default(ctx, nodeId);
    if (!item) {
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    if (item->setting.historizingUpdateStrategy != UA_HISTORIZINGUPDATESTRATEGY_POLL)
        return UA_STATUSCODE_BADNODEIDINVALID;
    if (item->monitoredResult.monitoredItemId == 0)
        return UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
    return stopPoll(server, item);
}

static UA_StatusCode
startPoll_gathering_default(UA_Server *server,
                            void *context,
                            const UA_NodeId *nodeId)
{
    UA_NodeIdStoreContext *ctx = (UA_NodeIdStoreContext *)context;
    UA_NodeIdStoreContextItem_gathering_default *item = getNodeIdStoreContextItem_gathering_default(ctx, nodeId);
    if (!item) {
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    if (item->setting.historizingUpdateStrategy != UA_HISTORIZINGUPDATESTRATEGY_POLL)
        return UA_STATUSCODE_BADNODEIDINVALID;
    if (item->monitoredResult.monitoredItemId > 0)
        return UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
    return startPoll(server, item);
}

static UA_StatusCode
registerNodeId_gathering_default(UA_Server *server,
                                 void *context,
                                 const UA_NodeId *nodeId,
                                 const UA_HistorizingNodeIdSettings setting)
{
    UA_NodeIdStoreContext *ctx = (UA_NodeIdStoreContext*)context;
    if (getNodeIdStoreContextItem_gathering_default(ctx, nodeId)) {
        return UA_STATUSCODE_BADNODEIDEXISTS;
    }
    if (ctx->storeEnd >= ctx->storeSize) {
        size_t newStoreSize = ctx->storeSize * 2;
        ctx->dataStore = (UA_NodeIdStoreContextItem_gathering_default*)UA_realloc(ctx->dataStore,  (newStoreSize * sizeof(UA_NodeIdStoreContextItem_gathering_default)));
        if (!ctx->dataStore) {
            ctx->storeSize = 0;
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        ctx->storeSize = newStoreSize;
    }
    UA_NodeId_copy(nodeId, &ctx->dataStore[ctx->storeEnd].nodeId);
    size_t current = ctx->storeEnd;
    ctx->dataStore[current].setting = setting;
    ++ctx->storeEnd;
    return UA_STATUSCODE_GOOD;
}

static const UA_HistorizingNodeIdSettings*
getHistorizingSetting_gathering_default(UA_Server *server,
                                        void *context,
                                        const UA_NodeId *nodeId)
{
    UA_NodeIdStoreContext *ctx = (UA_NodeIdStoreContext*)context;
    UA_NodeIdStoreContextItem_gathering_default *item = getNodeIdStoreContextItem_gathering_default(ctx, nodeId);
    if (item) {
        return &item->setting;
    }
    return NULL;
}

static void
deleteMembers_gathering_default(UA_HistoryDataGathering *gathering)
{
    if (gathering == NULL || gathering->context == NULL)
        return;
    UA_NodeIdStoreContext *ctx = (UA_NodeIdStoreContext*)gathering->context;
    for (size_t i = 0; i < ctx->storeEnd; ++i) {
        UA_NodeId_deleteMembers(&ctx->dataStore[i].nodeId);
        // There is still a monitored item present for this gathering
        // You need to remove it with UA_Server_deleteMonitoredItem
        UA_assert(ctx->dataStore[i].monitoredResult.monitoredItemId == 0);
    }
    UA_free(ctx->dataStore);
    UA_free(gathering->context);
}

static UA_Boolean
updateNodeIdSetting_gathering_default(UA_Server *server,
                                      void *context,
                                      const UA_NodeId *nodeId,
                                      const UA_HistorizingNodeIdSettings setting)
{
    UA_NodeIdStoreContext *ctx = (UA_NodeIdStoreContext*)context;
    UA_NodeIdStoreContextItem_gathering_default *item = getNodeIdStoreContextItem_gathering_default(ctx, nodeId);
    if (!item) {
        return false;
    }
    stopPoll_gathering_default(server, context, nodeId);
    item->setting = setting;
    return true;
}

static void
setValue_gathering_default(UA_Server *server,
                           void *context,
                           const UA_NodeId *sessionId,
                           void *sessionContext,
                           const UA_NodeId *nodeId,
                           UA_Boolean historizing,
                           const UA_DataValue *value)
{
    UA_NodeIdStoreContext *ctx = (UA_NodeIdStoreContext*)context;
    UA_NodeIdStoreContextItem_gathering_default *item = getNodeIdStoreContextItem_gathering_default(ctx, nodeId);
    if (!item) {
        return;
    }
    if (item->setting.historizingUpdateStrategy == UA_HISTORIZINGUPDATESTRATEGY_VALUESET) {
        item->setting.historizingBackend.serverSetHistoryData(server,
                                                              item->setting.historizingBackend.context,
                                                              sessionId,
                                                              sessionContext,
                                                              nodeId,
                                                              historizing,
                                                              value);
    }
}

UA_HistoryDataGathering
UA_HistoryDataGathering_Default(size_t initialNodeIdStoreSize)
{
    UA_HistoryDataGathering gathering;
    memset(&gathering, 0, sizeof(UA_HistoryDataGathering));
    gathering.setValue = &setValue_gathering_default;
    gathering.getHistorizingSetting = &getHistorizingSetting_gathering_default;
    gathering.registerNodeId = &registerNodeId_gathering_default;
    gathering.startPoll = &startPoll_gathering_default;
    gathering.stopPoll = &stopPoll_gathering_default;
    gathering.deleteMembers = &deleteMembers_gathering_default;
    gathering.updateNodeIdSetting = &updateNodeIdSetting_gathering_default;
    UA_NodeIdStoreContext *context = (UA_NodeIdStoreContext*)UA_calloc(1, sizeof(UA_NodeIdStoreContext));
    context->storeEnd = 0;
    context->storeSize = initialNodeIdStoreSize;
    context->dataStore = (UA_NodeIdStoreContextItem_gathering_default*)UA_calloc(initialNodeIdStoreSize, sizeof(UA_NodeIdStoreContextItem_gathering_default));
    gathering.context = context;
    return gathering;
}
