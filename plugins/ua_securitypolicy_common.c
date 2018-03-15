/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mbedtls/sha1.h>
#include <mbedtls/version.h>

#include "ua_securitypolicy_common.h"


void
sha1(const unsigned char *input, size_t ilen, unsigned char output[20]) {
    mbedtls_sha1_context sha1Context;
    mbedtls_sha1_init(&sha1Context);
#if MBEDTLS_VERSION_NUMBER >= 0x02070000
    // TODO check return status / actually only/always return 0
    mbedtls_sha1_starts_ret(&sha1Context);
    // TODO check return status / actually only/always return 0
    mbedtls_sha1_update_ret(&sha1Context, input, ilen);
    // TODO check return status / actually only/always return 0
    mbedtls_sha1_finish_ret(&sha1Context, output);
#else
    mbedtls_sha1_starts(&sha1Context);
    mbedtls_sha1_update(&sha1Context, input, ilen);
    mbedtls_sha1_finish(&sha1Context, output);
#endif
    mbedtls_sha1_free(&sha1Context);
}
