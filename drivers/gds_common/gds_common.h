/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Bastian Engel)
 */

#ifndef UA_DRIVER_GDS_COMMON_H_
#define UA_DRIVER_GDS_COMMON_H_

#include <open62541/types.h>

#ifdef UA_ENABLE_DRIVER_GDS_RECEIVER

_UA_BEGIN_DECLS

/* Helpers of the GDS drivers. They duplicate functionality that the core has
 * but does not expose. */

/* Returns the leaf certificate of a ByteString that contains a chain of
 * concatenated DER or PEM certificates. The leaf is the first certificate. The
 * result points into the chain and is not allocated. A ByteString in another
 * format is returned as is, so that the crypto backend reports the parse
 * error. A null ByteString is returned if the leaf certificate is truncated.
 *
 * The DER detection is the same as getLeafCertificate of the core. */
UA_ByteString
UA_GDSReceiver_getLeafCertificate(UA_ByteString chain);

_UA_END_DECLS

#endif /* UA_ENABLE_DRIVER_GDS_RECEIVER */

#endif /* UA_DRIVER_GDS_COMMON_H_ */
