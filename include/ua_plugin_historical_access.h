/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2018 (c) basysKom GmbH <opensource@basyskom.com> (Author: Peter Rustler)
 *    Copyright 2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#ifndef UA_PLUGIN_HISTORICAL_ACCESS_H_
#define UA_PLUGIN_HISTORICAL_ACCESS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_server.h"

/**
 * .. _historical-access:
 *
 * Historical Access Plugin API
 * ============================
 *
 * Storage and access to historical data is implemented in a user-defined
 * plugin. The plugin API does not mandate a specific kind of storage backend or
 * handling of continuation points.
 *
 * The plugin is set globally in the server and used by the HistoryRead and
 * HistoryUpdate services.
 *
 * The function pointers in the HistoryAccess plugin can be set to NULL. The
 * server returns ``UA_STATUSCODE_BADNOTIMPLEMENTED`` if a requested method is
 * not available.
 */

typedef struct {
    void *context;
    void (*deleteMembers)(void *context);

    /* Is the node historizing? The callback is used by the normal Read service
     * for the historizing attribute. */
    UA_Boolean (*isHistorizing)(UA_Server *server, void *haContext,
                                const UA_NodeId *nodeId);

    /***************/
    /* HistoryRead */
    /***************/

    UA_StatusCode
    (*historyRead_raw)(UA_Server *server, void *haContext,
                       const UA_NodeId *sessionId, void *sessionContext,
                       const UA_ReadRawModifiedDetails *details,
                       const UA_HistoryReadValueId *id,
                       UA_TimestampsToReturn timestampsToReturn,
                       UA_Boolean releaseContinuationPoints,
                       UA_ByteString *outContinuationPoint,
                       UA_HistoryData *outHistoryData);

    /* UA_StatusCode */
    /* (*historyRead_modified)(UA_Server *server, void *haContext, */
    /*                         const UA_NodeId *sessionId, void *sessionContext, */
    /*                         const UA_ReadRawModifiedDetails *details, */
    /*                         const UA_HistoryReadValueId *id, */
    /*                         UA_TimestampsToReturn timestampsToReturn, */
    /*                         UA_Boolean releaseContinuationPoints, */
    /*                         UA_ByteString *outContinuationPoint, */
    /*                         UA_HistoryModifiedData *outHistoryData); */

    /* UA_StatusCode */
    /* (*historyRead_processed)(UA_Server *server, void *haContext, */
    /*                          const UA_NodeId *sessionId, void *sessionContext, */
    /*                          const UA_ReadProcessedDetails *details, */
    /*                          const UA_HistoryReadValueId *id, */
    /*                          UA_TimestampsToReturn timestampsToReturn, */
    /*                          UA_Boolean releaseContinuationPoints, */
    /*                          UA_ByteString *outContinuationPoint, */
    /*                          UA_HistoryData *outHistoryData); */

    /* UA_StatusCode */
    /* (*historyRead_atTime)(UA_Server *server, void *haContext, */
    /*                       const UA_NodeId *sessionId, void *sessionContext, */
    /*                       const UA_ReadAtTimeDetails *details, */
    /*                       const UA_HistoryReadValueId *id, */
    /*                       UA_TimestampsToReturn timestampsToReturn, */
    /*                       UA_Boolean releaseContinuationPoints, */
    /*                       UA_ByteString *outContinuationPoint, */
    /*                       UA_HistoryData *outHistoryData); */
    
    /* UA_StatusCode */
    /* (*historyRead_event)(UA_Server *server, void *haContext, */
    /*                      const UA_NodeId *sessionId, void *sessionContext, */
    /*                      const UA_ReadEventDetails *details, */
    /*                      const UA_HistoryReadValueId *id, */
    /*                      UA_TimestampsToReturn timestampsToReturn, */
    /*                      UA_Boolean releaseContinuationPoints, */
    /*                      UA_ByteString *outContinuationPoint, */
    /*                      UA_HistoryEvent *outHistoryEvent); */

    /*****************/
    /* HistoryUpdate */
    /*****************/

    /* outResults is pre-allocated to the size of details->updateValue */
    /* UA_StatusCode */
    /* (*historyUpdate_updateData)(UA_Server *server, void *haContext, */
    /*                             const UA_NodeId *sessionId, void *sessionContext, */
    /*                             const UA_UpdateDataDetails *details, */
    /*                             UA_StatusCode *outResults); */

    
    /* outResults is pre-allocated to the size of details->updateValue */
    /* UA_StatusCode */
    /* (*historyUpdate_updateStructureData)(UA_Server *server, void *haContext, */
    /*                                      const UA_NodeId *sessionId, void *sessionContext, */
    /*                                      const UA_UpdateStructureDataDetails *details, */
    /*                                      UA_StatusCode *outResults); */

    /* outResults is pre-allocated to the size of details->eventData */
    /* UA_StatusCode */
    /* (*historyUpdate_updateEvent)(UA_Server *server, void *haContext, */
    /*                              const UA_NodeId *sessionId, void *sessionContext, */
    /*                              const UA_UpdateEventDetails *details, */
    /*                              UA_StatusCode *outResults); */
    
    /* UA_StatusCode */
    /* (*historyUpdate_deleteRawModified)(UA_Server *server, void *haContext, */
    /*                                    const UA_NodeId *sessionId, void *sessionContext, */
    /*                                    const UA_DeleteRawModifiedDetails *details); */
    
    /* outResults is pre-allocated to the size of details->reqTimes */
    /* UA_StatusCode */
    /* (*historyUpdate_deleteAtTime)(UA_Server *server, void *haContext, */
    /*                               const UA_NodeId *sessionId, void *sessionContext, */
    /*                               const UA_DeleteAtTimeDetails *details, */
    /*                               UA_StatusCode *outResults); */

    /* outResults is pre-allocated to the size of details->eventIds */
    /* UA_StatusCode */
    /* (*historyUpdate_deleteEvent)(UA_Server *server, void *haContext, */
    /*                              const UA_NodeId *sessionId, void *sessionContext, */
    /*                              const UA_DeleteEventDetails *details, */
    /*                              UA_StatusCode *outResults); */
} UA_HistoricalAccess;

#ifdef __cplusplus
}
#endif

#endif /* UA_PLUGIN_HISTORICAL_ACCESS_H_ */
