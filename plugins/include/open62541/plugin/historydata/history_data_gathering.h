/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) basysKom GmbH <opensource@basyskom.com> (Author: Peter Rustler)
 */

#ifndef UA_PLUGIN_HISTORY_DATA_GATHERING_H_
#define UA_PLUGIN_HISTORY_DATA_GATHERING_H_

#include "history_data_backend.h"

_UA_BEGIN_DECLS

typedef enum {
    UA_HISTORIZINGUPDATESTRATEGY_USER     = 0x00, /* The user of the api stores the values to the database himself.
                                                     The api will not store any value to the database. */
    UA_HISTORIZINGUPDATESTRATEGY_VALUESET = 0x01, /* Values will be stored when a node's value is set.
                                                     The values will be stored when a node is updated via write service.*/
    UA_HISTORIZINGUPDATESTRATEGY_POLL     = 0x02  /* The value of the node will be read periodically.
                                                     This is mainly relevant for datasource nodes which do
                                                     not use the write service.
                                                     Values will not be stored if the value is
                                                     equal to the old value. */
} UA_HistorizingUpdateStrategy;

typedef struct {
    UA_HistoryDataBackend historizingBackend; /* The database backend used for this node. */
    size_t maxHistoryDataResponseSize; /* The maximum number of values returned by the server in one response.
                                          If the result has more values, continuation points will be used. */
    UA_HistorizingUpdateStrategy historizingUpdateStrategy; /* Defines how the values in the database will be updated.
                                                               See UA_HistorizingUpdateStrategy for details. */
    size_t pollingInterval; /* The polling interval for UA_HISTORIZINGUPDATESTRATEGY_POLL. */
    void *userContext; /* A pointer to store your own settings. */
} UA_HistorizingNodeIdSettings;

typedef struct UA_HistoryDataGathering UA_HistoryDataGathering;
struct UA_HistoryDataGathering {
    void *context;

    void
    (*deleteMembers)(UA_HistoryDataGathering *gathering);

    /* This function registers a node for the gathering of historical data.
     *
     * server is the server the node lives in.
     * hdgContext is the context of the UA_HistoryDataGathering.
     * nodeId is the node id of the node to register.
     * setting contains the gatering settings for the node to register. */
    UA_StatusCode
    (*registerNodeId)(UA_Server *server,
                      void *hdgContext,
                      const UA_NodeId *nodeId,
                      const UA_HistorizingNodeIdSettings setting);

    /* This function stops polling a node for value changes.
     *
     * server is the server the node lives in.
     * hdgContext is the context of the UA_HistoryDataGathering.
     * nodeId is id of the node for which polling shall be stopped.
     * setting contains the gatering settings for the node. */
    UA_StatusCode
    (*stopPoll)(UA_Server *server,
                void *hdgContext,
                const UA_NodeId *nodeId);

    /* This function starts polling a node for value changes.
     *
     * server is the server the node lives in.
     * hdgContext is the context of the UA_HistoryDataGathering.
     * nodeId is the id of the node for which polling shall be started. */
    UA_StatusCode
    (*startPoll)(UA_Server *server,
                 void *hdgContext,
                 const UA_NodeId *nodeId);

    /* This function modifies the gathering settings for a node.
     *
     * server is the server the node lives in.
     * hdgContext is the context of the UA_HistoryDataGathering.
     * nodeId is the node id of the node for which gathering shall be modified.
     * setting contains the new gatering settings for the node. */
    UA_Boolean
    (*updateNodeIdSetting)(UA_Server *server,
                           void *hdgContext,
                           const UA_NodeId *nodeId,
                           const UA_HistorizingNodeIdSettings setting);

    /* Returns the gathering settings for a node.
     *
     * server is the server the node lives in.
     * hdgContext is the context of the UA_HistoryDataGathering.
     * nodeId is the node id of the node for which the gathering settings shall
     *        be retrieved. */
    const UA_HistorizingNodeIdSettings*
    (*getHistorizingSetting)(UA_Server *server,
                             void *hdgContext,
                             const UA_NodeId *nodeId);

    /* Sets a DataValue for a node in the historical data storage.
     *
     * server is the server the node lives in.
     * hdgContext is the context of the UA_HistoryDataGathering.
     * sessionId and sessionContext identify the session which wants to set this value.
     * nodeId is the node id of the node for which a value shall be set.
     * historizing is the historizing flag of the node identified by nodeId.
     * value is the value to set in the history data storage. */
    void
    (*setValue)(UA_Server *server,
                void *hdgContext,
                const UA_NodeId *sessionId,
                void *sessionContext,
                const UA_NodeId *nodeId,
                UA_Boolean historizing,
                const UA_DataValue *value);
};

_UA_END_DECLS

#endif /* UA_PLUGIN_HISTORY_DATA_GATHERING_H_ */
