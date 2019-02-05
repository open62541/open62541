/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

/**
 *
 * OPC UA PUBSUB PLUGFEST SUBSCRIBER.
 *
 * The code is able to receive and print the DataSets: DataSet1 (Simple), DataSet 2 (AllTypes),
 * DataSet 3 (MassTest), DataSet 4 (AllTypes Dynamic), defined in the 'Test Matrix & Prototype Administration'
 * draft 1.0 21.01.2019.
 *
 */

/**
 * IMPORTANT ANNOUNCEMENT
 * The PubSub subscriber API is currently not finished. This examples can be used to receive
 * and print the values, which are published by the tutorial_pubsub_publish example.
 * The following code uses internal API which will be later replaced by the higher-level
 * PubSub subscriber API.
*/
#include "ua_pubsub_networkmessage.h"
#include "ua_log_stdout.h"
#include "ua_server.h"
#include "ua_config_default.h"
#include "ua_pubsub.h"
#include "ua_network_pubsub_udp.h"
#ifdef UA_ENABLE_PUBSUB_ETH_UADP
#include "ua_network_pubsub_ethernet.h"
#endif
#include "src_generated/ua_types_generated.h"
#include <stdio.h>
#include <signal.h>
#include <ua_pubsub_networkmessage.h>

//UA_UInt32 publisherId = 11;

UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

static void
subscriptionPollingCallback(UA_Server *server, UA_PubSubConnection *connection) {
    UA_ByteString buffer;
    if (UA_ByteString_allocBuffer(&buffer, 4096) != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Message buffer allocation failed!");
        return;
    }
    /* Receive the message. Blocks for 5ms */
    UA_StatusCode retval =
        connection->channel->receive(connection->channel, &buffer, NULL, 5);
    if(retval != UA_STATUSCODE_GOOD || buffer.length == 0) {
        if (retval != UA_STATUSCODE_GOODNONCRITICALTIMEOUT) {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "PubSub receive statuscode: %s", UA_StatusCode_name(retval));
        }
        /* Workaround!! Reset buffer length. Receive can set the length to zero.
         * Then the buffer is not deleted because no memory allocation is
         * assumed.*/
        buffer.length = 4096;
        UA_ByteString_clear(&buffer);
        return;
    }
    /* Decode the message */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Message length: %lu", (unsigned long) buffer.length);
    UA_NetworkMessage networkMessage;
    memset(&networkMessage, 0, sizeof(UA_NetworkMessage));
    size_t currentPosition = 0;
    UA_NetworkMessage_decodeBinary(&buffer, &currentPosition, &networkMessage);
    UA_ByteString_clear(&buffer);

    /* Is this the correct message type? */
    if(networkMessage.networkMessageType != UA_NETWORKMESSAGE_DATASET)
        goto cleanup;

    /* At least one DataSetMessage in the NetworkMessage? */
    if(networkMessage.payloadHeaderEnabled &&
       networkMessage.payloadHeader.dataSetPayloadHeader.count < 1)
        goto cleanup;

 //   if (networkMessage.publisherIdType != UA_PUBLISHERDATATYPE_UINT32 ||
 //       networkMessage.publisherId.publisherIdUInt32 != publisherId)
 //       goto cleanup;

    for (size_t j = 0; j < networkMessage.payloadHeader.dataSetPayloadHeader.count; j++) {
        /* Is this a KeyFrame-DataSetMessage? */
        UA_DataSetMessage *dsm = &networkMessage.payload.dataSetPayload.dataSetMessages[j];
        if (dsm == NULL || dsm->header.dataSetMessageType != UA_DATASETMESSAGE_DATAKEYFRAME)
            //if(dsm->header.dataSetMessageType != UA_DATASETMESSAGE_DATAKEYFRAME)
            goto cleanup;

        /* print type of received plugfest dataset (attention: only based on field count!)*/
        switch (dsm->data.keyFrameData.fieldCount) {
        case 4:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "---- Received DataSet1 (Simple) ----");
            break;
        case 9:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "---- Received DataSet2 (AllTypes) ----");
            break;
        case 100:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "---- Received DataSet3 (MassTest) ----");
            break;
        case 16:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "---- Received DataSet4 (AllTypes Dynamic) ----");
            break;
        }
        switch (networkMessage.publisherIdType) {
        case UA_PUBLISHERDATATYPE_BYTE:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "---> Received From PublisherID (BYTE): %u", networkMessage.publisherId.publisherIdByte);
            break;
        case UA_PUBLISHERDATATYPE_UINT16:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "---> Received From PublisherID (UNIT16): %u", networkMessage.publisherId.publisherIdUInt16);
            break;
        case UA_PUBLISHERDATATYPE_UINT32:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "---> Received From PublisherID (UNIT32): %u", networkMessage.publisherId.publisherIdUInt32);
            break;
        case UA_PUBLISHERDATATYPE_UINT64:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "---> Received From PublisherID (UNIT64): %lu", networkMessage.publisherId.publisherIdUInt64);
            break;
        case UA_PUBLISHERDATATYPE_STRING:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "---> Received From PublisherID (UA_STRING): %s", networkMessage.publisherId.publisherIdString.data);
            break;
        }

        /* Loop over the fields and print well-known content types */
        for (int i = dsm->data.keyFrameData.fieldCount - 1; i >= 0; i--) {
            const UA_DataType *currentType = dsm->data.keyFrameData.dataSetFields[i].value.type;
            if (dsm->data.keyFrameData.dataSetFields[i].value.arrayLength > 1) {
                if (dsm->data.keyFrameData.dataSetFields[i].value.arrayLength == 10) {
                    UA_UInt32 *value = (UA_UInt32 *)dsm->data.keyFrameData.dataSetFields[i].value.data;
                    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Message content: [Array UInt32] "
                        "Received data: {%u, %u, %u, %u, %u, %u, %u, %u, %u, %u}", value[0], value[1],
                        value[2], value[3], value[4], value[5], value[6], value[7], value[8], value[9]);
                }
                else {
                    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Received an unknown array. No print function defined!");
                }

            }
            else if (currentType == &UA_TYPES[UA_TYPES_BYTE]) {
                UA_Byte value = *(UA_Byte *)dsm->data.keyFrameData.dataSetFields[i].value.data;
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Message content: [Byte] \t\tReceived data: %i", value);
            }
            else if (currentType == &UA_TYPES[UA_TYPES_DATETIME]) {
                UA_DateTime value = *(UA_DateTime *)dsm->data.keyFrameData.dataSetFields[i].value.data;
                UA_DateTimeStruct receivedTime = UA_DateTime_toStruct(value);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Message content: [DateTime] \t"
                    "Received date: %02i-%02i-%02i Received time: %02i:%02i:%02i",
                    receivedTime.year, receivedTime.month, receivedTime.day,
                    receivedTime.hour, receivedTime.min, receivedTime.sec);
            }
            else if (currentType == &UA_TYPES[UA_TYPES_BOOLEAN]) {
                UA_Boolean value = *(UA_Boolean *)dsm->data.keyFrameData.dataSetFields[i].value.data;
                char * state;
                if (value)
                    state = "True\0";
                else
                    state = "False\0";
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Message content: [Boolean] \t\t"
                    "Received State: %s", state);
            }
            else if (currentType == &UA_TYPES[UA_TYPES_INT16]) {
                UA_Int16 value = *(UA_Int16 *)dsm->data.keyFrameData.dataSetFields[i].value.data;
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Message content: [Int16] \t\t"
                    "Received data: %i", value);
            }
            else if (currentType == &UA_TYPES[UA_TYPES_INT32]) {
                UA_Int32 value = *(UA_Int32 *)dsm->data.keyFrameData.dataSetFields[i].value.data;
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Message content: [Int32] \t\t"
                    "Received data: %i", value);
            }
            else if (currentType == &UA_TYPES[UA_TYPES_INT64]) {
                UA_Int64 value = *(UA_Int64 *)dsm->data.keyFrameData.dataSetFields[i].value.data;
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Message content: [Int64] \t\t"
                    "Received data: %li", value);
            }
            else if (currentType == &UA_TYPES[UA_TYPES_UINT16]) {
                UA_UInt16 value = *(UA_UInt16 *)dsm->data.keyFrameData.dataSetFields[i].value.data;
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Message content: [UInt16] \t\t"
                    "Received data: %u", value);
            }
            else if (currentType == &UA_TYPES[UA_TYPES_UINT32]) {
                UA_UInt32 value = *(UA_UInt32 *)dsm->data.keyFrameData.dataSetFields[i].value.data;
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Message content: [UInt32] \t\t"
                    "Received data: %u", value);
            }
            else if (currentType == &UA_TYPES[UA_TYPES_UINT64]) {
                UA_UInt64 value = *(UA_UInt64 *)dsm->data.keyFrameData.dataSetFields[i].value.data;
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Message content: [UInt64] \t\t"
                    "Received data: %lu", value);
            }
            else if (currentType == &UA_TYPES[UA_TYPES_SBYTE]) {
                UA_SByte value = *(UA_SByte *)dsm->data.keyFrameData.dataSetFields[i].value.data;
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Message content: [SByte] \t\t"
                    "Received data: %i", value);
            }
            else if (currentType == &UA_TYPES[UA_TYPES_FLOAT]) {
                UA_Float value = *(UA_Float *)dsm->data.keyFrameData.dataSetFields[i].value.data;
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Message content: [Float] \t\t"
                    "Received data: %f", value);
            }
            else if (currentType == &UA_TYPES[UA_TYPES_DOUBLE]) {
                UA_Double value = *(UA_Double *)dsm->data.keyFrameData.dataSetFields[i].value.data;
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Message content: [Double] \t\t"
                    "Received data: %f", value);
            }
            else if (currentType == &UA_TYPES[UA_TYPES_STRING]) {
                UA_String value = *(UA_String *)dsm->data.keyFrameData.dataSetFields[i].value.data;
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Message content: [String] \t\t"
                    "Received data: %s", value.data);
            }
            else if (currentType == &UA_TYPES[UA_TYPES_BYTESTRING]) {
                UA_ByteString value = *(UA_ByteString *)dsm->data.keyFrameData.dataSetFields[i].value.data;
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Message content: [ByteString] \t"
                    "Received data: %s", value.data);
            }
            else if (currentType == &UA_TYPES[UA_TYPES_GUID]) {
                UA_Guid value = *(UA_Guid *)dsm->data.keyFrameData.dataSetFields[i].value.data;
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Message content: [Guid] \t\t"
                    "Received data: {%08u-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
                    value.data1, value.data2, value.data3, value.data4[0], value.data4[1],
                    value.data4[2], value.data4[3], value.data4[4], value.data4[5],
                    value.data4[6], value.data4[7]);
            }
        }
    }

 cleanup:
    UA_NetworkMessage_clear(&networkMessage);
}

static int
run(UA_String *transportProfile, UA_NetworkAddressUrlDataType *networkAddressUrl) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_ServerConfig *config = UA_ServerConfig_new_minimal(4801, NULL);
    /* Details about the PubSubTransportLayer can be found inside the
     * tutorial_pubsub_connection */
    config->pubsubTransportLayers = (UA_PubSubTransportLayer *)
        UA_calloc(2, sizeof(UA_PubSubTransportLayer));
    if (!config->pubsubTransportLayers) {
        UA_ServerConfig_delete(config);
        return -1;
    }
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;
#ifdef UA_ENABLE_PUBSUB_ETH_UADP
    config->pubsubTransportLayers[1] = UA_PubSubTransportLayerEthernet();
    config->pubsubTransportLayersSize++;
#endif
    UA_Server *server = UA_Server_new(config);

    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection 1");
    connectionConfig.transportProfileUri = *transportProfile;
    connectionConfig.enabled = UA_TRUE;
    UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    UA_NodeId connectionIdent;
    UA_StatusCode retval =
        UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
    if(retval == UA_STATUSCODE_GOOD)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                    "The PubSub Connection was created successfully!");

    /* The following lines register the listening on the configured multicast
     * address and configure a repeated job, which is used to handle received
     * messages. */
    UA_PubSubConnection *connection =
        UA_PubSubConnection_findConnectionbyId(server, connectionIdent);
    if(connection != NULL) {
        UA_StatusCode rv = connection->channel->regist(connection->channel, NULL, NULL);
        if (rv == UA_STATUSCODE_GOOD) {
            UA_UInt64 subscriptionCallbackId;
            UA_Server_addRepeatedCallback(server, (UA_ServerCallback)subscriptionPollingCallback,
                                          connection, 100, &subscriptionCallbackId);
        } else {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "register channel failed: %s!",
                           UA_StatusCode_name(rv));
        }
    }

    retval |= UA_Server_run(server, &running);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    return (int)retval;
}


static void
usage(char *progname) {
    printf("usage: %s <uri> [device]\n", progname);
}

int main(int argc, char **argv) {
    UA_String transportProfile =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL , UA_STRING("opc.udp://239.0.0.1:4840/")};

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0) {
            usage(argv[0]);
            return 0;
        } else if (strncmp(argv[1], "opc.udp://", 10) == 0) {
            networkAddressUrl.url = UA_STRING(argv[1]);
        } else if (strncmp(argv[1], "opc.eth://", 10) == 0) {
            transportProfile =
                UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
            if (argc < 3) {
                printf("Error: UADP/ETH needs an interface name\n");
                return 1;
            }
            networkAddressUrl.networkInterface = UA_STRING(argv[2]);
            networkAddressUrl.url = UA_STRING(argv[1]);
        } else {
            printf("Error: unknown URI\n");
            return 1;
        }
    }

    return run(&transportProfile, &networkAddressUrl);
}
