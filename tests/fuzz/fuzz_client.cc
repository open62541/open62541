/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/client_config_default.h>
#include <open62541/types.h>

#include "ua_client_internal.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if(size < 10)
        return 0;

    UA_Client *client = UA_Client_new();
    if(!client)
        return 0;
    
    UA_ClientConfig *config = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(config);
    if(config->logging)
        config->logging->log = NULL; // Disable logging

    // Manually set some states to allow processing responses
    client->channel.state = UA_SECURECHANNELSTATE_OPEN;
    client->sessionState = UA_SESSIONSTATE_ACTIVATED;

    UA_MessageType messageType = (UA_MessageType)data[0];
    UA_UInt32 requestId = *(UA_UInt32*)&data[1];
    
    UA_ByteString message;
    message.length = size - 5;
    message.data = (UA_Byte*)UA_malloc(message.length);
    memcpy(message.data, &data[5], message.length);

    // We need at least one async call to match the requestId
    AsyncServiceCall *ac = (AsyncServiceCall*)UA_malloc(sizeof(AsyncServiceCall));
    ac->requestId = requestId;
    ac->callback = NULL;
    ac->responseType = &UA_TYPES[UA_TYPES_READRESPONSE]; // Just some type
    ac->userdata = NULL;
    ac->syncResponse = NULL;
    LIST_INSERT_HEAD(&client->asyncServiceCalls, ac, pointers);

    processServiceResponse(client, &client->channel, messageType, requestId, &message);

    // Cleanup
    // processServiceResponse might have removed 'ac' if it matched
    AsyncServiceCall *ac2, *tmp;
    LIST_FOREACH_SAFE(ac2, &client->asyncServiceCalls, pointers, tmp) {
        LIST_REMOVE(ac2, pointers);
        UA_free(ac2);
    }

    UA_ByteString_clear(&message);
    UA_Client_delete(client);
    return 0;
}
