/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 */

#include "ua_plugin_securitypolicy.h"

size_t
UA_SecurityPolicy_getRemoteAsymEncryptionBufferLengthOverhead(const UA_SecurityPolicy *securityPolicy,
                                                              const void *channelContext,
                                                              size_t maxEncryptionLength) {
    if(maxEncryptionLength == 0)
        return 0;
    const size_t plainTextBlockSize = securityPolicy->asymmetricModule.cryptoModule.encryptionAlgorithm.
        getRemotePlainTextBlockSize(securityPolicy, channelContext);
    const size_t encryptedBlockSize = securityPolicy->asymmetricModule.cryptoModule.encryptionAlgorithm.
        getRemoteBlockSize(securityPolicy, channelContext);
    if(plainTextBlockSize == 0)
        return 0;
    const size_t maxNumberOfBlocks =
        maxEncryptionLength / plainTextBlockSize;
    return maxNumberOfBlocks * (encryptedBlockSize - plainTextBlockSize);
}
