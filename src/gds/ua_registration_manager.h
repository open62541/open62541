/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Markus Karch, Fraunhofer IOSB
 */


#ifndef OPEN62541_UA_REGISTRATION_MANAGER_H
#define OPEN62541_UA_REGISTRATION_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_plugin_log.h"
#include "ua_server.h"
#include "ua_record_datatype.h"
#include "open62541_queue.h"

#ifdef UA_ENABLE_GDS

UA_StatusCode
UA_GDS_RegistrationManager_init(UA_Server *server);

UA_StatusCode
UA_GDS_registerApplication(UA_Server *server,
                        UA_ApplicationRecordDataType *input,
                        size_t certificateGroupSize,
                        UA_NodeId *certificateGroupIds,
                        UA_NodeId *output);
UA_StatusCode
UA_GDS_findApplication(UA_Server *server,
                    UA_String *applicationUri,
                    size_t *outputSize,
                    UA_ApplicationRecordDataType **output);

UA_StatusCode
UA_GDS_unregisterApplication(UA_Server *server,
                          UA_NodeId *nodeId);

UA_StatusCode
UA_GDS_RegistrationManager_close(UA_Server *rm);

typedef struct gds_registeredServer_entry {
    LIST_ENTRY(gds_registeredServer_entry) pointers;
    UA_ApplicationRecordDataType gds_registeredServer;
    size_t certificateGroupSize;
    UA_NodeId *certificateGroups;
} gds_registeredServer_entry;

#endif /* UA_ENABLE_GDS */

#ifdef __cplusplus
}
#endif

#endif //OPEN62541_UA_REGISTRATION_MANAGER_H
