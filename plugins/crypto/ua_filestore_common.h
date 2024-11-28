/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include <open62541/util.h>

#ifdef UA_ENABLE_ENCRYPTION

#ifdef __linux__ /* Linux only so far */

UA_StatusCode
readFileToByteString(const char *const path,
                     UA_ByteString *data);

UA_StatusCode
writeByteStringToFile(const char *const path,
                      const UA_ByteString *data);

#endif /* __linux__ */

#endif /* UA_ENABLE_ENCRYPTION */
