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

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "ua_types_encoding_binary.h"
#include "ua_pubsub_networkmessage.h"
#include "ua_util_internal.h"

#include <signal.h>

#ifdef UA_ENABLE_PUBSUB_ETH_UADP
#include <open62541/plugin/pubsub_ethernet.h>
#endif

UA_Boolean running = true;
static UA_StatusCode
customDecodeAndProcessCallback(UA_PubSubChannel *psc, void* ctx, const UA_ByteString *buffer);
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "received ctrl-c");
    running = false;
}

static UA_StatusCode
subscriberListen(UA_PubSubChannel *psc) {
    UA_ByteString buffer;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&buffer, 512);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Message buffer allocation failed!");
        return retval;
    }

    /* Receive the message. Blocks for 100ms */
    UA_StatusCode rv = psc->receive(psc, NULL, customDecodeAndProcessCallback, NULL, 100);

    UA_ByteString_clear(&buffer);
    return rv;
}
static UA_StatusCode
customDecodeAndProcessCallback(UA_PubSubChannel *psc, void *ctx, const UA_ByteString *buffer) {

    /* Decode the message */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Message length: %lu", (unsigned long) (*buffer).length);
    UA_NetworkMessage networkMessage;
    memset(&networkMessage, 0, sizeof(UA_NetworkMessage));
    size_t currentPosition = 0;
    UA_NetworkMessage_decodeBinary(buffer, &currentPosition, &networkMessage);

    /* Is this the correct message type? */
    if(networkMessage.networkMessageType != UA_NETWORKMESSAGE_DATASET)
        goto cleanup;

    /* At least one DataSetMessage in the NetworkMessage? */
    if(networkMessage.payloadHeaderEnabled &&
       networkMessage.payloadHeader.dataSetPayloadHeader.count < 1)
        goto cleanup;

    /* Is this a KeyFrame-DataSetMessage? */
    for(size_t j = 0; j < networkMessage.payloadHeader.dataSetPayloadHeader.count; j++) {
        UA_DataSetMessage *dsm = &networkMessage.payload.dataSetPayload.dataSetMessages[j];
        if(dsm->header.dataSetMessageType != UA_DATASETMESSAGE_DATAKEYFRAME)
            continue;
        if(dsm->header.fieldEncoding == UA_FIELDENCODING_RAWDATA){
            //The RAW-Encoded payload contains no fieldCount information
            UA_DateTime dateTime;
            size_t offset = 0;
            UA_DateTime_decodeBinary(&dsm->data.keyFrameData.rawFields, &offset, &dateTime);
            //UA_DateTime value = *(UA_DateTime *)dsm->data.keyFrameData.rawFields->data;
            UA_DateTimeStruct receivedTime = UA_DateTime_toStruct(dateTime);
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Message content: [DateTime] \t"
                        "Received date: %02i-%02i-%02i Received time: %02i:%02i:%02i",
                        receivedTime.year, receivedTime.month, receivedTime.day,
                        receivedTime.hour, receivedTime.min, receivedTime.sec);
        } else {
            /* Loop over the fields and print well-known content types */
            for(int i = 0; i < dsm->data.keyFrameData.fieldCount; i++) {
                const UA_DataType *currentType = dsm->data.keyFrameData.dataSetFields[i].value.type;
                if(currentType == &UA_TYPES[UA_TYPES_BYTE]) {
                    UA_Byte value = *(UA_Byte *)dsm->data.keyFrameData.dataSetFields[i].value.data;
                    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                "Message content: [Byte] \tReceived data: %i", value);
                } else if (currentType == &UA_TYPES[UA_TYPES_UINT32]) {
                    UA_UInt32 value = *(UA_UInt32 *)dsm->data.keyFrameData.dataSetFields[i].value.data;
                    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                "Message content: [UInt32] \tReceived data: %u", value);
                } else if (currentType == &UA_TYPES[UA_TYPES_DATETIME]) {
                    UA_DateTime value = *(UA_DateTime *)dsm->data.keyFrameData.dataSetFields[i].value.data;
                    UA_DateTimeStruct receivedTime = UA_DateTime_toStruct(value);
                    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                "Message content: [DateTime] \t"
                                "Received date: %02i-%02i-%02i Received time: %02i:%02i:%02i",
                                receivedTime.year, receivedTime.month, receivedTime.day,
                                receivedTime.hour, receivedTime.min, receivedTime.sec);
                }
            }
        }
    }

cleanup:
    UA_NetworkMessage_clear(&networkMessage);
    return UA_STATUSCODE_GOOD;
}

int main(int argc, char **argv) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_PubSubTransportLayer udpLayer = UA_PubSubTransportLayerUDPMP();

    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection 1");
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    connectionConfig.enabled = UA_TRUE;

    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL , UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);

    UA_PubSubChannel *psc =
        udpLayer.createPubSubChannel(&connectionConfig);
    psc->regist(psc, NULL, NULL);

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    while(running && retval == UA_STATUSCODE_GOOD)
        retval = subscriberListen(psc);

    psc->close(psc);

    return 0;
}
