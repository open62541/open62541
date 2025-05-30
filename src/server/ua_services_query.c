/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "ua_services.h"
#include "ua_server_internal.h"

#ifdef UA_ENABLE_QUERY /* conditional compilation */

void Service_QueryFirst(UA_Server *server, UA_Session *session,
                        const UA_QueryFirstRequest *request,
                        UA_QueryFirstResponse *response) {
    response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTIMPLEMENTED;
}

void Service_QueryNext(UA_Server *server, UA_Session *session,
                       const UA_QueryNextRequest *request,
                       UA_QueryNextResponse *response) {
    response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTIMPLEMENTED;
}

#endif /* UA_ENABLE_QUERY */
