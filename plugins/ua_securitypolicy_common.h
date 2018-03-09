/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
 
#ifndef UA_SECURITYPOLICY_COMMON_H_
#define UA_SECURITYPOLICY_COMMON_H_

void
sha1(const unsigned char *input, size_t ilen, unsigned char output[20]);

#endif // UA_SECURITYPOLICY_COMMON_H_
