/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2022 (c) Mark Giraud, Fraunhofer IOSB
 */


#ifndef OPEN62541_CERTSTORE_DEFAULT_H
#define OPEN62541_CERTSTORE_DEFAULT_H

#include <open62541/types.h>
#include <open62541/plugin/certstore.h>

UA_StatusCode
UA_PKIStore_File(UA_PKIStore *pkiStore, UA_NodeId *certificateGroupId);

#endif //OPEN62541_CERTSTORE_DEFAULT_H
