/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
  *    Copyright 2019 (c) Fraunhofer IOSB (Author: Klaus Schick)
  *    Copyright 2020 (c) SPE UGPA (Author: Andranik Simonian)
 * based on
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014, 2017 (c) Florian Palm
 *    Copyright 2015 (c) Sten Grüner
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_SERVER_ASYNC_H_
#define UA_SERVER_ASYNC_H_

#include "ua_server_internal.h"

_UA_BEGIN_DECLS

#if UA_MULTITHREADING >= 100

/* Async Services */
typedef void (*_UA_Service)(UA_Server*, UA_Session*,
                            const void *request, void *response);
typedef void (*UA_Service_async)(UA_Server *server, UA_Session *session,
                                 UA_UInt32 requestId,
                                 const UA_RequestHeader *request,
                                 UA_ResponseHeader *response);

UA_Service_async
UA_getAsyncService(const UA_DataType *requestType);


#ifdef UA_ENABLE_METHODCALLS
void Service_Call_async(UA_Server *server, UA_Session *session,
                        UA_UInt32 requestId,
                        const UA_RequestHeader *request,
                        UA_ResponseHeader *response);
#endif
#ifdef UA_ENABLE_HISTORIZING
void
Service_HistoryRead_async(UA_Server *server, UA_Session *session,
                          UA_UInt32 requestId,
                          const UA_RequestHeader *request,
                          UA_ResponseHeader *response);
#endif

/* AsyncManager */

struct UA_AsyncManager;
typedef struct UA_AsyncManager UA_AsyncManager;

UA_AsyncManager *UA_AsyncManager_new(UA_Server *server);
void UA_AsyncManager_stop(UA_AsyncManager *am);
void UA_AsyncManager_delete(UA_AsyncManager *am);

UA_StatusCode
UA_AsyncManager_addAsyncTask(UA_AsyncManager *am,
                             const UA_Session *session,
                             UA_UInt32 requestId,
                             const UA_DataType *requestType,
                             const UA_RequestHeader *request,
                             const UA_DataType *responseType,
                             UA_ResponseHeader *response,
                             _UA_Service service);

/* The only difference to UA_Server_processServiceOperations is that
 * UA_Server_processServiceOperationsAsync checks if memory
 * for operations is already allocated */
/* TODO: merge these two functions? */
UA_StatusCode
UA_Server_processServiceOperationsAsync(UA_Server *server, UA_Session *session,
                                        UA_ServiceOperation operationCallback,
                                        const void *context, const size_t *requestOperations,
                                        const UA_DataType *requestOperationsType,
                                        size_t *responseOperations,
                                        const UA_DataType *responseOperationsType)
    UA_FUNC_ATTR_WARN_UNUSED_RESULT;

#endif /* UA_MULTITHREADING >= 100 */

_UA_END_DECLS

#endif /* UA_SERVER_ASYNC_H_ */
