/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
*    Copyright 2019 (c) Fraunhofer IOSB (Author: Klaus Schick)
*/

#ifndef UA_SERVER_METHODQUEUE_H_
#define	UA_SERVER_METHODQUEUE_H_

#include <open62541/types_generated.h>
#include "open62541_queue.h"
#include "ua_server_internal.h"

#if UA_MULTITHREADING >= 100

_UA_BEGIN_DECLS

/* Public API, see server.h  */

/* Private API */
/* Initialize Request Queue
*
* @param server The server object */
void UA_Server_MethodQueues_init(UA_Server *server);

/* Cleanup and terminate queues
*
* @param server The server object */
void UA_Server_MethodQueues_delete(UA_Server *server);

/* Checks queue element timeouts
*
* @param server The server object */
void UA_Server_CheckQueueIntegrity(UA_Server *server, void *data);

/* Deep delete of queue element (except UA_CallMethodRequest which is managed by the caller)
*
* @param server The server object
* @param pElem Pointer to queue element */
void UA_Server_DeleteMethodQueueElement(UA_Server *server, struct AsyncMethodQueueElement *pElem);


/* Internal functions, declared here to be accessible by unit test(s) */

/* Add element to pending list 
*
* @param server The server object
* @param pElem Pointer to queue element */
void UA_Server_AddPendingMethodCall(UA_Server* server, struct AsyncMethodQueueElement *pElem);

/* Remove element from pending list
*
* @param server The server object
* @param pElem Pointer to queue element */
void UA_Server_RmvPendingMethodCall(UA_Server *server, struct AsyncMethodQueueElement *pElem);

/* check if element is in pending list
*
* @param server The server object
* @param pElem Pointer to queue element
* @return UA_TRUE if element was found, UA_FALSE else */
UA_Boolean UA_Server_IsPendingMethodCall(UA_Server *server, struct AsyncMethodQueueElement *pElem);


/* Enqueue next MethodRequest
*
* @param server The server object
* @param Pointer to UA_NodeId (pSessionIdentifier)
* @param UA_UInt32 (unique requestId/server)
* @param Index of request within UA_Callrequest
* @param Pointer to UA_CallMethodRequest
* @return UA_STATUSCODE_GOOD if request was enqueue,
    UA_STATUSCODE_BADUNEXPECTEDERROR if queue full
*	UA_STATUSCODE_BADOUTOFMEMORY if no memory available */
UA_StatusCode UA_Server_SetNextAsyncMethod(UA_Server *server,
    const UA_UInt32 nRequestId,
    const UA_NodeId *nSessionId,
    const UA_UInt32 nIndex,
    const UA_CallMethodRequest* pRequest);

/* Get next Method Call Response, user has to call 'UA_DeleteMethodQueueElement(...)' to cleanup memory
*
* @param server The server object
* @param Receives pointer to AsyncMethodQueueElement
* @return UA_FALSE if queue is empty, UA_TRUE else */
UA_Boolean UA_Server_GetAsyncMethodResult(UA_Server *server, struct AsyncMethodQueueElement **pResponse);

_UA_END_DECLS

#endif /* !UA_MULTITHREADING >= 100 */

#endif /* !UA_SERVER_METHODQUEUE_H_ */
