/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

/**
 * IMPORTANT ANNOUNCEMENT
 * The PubSub subscriber API is currently not finished. This examples can be used to receive
 * and print the values, which are published by the tutorial_pubsub_publish example.
 * The following code uses internal API which will be later replaced by the higher-level
 * PubSub subscriber API.
*/
#include <signal.h>
#include <stdio.h>
#include "ua_pubsub_networkmessage.h"
#include "ua_log_stdout.h"
#include "ua_server.h"
#include "ua_config_default.h"
#include "ua_pubsub.h"
#include "ua_network_pubsub_udp.h"
#include "src_generated/ua_types_generated.h"

UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

static void
subscriptionPollingCallback(UA_Server *server, UA_PubSubConnection *connection) {
    UA_ByteString buffer;
    if (UA_ByteString_allocBuffer(&buffer, 512) != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Message buffer allocation failed!");
        return;
    }

    UA_StatusCode retval = connection->channel->receive(connection->channel, &buffer, NULL, 300000);

    if(retval != UA_STATUSCODE_GOOD || buffer.length == 0) {
        /* Workaround!! Reset buffer length. Receive can set the length to zero.
         * Then the buffer is not deleted because no memory allocation is
         * assumed.
         * TODO: Return an error code in 'receive' instead of setting the buf
         * length to zero. */
        buffer.length = 512;
        UA_ByteString_deleteMembers(&buffer);
        return;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Message received:");
    UA_NetworkMessage actualNetworkMessage;
    memset(&actualNetworkMessage, 0, sizeof(UA_NetworkMessage));
    size_t currentPosition = 0;
    UA_NetworkMessage_decodeBinary(&buffer, &currentPosition, &actualNetworkMessage);
    UA_ByteString_deleteMembers(&buffer);

    printf("Message length: %zu\n", buffer.length);
    if (actualNetworkMessage.networkMessageType == UA_NETWORKMESSAGE_DATASET) {
        if ((actualNetworkMessage.payloadHeaderEnabled && (actualNetworkMessage.payloadHeader.dataSetPayloadHeader.count >= 1)) ||
            (!actualNetworkMessage.payloadHeaderEnabled)) {
            if (actualNetworkMessage.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageType ==
                UA_DATASETMESSAGE_DATAKEYFRAME) {
                for (int i = 0; i < actualNetworkMessage.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.fieldCount; i++) {
                    const UA_DataType *currentType = actualNetworkMessage.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[i].value.type;
                    if (currentType == &UA_TYPES[UA_TYPES_BYTE]) {
                        printf("Message content: [Byte] \n\tReceived data: %i\n",
                               *((UA_Byte *) actualNetworkMessage.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[i].value.data));
                    } else if (currentType == &UA_TYPES[UA_TYPES_DATETIME]) {
                        UA_DateTimeStruct receivedTime = UA_DateTime_toStruct(
                                                                              *((UA_DateTime *) actualNetworkMessage.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[i].value.data));
                        printf("Message content: [DateTime] \n\tReceived date: %02i-%02i-%02i Received time: %02i:%02i:%02i\n", receivedTime.year, receivedTime.month,
                               receivedTime.day, receivedTime.hour, receivedTime.min, receivedTime.sec);
                    }
                }
            }
        }
    }
    UA_NetworkMessage_deleteMembers(&actualNetworkMessage);
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_ServerConfig *config = UA_ServerConfig_new_minimal(4801, NULL);
    /* Details about the PubSubTransportLayer can be found inside the tutorial_pubsub_connection */
    config->pubsubTransportLayers = (UA_PubSubTransportLayer *) UA_malloc(sizeof(UA_PubSubTransportLayer));
    if (!config->pubsubTransportLayers) {
        UA_ServerConfig_delete(config);
        return -1;
    }
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;
    UA_Server *server = UA_Server_new(config);

    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("UDP-UADP Connection 1");
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    connectionConfig.enabled = UA_TRUE;
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL , UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    UA_NodeId connectionIdent;
    UA_StatusCode retval = UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
    if(retval == UA_STATUSCODE_GOOD){
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "The PubSub Connection was created successfully!");
    }

    /* The following lines register the listening on the configured multicast address and configure
     * a repeated job, which is used to handle received messages. */
    UA_PubSubConnection *connection = UA_PubSubConnection_findConnectionbyId(server, connectionIdent);
    if (connection != NULL) {
        UA_StatusCode rv = connection->channel->regist(connection->channel, NULL);
        if (rv == UA_STATUSCODE_GOOD) {
            UA_UInt64 subscriptionCallbackId;
            UA_Server_addRepeatedCallback(server, (UA_ServerCallback)subscriptionPollingCallback, connection, 5, &subscriptionCallbackId);
        } else {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "register channel failed: 0x%x!", rv);
        }
    }

    retval |= UA_Server_run(server, &running);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    return (int)retval;
}
