/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef TEST_HELPERS_H_
#define TEST_HELPERS_H_

#include <open62541/client.h>
#include <open62541/server.h>
#include "testing_clock.h"

UA_Server *
UA_Server_newForUnitTest(void);

UA_Server *
UA_Server_newForUnitTestWithSecurityPolicies(UA_UInt16 portNumber,
                                             const UA_ByteString *certificate,
                                             const UA_ByteString *privateKey,
                                             const UA_ByteString *trustList,
                                             size_t trustListSize,
                                             const UA_ByteString *issuerList,
                                             size_t issuerListSize,
                                             const UA_ByteString *revocationList,
                                             size_t revocationListSize);

#if defined(__linux__) || defined(UA_ARCHITECTURE_WIN32)
UA_Server *
UA_Server_newForUnitTestWithSecurityPolicies_Filestore(UA_UInt16 portNumber,
                                                       const UA_ByteString *certificate,
                                                       const UA_ByteString *privateKey,
                                                       const UA_String storePath);
#endif /* defined(__linux__) || defined(UA_ARCHITECTURE_WIN32) */

UA_Client * UA_Client_newForUnitTest(void);

#endif /* TEST_HELPERS_H_ */
