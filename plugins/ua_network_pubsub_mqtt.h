/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 * 
 *    Copyright 2018 (c) Fraunhofer IOSB (Author: Lukas Meling)
 */

#ifndef UA_NETWORK_MQTT_H_
#define UA_NETWORK_MQTT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_plugin_pubsub.h"
#include "ua_network_tcp.h"
    
/* mqtt network layer specific internal data */
typedef struct {
    UA_NetworkAddressUrlDataType address;
    
    UA_UInt32 mqttRecvBufferSize;
    UA_UInt32 mqttSendBufferSize;
    
    /* sendbuf should be large enough to hold multiple whole mqtt messages */
    uint8_t *mqttSendBuffer; 
    /* recvbuf should be large enough any whole mqtt message expected to be received */
    uint8_t *mqttRecvBuffer; 
    
    UA_String *mqttClientId;
    
    UA_Connection *connection; //Holds the connection with the socket fd.
    void * mqttClient; //Holds the mqtt client
    
    void (*callback)(UA_ByteString *encodedBuffer, UA_ByteString *topic);
} UA_PubSubChannelDataMQTT;
/*TODO: add more configuration
 * will topic,
 * will message,
 * user name,
 * password,
 * keep alive
 * ssl: cert, flag
 */    


UA_PubSubTransportLayer
UA_PubSubTransportLayerMQTT(void);


#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_NETWORK_MQTT_H_ */
