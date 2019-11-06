/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 */

#ifndef UA_SERVER_SKS_H
#define UA_SERVER_SKS_H

#include <open62541/server.h>

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_PUBSUB_SECURITY

/* todo remove those later when switching to NS0 */
#define NODEID_SKS_GetSecurityKeys UA_NODEID_NUMERIC(0, UA_NS0ID_PUBSUBKEYSERVICETYPE_GETSECURITYKEYS)
#define NODEID_SKS_SetSecurityKeys UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBETYPE_SETSECURITYKEYS)
#define NODEID_SKS_GetSecurityGroup UA_NODEID_NUMERIC(0, UA_NS0ID_PUBSUBKEYSERVICETYPE_GETSECURITYGROUP)
#define NODEID_SKS_AddSecurityGroup UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_SECURITYGROUPS_ADDSECURITYGROUP)
#define NODEID_SKS_RemoveSecurityGroup UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_SECURITYGROUPS_REMOVESECURITYGROUP)
#define NODEID_SKS_SecurityRootFolder UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_SECURITYGROUPS)
#define NODEID_SKS_SecurityGroupType UA_NODEID_NUMERIC(0, UA_NS0ID_SECURITYGROUPTYPE)
#define NODEID_SKS_PublishSubscribe UA_NODEID_NUMERIC(0, UA_NS0ID_PUBSUBKEYSERVICETYPE)

/**
 * Adds the SKS features to the specified server
 * @param server server instance
 * @return UA_STATUSCODE_GOOD on success,
 * Error codes of UA_Server_setMethodNode_callback
 * UA_STATUSCODE_BADINVALIDARGUMENT for NULL pointers in input
 */
UA_StatusCode UA_EXPORT
UA_Server_addSKS(UA_Server *server);

#endif /* UA_ENABLE_PUBSUB_SECURITY */

_UA_END_DECLS

#endif /* UA_SERVER_SKS_H */

