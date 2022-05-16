/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) basysKom GmbH <opensource@basyskom.com> (Author: Peter Rustler)
 */

#ifndef UA_PLUGIN_HISTORYDATABASE_H_
#define UA_PLUGIN_HISTORYDATABASE_H_

#include <open62541/util.h>

_UA_BEGIN_DECLS

typedef struct UA_HistoryDatabase UA_HistoryDatabase;

struct UA_HistoryDatabase {
    void *context;

    void (*clear)(UA_HistoryDatabase *hdb);

    /* This function will be called when a nodes value is set.
     * Use this to insert data into your database(s) if polling is not suitable
     * and you need to get all data changes.
     * Set it to NULL if you do not need it.
     *
     * server is the server this node lives in.
     * hdbContext is the context of the UA_HistoryDatabase.
     * sessionId and sessionContext identify the session which set this value.
     * nodeId is the node id for which data was set.
     * historizing is the nodes boolean flag for historizing
     * value is the new value. */
    void
    (*setValue)(UA_Server *server,
                void *hdbContext,
                const UA_NodeId *sessionId,
                void *sessionContext,
                const UA_NodeId *nodeId,
                UA_Boolean historizing,
                const UA_DataValue *value);

    /* This function will be called when an event is triggered.
     * Use it to insert data into your event database.
     * No default implementation is provided by UA_HistoryDatabase_default.
     *
     * server is the server this node lives in.
     * hdbContext is the context of the UA_HistoryDatabase.
     * originId is the node id of the event's origin.
     * emitterId is the node id of the event emitter.
     * historicalEventFilter is the value of the HistoricalEventFilter property of
     *                       the emitter (OPC UA Part 11, 5.3.2), it is NULL if
     *                       the property does not exist or is not set.
     * fieldList is the event field list returned after application of
     *           historicalEventFilter to the event node. */
    void
    (*setEvent)(UA_Server *server,
                void *hdbContext,
                const UA_NodeId *originId,
                const UA_NodeId *emitterId,
                const UA_EventFilter *historicalEventFilter,
                UA_EventFieldList *fieldList);

    /* This function is called if a history read is requested with
     * isRawReadModified set to false. Setting it to NULL will result in a
     * response with statuscode UA_STATUSCODE_BADHISTORYOPERATIONUNSUPPORTED.
     *
     * server is the server this node lives in.
     * hdbContext is the context of the UA_HistoryDatabase.
     * sessionId and sessionContext identify the session which set this value.
     * requestHeader, historyReadDetails, timestampsToReturn, releaseContinuationPoints
     * nodesToReadSize and nodesToRead is the requested data from the client. It
     *                 is from the request object.
     * response the response to fill for the client. If the request is ok, there
     *          is no need to use it. Use this to set status codes other than
     *          "Good" or other data. You find an already allocated
     *          UA_HistoryReadResult array with an UA_HistoryData object in the
     *          extension object in the size of nodesToReadSize. If you are not
     *          willing to return data, you have to delete the results array,
     *          set it to NULL and set the resultsSize to 0. Do not access
     *          historyData after that.
     * historyData is a proper typed pointer array pointing in the
     *             UA_HistoryReadResult extension object. use this to provide
     *             result data to the client. Index in the array is the same as
     *             in nodesToRead and the UA_HistoryReadResult array. */
    void
    (*readRaw)(UA_Server *server,
               void *hdbContext,
               const UA_NodeId *sessionId,
               void *sessionContext,
               const UA_RequestHeader *requestHeader,
               const UA_ReadRawModifiedDetails *historyReadDetails,
               UA_TimestampsToReturn timestampsToReturn,
               UA_Boolean releaseContinuationPoints,
               size_t nodesToReadSize,
               const UA_HistoryReadValueId *nodesToRead,
               UA_HistoryReadResponse *response,
               UA_HistoryData * const * const historyData);

    /* No default implementation is provided by UA_HistoryDatabase_default
     * for the following function */
    void
    (*readModified)(UA_Server *server,
               void *hdbContext,
               const UA_NodeId *sessionId,
               void *sessionContext,
               const UA_RequestHeader *requestHeader,
               const UA_ReadRawModifiedDetails *historyReadDetails,
               UA_TimestampsToReturn timestampsToReturn,
               UA_Boolean releaseContinuationPoints,
               size_t nodesToReadSize,
               const UA_HistoryReadValueId *nodesToRead,
               UA_HistoryReadResponse *response,
               UA_HistoryModifiedData * const * const historyData);

    /* No default implementation is provided by UA_HistoryDatabase_default
     * for the following function */
    void
    (*readEvent)(UA_Server *server,
               void *hdbContext,
               const UA_NodeId *sessionId,
               void *sessionContext,
               const UA_RequestHeader *requestHeader,
               const UA_ReadEventDetails *historyReadDetails,
               UA_TimestampsToReturn timestampsToReturn,
               UA_Boolean releaseContinuationPoints,
               size_t nodesToReadSize,
               const UA_HistoryReadValueId *nodesToRead,
               UA_HistoryReadResponse *response,
               UA_HistoryEvent * const * const historyData);

    /* No default implementation is provided by UA_HistoryDatabase_default
     * for the following function */
    void
    (*readProcessed)(UA_Server *server,
               void *hdbContext,
               const UA_NodeId *sessionId,
               void *sessionContext,
               const UA_RequestHeader *requestHeader,
               const UA_ReadProcessedDetails *historyReadDetails,
               UA_TimestampsToReturn timestampsToReturn,
               UA_Boolean releaseContinuationPoints,
               size_t nodesToReadSize,
               const UA_HistoryReadValueId *nodesToRead,
               UA_HistoryReadResponse *response,
               UA_HistoryData * const * const historyData);

    /* No default implementation is provided by UA_HistoryDatabase_default
     * for the following function */
    void
    (*readAtTime)(UA_Server *server,
               void *hdbContext,
               const UA_NodeId *sessionId,
               void *sessionContext,
               const UA_RequestHeader *requestHeader,
               const UA_ReadAtTimeDetails *historyReadDetails,
               UA_TimestampsToReturn timestampsToReturn,
               UA_Boolean releaseContinuationPoints,
               size_t nodesToReadSize,
               const UA_HistoryReadValueId *nodesToRead,
               UA_HistoryReadResponse *response,
               UA_HistoryData * const * const historyData);

    void
    (*updateData)(UA_Server *server,
                  void *hdbContext,
                  const UA_NodeId *sessionId,
                  void *sessionContext,
                  const UA_RequestHeader *requestHeader,
                  const UA_UpdateDataDetails *details,
                  UA_HistoryUpdateResult *result);

    void
    (*deleteRawModified)(UA_Server *server,
                         void *hdbContext,
                         const UA_NodeId *sessionId,
                         void *sessionContext,
                         const UA_RequestHeader *requestHeader,
                         const UA_DeleteRawModifiedDetails *details,
                         UA_HistoryUpdateResult *result);

    /* Add more function pointer here.
     * For example for read_event, read_annotation, update_details */
};

_UA_END_DECLS

#endif /* UA_PLUGIN_HISTORYDATABASE_H_ */
