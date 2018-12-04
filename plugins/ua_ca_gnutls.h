/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 *
 *    Copyright 2018 (c) Markus Karch, Fraunhofer IOSB
 */

#ifndef OPEN62541_UA_CA_GNUTLS_H
#define OPEN62541_UA_CA_GNUTLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_plugin_ca.h"
#include "ua_plugin_log.h"

#ifdef UA_ENABLE_GDS_CM

UA_EXPORT UA_StatusCode UA_initCA(UA_GDS_CA *scg,
                                  UA_String caName,
                                  unsigned int caDays,
                                  size_t startSerialNumberSize,
                                  char *startSerialNumber,
                                  unsigned int caBitKeySize,
                                  UA_Logger *logger);

#endif /* UA_ENABLE_GDS_CM */

#ifdef __cplusplus
}
#endif

#endif //OPEN62541_UA_CA_GNUTLS_H
