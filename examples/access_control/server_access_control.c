/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <stdlib.h>
#include <stdio.h>

static UA_Boolean
allowAddNode(UA_Server *server, UA_AccessControl *ac,
             const UA_NodeId *sessionId, void *sessionContext,
             const UA_AddNodesItem *item) {
    printf("Called allowAddNode\n");
    return UA_TRUE;
}

static UA_Boolean
allowAddReference(UA_Server *server, UA_AccessControl *ac,
                  const UA_NodeId *sessionId, void *sessionContext,
                  const UA_AddReferencesItem *item) {
    printf("Called allowAddReference\n");
    return UA_TRUE;
}

static UA_Boolean
allowDeleteNode(UA_Server *server, UA_AccessControl *ac,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_DeleteNodesItem *item) {
    printf("Called allowDeleteNode\n");
    return UA_FALSE; // Do not allow deletion from client
}

static UA_Boolean
allowDeleteReference(UA_Server *server, UA_AccessControl *ac,
                     const UA_NodeId *sessionId, void *sessionContext,
                     const UA_DeleteReferencesItem *item) {
    printf("Called allowDeleteReference\n");
    return UA_TRUE;
}

static UA_UsernamePasswordLogin userNamePW[2] = {
    {UA_STRING_STATIC("peter"), UA_STRING_STATIC("peter123")},
    {UA_STRING_STATIC("paula"), UA_STRING_STATIC("paula123")}
};

static void
setCustomAccessControl(UA_ServerConfig *config) {
    /* Use the default AccessControl plugin as the starting point */
    UA_Boolean allowAnonymous = false;
    UA_String encryptionPolicy =
        config->securityPolicies[config->securityPoliciesSize-1].policyUri;
    config->accessControl.clear(&config->accessControl);
    UA_AccessControl_default(config, allowAnonymous, &encryptionPolicy, 2, userNamePW);

    /* Override accessControl functions for nodeManagement */
    config->accessControl.allowAddNode = allowAddNode;
    config->accessControl.allowAddReference = allowAddReference;
    config->accessControl.allowDeleteNode = allowDeleteNode;
    config->accessControl.allowDeleteReference = allowDeleteReference;
}

int main(void) {
    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    setCustomAccessControl(config);

    UA_StatusCode retval = UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server);

    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
