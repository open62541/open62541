/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#ifndef UA_CONFIG_STANDARD_H_
#define UA_CONFIG_STANDARD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_server.h"
#include "ua_client.h"
#include "ua_client_highlevel.h"

/**********************/
/* Default Connection */
/**********************/

extern const UA_EXPORT UA_ConnectionConfig UA_ConnectionConfig_default;

/*************************/
/* Default Server Config */
/*************************/

/* Creates a new server config with one endpoint.
 * 
 * The config will set the tcp network layer to the given port and adds a single
 * endpoint with the security policy ``SecurityPolicy#None`` to the server. A
 * server certificate may be supplied but is optional.
 *
 * @param portNumber The port number for the tcp network layer
 * @param certificate Optional certificate for the server endpoint. Can be
 *        ``NULL``. */
UA_EXPORT UA_ServerConfig *
UA_ServerConfig_new_minimal(UA_UInt16 portNumber,
                            const UA_ByteString *certificate);

/* Creates a server config on the standard port 4840 with no server
 * certificate. */
static UA_INLINE UA_ServerConfig *
UA_ServerConfig_new_default(void) {
    return UA_ServerConfig_new_minimal(4840, NULL);
}

/* Frees allocated memory in the server config */
UA_EXPORT void
UA_ServerConfig_delete(UA_ServerConfig *config);

/*************************/
/* Default Client Config */
/*************************/

extern const UA_EXPORT UA_ClientConfig UA_ClientConfig_default;

#ifdef __cplusplus
}
#endif

#endif /* UA_CONFIG_STANDARD_H_ */
