/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) basysKom GmbH <opensource@basyskom.com> (Author: Peter Rustler)
 */

#ifndef UA_PLUGIN_HISTORY_DATA_BACKEND_H_
#define UA_PLUGIN_HISTORY_DATA_BACKEND_H_

#include <open62541/server.h>

_UA_BEGIN_DECLS

typedef enum {
    MATCH_EQUAL, /* Match with the exact timestamp. */
    MATCH_AFTER, /* Match the value with the timestamp in the
                    database that is the first later in time from the provided timestamp. */
    MATCH_EQUAL_OR_AFTER, /* Match exactly if possible, or the first timestamp
                             later in time from the provided timestamp. */
    MATCH_BEFORE, /* Match the first timestamp in the database that is earlier
                     in time from the provided timestamp. */
    MATCH_EQUAL_OR_BEFORE /* Match exactly if possible, or the first timestamp
                             that is earlier in time from the provided timestamp. */
} MatchStrategy;

typedef struct UA_HistoryDataBackend UA_HistoryDataBackend;

struct UA_HistoryDataBackend {
    void *context;

    void
    (*deleteMembers)(UA_HistoryDataBackend *backend);

    /* This function sets a DataValue for a node in the historical data storage.
     *
     * server is the server the node lives in.
     * hdbContext is the context of the UA_HistoryDataBackend.
     * sessionId and sessionContext identify the session that wants to read historical data.
     * nodeId is the node for which the value shall be stored.
     * value is the value which shall be stored.
     * historizing is the historizing flag of the node identified by nodeId.
     * If sessionId is NULL, the historizing flag is invalid and must not be used. */
    UA_StatusCode
    (*serverSetHistoryData)(UA_Server *server,
                            void *hdbContext,
                            const UA_NodeId *sessionId,
                            void *sessionContext,
                            const UA_NodeId *nodeId,
                            UA_Boolean historizing,
                            const UA_DataValue *value);

    /* This function is the high level interface for the ReadRaw operation. Set
     * it to NULL if you use the low level API for your plugin. It should be
     * used if the low level interface does not suite your database. It is more
     * complex to implement the high level interface but it also provide more
     * freedom. If you implement this, then set all low level api function
     * pointer to NULL.
     *
     * server is the server the node lives in.
     * hdbContext is the context of the UA_HistoryDataBackend.
     * sessionId and sessionContext identify the session that wants to read historical data.
     * backend is the HistoryDataBackend whose storage is to be queried.
     * start is the start time of the HistoryRead request.
     * end is the end time of the HistoryRead request.
     * nodeId is the node id of the node for which historical data is requested.
     * maxSizePerResponse is the maximum number of items per response the server can provide.
     * numValuesPerNode is the maximum number of items per response the client wants to receive.
     * returnBounds determines if the client wants to receive bounding values.
     * timestampsToReturn contains the time stamps the client is interested in.
     * range is the numeric range the client wants to read.
     * releaseContinuationPoints determines if the continuation points shall be released.
     * continuationPoint is the continuation point the client wants to release or start from.
     * outContinuationPoint is the continuation point that gets passed to the
     *                      client by the HistoryRead service.
     * result contains the result histoy data that gets passed to the client. */
    UA_StatusCode
    (*getHistoryData)(UA_Server *server,
                      const UA_NodeId *sessionId,
                      void *sessionContext,
                      const UA_HistoryDataBackend *backend,
                      const UA_DateTime start,
                      const UA_DateTime end,
                      const UA_NodeId *nodeId,
                      size_t maxSizePerResponse,
                      UA_UInt32 numValuesPerNode,
                      UA_Boolean returnBounds,
                      UA_TimestampsToReturn timestampsToReturn,
                      UA_NumericRange range,
                      UA_Boolean releaseContinuationPoints,
                      const UA_ByteString *continuationPoint,
                      UA_ByteString *outContinuationPoint,
                      UA_HistoryData *result);

    /* This function is part of the low level HistoryRead API. It returns the
     * index of a value in the database which matches certain criteria.
     *
     * server is the server the node lives in.
     * hdbContext is the context of the UA_HistoryDataBackend.
     * sessionId and sessionContext identify the session that wants to read historical data.
     * nodeId is the node id of the node for which the matching value shall be found.
     * timestamp is the timestamp of the requested index.
     * strategy is the matching strategy which shall be applied in finding the index. */
    size_t
    (*getDateTimeMatch)(UA_Server *server,
                        void *hdbContext,
                        const UA_NodeId *sessionId,
                        void *sessionContext,
                        const UA_NodeId *nodeId,
                        const UA_DateTime timestamp,
                        const MatchStrategy strategy);

    /* This function is part of the low level HistoryRead API. It returns the
     * index of the element after the last valid entry in the database for a
     * node.
     *
     * server is the server the node lives in.
     * hdbContext is the context of the UA_HistoryDataBackend.
     * sessionId and sessionContext identify the session that wants to read historical data.
     * nodeId is the node id of the node for which the end of storage shall be returned. */
    size_t
    (*getEnd)(UA_Server *server,
              void *hdbContext,
              const UA_NodeId *sessionId,
              void *sessionContext,
              const UA_NodeId *nodeId);

    /* This function is part of the low level HistoryRead API. It returns the
     * index of the last element in the database for a node.
     *
     * server is the server the node lives in.
     * hdbContext is the context of the UA_HistoryDataBackend.
     * sessionId and sessionContext identify the session that wants to read historical data.
     * nodeId is the node id of the node for which the index of the last element
     *        shall be returned. */
    size_t
    (*lastIndex)(UA_Server *server,
                 void *hdbContext,
                 const UA_NodeId *sessionId,
                 void *sessionContext,
                 const UA_NodeId *nodeId);

    /* This function is part of the low level HistoryRead API. It returns the
     * index of the first element in the database for a node.
     *
     * server is the server the node lives in.
     * hdbContext is the context of the UA_HistoryDataBackend.
     * sessionId and sessionContext identify the session that wants to read historical data.
     * nodeId is the node id of the node for which the index of the first
     *        element shall be returned. */
    size_t
    (*firstIndex)(UA_Server *server,
                  void *hdbContext,
                  const UA_NodeId *sessionId,
                  void *sessionContext,
                  const UA_NodeId *nodeId);

    /* This function is part of the low level HistoryRead API. It returns the
     * number of elements between startIndex and endIndex including both.
     *
     * server is the server the node lives in.
     * hdbContext is the context of the UA_HistoryDataBackend.
     * sessionId and sessionContext identify the session that wants to read historical data.
     * nodeId is the node id of the node for which the number of elements shall be returned.
     * startIndex is the index of the first element in the range.
     * endIndex is the index of the last element in the range. */
    size_t
    (*resultSize)(UA_Server *server,
                  void *hdbContext,
                  const UA_NodeId *sessionId,
                  void *sessionContext,
                  const UA_NodeId *nodeId,
                  size_t startIndex,
                  size_t endIndex);

    /* This function is part of the low level HistoryRead API. It copies data
     * values inside a certain range into a buffer.
     *
     * server is the server the node lives in.
     * hdbContext is the context of the UA_HistoryDataBackend.
     * sessionId and sessionContext identify the session that wants to read historical data.
     * nodeId is the node id of the node for which the data values shall be copied.
     * startIndex is the index of the first value in the range.
     * endIndex is the index of the last value in the range.
     * reverse determines if the values shall be copied in reverse order.
     * valueSize is the maximal number of data values to copy.
     * range is the numeric range which shall be copied for every data value.
     * releaseContinuationPoints determines if the continuation points shall be released.
     * continuationPoint is a continuation point the client wants to release or start from.
     * outContinuationPoint is a continuation point which will be passed to the client.
     * providedValues contains the number of values that were copied.
     * values contains the values that have been copied from the database. */
    UA_StatusCode
    (*copyDataValues)(UA_Server *server,
                      void *hdbContext,
                      const UA_NodeId *sessionId,
                      void *sessionContext,
                      const UA_NodeId *nodeId,
                      size_t startIndex,
                      size_t endIndex,
                      UA_Boolean reverse,
                      size_t valueSize,
                      UA_NumericRange range,
                      UA_Boolean releaseContinuationPoints,
                      const UA_ByteString *continuationPoint,
                      UA_ByteString *outContinuationPoint,
                      size_t *providedValues,
                      UA_DataValue *values);

    /* This function is part of the low level HistoryRead API. It returns the
     * data value stored at a certain index in the database.
     *
     * server is the server the node lives in.
     * hdbContext is the context of the UA_HistoryDataBackend.
     * sessionId and sessionContext identify the session that wants to read historical data.
     * nodeId is the node id of the node for which the data value shall be returned.
     * index is the index in the database for which the data value is requested. */
    const UA_DataValue*
    (*getDataValue)(UA_Server *server,
                    void *hdbContext,
                    const UA_NodeId *sessionId,
                    void *sessionContext,
                    const UA_NodeId *nodeId,
                    size_t index);

    /* This function returns UA_TRUE if the backend supports returning bounding
     * values for a node. This function is mandatory.
     *
     * server is the server the node lives in.
     * hdbContext is the context of the UA_HistoryDataBackend.
     * sessionId and sessionContext identify the session that wants to read
     *           historical data.
     * nodeId is the node id of the node for which the capability to return
     *        bounds shall be queried. */
    UA_Boolean
    (*boundSupported)(UA_Server *server,
                      void *hdbContext,
                      const UA_NodeId *sessionId,
                      void *sessionContext,
                      const UA_NodeId *nodeId);

    /* This function returns UA_TRUE if the backend supports returning the
     * requested timestamps for a node. This function is mandatory.
     *
     * server is the server the node lives in.
     * hdbContext is the context of the UA_HistoryDataBackend.
     * sessionId and sessionContext identify the session that wants to read historical data.
     * nodeId is the node id of the node for which the capability to return
     *        certain timestamps shall be queried. */
    UA_Boolean
    (*timestampsToReturnSupported)(UA_Server *server,
                                   void *hdbContext,
                                   const UA_NodeId *sessionId,
                                   void *sessionContext,
                                   const UA_NodeId *nodeId,
                                   const UA_TimestampsToReturn timestampsToReturn);

    UA_StatusCode
    (*insertDataValue)(UA_Server *server,
                       void *hdbContext,
                       const UA_NodeId *sessionId,
                       void *sessionContext,
                       const UA_NodeId *nodeId,
                       const UA_DataValue *value);
    UA_StatusCode
    (*replaceDataValue)(UA_Server *server,
                        void *hdbContext,
                        const UA_NodeId *sessionId,
                        void *sessionContext,
                        const UA_NodeId *nodeId,
                        const UA_DataValue *value);
    UA_StatusCode
    (*updateDataValue)(UA_Server *server,
                       void *hdbContext,
                       const UA_NodeId *sessionId,
                       void *sessionContext,
                       const UA_NodeId *nodeId,
                       const UA_DataValue *value);
    UA_StatusCode
    (*removeDataValue)(UA_Server *server,
                       void *hdbContext,
                       const UA_NodeId *sessionId,
                       void *sessionContext,
                       const UA_NodeId *nodeId,
                       UA_DateTime startTimestamp,
                       UA_DateTime endTimestamp);
};

_UA_END_DECLS

#endif /* UA_PLUGIN_HISTORY_DATA_BACKEND_H_ */
