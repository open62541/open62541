/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Building a simple server
 * ------------------------
 *
 * This series of tutorial guide you through your first steps with open62541.
 * For compiling the examples, you need a compiler (MS Visual Studio 2015 or
 * newer, GCC, Clang and MinGW32 are all known to be working). The compilation
 * instructions are given for GCC but should be straightforward to adapt.
 *
 * It will also be very helpful to install an OPC UA Client with a graphical
 * frontend, such as UAExpert by Unified Automation. That will enable you to
 * examine the information model of any OPC UA server.
 *
 * To get started, downdload the open62541 single-file release from
 * http://open62541.org or generate it according to the :ref:`build instructions
 * <building>` with the "amalgamation" option enabled. From now on, we assume
 * you have the ``open62541.c/.h`` files in the current folder.
 *
 * Now create a new C source-file called ``myServer.c`` with the following content: */

#include <signal.h>
#include "open62541.h"

UA_Boolean running = true;
static void stopHandler(int sig) {
    running = false;
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    // Set desired network layers
    UA_ServerConfig config = UA_ServerConfig_standard;
    UA_ServerNetworkLayer nl;
    nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 4840);
    config.networkLayers = &nl;
    config.networkLayersSize = 1;

    // Set desired security policies
    UA_SecurityPolicy securityPolicies[] = { UA_SecurityPolicy_None };
    config.securityPolicies.count = sizeof(securityPolicies) / sizeof(securityPolicies[0]);
    config.securityPolicies.policies = securityPolicies;

    // Populate policy with needed information
    UA_SecurityPolicy* policy = &config.securityPolicies.policies[0];
    policy->init(policy, config.logger);
    UA_ByteString privateKey = { 1, "A" };
    UA_ByteString certList = { 1,"B" };
    policy->context.setServerPrivateKey(&policy->context, &privateKey);
    policy->context.setCertificateTrustList(&policy->context, &certList);
    policy->context.setCertificateRevocationList(&policy->context, &certList);

    UA_Server *server = UA_Server_new(config);

    if (server == NULL)
        return -1;

    UA_Server_run(server, &running);
    UA_Server_delete(server);
    nl.deleteMembers(&nl);
    return 0;
}

/**
 * This is all that is needed for a simple OPC UA server. Compile the the server
 * with GCC using the following command:
 *
 * .. code-block:: bash
 *
 *    $ gcc -std=c99 open62541.c myServer.c -o myServer
 *
 * Now start the server (and stop with ctrl-c):
 *
 * .. code-block:: bash
 *
 *    $ ./myServer
 *
 * You have now compiled and run your first OPC UA server. You can go ahead and
 * browse the information model with a generic client, such as UAExpert. The
 * server will be listening on ``opc.tcp://localhost:4840`` - go ahead and give
 * it a try.
 *
 * In the following tutorials, you will be shown how to populate the server's
 * information model and how to create a client application. */
