/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/* This example shows how a server-side application defines its own policy
 * for how strictly to limit the size of in-flight messages on SecureChannels
 * it does not (yet) consider trusted, via UA_ServerConfig.
 * messageSizeLimitCallback.
 *
 * The server always enforces UA_ServerConfig.tcpMaxMsgSize as a hard ceiling
 * for every channel. messageSizeLimitCallback is an optional, additional
 * hook: it is invoked for every chunk before it is fully received (so it
 * must be cheap and non-blocking) and may tighten -- or loosen -- that
 * ceiling for the specific channel currently receiving data. It exists
 * because open62541 does not dictate a single definition of "trusted": some
 * deployments want a strict cap on unauthenticated traffic (e.g. to blunt
 * resource-exhaustion attempts against the Discovery Service Set before any
 * Session exists), while others run in a closed network and want no extra
 * restriction at all. Leaving the callback unset (the default) keeps
 * today's behavior of a single static tcpMaxMsgSize for every channel. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

static UA_Boolean running = true;
static void stopHandler(int sig) {
    running = false;
}

/* SecureChannels that are neither signed nor encrypted, and that have no
 * Session with a completed ActivateSession yet, are capped to this size.
 * This mainly limits what an anonymous, unauthenticated client can throw at
 * the Discovery Service Set before it has proven anything about itself. */
#define UA_UNTRUSTED_MAX_MESSAGE_SIZE (64 * 1024) /* 64 KB */

static UA_UInt32
limitUntrustedMessageSize(UA_Server *server, void *context,
                          UA_UInt32 secureChannelId,
                          UA_MessageSecurityMode securityMode,
                          UA_Boolean sessionActivated,
                          UA_UInt32 defaultMaxMessageSize) {
    (void)server;
    (void)context;
    (void)secureChannelId;

    UA_Boolean trusted = sessionActivated ||
        securityMode == UA_MESSAGESECURITYMODE_SIGN ||
        securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    if(trusted)
        return 0; /* No override -- fall back to defaultMaxMessageSize
                    * (the server's tcpMaxMsgSize) */

    return UA_UNTRUSTED_MAX_MESSAGE_SIZE;
}

int
main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    config->messageSizeLimitCallback = limitUntrustedMessageSize;
    config->messageSizeLimitContext = NULL; /* No extra state needed here */

    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
