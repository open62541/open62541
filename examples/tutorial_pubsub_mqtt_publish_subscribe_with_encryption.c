#define DEBUG_MODE
//#define TEST_MOSQUITTO_ORG

#ifdef JSON5
#include "json5.h"
#endif

#include "open62541.h"
#include "ua_pubsub_networkmessage.h"
#include "ua_pubsub.h"
#include "json_checker.h"
#include <json-c/json.h>	// https://json-c.github.io/json-c; install in /usr/local/include/
//#include "json.h"		//https://raw.githubusercontent.com/udp/json-parser/master/json.h

#include "ua_pubsub_amqp.h"	//https://github.com/open62541/open62541/pull/3850

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

char* itoa(int num, char* str, int base);

#define MAX_STRING_SIZE 64
#define APPLICATION_URI "urn:virtualskies.com.sg"

#define UA_ENABLE_PUBSUB		// activate open62541.h PUBSUB section
#define UA_EANBLE_PUBSUB_ETH_UDAP	// activate open62541.h PUBSUB section
#define UA_ENABLE_PUBSUB_MQTT		// activate open62541.h PUBSUB section
#define UA_ENABLE_JSON_ENCODING		// activate open62541.h JSON section => use : open62541.c.json & open62541.h.json

#define CONNECTION_CONFIGNAME           "Publisher Connection"
#define NETWORKADDRESSURL_PROTOCOL      "opc.udp://224.0.0.22:4840/"	// multicast address range 224.0.0.0 to 224.0.0.255 ; for verifying using wireshark

#define CONNECTION_NAME_UADP		"ttl"	// ""UADP Publisher Connection"
#define TRANSPORT_PROFILE_URI_UDP       "http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp"
#define TRANSPORT_PROFILE_URI_ETH	"http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp"
#define PUBLISH_INTERVAL_UDP            9000

#define PUBLISHERID			2234	// used in addDataSetReader() and addPubSubConnection()
#define WRITERGROUPID			100	// used in addDataSetReader() and addWriterGroup()
#define DATASETWRITERID			2234	// used in addDataSetReader() and addDataSetWriter()

#ifdef UA_ENABLE_PUBSUB_MQTT
 #define TRANSPORT_PROFILE_URI_MQTT     "http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt"
 #define TRANSPORT_PROFILE_URI_AMQP	"http://opcfoundation.org/UA-Profile/Transport/pubsub-amqp"
 #define PUBLISH_INTERVAL_MQTT          9000
 #define PUBLISH_INTERVAL_AMQP		9000
 #define KEEP_ALIVE_MQTT		10000
 #define KEEP_ALIVE_AMQP		10000
 #define MESSAGE_TIMEOUT_MQTT		12000
 #define MESSAGE_TIMEOUT_AMQP		12000
 #define CONNECTIONOPTION_NAME_MQTT	"mqttClientId"
 #define CONNECTIONOPTION_NAME_AMQP	"amqpClientId"
 #define CLIENT_ID_MQTT               	"OPCServerUAT-115"//"OPCServer-115-Mqtt" // during execution, mqtt-11 will show : New client connected from 192.168.1.33 as OPCServer-33-Mqtt (c0, k400, u'jackybek')
 #define CLIENT_ID_AMQP			"TESTCLIENTPUBSUBAMQP"
 #define PUBLISHER_METADATAUPDATETIME_MQTT 	0
 #define PUBLISHER_METADATAUPDATETIME_AMQP	0
 #define PUBLISHER_METADATAQUEUENAME_MQTT  	"MetaDataTopic"
 #define PUBLISHER_METADATAQUEUENAME_AMQP	"MetaDataTopic"
 #define PUBLISHER_TOPIC_EDGE2CLOUD_MQTT        "AirgardTopicEdgeToCloud"
 #define PUBLISHER_TOPIC_CLOUD2EDGE_MQTT	"AirgardTopicCloudToEdge"
 #define PUBLISHER_QUEUE_EDGE2CLOUD_AMQP	"AirgardQueueEdgeToCloud"
 #define PUBLISHER_QUEUE_CLOUD2EDGE_AMQP	"AirgardQueueCloudToEdge"
 //#define BROKER_ADDRESS_URL_MQTT           	"opc.mqtt://192.168.1.119:1883/"
 //#define BROKER_ADDRESS_URL_AMQP		"opc.amqp://192.168.1.119:5672/"
 #define USERNAME_OPTION_NAME_MQTT         	"mqttUsername"
 #define USERNAME_OPTION_NAME_AMQP		"amqpUsername"
 #define PASSWORD_OPTION_NAME_MQTT         	"mqttPassword"
 #define PASSWORD_OPTION_NAME_AMQP		"amqpPassword"
 #define USERNAME_MQTT				"jackybek" // during execution, mqtt-11 will show : New client connected from 192.168.1.33 as OPCServer-33-Mqtt (c0, k400, u'jackybek')
 #define USERNAME_AMQP				"jackybek"
 #define PASSWORD_MQTT                		"molekhaven24"
 #define PASSWORD_AMQP				"molekhaven24"

#ifdef TEST_MOSQUITTO_ORG
 #define CA_FILE_PATH_MQTT			"/etc/ssl/certs/mosquitto.org.crt"	// symbolic link = /home/pi/Downloads/testmosquitto.org/mosquitto.org.crt
 #define CA_PATH_MQTT				"/etc/ssl/certs"
 #define CLIENT_CERT_PATH_MQTT			"/etc/ssl/certs/client.crt"		// symbolic link = /home/pi/Downloads/testmosquitto.org/client.crt
 #define CLIENT_KEY_PATH_MQTT			"/etc/ssl/certs/client.crt"		// client.crt contains both certificate and key
#else
 // confirmed that CA_FILE_PATH_MQTT without the path will get internal error
 #define USE_TLS_OPTION_NAME			"mqttUseTLS"
 #define MQTT_CA_FILE_PATH_OPTION_NAME		"mqttCaFilePath"
 #define CA_FILE_PATH_MQTT                      "/etc/ssl/certs/mosq-ca.crt"		// symbolic link => /home/pi/Downloads/openest.io/mosq-ca.crt
 #define CA_PATH_MQTT				"/etc/ssl/certs"
 #define CLIENT_CERT_PATH_MQTT			"/etc/ssl/certs/mosq-client-115.crt"	// symbolic link => /home/pi/Downloads/openest.io/mosq-client-115.crt
 #define CLIENT_KEY_PATH_MQTT			"/etc/ssl/certs/mosq-client-115.key"	// symbolic link => /home/pi/Downloads/openest.io/mosq-client-115.key
 // tested successfully on command line
 // mosquitto_sub -u jackybek -P molekhaven24 -h 192.168.1.11 -p 8883 -t AirgardTopicEdgeToCloud
 // --capath /etc/ssl/certs --cafile /etc/ssl/certs/mosq-ca.crt --cert /etc/ssl/certs/mosq-client-115.crt
 // --key /etc/ssl/certs/mosq-client-115.key --tls-version tlsv1.2 -d -i OPCServer-115
#endif

 #define SUBSCRIBER_METADATAUPDATETIME_MQTT      0
 #define SUBSCRIBER_METADATAQUEUENAME_MQTT       "MetaDataTopic"
 #define SUBSCRIBER_TOPIC_MQTT                   "AirgardTopic"
 #define MILLI_AS_NANO_SECONDS 			(1000 * 1000)
 #define SECONDS_AS_NANO_SECONDS		(1000 * 1000 * 1000)

 #define MAJOR_SOFTWARE_VERSION                 2
 #define MINOR_SOFTWARE_VERSION                 1
#endif

#ifdef UA_ENABLE_WEBSOCKET_SERVER
 #define TRANSPORT_PROFILE_URI_WSSBIN	"http://opcfoundation.org/UA_Profile/Transport/wss-uasc-uabinary"
 #define TRANSPORT_PROFILE_URI_WSSJSON  "http://opcfoundation.org/UA-Profile/Transport/wss-uajson"
 #define BROKER_ADDRESS_URL_WSS		"opc.wss://192.168.1.165:7681/"
#endif

#ifdef UA_ENABLE_JSON_ENCODING
 static UA_Boolean useJson = UA_TRUE;
#else
 static UA_Boolean useJson = UA_FALSE;
#endif

#define UA_AES128CTR_SIGNING_KEY_LENGTH 32
#define UA_AES128CTR_KEY_LENGTH 16
#define UA_AES128CTR_KEYNONCE_LENGTH 4

UA_Byte signingKey[UA_AES128CTR_SIGNING_KEY_LENGTH]= {0};
UA_Byte encryptingKey[UA_AES128CTR_KEY_LENGTH] = {0};
UA_Byte keyNonce[UA_AES128CTR_KEYNONCE_LENGTH] = {0};

typedef struct MonitoringParams
{
	time_t timer;
	struct sigevent event;
	UA_Server *server;
	void *componentData;
	UA_ServerCallback callback;
} TMonitoringParams;

/* In this example we only configure 1 DataSetReader and therefore only provide 1 data structure for monitoring params.
 * If more DataSetReaders is required, one can create a new data structure at every pubSubComponent_createMonitoring() call
 * and administer them with a list/ map/ vector ....
*/
TMonitoringParams monitoringParams;


//extern UA_NodeId outSoftwareVersion_Id;
UA_String subscribedSoftwareVersion;
UA_String subscribedDataBlockVersion;
UA_String subscribedInstrumentTime;
UA_String subscribedMeasurementTime;
UA_String subscribedBootStatus;
UA_String subscribedSnapshotStatus;
UA_String subscribedSCPStatus;
UA_String subscribedSFTPStatus;
UA_String subscribedRunScriptStatus;
UA_String subscribedArchiveStatus;
UA_String subscribedAncillarySensorStatus;
UA_String subscribedSensor;
/**/UA_Int16 subscribedOperatingTime;
UA_String subscribedWarningMessage;

UA_Float subscribedIgramPP;
UA_Float subscribedIgramDC;
UA_Float subscribedLaserPP;
UA_Float subscribedLaserDC;
UA_Float subscribedSingleBeamAt900;
UA_Float subscribedSingleBeamAt2500;
/**/UA_Int16 subscribedSignalToNoiseAt2500;
UA_Float subscribedCenterBurstLocation;
UA_Float subscribedDetectorTemp;
UA_Float subscribedLaserFrequency;

/**/UA_Int16 subscribedHardDriveSpace;
/**/UA_Int16 subscribedFlow;
/**/UA_Int16 subscribedTemperature;
UA_Float subscribedPressure;
/**/UA_Int16 subscribedTempOptics;
/**/UA_Int16 subscribedBadScanCounter;
/**/UA_Int16 subscribedFreeMemorySpace;
UA_String subscribedLABFilename;
UA_String subscribedLOGFilename;
UA_String subscribedLgFilename;
UA_String subscribedSecondLgFilename;
UA_Float subscribedSystemCounter;
UA_Float subscribedDetectorCounter;
UA_Float subscribedLaserCounter;
UA_Float subscribedFlowPumpCounter;
UA_Float subscribedDesiccantCounter;

UA_ByteString loadFile(const char *const path);
int CreateServerWebSockets(UA_NodeId *publishedDataSetidentifier, UA_NetworkAddressUrlDataType *networkAddressUrl);

// only 1 PubSubConnection for both reader and writer
UA_NodeId PubSubconnectionIdentifier;

// publisher section : variables & functions
static UA_NodeId publishedDataSetIdentifier;
static UA_NodeId writerGroupIdentifier;

//void CreateServerPubSub(UA_Server *, char *, UA_Int16);
void CreateServerPubSub(UA_Server *, char *, int, char *);
static void addPubSubConnection(UA_Server *, UA_String *, UA_NetworkAddressUrlDataType *);
static void addPublishedDataSet(UA_Server *);
static void addDataSetField(UA_Server *);
static void addWriterGroup(UA_Server *);
static void addDataSetWriter(UA_Server *);


// Subscriber section variables & functions
UA_DataSetReaderConfig dataSetReaderConfig;
UA_NodeId readerGroupIdentifier;
UA_NodeId readerIdentifier;
UA_NodeId reader2Id;

static UA_StatusCode addReaderGroup(UA_Server *uaServer);
static UA_StatusCode addDataSetReader(UA_Server *uaServer);
static UA_StatusCode addSubscribedVariables(UA_Server *uaServer, UA_NodeId dataSetReaderId);
static void fillDataSetMetaData(UA_DataSetMetaDataType *pMetaData);
static void pubSubStateChangeCallback(UA_NodeId *pubsubComponentId, UA_PubSubState state, UA_StatusCode code);
static UA_StatusCode
pubSubComponent_createMonitoring(UA_Server *uaServer, UA_NodeId Id, UA_PubSubComponentEnumType eComponentType,
                                UA_PubSubMonitoringType eMonitoringType, void *data, UA_ServerCallback callback);
static UA_StatusCode
pubSubComponent_startMonitoring(UA_Server *uaServer, UA_NodeId Id, UA_PubSubComponentEnumType eComponentType,
                                UA_PubSubMonitoringType eMonitoringType, void *data);
static UA_StatusCode
pubSubComponent_stopMonitoring(UA_Server *uaServer, UA_NodeId Id, UA_PubSubComponentEnumType eComponentType,
                                UA_PubSubMonitoringType eMonitoringType, void *data);
static UA_StatusCode
pubSubComponent_updateMonitoringInterval(UA_Server *uaServer, UA_NodeId Id, UA_PubSubComponentEnumType eComponentType,
                                UA_PubSubMonitoringType eMonitoringType, void *data);
static UA_StatusCode
pubSubComponent_deleteMonitoring(UA_Server *uaServer, UA_NodeId Id, UA_PubSubComponentEnumType eComponentType,
                                UA_PubSubMonitoringType eMonitoringType, void *data);

//void addValueCallbackToVariableNode( UA_Server *);

// somehow the compiler is not able to local this header definition in open62541.c
//UA_StatusCode UA_PubSubConnection_regist(UA_Server *server, UA_NodeId *connectionIdentifier);
//void pubsubStateChangeCallback(UA_NodeId *Id, UA_PubSubState state, UA_StatusCode status);

UA_Boolean MQTT_Enable = UA_FALSE;
UA_Boolean MQTT_TLS_Enable = UA_FALSE;
UA_Boolean AMQP_Enable = UA_FALSE;
short MQTT_Port = 1883;

static void
addPubSubConnection(UA_Server *uaServer, UA_String *transportProfile,
			UA_NetworkAddressUrlDataType *networkAddressUrl)
{
    /* Details about the connection configuration and handling are located
     * in the pubsub connection tutorial */

/*
struct UA_PubSubConnectionConfig {
    1 UA_String name;                           // set
    2 UA_Boolean enabled;                       // set
    3 UA_PublisherIdType publisherIdType;       // set
    union { // std: valid types UInt or String
       UA_UInt32 numeric;
        UA_String string;
    4 } publisherId;                            // set numeric
    5 UA_String transportProfileUri;            // set
    6 UA_Variant address;                       // set
    7 UA_Variant connectionTransportSettings;   // set
    8 size_t connectionPropertiesSize;          // set
    9 UA_KeyValuePair *connectionProperties;    // set
};
*/
    #ifdef DEBUG_MODE
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SubSV_PubSub.c : In addPubSubConnection() \n");
    #endif

    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));

    /*1*/connectionConfig.name = UA_STRING(CONNECTION_CONFIGNAME);	// UA_STRING("Publisher Connection")
    /*2*/connectionConfig.enabled = UA_TRUE;
    /*3*/connectionConfig.publisherIdType = UA_PUBSUB_PUBLISHERID_NUMERIC;
    /*4*/connectionConfig.publisherId.numeric = PUBLISHERID;	// Changed to static publisherId from random generation to identify the publisher on subscriber side
    /*5*/connectionConfig.transportProfileUri = *transportProfile;
    /*6*/UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);


    //UA_ServerConfig *config = UA_Server_getConfig(uaServer);
    /*7*///UA_Variant_setScalar(&connectionConfig.connectionTransportSettings,
	//	config->pubsubTransportLayers, &UA_TYPES[UA_TYPES_BROKERCONNECTIONTRANSPORTDATATYPE]); // UA_TYPES_DATAGRAMCONNECTIONTRANSPORTDATATYPE
    // UA_TYPES_BROKERCONNECTIONTRANSPORTDATATYPE == 31
    // UA_TYPES_DATAGRAMCONNECTIONTRANSPORTDATATYPE == 85
    // UA_TYPES[31]
    // UA_TYPES[85]

    /*8*///connectionConfig.connectionPropertiesSize = 3;	// to match connectionOptions[N]
    /*9*///connectionConfig.connectionProperties = (UA_KeyValuePair *)calloc(3, sizeof(UA_KeyValuePair));

/*
typedef struct {
    UA_QualifiedName key;
    UA_Variant value;
} UA_KeyValuePair;
*/
    if (MQTT_Enable == UA_TRUE && AMQP_Enable == UA_FALSE)
    {
	/*
	UA_ServerConfig *config = UA_Server_getConfig(uaServer);
    	/*7*/
	/*UA_Variant_setScalar(&connectionConfig.connectionTransportSettings,
              config->pubsubTransportLayers, &UA_TYPES[UA_TYPES_BROKERCONNECTIONTRANSPORTDATATYPE]);
	*/
	#ifdef DEBUG_MODE
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : addPubSubConnection(): MQTT segment \n");
	#endif
// line 236
	int connectionOptionsCount;
	if (MQTT_TLS_Enable == UA_TRUE)
		connectionOptionsCount = 10;
	else
		connectionOptionsCount = 5;

	size_t connectionOptionsIndex = 0;
	UA_KeyValuePair connectionOptions[connectionOptionsCount];
    size_t namespaceIndex;

    UA_Server_getNamespaceByName(uaServer, UA_STRING("virtualskies.com.sg/MKS/"), &namespaceIndex);

	//--connectionOptionsIndex start at 0
	connectionOptions[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, CONNECTIONOPTION_NAME_MQTT);	// mqttClientId
	UA_String mqttClientId = UA_STRING(CLIENT_ID_MQTT);
	UA_Variant_setScalar(&connectionOptions[connectionOptionsIndex].value, &mqttClientId, &UA_TYPES[UA_TYPES_STRING]);

	// preallocate sendBufferSize
	connectionOptionsIndex++; 	// 1
	connectionOptions[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, "sendBufferSize");
	UA_UInt32 sendBufferSize = 32767;	// must be UA_UInt32 to match open62541.c codes
	UA_Variant_setScalar(&connectionOptions[connectionOptionsIndex].value, &sendBufferSize, &UA_TYPES[UA_TYPES_UINT32]);

	// preallocate recvBufferSize
	connectionOptionsIndex++; 	// 2
	connectionOptions[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, "recvBufferSize");
	UA_Int32 recvBufferSize = 32767;	// must be UA_UInt32 to match open62541.c codes
	UA_Variant_setScalar(&connectionOptions[connectionOptionsIndex].value, &recvBufferSize, &UA_TYPES[UA_TYPES_UINT32]);

	//if ( (MQTT_Port == 1884) || (MQTT_Port == 8884 || MQTT_Port == 8885) )
	//{
	// login credentials to mqtt broker on 192.168.1.11
	connectionOptionsIndex++;	// 3
    	connectionOptions[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, USERNAME_OPTION_NAME_MQTT);	// mqttUsername
    	UA_String mqttUsername = UA_STRING(USERNAME_MQTT);
    	UA_Variant_setScalar(&connectionOptions[connectionOptionsIndex].value, &mqttUsername, &UA_TYPES[UA_TYPES_STRING]);

	connectionOptionsIndex++;	// 4
    	connectionOptions[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, PASSWORD_OPTION_NAME_MQTT);	// mqttPassword
    	UA_String mqttPassword = UA_STRING(PASSWORD_MQTT);
    	UA_Variant_setScalar(&connectionOptions[connectionOptionsIndex].value, &mqttPassword, &UA_TYPES[UA_TYPES_STRING]);
	//}

// the following works on command prompt with tls_version removed
#ifdef MOSQUITTO
mosquitto_pub -m "TEST" -t AirgardTopic -u jackybek -P molekhaven24 -p 8884
--capath /etc/ssl/certs --cafile /etc/ssl/certs/mosq-ca.crt
--cert /etc/ssl/certs/mosq-client-115.crt
--key /etc/ssl/certs/mosq-client-115.key -d -i OPCServerUAT-115 -h 192.168.1.11

mosquitto_sub -t AirgardTopic -u jackybek -P molekhaven24 -p 8884
--capath /etc/ssl/certs --cafile /etc/ssl/certs/mosq-ca.crt
--cert /etc/ssl/certs/mosq-client-115.crt
--key /etc/ssl/certs/mosq-client-115.key -d -i OPCServerUAT-115 -h 192.168.1.11
#endif

	if (MQTT_TLS_Enable == UA_TRUE)
	{
// https://github.com/open62541/open62541/blob/master/examples/pubsub/tutorial_pubsub_mqtt_publish.c
/*  * MQTT via TLS
 * ^^^^^^^^^^^^
 * Defining EXAMPLE_USE_MQTT_TLS enables TLS for the MQTT connection. This is
 * experimental and currently only available with OpenSSL.
 *
 * https://test.mosquitto.org/ offers a public MQTT broker with TLS support.
 * The access via port 8883 can be used with this example. Download the file
 * ``mosquitto.org.crt`` and point the CA_FILE_PATH define to its location.
 *
 * If the server requires a client certificate, the two options ``mqttClientCertPath``
 * and ``mqttClientKeyPath`` must be added in ``addPubSubConnection()``.
 *
 * To use a directory containing hashed certificates generated by ``c_rehash``,
 * use the ``mqttCaPath`` option instead of ``mqttCaFilePath``. If none of these two
 * options is provided, the system's default certificate location will be used.
 *
 * #define CA_FILE_PATH_MQTT                      "/etc/ssl/certs/ca-11.pem"
 * #define CLIENT_CERT_PATH_MQTT                  "/etc/ssl/certs/broker-cert11.pem"
 * #define CLIENT_KEY_PATH_MQTT                   "/etc/ssl/certs/brokerprivate-key11.pem"
 */

		// the following is tested successfully
		// sudo mosquitto_pub -d -h Mqtt-11 -p 8883 -t AirgardTopic --capath /etc/ssl/certs/ --cafile mosq-ca.crt -m "Secure Message"
		// ./myNewServer 192.168.1.33 192.168.1.119 192.168.1.11 8883 pub ==> can run

		//if ( (MQTT_Port == 8883) || (MQTT_Port == 8884 || MQTT_Port == 8885) )
		//{

			connectionOptionsIndex++; // 5
			connectionOptions[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, USE_TLS_OPTION_NAME); 	// "mqttUseTLS");
			UA_Boolean mqttUseTLS = UA_TRUE;
			UA_Variant_setScalar(&connectionOptions[connectionOptionsIndex].value, &mqttUseTLS, &UA_TYPES[UA_TYPES_BOOLEAN]);

			// <https://github.com/open62541/open62541/blob/master/examples/pubsub/tutorial_pubsub_mqtt_publish.c> does not contain this parameter
			connectionOptionsIndex++; // 6
			connectionOptions[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, "mqttCaPath");
			UA_String mqttCaPath = UA_STRING(CA_PATH_MQTT);
			UA_Variant_setScalar(&connectionOptions[connectionOptionsIndex].value, &mqttCaPath, &UA_TYPES[UA_TYPES_STRING]);

			connectionOptionsIndex++; // 7
			connectionOptions[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, MQTT_CA_FILE_PATH_OPTION_NAME);	// "mqttCaFilePath");
	                UA_String mqttCaFile = UA_STRING(CA_FILE_PATH_MQTT);
        	        UA_Variant_setScalar(&connectionOptions[connectionOptionsIndex].value, &mqttCaFile, &UA_TYPES[UA_TYPES_STRING]);
		//}

		//if ( MQTT_Port == 8884 )
		//{
	                connectionOptionsIndex++; // 8
			connectionOptions[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, "mqttClientCertPath");
                	UA_String mqttClientCertPath = UA_STRING(CLIENT_CERT_PATH_MQTT);
	                UA_Variant_setScalar(&connectionOptions[connectionOptionsIndex].value, &mqttClientCertPath, &UA_TYPES[UA_TYPES_STRING]);

        	        connectionOptionsIndex++; // 9
			connectionOptions[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, "mqttClientKeyPath");
	                UA_String mqttClientKeyPath = UA_STRING(CLIENT_KEY_PATH_MQTT);
        	        UA_Variant_setScalar(&connectionOptions[connectionOptionsIndex].value, &mqttClientKeyPath, &UA_TYPES[UA_TYPES_STRING]);
		//}
	}
   	connectionConfig.connectionProperties = connectionOptions;
    	connectionConfig.connectionPropertiesSize = connectionOptionsIndex+1;	// add 1 because the index start at 0;

        // to extract value from a VARIANT
        //UA_Int16 raw_data = *(UA_Int16 *)varStrNonAlarms->data;

	//#ifdef DEBUG_MODE
        printf("in addPubSubConnection : MQTT segment \n");
        printf("connectionConfig.name                                           : %s \n", connectionConfig.name.data);
        printf("connectionConfig.enabled                                        : %d \n", connectionConfig.enabled);
        printf("connectionConfig.PublisherIdType                                : %d \n", connectionConfig.publisherIdType);
        printf("connectionConfig.PublisherId                                    : %d \n", connectionConfig.publisherId.numeric);
        printf("connectionConfig.transportProfileUri                            : %s \n", connectionConfig.transportProfileUri.data);

	//UA_String myaddress = *(UA_String *)connectionConfig.address;
        //printf("connectionConfig.address                                        : %s \n", myaddress.data); //connectionConfig.address.url.data);
        //printf("connectionConfig.connectionTransportSettings                  : %d \n", *(UA_Int16 *)connectionConfig.connectionTransportSettings.data); // variant
        printf("connectionConfig.connectionPropertiesSize                       : %d \n", connectionConfig.connectionPropertiesSize);
        printf("---------------------------------------------------------------------\n");
        printf("0. connectionConfig.connectionProperties[0].key : mqttClientId  : %s \n", connectionConfig.connectionProperties[0].key.name.data); // [0].key.name is of t$
		//connectionConfig.connectionProperties[0].value is of type UA_Variant
		UA_String raw_data_str = *(UA_String *)connectionConfig.connectionProperties[0].value.data;
        printf("0. connectionConfig.connectionProperties[0].value (UA_String)   : %s \n", raw_data_str.data);  // [$

        printf("1. connectionConfig.connectionProperties[1].key : sendBufferSize: %s \n", connectionConfig.connectionProperties[1].key.name.data);
		//connectionConfig.connectionProperties[1].value is of type UA_Variant
		UA_Int16 raw_data_int16 = *(UA_Int16 *)connectionConfig.connectionProperties[1].value.data;
        printf("1. connectionConfig.connectionProperties[1].value (UInt16)      : %d \n", raw_data_int16);

        printf("2. connectionConfig.connectionProperties[2].key : recvBufferSize: %s \n", connectionConfig.connectionProperties[2].key.name.data);
		         raw_data_int16 = *(UA_Int16 *)connectionConfig.connectionProperties[2].value.data;
        printf("2. connectionConfig.connectionProperties[2].value (UInt16)      : %d \n", raw_data_int16);

        printf("3. connectionConfig.connectionProperties[3].key : mqttUsername  : %s \n", connectionConfig.connectionProperties[3].key.name.data);
		raw_data_str = *(UA_String *)connectionConfig.connectionProperties[3].value.data;
        printf("3. connectionConfig.connectionProperties[3].value (UA_String)   : %s \n", raw_data_str.data);

        printf("4. connectionConfig.connectionProperties[4].key : mqttPassword  : %s \n", connectionConfig.connectionProperties[4].key.name.data);
		raw_data_str = *(UA_String *)connectionConfig.connectionProperties[4].value.data;
        printf("4. connectionConfig.connectionProperties[4].value (UA_String)   : %s \n", raw_data_str.data);

	if (MQTT_TLS_Enable == UA_TRUE)
	{
        	printf("5. connectionConfig.connectionProperties[5].key : mqttUseTLS    : %s \n", connectionConfig.connectionProperties[5].key.name.data);
			raw_data_int16 = *(UA_Boolean *)connectionConfig.connectionProperties[5].value.data;
        	printf("5. connectionConfig.connectionProperties[5].value (UA_Boolean)  : %d \n", raw_data_int16);

		printf("6. connectionConfig.connectionProperties[6].key : mqttCaPath    : %s \n", connectionConfig.connectionProperties[6].key.name.data);
			raw_data_str = *(UA_String *)connectionConfig.connectionProperties[6].value.data;
        	printf("6. connectionConfig.connectionProperties[6].value (UA_String)   : %s \n", raw_data_str.data);

         	printf("7. connectionConfig.connectionProperties[7].key : mqttCaFilePath: %s \n", connectionConfig.connectionProperties[7].key.name.data);
			raw_data_str = *(UA_String *)connectionConfig.connectionProperties[7].value.data;
			char caFilePath[128];
			strcpy(caFilePath, raw_data_str.data);
        	printf("7. connectionConfig.connectionProperties[7].value (UA_String)   : %s %s\n", raw_data_str.data, caFilePath);

        	printf("8. connectionConfig.connectionProperties[8].key : mqttClientCertPath : %s \n", connectionConfig.connectionProperties[8].key.name.data);
			raw_data_str = *(UA_String *)connectionConfig.connectionProperties[8].value.data;
			char clientCertPath[128];
			strcpy(clientCertPath, raw_data_str.data);
        	printf("8. connectionConfig.connectionProperties[8].value (UA_String)        : %s %s\n", raw_data_str.data, clientCertPath);

        	printf("9. connectionConfig.connectionProperties[9].key : mqttClientKeyPath  : %s \n", connectionConfig.connectionProperties[9].key.name.data);
			raw_data_str = *(UA_String *)connectionConfig.connectionProperties[9].value.data;
			char clientKeyPath[128];
			strcpy(clientKeyPath, raw_data_str.data);
        	printf("9. connectionConfig.connectionProperties[9].value (UA_String)        : %s %s\n", raw_data_str.data, clientKeyPath);
		printf("-----------------------Inspecting file contents -------------------------\n");
		/*
			int stringlen;
			char strcaFileContents[10000];
                        char strcaCertPathContents[10000];
			UA_ByteString caFileContents = loadFile(caFilePath);
                        UA_ByteString caCertPathContents = loadFile(caCertPath);

			// remove extra characters at the end of the crt files
			stringlen = caFileContents.length;
                        strncpy(strcaFileContents, (char*)caFileContents.data, stringlen);
                        strcaFileContents[stringlen]='\0';

			stringlen = caCertPathContents.length;
			strncpy(strcaCertPathContents, (char*)caCertPathContents.data, stringlen);
			strcaCertPathContents[stringlen]='\0';

			printf("mqttCAFilePath\n%s###\n", strcaFileContents); 			//caFileContents.data);
			printf("mqttCLientCertPath\n%s###\n", strcaCertPathContents);			//caCertPathContents.data);
		*/
	}
        printf("---------------------------------------------------------------------\n");
        printf("networkAddressUrl.networkInterface                              : %s \n", networkAddressUrl->networkInterface.data);
        printf("networkAddressUrl.url                                           : %s \n", networkAddressUrl->url.data);
	//#endif

	// initiatise the value of PubSubconnectionIdentifier (UA_NodeId) so that we can check later in addSubscription()
	#ifdef DEBUG_MODE
    	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : Before calling UA_Server_addPubSubConnection() : MQTT_Enable == UA_TRUE && AMQP_Enable == UA_FALSE");
	#endif

    	UA_StatusCode retval = UA_Server_addPubSubConnection(uaServer, &connectionConfig, &PubSubconnectionIdentifier);
    	if (retval == UA_STATUSCODE_GOOD)
    	{
		UA_String output;
		UA_String_init(&output);

		#ifdef DEBUG_MODE
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : The output of UA_Server_addPubSubConnection is a NodeId : PubSubconnectionIdentifier .. check isNull= %d", UA_NodeId_isNull(&PubSubconnectionIdentifier));
		#endif

		UA_NodeId_print(&PubSubconnectionIdentifier, &output);

		#ifdef DEBUG_MODE
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : addPubSubConnection() : NodeId : <%s>", output.data);
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"addPubSubConnection : PubSubconnectionIdentifier success : %s", UA_StatusCode_name(retval));
		#endif
    	}
    	else
    	{
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"addPubSubConnection : PubSubconnectionIdentifier failure : %s", UA_StatusCode_name(retval));
		exit(EXIT_FAILURE);
    	}
	//sleep(5);
    }
    else if (MQTT_Enable == UA_FALSE && AMQP_Enable == UA_TRUE)
    {
	#ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : addPubSubConnection(): AMQP segment");
	#endif
	//sleep(1000);
    }
    else // MQTT_Enable = UA_FALSE, AMQP_Enable = UA_FALSE
    {
	#ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : addPubSubConnection(): UADP segment");
	#endif
	// the command line options does not cater for this; so we need to hardcode networkAddressUrl = "opc.udp://224.0.0.22:4840/")

	UA_ServerConfig *config = UA_Server_getConfig(uaServer);
	/* It is possible to use multiple PubSubTransportLayers on runtime. The correct factory
     	* is selected on runtime by the standard defined PubSub TransportProfileUri's. */

	/* defunct: v1.2.1 */ //config->pubsubTransportLayers = (UA_PubSubTransportLayer *) UA_malloc(sizeof(UA_PubSubTransportLayer));
    	/* defunct: v1.2.1 */ //config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    	/* defunct v1.2.1 :*/ //config->pubsubTransportLayersSize++;
	/* v1.2.2 */ UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerUDPMP());

    	/* Create a new ConnectionConfig. The addPubSubConnection function takes the
     	* config and create a new connection. The Connection identifier is
     	* copied to the NodeId parameter.*/

    connectionConfig.name = UA_STRING("UDP-UADP Connection 1");
    connectionConfig.enabled = UA_TRUE;
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    /* The address and interface is part of the standard
     * defined UA_NetworkAddressUrlDataType. */
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL , UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.numeric = PUBLISHERID;

    	/*7*/
	/*UA_Variant_setScalar(&connectionConfig.connectionTransportSettings,
              config->pubsubTransportLayers, &UA_TYPES[UA_TYPES_DATAGRAMCONNECTIONTRANSPORTDATATYPE]);
	*/
	size_t connectionOptionsIndex = 0;

    // refer to github/open62541/examples/pubsub/tutorial_pubsub_connection.c
    	UA_KeyValuePair connectionProperties[3];

	size_t namespaceIndex;
	UA_Server_getNamespaceByName(uaServer, UA_STRING("virtualskies.com.sg/MKS/"), &namespaceIndex);

    	connectionProperties[0].key = UA_QUALIFIEDNAME(namespaceIndex, "ttl"); //CONNECTION_NAME_UADP); //"ttl");
    	UA_UInt16 ttl = 10;
    	UA_Variant_setScalar(&connectionProperties[connectionOptionsIndex].value, &ttl, &UA_TYPES[UA_TYPES_UINT16]);
	connectionOptionsIndex++;

    	connectionProperties[1].key = UA_QUALIFIEDNAME(namespaceIndex, "loopback");
    	UA_Boolean loopback = UA_FALSE;
    	UA_Variant_setScalar(&connectionProperties[connectionOptionsIndex].value, &loopback, &UA_TYPES[UA_TYPES_BOOLEAN]);
        connectionOptionsIndex++;

    	connectionProperties[2].key = UA_QUALIFIEDNAME(namespaceIndex, "reuse");
    	UA_Boolean reuse = UA_TRUE;
    	UA_Variant_setScalar(&connectionProperties[connectionOptionsIndex].value, &reuse, &UA_TYPES[UA_TYPES_BOOLEAN]);
        connectionOptionsIndex++;

    	connectionConfig.connectionProperties = connectionProperties;
	connectionConfig.connectionPropertiesSize = connectionOptionsIndex;

	/*
    	for (int index=0; index < connectionConfig.connectionPropertiesSize; index++)
    	{
    		printf("assigning connectionProperties KeyValuePair \n");
		connectionConfig.connectionProperties = UA_KeyValuePair_new();	// UA_KeyValuePair *UA_KeyValuePair_new(void)
    		UA_KeyValuePair_copy(&connectionProperties[index], &connectionConfig.connectionProperties[index]);
    	}
	*/
    	UA_NodeId connectionIdentifier = UA_NODEID_STRING_ALLOC(namespaceIndex, "ConnectionIdentifier");

	// to extract value from a VARIANT
	//UA_Int16 raw_data = *(UA_Int16 *)varStrNonAlarms->data;

	#ifdef DEBUG_MODE
        printf("in addPubSubConnection : in UDP segment \n");
        printf("connectionConfig.name                           		: %s \n", connectionConfig.name.data);
	printf("connectionConfig.enabled					: %d \n", connectionConfig.enabled);
	printf("connectionConfig.PublisherIdType				: %d \n", connectionConfig.publisherIdType);
	printf("connectionConfig.PublisherId					: %d \n", connectionConfig.publisherId.numeric);
        printf("connectionConfig.transportProfileUri            		: %s \n", connectionConfig.transportProfileUri.data);
        printf("connectionConfig.address                        		: %s \n", connectionConfig.address.data);
	//printf("connectionConfig.connectionTransportSettings			: %d \n", *(UA_Int16 *)connectionConfig.connectionTransportSettings.data); // variant
	printf("connectionConfig.connectionPropertiesSize			: %d \n", connectionConfig.connectionPropertiesSize);
        printf("---------------------------------------------------------------------\n");
	printf("connectionConfig.connectionProperties[0].key   (string)		: %s \n", connectionConfig.connectionProperties[0].key.name.data); // [0].key.name is of type UA_STRING
	//printf("connectionConfig.connectionProperties[0].value (int16)		: %d \n", *(UA_Int16 *)connectionConfig.connectionProperties[0].value.data);  // [0].value is a variant;
        printf("connectionConfig.connectionProperties[1].key   (string) 	: %s \n", connectionConfig.connectionProperties[1].key.name.data);
        //printf("connectionConfig.connectionProperties[1].value (boolean) 	: %d \n", *(UA_Boolean *)connectionConfig.connectionProperties[1].value.data);
        printf("connectionConfig.connectionProperties[2].key   (string) 	: %s \n", connectionConfig.connectionProperties[2].key.name.data);
        //printf("connectionConfig.connectionProperties[2].value (boolean)	: %d \n", *(UA_Boolean *)connectionConfig.connectionProperties[2].value.data);
        printf("---------------------------------------------------------------------\n");
        printf("networkAddressUrl.networkInterface              		: %s \n", networkAddressUrl.networkInterface.data);
        printf("networkAddressUrl.url                          	 		: %s \n", networkAddressUrl.url.data);
	#endif
	//sleep(5);

        // initiatise the value of PubSubconnectionIdentifier (UA_NodeId) so that we can check later in addSubscription()
	#ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : Before calling UA_Server_addPubSubConnection() : MQTT_Enable == UA_FALSE && AMQP_Enable == UA_FALSE");
	#endif

        UA_StatusCode retval = UA_Server_addPubSubConnection(uaServer, &connectionConfig, &PubSubconnectionIdentifier);
        if (retval == UA_STATUSCODE_GOOD)
        {
                UA_String output;
                UA_String_init(&output);

		#ifdef DEBUG_MODE
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"The output of UA_Server_addPubSubConnection is a NodeId : PubSubconnectionIdentifier .. check isNull= %d", UA_NodeId_isNull(&PubSubconnectionIdentifier));
		#endif

                UA_NodeId_print(&PubSubconnectionIdentifier, &output);

		#ifdef DEBUG_MODE
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : addPubSubConnection() : NodeId : <%s>", output.data);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"SV_PubSub.c : addPubSubConnection : PubSubconnectionIdentifier success %s", UA_StatusCode_name(retval));
		#endif
        }
        else
        {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"SV_PubSub.c : addPubSubConnection : PubSubconnectionIdentifier failure %s", UA_StatusCode_name(retval));
                exit(EXIT_FAILURE);
        }

    }
    //printf("networkAddressUrl.url                          	 		: %s \n", networkAddressUrl->url.data);


// this statement appears in tutorial_pubsub_subscribe.c but not in tutorial_pubsub_mqtt_publish.c or server_pubsub_subscribe_custom_monitoring.c
// !!! addPubSubConnection : PubSubconnectionIdentifier registration failure BadArgumentsMissing !!!
/*    retval |= UA_PubSubConnection_regist(uaServer, &PubSubconnectionIdentifier);
    if (retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"addPubSubConnection : PubSubconnectionIdentifier registration failure %s", UA_StatusCode_name(retval));
        exit(EXIT_FAILURE);
    }
*/
}


/**
 * **PublishedDataSet handling**
 * The PublishedDataSet (PDS) and PubSubConnection are the toplevel entities and
 * can exist alone. The PDS contains the collection of the published fields. All
 * other PubSub elements are directly or indirectly linked with the PDS or
 * connection.
 */
static void
addPublishedDataSet(UA_Server *uaServer) {
    /* The PublishedDataSetConfig contains all necessary public
    * informations for the creation of a new PublishedDataSet */
    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name = UA_STRING("Airgard PDS");

    /* Create new PublishedDataSet based on the PublishedDataSetConfig. */
    UA_Server_addPublishedDataSet(uaServer, &publishedDataSetConfig, &publishedDataSetIdentifier);

   // added by Jacky on 4/4/2021 to update MQTT payload (MetaDataVersion)
   // the corresponding change has to take place in open62541.c : UA_Server_addDataSetField() however this change cause publisher to have error
        //UA_open62541/src/pubsub/ua_pubsub_writer.c
        //At line 691
/*   UA_PublishedDataSet *currentDataSet = UA_PublishedDataSet_findPDSbyId(uaServer, publishedDataSetIdentifier);
   currentDataSet->dataSetMetaData.configurationVersion.majorVersion = MAJOR_SOFTWARE_VERSION;
   currentDataSet->dataSetMetaData.configurationVersion.minorVersion = MINOR_SOFTWARE_VERSION;
*/
}

/**
 * **DataSetField handling**
 * The DataSetField (DSF) is part of the PDS and describes exactly one published field.
 */
static void
addDataSetField(UA_Server *uaServer)
{
    /* Add a field to the previous created PublishedDataSet */
    //UA_NodeId dataSetFieldIdentifier;

//    UA_NodeId f_SoftwareVersion_Id, f_DataBlockVersion_Id, f_InstrumentTime_Id, f_MeasurementTime_Id;
//    UA_NodeId f_BootStatus_Id, f_SnapshotStatus_Id, f_SCPStatus_Id, f_SFTPStatus_Id, f_RunscriptStatus_Id, f_ArchiveStatus_Id, f_AncillarySensorStatus_Id;
//    UA_NodeId f_Sensor_Id, f_OperatingTime_Id, f_WarningMessage_Id;
//    UA_NodeId f_IgramPP_Id, f_IgramDC_Id, f_LaserPP_Id, f_LaserDC_Id, f_SingleBeamAt900_Id, f_SingleBeamAt2500_Id, f_SignalToNoiseAt2500_Id;
//    UA_NodeId f_CenterBurstLocation_Id, f_DetectorTemp_Id, f_LaserFrequency_Id, f_HardDriveSpace_Id, f_Flow_Id, f_Temperature_Id, f_Pressure_Id;
//    UA_NodeId f_TempOptics_Id, f_BadScanCounter_Id, f_FreeMemorySpace_Id, f_LABFilename_Id, f_LOGFilename_Id, f_LgFilename_Id, f_SecondLgFilename_Id;
//    UA_NodeId f_SystemCounter_Id, f_DetectorCounter_Id, f_LaserCounter_Id, f_FlowPumpCounter_Id, f_DesiccantCounter_Id;

    size_t namespaceIndex;
    UA_Server_getNamespaceByName(uaServer, UA_STRING("virtualskies.com.sg/MKS/"), &namespaceIndex);

//0
    UA_DataSetFieldConfig dsCfgSoftwareVersion;
    memset(&dsCfgSoftwareVersion, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgSoftwareVersion.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgSoftwareVersion.field.variable.fieldNameAlias = UA_STRING("SoftwareVersion");
    dsCfgSoftwareVersion.field.variable.promotedField = UA_FALSE;
    dsCfgSoftwareVersion.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10001);
    dsCfgSoftwareVersion.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//1
    UA_DataSetFieldConfig dsCfgDataBlockVersion;
    memset(&dsCfgDataBlockVersion, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgDataBlockVersion.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgDataBlockVersion.field.variable.fieldNameAlias = UA_STRING("DataBlockVersion");
    dsCfgDataBlockVersion.field.variable.promotedField = UA_FALSE;
    dsCfgDataBlockVersion.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10002);
    dsCfgDataBlockVersion.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//2
    UA_DataSetFieldConfig dsCfgInstrumentTime;
    memset(&dsCfgInstrumentTime, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgInstrumentTime.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgInstrumentTime.field.variable.fieldNameAlias = UA_STRING("InstrumentTime");
    dsCfgInstrumentTime.field.variable.promotedField = UA_FALSE;
    dsCfgInstrumentTime.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10101);
    dsCfgInstrumentTime.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//3
    UA_DataSetFieldConfig dsCfgMeasurementTime;
    memset(&dsCfgMeasurementTime, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgMeasurementTime.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgMeasurementTime.field.variable.fieldNameAlias = UA_STRING("MeasurementTime");
    dsCfgMeasurementTime.field.variable.promotedField = UA_FALSE;
    dsCfgMeasurementTime.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10102);
    dsCfgMeasurementTime.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//4
    UA_DataSetFieldConfig dsCfgSensor;
    memset(&dsCfgSensor, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgSensor.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgSensor.field.variable.fieldNameAlias = UA_STRING("Sensor");
    dsCfgSensor.field.variable.promotedField = UA_FALSE;
    dsCfgSensor.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10201);
    dsCfgSensor.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//5
    UA_DataSetFieldConfig dsCfgOperatingTime;
    memset(&dsCfgOperatingTime, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgOperatingTime.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgOperatingTime.field.variable.fieldNameAlias = UA_STRING("OperatingTime");
    dsCfgOperatingTime.field.variable.promotedField = UA_FALSE;
    dsCfgOperatingTime.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10202);
    dsCfgOperatingTime.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//6
    UA_DataSetFieldConfig dsCfgWarningMessage;
    memset(&dsCfgWarningMessage, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgWarningMessage.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgWarningMessage.field.variable.fieldNameAlias = UA_STRING("WarningMessage");
    dsCfgWarningMessage.field.variable.promotedField = UA_FALSE;
    dsCfgWarningMessage.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10203);
    dsCfgWarningMessage.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//7
    UA_DataSetFieldConfig dsCfgBootStatus;
    memset(&dsCfgBootStatus, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgBootStatus.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgBootStatus.field.variable.fieldNameAlias = UA_STRING("BootStatus");
    dsCfgBootStatus.field.variable.promotedField = UA_FALSE;
    dsCfgBootStatus.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10211);
    dsCfgBootStatus.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//8
    UA_DataSetFieldConfig dsCfgSnapshotStatus;
    memset(&dsCfgSnapshotStatus, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgSnapshotStatus.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgSnapshotStatus.field.variable.fieldNameAlias = UA_STRING("SnapshotStatus");
    dsCfgSnapshotStatus.field.variable.promotedField = UA_FALSE;
    dsCfgSnapshotStatus.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10212);
    dsCfgSnapshotStatus.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//9
    UA_DataSetFieldConfig dsCfgSCPStatus;
    memset(&dsCfgSCPStatus, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgSCPStatus.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgSCPStatus.field.variable.fieldNameAlias = UA_STRING("SCPStatus");
    dsCfgSCPStatus.field.variable.promotedField = UA_FALSE;
    dsCfgSCPStatus.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10213);
    dsCfgSCPStatus.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//10
    UA_DataSetFieldConfig dsCfgSFTPStatus;
    memset(&dsCfgSFTPStatus, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgSFTPStatus.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgSFTPStatus.field.variable.fieldNameAlias = UA_STRING("SFTPStatus");
    dsCfgSFTPStatus.field.variable.promotedField = UA_FALSE;
    dsCfgSFTPStatus.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10214);
    dsCfgSFTPStatus.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//11
    UA_DataSetFieldConfig dsCfgRunscriptStatus;
    memset(&dsCfgRunscriptStatus, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgRunscriptStatus.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgRunscriptStatus.field.variable.fieldNameAlias = UA_STRING("RunscriptStatus");
    dsCfgRunscriptStatus.field.variable.promotedField = UA_FALSE;
    dsCfgRunscriptStatus.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10215);
    dsCfgRunscriptStatus.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//12
    UA_DataSetFieldConfig dsCfgArchiveStatus;
    memset(&dsCfgArchiveStatus, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgArchiveStatus.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgArchiveStatus.field.variable.fieldNameAlias = UA_STRING("ArchiveStatus");
    dsCfgArchiveStatus.field.variable.promotedField = UA_FALSE;
    dsCfgArchiveStatus.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10216);
    dsCfgArchiveStatus.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//13
    UA_DataSetFieldConfig dsCfgAncillarySensorStatus;
    memset(&dsCfgAncillarySensorStatus, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgAncillarySensorStatus.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgAncillarySensorStatus.field.variable.fieldNameAlias = UA_STRING("AncillarySensorStatus");
    dsCfgAncillarySensorStatus.field.variable.promotedField = UA_FALSE;
    dsCfgAncillarySensorStatus.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10217);
    //out_AncillarySensorStatus; //UA_NODEID_NUMERIC(1, 10217);
    dsCfgAncillarySensorStatus.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//14
    UA_DataSetFieldConfig dsCfgIgramPP;
    memset(&dsCfgIgramPP, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgIgramPP.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgIgramPP.field.variable.fieldNameAlias = UA_STRING("IgramPP");
    dsCfgIgramPP.field.variable.promotedField = UA_FALSE;
    dsCfgIgramPP.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10301);
    //outIgramPP_Id; //UA_NODEID_NUMERIC(1, 10301);
    dsCfgIgramPP.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgIgramDC;
    memset(&dsCfgIgramDC, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgIgramDC.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgIgramDC.field.variable.fieldNameAlias = UA_STRING("IgramDC");
    dsCfgIgramDC.field.variable.promotedField = UA_FALSE;
    dsCfgIgramDC.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10302);
    //outIgramDC_Id; //UA_NODEID_NUMERIC(1, 10302);
    dsCfgIgramDC.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgLaserPP;
    memset(&dsCfgLaserPP, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgLaserPP.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgLaserPP.field.variable.fieldNameAlias = UA_STRING("LaserPP");
    dsCfgLaserPP.field.variable.promotedField = UA_FALSE;
    dsCfgLaserPP.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10303);
    //outLaserPP_Id; //UA_NODEID_NUMERIC(1, 10303);
    dsCfgLaserPP.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgLaserDC;
    memset(&dsCfgLaserDC, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgLaserDC.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgLaserDC.field.variable.fieldNameAlias = UA_STRING("LaserDC");
    dsCfgLaserDC.field.variable.promotedField = UA_FALSE;
    dsCfgLaserDC.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10304);
    //outLaserDC_Id; //UA_NODEID_NUMERIC(1, 10304);
    dsCfgLaserDC.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgSingleBeamAt900;
    memset(&dsCfgSingleBeamAt900, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgSingleBeamAt900.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgSingleBeamAt900.field.variable.fieldNameAlias = UA_STRING("SingleBeamAt900");
    dsCfgSingleBeamAt900.field.variable.promotedField = UA_FALSE;
    dsCfgSingleBeamAt900.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10305);
    //outSingleBeamAt900_Id; //UA_NODEID_NUMERIC(1, 10305);
    dsCfgSingleBeamAt900.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgSingleBeamAt2500;
    memset(&dsCfgSingleBeamAt2500, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgSingleBeamAt2500.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgSingleBeamAt2500.field.variable.fieldNameAlias = UA_STRING("SingleBeamAt2500");
    dsCfgSingleBeamAt2500.field.variable.promotedField = UA_FALSE;
    dsCfgSingleBeamAt2500.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10306);
    //outSingleBeamAt2500_Id; //UA_NODEID_NUMERIC(1, 10306);
    dsCfgSingleBeamAt2500.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgSignalToNoiseAt2500;
    memset(&dsCfgSignalToNoiseAt2500, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgSignalToNoiseAt2500.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgSignalToNoiseAt2500.field.variable.fieldNameAlias = UA_STRING("SignalToNoiseAt2500");
    dsCfgSignalToNoiseAt2500.field.variable.promotedField = UA_FALSE;
    dsCfgSignalToNoiseAt2500.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10307);
    //outSignalToNoiseAt2500_Id; //UA_NODEID_NUMERIC(1, 10307);
    dsCfgSignalToNoiseAt2500.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgCenterBurstLocation;
    memset(&dsCfgCenterBurstLocation, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgCenterBurstLocation.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgCenterBurstLocation.field.variable.fieldNameAlias = UA_STRING("CenterBurstLocation");
    dsCfgCenterBurstLocation.field.variable.promotedField = UA_FALSE;
    dsCfgCenterBurstLocation.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10308);
    //outCenterBurstLocation_Id; //UA_NODEID_NUMERIC(1, 10308);
    dsCfgCenterBurstLocation.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgDetectorTemp;
    memset(&dsCfgDetectorTemp, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgDetectorTemp.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgDetectorTemp.field.variable.fieldNameAlias = UA_STRING("DetectorTemperature");
    dsCfgDetectorTemp.field.variable.promotedField = UA_FALSE;
    dsCfgDetectorTemp.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10309);
    //outDetectorTemp_Id; //UA_NODEID_NUMERIC(1, 10309);
    dsCfgDetectorTemp.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgLaserFrequency;
    memset(&dsCfgLaserFrequency, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgLaserFrequency.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgLaserFrequency.field.variable.fieldNameAlias = UA_STRING("LaserFrequency");
    dsCfgLaserFrequency.field.variable.promotedField = UA_FALSE;
    dsCfgLaserFrequency.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10310);
    //outLaserFrequency_Id; //UA_NODEID_NUMERIC(1, 10310);
    dsCfgLaserFrequency.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgHardDriveSpace;
    memset(&dsCfgHardDriveSpace, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgHardDriveSpace.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgHardDriveSpace.field.variable.fieldNameAlias = UA_STRING("HardDriveSpace");
    dsCfgHardDriveSpace.field.variable.promotedField = UA_FALSE;
    dsCfgHardDriveSpace.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10311);
    //outHardDriveSpace_Id; //UA_NODEID_NUMERIC(1, 10311);
    dsCfgHardDriveSpace.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgFlow;
    memset(&dsCfgFlow, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgFlow.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgFlow.field.variable.fieldNameAlias = UA_STRING("Flow");
    dsCfgFlow.field.variable.promotedField = UA_FALSE;
    dsCfgFlow.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10312);
    //outFlow_Id; //UA_NODEID_NUMERIC(1, 10312);
    dsCfgFlow.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgTemperature;
    memset(&dsCfgTemperature, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgTemperature.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgTemperature.field.variable.fieldNameAlias = UA_STRING("Temperature");
    dsCfgTemperature.field.variable.promotedField = UA_FALSE;
    dsCfgTemperature.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10313);
    //outTemperature_Id; //UA_NODEID_NUMERIC(1, 10313);
    dsCfgTemperature.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgPressure;
    memset(&dsCfgPressure, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgPressure.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgPressure.field.variable.fieldNameAlias = UA_STRING("Pressure");
    dsCfgPressure.field.variable.promotedField = UA_FALSE;
    dsCfgPressure.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10314);
    //outPressure_Id; //UA_NODEID_NUMERIC(1, 10314);
    dsCfgPressure.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgTempOptics;
    memset(&dsCfgTempOptics, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgTempOptics.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgTempOptics.field.variable.fieldNameAlias = UA_STRING("TempOptics");
    dsCfgTempOptics.field.variable.promotedField = UA_FALSE;
    dsCfgTempOptics.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10315);
    //outTempOptics_Id; //UA_NODEID_NUMERIC(1, 10315);
    dsCfgTempOptics.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgBadScanCounter;
    memset(&dsCfgBadScanCounter, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgBadScanCounter.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgBadScanCounter.field.variable.fieldNameAlias = UA_STRING("BadScanCounter");
    dsCfgBadScanCounter.field.variable.promotedField = UA_FALSE;
    dsCfgBadScanCounter.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10316);
    //outBadScanCounter_Id; //UA_NODEID_NUMERIC(1, 10316);
    dsCfgBadScanCounter.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgFreeMemorySpace;
    memset(&dsCfgFreeMemorySpace, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgFreeMemorySpace.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgFreeMemorySpace.field.variable.fieldNameAlias = UA_STRING("FreeMemorySpace");
    dsCfgFreeMemorySpace.field.variable.promotedField = UA_FALSE;
    dsCfgFreeMemorySpace.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10317);
    //outFreeMemorySpace_Id; //UA_NODEID_NUMERIC(1, 10317);
    dsCfgFreeMemorySpace.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgLABFilename;
    memset(&dsCfgLABFilename, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgLABFilename.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgLABFilename.field.variable.fieldNameAlias = UA_STRING("LABFilename");
    dsCfgLABFilename.field.variable.promotedField = UA_FALSE;
    dsCfgLABFilename.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10318);
    //outLABFilename_Id; //UA_NODEID_NUMERIC(1, 10318);
    dsCfgLABFilename.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgLOGFilename;
    memset(&dsCfgLOGFilename, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgLOGFilename.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgLOGFilename.field.variable.fieldNameAlias = UA_STRING("LOGFilename");
    dsCfgLOGFilename.field.variable.promotedField = UA_FALSE;
    dsCfgLOGFilename.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10319);
    //outLOGFilename_Id; //UA_NODEID_NUMERIC(1, 10319);
    dsCfgLOGFilename.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgLgFilename;
    memset(&dsCfgLgFilename, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgLgFilename.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgLgFilename.field.variable.fieldNameAlias = UA_STRING("LgFilename");
    dsCfgLgFilename.field.variable.promotedField = UA_FALSE;
    dsCfgLgFilename.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10320);
    //outLgFilename_Id; //UA_NODEID_NUMERIC(1, 10320);
    dsCfgLgFilename.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgSecondLgFilename;
    memset(&dsCfgSecondLgFilename, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgSecondLgFilename.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgSecondLgFilename.field.variable.fieldNameAlias = UA_STRING("SecondLgFilename");
    dsCfgSecondLgFilename.field.variable.promotedField = UA_FALSE;
    dsCfgSecondLgFilename.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10321);
    //outSecondLgFilename_Id; //UA_NODEID_NUMERIC(1, 10321);
    dsCfgSecondLgFilename.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgSystemCounter;
    memset(&dsCfgSystemCounter, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgSystemCounter.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgSystemCounter.field.variable.fieldNameAlias = UA_STRING("SystemCounter");
    dsCfgSystemCounter.field.variable.promotedField = UA_FALSE;
    dsCfgSystemCounter.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10322);
    //outSystemCounter_Id; //UA_NODEID_NUMERIC(1, 10322);
    dsCfgSystemCounter.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgDetectorCounter;
    memset(&dsCfgDetectorCounter, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgDetectorCounter.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgDetectorCounter.field.variable.fieldNameAlias = UA_STRING("DetectorCounter");
    dsCfgDetectorCounter.field.variable.promotedField = UA_FALSE;
    dsCfgDetectorCounter.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10323);
    //outDetectorCounter_Id; //UA_NODEID_NUMERIC(1, 10323);
    dsCfgDetectorCounter.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgLaserCounter;
    memset(&dsCfgLaserCounter, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgLaserCounter.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgLaserCounter.field.variable.fieldNameAlias = UA_STRING("LaserCounter");
    dsCfgLaserCounter.field.variable.promotedField = UA_FALSE;
    dsCfgLaserCounter.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10324);
    //outLaserCounter_Id; //UA_NODEID_NUMERIC(1, 10324);
    dsCfgLaserCounter.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgFlowPumpCounter;
    memset(&dsCfgFlowPumpCounter, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgFlowPumpCounter.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgFlowPumpCounter.field.variable.fieldNameAlias = UA_STRING("FlowPumpCounter");
    dsCfgFlowPumpCounter.field.variable.promotedField = UA_FALSE;
    dsCfgFlowPumpCounter.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10325);
    //outFlowPumpCounter_Id; //UA_NODEID_NUMERIC(1, 10325);
    dsCfgFlowPumpCounter.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgDesiccantCounter;
    memset(&dsCfgDesiccantCounter, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgDesiccantCounter.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgDesiccantCounter.field.variable.fieldNameAlias = UA_STRING("DesiccantCounter");
    dsCfgDesiccantCounter.field.variable.promotedField = UA_FALSE;
    dsCfgDesiccantCounter.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10326);
    //outDesiccantCounter_Id; //UA_NODEID_NUMERIC(1, 10326);
    dsCfgDesiccantCounter.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    // add in order :: limit seemed to be 32
    // count = 4
    #ifdef DEBUB
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"SV_PubSub.c 733 : addDataSetField()");
    #endif
    // added by Jacky on 4/4/2021 to update MQTT payload (MetaDataVersion)
    // the corresponding change has to take place in open62541.c : UA_Server_addDataSetField() - however this change cause publisher to have error
        //UA_open62541/src/pubsub/ua_pubsub_writer.c
        //At line 691
    UA_PublishedDataSet *currentDataSet = UA_PublishedDataSet_findPDSbyId(uaServer, publishedDataSetIdentifier);
    currentDataSet->dataSetMetaData.configurationVersion.majorVersion = MAJOR_SOFTWARE_VERSION;
    currentDataSet->dataSetMetaData.configurationVersion.minorVersion = MINOR_SOFTWARE_VERSION;


    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgSoftwareVersion, NULL); // &f_SoftwareVersion_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgDataBlockVersion, NULL); //&f_DataBlockVersion_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgInstrumentTime, NULL); // &f_InstrumentTime_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgMeasurementTime, NULL); //&f_MeasurementTime_Id);

    // count = 3
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgSensor, NULL); //&f_RunscriptStatus_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgOperatingTime, NULL); //&f_ArchiveStatus_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgWarningMessage, NULL); //&f_AncillarySensorStatus_Id);

    // count = 7 => PubSub MQTT: publish: Send buffer is full. Possible reasons: send buffer is to small, sending to fast, broker not responding.

    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgBootStatus, NULL); //&f_BootStatus_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgSnapshotStatus, NULL); //&f_SnapshotStatus_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgSCPStatus, NULL); //&f_SCPStatus_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgSFTPStatus, NULL); //&f_SFTPStatus_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgRunscriptStatus, NULL); //&f_RunscriptStatus_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgArchiveStatus, NULL); //&f_ArchiveStatus_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgAncillarySensorStatus, NULL); //&f_AncillarySensorStatus_Id);

    #ifdef DEBUG_MODE
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"SV_PubSub.c :755 : addDataSetField()");
    #endif
// count = 25
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgIgramPP, NULL); //&f_IgramPP_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgIgramDC, NULL); //&f_IgramDC_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgLaserPP, NULL); //&f_LaserPP_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgLaserDC, NULL); //&f_LaserDC_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgSingleBeamAt900, NULL); //&f_SingleBeamAt900_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgSingleBeamAt2500, NULL); //&f_SingleBeamAt2500_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgSignalToNoiseAt2500, NULL); //&f_SignalToNoiseAt2500_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgCenterBurstLocation, NULL); //&f_CenterBurstLocation_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgDetectorTemp, NULL); //&f_DetectorTemp_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgLaserFrequency, NULL); //&f_LaserFrequency_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgHardDriveSpace, NULL);//&f_HardDriveSpace_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgFlow, NULL);//&f_Flow_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgTemperature, NULL); //&f_Temperature_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgPressure, NULL); //&f_Pressure_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgTempOptics, NULL); //&f_TempOptics_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgBadScanCounter, NULL); //&f_BadScanCounter_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgFreeMemorySpace, NULL); //&f_FreeMemorySpace_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgLABFilename, NULL); //&f_LABFilename_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgLOGFilename, NULL); //&f_LOGFilename_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgLgFilename, NULL); //&f_LgFilename_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgSecondLgFilename, NULL); //&f_SecondLgFilename_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgDetectorCounter, NULL); //&f_DetectorCounter_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgLaserCounter, NULL); //&f_LaserCounter_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgFlowPumpCounter, NULL); //&f_FlowPumpCounter_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgDesiccantCounter, NULL); //&f_DesiccantCounter_Id);

    #ifdef DEBUG_MODE
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"SV_PubSub.c :784 : addDataSetField()");
    #endif
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"SV_PubSub.c : addDataSetField : success");

}


/**
 * **WriterGroup handling**
 * The WriterGroup (WG) is part of the connection and contains the primary configuration
 * parameters for the message creation.
 */
static void
addWriterGroup(UA_Server *uaServer)
{
    /* Now we create a new WriterGroupConfig and add the group to the existing PubSubConnection. */
    UA_ServerConfig *ua_config = UA_Server_getConfig(uaServer);

    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Airgard WriterGroup");
	/* addDataSetReader Timeout= must be greater than publishing interval of corresponding WriterGroup */
    writerGroupConfig.publishingInterval = (UA_Double) MESSAGE_TIMEOUT_MQTT;	// set as 12000 millisec
    writerGroupConfig.enabled = UA_TRUE;
    writerGroupConfig.keepAliveTime = KEEP_ALIVE_MQTT;	// set as 10000 millisec
    writerGroupConfig.writerGroupId = WRITERGROUPID;
    writerGroupConfig.maxEncapsulatedDataSetMessageCount = 100;
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_NONE;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* decide whether to use JSON or UADP encoding*/
    if(useJson)
    {
	#ifdef DEBUG_MODE
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c :addWriterGroup : useJson = UA_TRUE");
	#endif
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_JSON;
        writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_JSONWRITERGROUPMESSAGEDATATYPE];
        /* The configuration flags for the messages are encapsulated inside the
         * message- and transport settings extension objects. These extension
         * objects are defined by the standard. e.g.
         * UadpWriterGroupMessageDataType */

        // Add the encryption key information
        UA_ServerConfig *config = UA_Server_getConfig(uaServer);
        writerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
        writerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];

        UA_JsonWriterGroupMessageDataType *Json_writerGroupMessage = UA_JsonWriterGroupMessageDataType_new();
        /* Change message settings of writerGroup to send PublisherId,
         * DataSetMessageHeader, SingleDataSetMessage and DataSetClassId in PayloadHeader
         * of NetworkMessage */
        Json_writerGroupMessage->networkMessageContentMask = 	(UA_JsonNetworkMessageContentMask)(UA_JSONNETWORKMESSAGECONTENTMASK_NETWORKMESSAGEHEADER |
            							(UA_JsonNetworkMessageContentMask)UA_JSONNETWORKMESSAGECONTENTMASK_DATASETMESSAGEHEADER |
            							(UA_JsonNetworkMessageContentMask)UA_JSONNETWORKMESSAGECONTENTMASK_SINGLEDATASETMESSAGE |
            							(UA_JsonNetworkMessageContentMask)UA_JSONNETWORKMESSAGECONTENTMASK_PUBLISHERID |
            							(UA_JsonNetworkMessageContentMask)UA_JSONNETWORKMESSAGECONTENTMASK_DATASETCLASSID);
        writerGroupConfig.messageSettings.content.decoded.data = Json_writerGroupMessage;
        UA_Server_addWriterGroup(uaServer, PubSubconnectionIdentifier, &writerGroupConfig, &writerGroupIdentifier);
        UA_Server_setWriterGroupOperational(uaServer, writerGroupIdentifier);
        UA_JsonWriterGroupMessageDataType_delete(Json_writerGroupMessage);
    }
    else
    {
	#ifdef DEBUG_MODE
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : addWriterGroup : useJson = UA_FALSE");
	#endif

        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        /* The configuration flags for the messages are encapsulated inside the
         * message- and transport settings extension objects. These extension
         * objects are defined by the standard. e.g.
         * UadpWriterGroupMessageDataType */

    	// Add the encryption key information
	UA_ServerConfig *config = UA_Server_getConfig(uaServer);
    	writerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    	writerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];

        UA_UadpWriterGroupMessageDataType *writerGroupMessage = UA_UadpWriterGroupMessageDataType_new();
        /* Change message settings of writerGroup to send PublisherId,
         * WriterGroupId in GroupHeader and DataSetWriterId in PayloadHeader
         * of NetworkMessage */
        writerGroupMessage->networkMessageContentMask =	(UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
            						(UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
            						(UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
            						(UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
        writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
	UA_Server_addWriterGroup(uaServer, PubSubconnectionIdentifier, &writerGroupConfig, &writerGroupIdentifier);
	UA_Server_setWriterGroupOperational(uaServer, writerGroupIdentifier);
	UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
     } // if (useJson)

     #ifdef KIV_FOR_OPENSSL_TEST
     // Now Add the encryption key information for UADP - default is ON
     UA_ByteString sk = {UA_AES128CTR_SIGNING_KEY_LENGTH, signingKey};
     UA_ByteString ek = {UA_AES128CTR_KEY_LENGTH, encryptingKey};
     UA_ByteString kn = {UA_AES128CTR_KEYNONCE_LENGTH, keyNonce};

     // Set the group key for the message encryption
     retval = UA_Server_setWriterGroupEncryptionKeys(uaServer, writerGroupIdentifier, 1, sk, ek, kn);        // 1 = securityTokenId
     if (retval!= UA_STATUSCODE_GOOD)
     {
             UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"SV_PubSub.c : addWriterGroup : UA_Server_setWriterGroupEncryptionKeys : failure %s", UA_StatusCode_name(retval));
	     //sleep(2);
             exit (EXIT_FAILURE);
     }
     #endif

    // The above is for UDAP; now this is for MQTT
    if (MQTT_Enable == UA_TRUE) // publish to MQTT Broker
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c addWriterGroup : MQTT_Enable = %d \n", MQTT_Enable);
    	// configure the mqtt publish topic
    	UA_BrokerWriterGroupTransportDataType brokerTransportSettings;
    	memset(&brokerTransportSettings, 0, sizeof(UA_BrokerWriterGroupTransportDataType));
    	// Assign the Topic at which MQTT publish should happen
    	//ToDo: Pass the topic as argument from the writer group
    	brokerTransportSettings.queueName = UA_STRING(PUBLISHER_TOPIC_EDGE2CLOUD_MQTT);
    	brokerTransportSettings.resourceUri = UA_STRING_NULL;
    	brokerTransportSettings.authenticationProfileUri = UA_STRING_NULL;

    	// Choose the QOS Level for MQTT
    	brokerTransportSettings.requestedDeliveryGuarantee = UA_BROKERTRANSPORTQUALITYOFSERVICE_BESTEFFORT;

    	// Encapsulate config in transportSettings
    	UA_ExtensionObject transportSettings;
    	memset(&transportSettings, 0, sizeof(UA_ExtensionObject));
    	transportSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    	transportSettings.content.decoded.type = &UA_TYPES[UA_TYPES_BROKERWRITERGROUPTRANSPORTDATATYPE];
    	transportSettings.content.decoded.data = &brokerTransportSettings;

    	writerGroupConfig.transportSettings = transportSettings;
        //UA_Server_addWriterGroup(uaServer, PubSubconnectionIdentifier, &writerGroupConfig, &writerGroupIdentifier);
        //UA_Server_setWriterGroupOperational(uaServer, writerGroupIdentifier);

	#ifdef DEBUG_MODE
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c :addWriterGroup : MQTT_TLS_Enable = %d", MQTT_TLS_Enable);
	#endif
     	if (MQTT_TLS_Enable == UA_TRUE)	//port 888x
        {

printf("I am here : in MQTT_TLS_Enable section and MQTT_Enable section \n");
//sleep(3);

	   if (!ua_config->pubSubConfig.securityPolicies)
	   {
		ua_config->pubSubConfig.securityPolicies = (UA_PubSubSecurityPolicy *) UA_malloc(sizeof(UA_PubSubSecurityPolicy));
		ua_config->pubSubConfig.securityPoliciesSize = 1;
		UA_PubSubSecurityPolicy_Aes128Ctr(ua_config->pubSubConfig.securityPolicies, &ua_config->logger);
	   }
	   assert(ua_config->pubSubConfig.securityPolicies);

	   // Encryption settings
    	   // Message are encrypted if a SecurityPolicy is configured and the
     	   // securityMode set accordingly. The symmetric key is a runtime information
     	   // and has to be set via UA_Server_setWriterGroupEncryptionKey. */
    	   writerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    	   writerGroupConfig.securityPolicy = &ua_config->pubSubConfig.securityPolicies[0];

           UA_Server_addWriterGroup(uaServer, PubSubconnectionIdentifier, &writerGroupConfig, &writerGroupIdentifier);
           UA_Server_setWriterGroupOperational(uaServer, writerGroupIdentifier);

		#ifdef KIV_FOR_OPENSSL_TEST
	     	// Now Add the encryption key information for UADP - default is ON
     		UA_ByteString sk = {UA_AES128CTR_SIGNING_KEY_LENGTH, signingKey};
     		UA_ByteString ek = {UA_AES128CTR_KEY_LENGTH, encryptingKey};
     		UA_ByteString kn = {UA_AES128CTR_KEYNONCE_LENGTH, keyNonce};

     		// Set the group key for the message encryption
     		retval = UA_Server_setWriterGroupEncryptionKeys(uaServer, writerGroupIdentifier, 1, sk, ek, kn);        // 1 = securityTokenId
     		if (retval!= UA_STATUSCODE_GOOD)
     		{
             		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"SV_PubSub.c : addWriterGroup (with TLS) : UA_Server_setWriterGroupEncryptionKeys : failure %s", UA_StatusCode_name(retval));
                	//sleep(2);
             		exit (EXIT_FAILURE);
     		}
		#endif
	}
	else
	{
           UA_Server_addWriterGroup(uaServer, PubSubconnectionIdentifier, &writerGroupConfig, &writerGroupIdentifier);
           UA_Server_setWriterGroupOperational(uaServer, writerGroupIdentifier);
	}
     }
     else // MQTT_Enable == UA_FALSE
     {
	#ifdef DEBUG_MODE
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : addWriterGroup : MQTT_Enable = %d", MQTT_Enable);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : addWriterGroup : MQTT_TLS_Enable = %d", MQTT_TLS_Enable);
	// do nothing
	#endif
     }
}


/**
 * **DataSetWriter handling**
 * A DataSetWriter (DSW) is the glue between the WG and the PDS. The DSW is
 * linked to exactly one PDS and contains additional informations for the
 * message generation.
 */

//static void
//addDataSetWriter(UA_Server *server, char *topic) {
static void
addDataSetWriter(UA_Server *uaServer) {

    // We need now a DataSetWriter within the WriterGroup. This means we must
    // create a new DataSetWriterConfig and add call the addWriterGroup function.
    UA_NodeId dataSetWriterIdentifier;
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Airgard DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = DATASETWRITERID;
    dataSetWriterConfig.dataSetName = UA_STRING("Airgard Dataset");
    dataSetWriterConfig.keyFrameCount = 10;

 	// refer to https://github.com/open62541/open62541/blob/master/tests/pubsub/check_pubsub_encoding_json.c
    	// enable metaDataVersion in UA_DataSetMessageHeader
	/*
	m.payload.dataSetPayload.dataSetMessages[0].header.configVersionMajorVersionEnabled = UA_TRUE;
    	m.payload.dataSetPayload.dataSetMessages[0].header.configVersionMinorVersionEnabled = UA_TRUE;
    	m.payload.dataSetPayload.dataSetMessages[0].header.configVersionMajorVersion = 42;
    	m.payload.dataSetPayload.dataSetMessages[0].header.configVersionMinorVersion = 7;
	*/
	/* no effect
    UA_KeyValuePair dataSetWriterProperties[4];
    size_t dataSetWriterPropertiesIndex = 0;
    UA_Boolean IsTrue = UA_TRUE;

    dataSetWriterProperties[dataSetWriterPropertiesIndex].key = UA_QUALIFIEDNAME(namespaceIndex,"configVersionMajorVersionEnabled");
    UA_Variant_setScalar(&dataSetWriterProperties[dataSetWriterPropertiesIndex].value, &IsTrue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    dataSetWriterPropertiesIndex++;

    dataSetWriterProperties[dataSetWriterPropertiesIndex].key = UA_QUALIFIEDNAME(namespaceIndex, "configVersionMinorVersionEnabled");
    UA_Variant_setScalar(&dataSetWriterProperties[dataSetWriterPropertiesIndex].value, &IsTrue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    dataSetWriterPropertiesIndex++;

    UA_Int16 majorVersion = MAJOR_SOFTWARE_VERSION;
    dataSetWriterProperties[dataSetWriterPropertiesIndex].key = UA_QUALIFIEDNAME(namespaceIndex,"configVersionMajorVersion");
    UA_Variant_setScalar(&dataSetWriterProperties[dataSetWriterPropertiesIndex].value, &majorVersion, &UA_TYPES[UA_TYPES_INT16]);
    dataSetWriterPropertiesIndex++;

    UA_Int16 minorVersion = MINOR_SOFTWARE_VERSION;
    dataSetWriterProperties[dataSetWriterPropertiesIndex].key = UA_QUALIFIEDNAME(namespaceIndex,"configVersionMinorVersion");
    UA_Variant_setScalar(&dataSetWriterProperties[dataSetWriterPropertiesIndex].value, &minorVersion, &UA_TYPES[UA_TYPES_INT16]);

    dataSetWriterConfig.dataSetWriterProperties = dataSetWriterProperties;

// line 1002
	*/
	/* addDataSetReader Timeout must be greater than publishing interval of corresponding WriterGroup */


#ifdef UA_ENABLE_JSON_ENCODING		// currently set as UA_TRUE in ccmake
    UA_JsonDataSetWriterMessageDataType jsonDswMd;
    UA_ExtensionObject messageSettings;
    if(useJson)
    {
	#ifdef DEBUG_MODE
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c :addDataSetWriter : useJson = UA_TRUE");
	#endif
        // JSON config for the dataSetWriter
        jsonDswMd.dataSetMessageContentMask = (UA_JsonDataSetMessageContentMask)
            (UA_JSONDATASETMESSAGECONTENTMASK_DATASETWRITERID |
             UA_JSONDATASETMESSAGECONTENTMASK_SEQUENCENUMBER |
             UA_JSONDATASETMESSAGECONTENTMASK_STATUS |
             UA_JSONDATASETMESSAGECONTENTMASK_METADATAVERSION |
             UA_JSONDATASETMESSAGECONTENTMASK_TIMESTAMP);

        messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_JSONDATASETWRITERMESSAGEDATATYPE];
        messageSettings.content.decoded.data = &jsonDswMd;

        dataSetWriterConfig.messageSettings = messageSettings;
    }
#endif

    if (MQTT_Enable)
    {
    	//TODO: Modify MQTT send to add DataSetWriters broker transport settings
    	//TODO: Pass the topic as argument from the writer group
    	//TODO: Publish Metadata to metaDataQueueName
    	// configure the mqtt publish topic
    	UA_BrokerDataSetWriterTransportDataType brokerTransportSettings;
    	memset(&brokerTransportSettings, 0, sizeof(UA_BrokerDataSetWriterTransportDataType));

    	// Assign the Topic at which MQTT publish should happen
    	brokerTransportSettings.queueName = UA_STRING(PUBLISHER_TOPIC_EDGE2CLOUD_MQTT);
    	brokerTransportSettings.resourceUri = UA_STRING_NULL;
    	brokerTransportSettings.authenticationProfileUri = UA_STRING_NULL;
    	brokerTransportSettings.metaDataQueueName = UA_STRING(PUBLISHER_METADATAQUEUENAME_MQTT);
    	brokerTransportSettings.metaDataUpdateTime = PUBLISHER_METADATAUPDATETIME_MQTT;

    	// Choose the QOS Level for MQTT
    	brokerTransportSettings.requestedDeliveryGuarantee = UA_BROKERTRANSPORTQUALITYOFSERVICE_BESTEFFORT;

    	// Encapsulate config in transportSettings
    	UA_ExtensionObject transportSettings;
    	memset(&transportSettings, 0, sizeof(UA_ExtensionObject));
    	transportSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    	transportSettings.content.decoded.type = &UA_TYPES[UA_TYPES_BROKERDATASETWRITERTRANSPORTDATATYPE];
    	transportSettings.content.decoded.data = &brokerTransportSettings;

    	dataSetWriterConfig.transportSettings = transportSettings;
    }

    UA_Server_addDataSetWriter(uaServer, writerGroupIdentifier, publishedDataSetIdentifier,
                               &dataSetWriterConfig, &dataSetWriterIdentifier);

}

//==============================================================================

static void fillDataSetMetaData(UA_DataSetMetaDataType *pMetaData)
{

        UA_Variant varData;
        UA_Float data = 500.4;
        UA_Variant_setScalar(&varData,&data, &UA_TYPES[UA_TYPES_FLOAT]);

	#ifdef DEBUG_MODE
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : fillDataSetMetaData");
	#endif
	if (pMetaData == NULL)
		return;

	UA_DataSetMetaDataType_init(pMetaData);

	//pMetaData->namespacesSize = 1;
	//pMetaData->namespaces[0] = (UA_STRING *)calloc(pMetaData->namespacesSize, sizeof(UA_STRING("MKS")) );

			//UA_FieldTargetVariable *targetVars = (UA_FieldTargetVariable *) UA_calloc(dataSetReaderConfig.dataSetMetaData.fieldsSize, sizeof(UA_FieldTargetVariable));

	//pMetaData->namespaces[0] = UA_STRING("MKS");
	//pMetaData->structureDataTypesSize = ??;
	//pMetaData->enumDataTypesSize = ??
	//pMetaData->enumDataTypes = "";
	//pMetaData->simpleDataTypesSize == ??;
	//pMetadata->simpleDataTypes = ??

	pMetaData->name = UA_STRING("AirGardSensor MetaData");
	pMetaData->description = UA_LOCALIZEDTEXT("en-US","AirGardSensor MetaData");
	pMetaData->fieldsSize = (size_t)40;	// number of nodes : 40 or 26
	pMetaData->fields = (UA_FieldMetaData*) UA_Array_new (pMetaData->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);

	// non-diagnostics = 14 (n)
	// diagnostics = 26 (m)
	// alarms/ non alarms

	// static definition of number of fields size to 4 to create four different
	//  targetVariables of distinct datatype
	//  Currently the publisher sneds only DateTime data type 
	//

	// header section

/*
typedef struct {
    UA_String name;
    UA_LocalizedText description;
    UA_DataSetFieldFlags fieldFlags;
    UA_Byte builtInType;
    UA_NodeId dataType;
    UA_Int32 valueRank;
    size_t arrayDimensionsSize;
    UA_UInt32 *arrayDimensions;
    UA_UInt32 maxStringLength;
    UA_Guid dataSetFieldId;
    size_t propertiesSize;
    UA_KeyValuePair *properties;
} UA_FieldMetaData;

*/
	UA_UInt32 *t_arrayDimensions;
	t_arrayDimensions = calloc(1, sizeof(UA_UInt32));
	t_arrayDimensions[0] = -1;	// match 10001

	int m=0, n=0;
	// 80300
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("1. Software Version");
	pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "1. Software Version");
	pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;	//UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR;	//UA_VALUERANK_ONE_DIMENSION;
	//pMetaData->fields[n].arrayDimensionsSize = (size_t)-1;
	//pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
	pMetaData->fields[n].maxStringLength = 1;
	pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

	n++;	//1 : 80301
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("2. Data Block Version");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "2. DataBlock Version");
	pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
	pMetaData->fields[n].maxStringLength = 1;
	pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

	n++;	//2
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("1. Instrument Time");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "1. Instrument Time");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1;
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;	//3
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("2. Measurement Time");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "2. Measurement Time");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1;
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;	//4
	// Sensor section
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("1. Sensor");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "1. Sensor");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1; // MAX_STRING_SIZE
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;	//5
        UA_FieldMetaData_init (&pMetaData->fields[n]);
	pMetaData->fields[n].name = UA_STRING("2. Operating Time");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "2. Operating Time");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_INT16;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT16].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;	//6
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("3. Warning Message");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "3. Warning Message");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;	//7
	// Status->Info
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("1. Boot Status");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "1. Boot Status");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;	//8
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("2. Snapshot Status");
	pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "2. Snapshot Status");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;	//9
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("3. SCP Status");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "3. SCP Status");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;	//10 : 80310
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("4. SFTP Status");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "4. SFTP Status");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_ANY; // scalar;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;	//11
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("5. Runscript Status");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "5. Runscript Status");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;	//12
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("6. Archive Status");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "6. Archive Status");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;	//13
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("7. Ancillary Sensor Status");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "7. Ancillary Sensor Status");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;	//14
	m = n;
	// Diagnostics section
	UA_FieldMetaData_init (&pMetaData->fields[m]);
	pMetaData->fields[m].name = UA_STRING("01. Igram PP");
	pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "01. Igram PP");
	pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
	pMetaData->fields[n].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
	UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
	pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
	//UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

	m++; // 1, 15
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("02. Igram DC");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "02. Igram DC");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
	//UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbling from the MQTT message

	m++; // 2, 16
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("03. Laser PP");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "03. Laser PP");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbling from the MQTT message

	m++; // 3, 17
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("04. Laser DC");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "04. Laser DC");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

	m++; // 4, 18
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("05. Single Beam At 900");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "05. Single Beam At 900");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setSc4lar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

	m++; // 5, 19
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("06. Single Beam At 2500");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "06. Single Beam At 2500");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

	m++; // 6, 20 : 80320
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("07. SignalToNoiseAt2500");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "07. SignalToNoiseAt2500");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_INT16;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT16].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

	m++; // 7, 21
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("08. Center Burst Location");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "08. Center Burst Location");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

	m++; // 8, 22
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("09. Detector Temperature");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "09. Detector Temperature");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

	m++; // 9, 23
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("10. Laser Frequency");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "10. Laser Frequency");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

	m++; //10, 24
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("11. Hard Drive Space");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "11. Hard Drive Space");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_INT16;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT16].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

	m++; //11, 25
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("12. Flow");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "12. Flow");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_INT16;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT16].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

	m++; //12, 26
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("13. Temperature");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "13. Temperature");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_INT16;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT16].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

	m++; //13, 27
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("14. Pressure");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "14. Pressure");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

	m++; //14, 28
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("15. Temp Optics");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "15. Temp Optics");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_INT16;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT16].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

	m++; //15, 29
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("16. Bad Scan Counter");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "16. Bad Scan Counter");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_INT16;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT16].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

	m++; //16, 30 : 80330
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("17. Free Memory Space");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "17. Free Memory Space");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_INT16;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT16].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

	m++; //17, 31
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("18. LAB Filename");
	pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "18. LAB Filename");
	pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_STRING;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[m].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[m].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[m].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[m].propertiesSize = 0;
       //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

	m++; //18, 32
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("19. LOG Filename");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "19. LOG Filename");
	pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_STRING;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[m].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[m].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[m].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

	m++; //19, 33
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("20. Lg Filename");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "20. Lg Filename");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_STRING;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[m].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[m].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[m].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

	m++; //20, 34
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("21. Second Lg Filename");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "21. Second Lg Filename");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_STRING;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[m].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[m].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[m].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

	m++; //21, 35
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("22. System Counter");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "22. System Counter");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
	pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

	m++; // 22, 36
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("23. Detector Counter");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "23. Detector Counter");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

	m++; // 23, 37
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("24. Laser Counter");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "24. Laser Counter");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

	m++; // 24, 38
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("25. Flow Pump Counter");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "25. Flow Pump Counter");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

	m++; // 25, 39
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("26. Desiccant Counter");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "26. Desiccant Counter");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

	m++; // 26, 40

	#ifdef DEBUG_MODE
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c (line 966) : leaving fillDataSetMetaData");
	#endif
}


static UA_StatusCode
addDataSetReader(UA_Server *uaServer)
{
	if (uaServer == NULL)
		return UA_STATUSCODE_BADINTERNALERROR;

	#ifdef DEBUG_MODE
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : addDataSetReader()");
	#endif

	UA_StatusCode retval = UA_STATUSCODE_GOOD;
	//UA_Variant varPublisherId;
	//UA_Int32 publisherId = 1000;

	//UA_Variant_setScalar(&varPublisherId, &publisherId, &UA_TYPES[UA_TYPES_INT32]);

	memset(&dataSetReaderConfig, 0, sizeof(UA_DataSetReaderConfig));
	dataSetReaderConfig.name = UA_STRING("Airgard DataSetReader");
        /* Parameters to filter which DataSetMessage has to be processed
         * by the DataSetReader */
        /* The following parameters are used to show that the data published by
         * tutorial_pubsub_publish.c is being subscribed and is being updated in
         * the information model */
	UA_UInt16 publisherIdentifier = PUBLISHERID;
	dataSetReaderConfig.publisherId.type = &UA_TYPES[UA_TYPES_UINT16];
	dataSetReaderConfig.publisherId.data = &publisherIdentifier;
	UA_Variant_setScalar(&dataSetReaderConfig.publisherId, &publisherIdentifier, &UA_TYPES[UA_TYPES_INT16]); // filter away messages based on this parameter : UA_Variant
	dataSetReaderConfig.writerGroupId = WRITERGROUPID;		// filter away messages based on this parameter
	dataSetReaderConfig.dataSetWriterId = DATASETWRITERID;		// filter away messages based on this parameter
	dataSetReaderConfig.messageReceiveTimeout = (UA_Double) MESSAGE_TIMEOUT_MQTT;	// * Timeout must be greater than publishing interval of corresponding WriterGroup */

// the following is not in the sample (section X)
	dataSetReaderConfig.dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_RAWDATA;	// UA_DataSetFieldContentMask
	dataSetReaderConfig.securityParameters.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT; 	// UA_PubSubSecurityParameters
	dataSetReaderConfig.securityParameters.securityGroupId = UA_STRING("AirgardSecurityGroupId-1");
	//dataSetReaderConfig.securityParameters.keyServersSize = (size_t)1;	// size_t;
	//dataSetReaderConfig.messageSettings = ??		// UA_ExtensionObject
	//dataSetReaderConfig.transportSettings = ??		// UA_ExtensionObject
	dataSetReaderConfig.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;	// UA_SubscribedDataSetEnumType
//	dataSetReaderConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize = 2;	// UA_TargetVariables
//	dataSetReaderConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables.externalDataValue = *MQTT data* 	// UA_FieldTargetVariable .. UA_DataValue**
// the above is not in the sample

// added by Jacky by comparing the code with addDataSetWriter()
#ifdef UA_ENABLE_JSON_ENCODING
        UA_JsonDataSetReaderMessageDataType jsonDsrMd;
        UA_ExtensionObject messageSettings;
        if(useJson)
        {
		#ifdef DEBUG_MODE
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c :addDataSetReader : useJson = UA_TRUE");
		#endif
                // JSON config for the dataSetReader
                jsonDsrMd.dataSetMessageContentMask = (UA_JsonDataSetMessageContentMask)
                       (UA_JSONDATASETMESSAGECONTENTMASK_DATASETWRITERID |
                        UA_JSONDATASETMESSAGECONTENTMASK_SEQUENCENUMBER |
                        UA_JSONDATASETMESSAGECONTENTMASK_STATUS |
                        UA_JSONDATASETMESSAGECONTENTMASK_METADATAVERSION |
                        UA_JSONDATASETMESSAGECONTENTMASK_TIMESTAMP);

                messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
                messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_JSONDATASETWRITERMESSAGEDATATYPE];
                messageSettings.content.decoded.data = &jsonDsrMd;

                dataSetReaderConfig.messageSettings = messageSettings;
        }
#endif

    if (MQTT_Enable)
    {
        //TODO: Modify MQTT send to add DataSetWriters broker transport settings
        //TODO: Pass the topic as argument from the writer group
        //TODO: Publish Metadata to metaDataQueueName
        // configure the mqtt publish topic
        UA_BrokerDataSetReaderTransportDataType brokerTransportSettings;
        memset(&brokerTransportSettings, 0, sizeof(UA_BrokerDataSetReaderTransportDataType));

        // Assign the Topic at which MQTT subscribe should happen
        brokerTransportSettings.queueName = UA_STRING(SUBSCRIBER_TOPIC_MQTT);
        brokerTransportSettings.resourceUri = UA_STRING_NULL;
        brokerTransportSettings.authenticationProfileUri = UA_STRING_NULL;
        brokerTransportSettings.metaDataQueueName = UA_STRING(SUBSCRIBER_METADATAQUEUENAME_MQTT);

        // Choose the QOS Level for MQTT
        brokerTransportSettings.requestedDeliveryGuarantee = UA_BROKERTRANSPORTQUALITYOFSERVICE_BESTEFFORT;

        // Encapsultate config in transportSettings
        UA_ExtensionObject transportSettings;
        memset(&transportSettings, 0, sizeof(UA_ExtensionObject));
        transportSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        transportSettings.content.decoded.type = &UA_TYPES[UA_TYPES_BROKERDATASETREADERTRANSPORTDATATYPE];
        transportSettings.content.decoded.data = &brokerTransportSettings;

        dataSetReaderConfig.transportSettings = transportSettings;
    }
    // end added by Jacky by comparing the code with addDataSetWriter()

	fillDataSetMetaData(&dataSetReaderConfig.dataSetMetaData);
	// the following caused segmentation fault if section X is not configured properly
	retval = UA_Server_addDataSetReader(uaServer, readerGroupIdentifier, &dataSetReaderConfig, &readerIdentifier);


	// Create a second reader for the same writer in the same readergroup - future use.. currently cannot think of use case
	UA_NodeId reader2Id;
	retval = UA_Server_addDataSetReader(uaServer, readerGroupIdentifier, &dataSetReaderConfig, &reader2Id);

	return retval;
}// end addDataSetReader()

static UA_StatusCode
addReaderGroup(UA_Server *uaServer)
{
    if (uaServer == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    #ifdef DEBUG_MODE
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : addReaderGroup()");
    #endif

    UA_ReaderGroupConfig readerGroupConfig;
    memset(&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
/*
typedef struct {
    UA_String name;
    UA_PubSubSecurityParameters securityParameters;
    // PubSub Manager Callback
    UA_PubSub_CallbackLifecycle pubsubManagerCallback;
    // non std. field
    UA_Duration subscribingInterval; // Callback interval for subscriber: set the least publishingInterval value of all DSRs in this RG
    UA_Boolean enableBlockingSocket; // To enable or disable blocking socket option
    UA_UInt32 timeout; // Timeout for receive to wait for the packets
    UA_PubSubRTLevel rtLevel;
} UA_ReaderGroupConfig;
*/
    readerGroupConfig.name = UA_STRING("Airgard ReaderGroup");
    //readerGroupConfig.securityParameters = ???
    //readerGroupConfig.pubSubManagerCallback = ???

    // removed from v.1.2.1
    //readerGroupConfig.subscribingInterval = 4500;	// milliseconds
    //readerGroupConfig.enableBlockingSocket = UA_FALSE;
    //readerGroupConfig.timeout = 30;	// 30 seconds

    readerGroupConfig.rtLevel = UA_PUBSUB_RT_NONE;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* decide whether to use JSON or UADP encoding */
#ifdef UA_ENABLE_JSON_ENCODING
    if(useJson)
    {
	#ifdef DEBUG_MODE
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : addedReaderGroup : useJson = UA_TRUE");
	#endif
	//readerGroupConfig.name = ??
	//readerGroupConfig.securityParameters = ??
	//readerGroupConfig.pubsubManagerCallback = ??
	//readerGroupConfig.subscribingInterval = ???
    	//readerGroupConfig.enableBlockingSocket = UA_FALSE;
    	//readerGroupConfig.timeout = 30    // 30 seconds
    	//readerGroupConfig.rtLevel = UA_PUBSUB_RT_NONE;
     }
     else
#endif
    {
	#ifdef DEBUG_MODE
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : addedReaderGroup : useJson = UA_FALSE");
	#endif

        retval = UA_Server_addReaderGroup(uaServer, PubSubconnectionIdentifier, &readerGroupConfig, &readerGroupIdentifier);
        //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"SV_PubSub.c : addReaderGroup %s", UA_StatusCode_name(retval));

	/* this following caused segmentation fault
    	retval = UA_Server_setReaderGroupOperational(uaServer, readerGroupIdentifier);
   	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"SV_PubSub.c : addReaderGroupOperational %s", UA_StatusCode_name(retval));
	*/
    }
/*
    if (MQTT_Enable)
    {
 	printf("addReaderGroup : MQTT_Enable = %d \n", MQTT_Enable);
	// configure the mqtt subscribe topic
	UA_BrokerReaderGroupTransportDataType brokerTransportSettings;
	memset(&brokerTransportSettings, 0, sizeof(UA_BrokerReaderGroupTransportDataType));
	// Assign the Topic at which MQTT subscribe should happen
	brokerTransportSettings.queueName = UA_STRING(SUBSCRIBER_TOPIC_MQTT);
        brokerTransportSettings.resourceUri = UA_STRING_NULL;
        brokerTransportSettings.authenticationProfileUri = UA_STRING_NULL;

       // Choose the QOS Level for MQTT
        brokerTransportSettings.requestedDeliveryGuarantee = UA_BROKERTRANSPORTQUALITYOFSERVICE_BESTEFFORT;

        // Encapsulate config in transportSettings
        UA_ExtensionObject transportSettings;
        memset(&transportSettings, 0, sizeof(UA_ExtensionObject));
	transportSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        transportSettings.content.decoded.type = &UA_TYPES[UA_TYPES_BROKERREADERGROUPTRANSPORTDATATYPE];
        transportSettings.content.decoded.data = &brokerTransportSettings;

        readerGroupConfig.transportSettings = transportSettings;
    }
    retval = UA_Server_addReaderGroup(uaServer, PubSubconnectionIdentifier, &readerGroupConfig, &readerGroupIdentifier);
*/
    if (retval == UA_STATUSCODE_GOOD)
    {
	#ifdef DEBUG_MODE
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : UA_Server_addReaderGroup : success");
	#endif

	/* Add the encryption key informaton */
    	UA_ByteString sk = {UA_AES128CTR_SIGNING_KEY_LENGTH, signingKey};
    	UA_ByteString ek = {UA_AES128CTR_KEY_LENGTH, encryptingKey};
    	UA_ByteString kn = {UA_AES128CTR_KEYNONCE_LENGTH, keyNonce};

    	// TODO security token not necessary for readergroup (extracted from security-header)
    	UA_Server_setReaderGroupEncryptionKeys(uaServer, readerGroupIdentifier, 1, sk, ek, kn);

        UA_Server_setReaderGroupOperational(uaServer, readerGroupIdentifier);

	#ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"SV_PubSub.c : addReaderGroup : success %s", UA_StatusCode_name(retval));
	#endif
    }
    else
    {
	#ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"SV_PubSub.c : addReaderGroup : failure %s", UA_StatusCode_name(retval));
	#endif
	exit(EXIT_FAILURE);
    }
}

static UA_NodeId *newSubscribedNodeId;	// an array of UA_NodeId

static UA_StatusCode
addSubscribedVariables(UA_Server *uaServer, UA_NodeId dataSetReaderId)
{

	// add the variables subscribed from the broker to the OPCUA Address space
	static int firsttime_addSubscribedVariableFlag = UA_TRUE;
	static UA_NodeId airgardfolderId;
	static UA_NodeId datafolderId;
	static UA_NodeId diagnosticfolderId;
	static UA_NodeId methodsfolderId;
	static UA_NodeId statusfolderId;
	static UA_NodeId statusinfofolderId;
	static UA_NodeId timestampfolderId;
	UA_NodeId newNodeId;
	UA_StatusCode retval = UA_STATUSCODE_GOOD;
	UA_FieldTargetVariable *targetVars ;

	if (uaServer == NULL)
		return UA_STATUSCODE_BADINTERNALERROR;
	// TODO:
	// check if there is any changes to the metadata version
	// if firsttime_addSubscribedVariableFlag == UA_TRUE, no action required, continue to create nodes
	// if firsttime_addSubscribedVariableFlag == UA_FALSE and if there are changes, we need to :
	// 	1. reset the array of newNode
	// 	2. reallocate the array of newNode based on the new metadata version
	//	3. reset & recreate targetVars
	// 	3. recreate the nodes and repopulate the values again

	#ifdef DEBUG_MODE
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : addSubscribeVariables() - firstime = %d", firsttime_addSubscribedVariableFlag);
	#endif

	if (firsttime_addSubscribedVariableFlag == UA_TRUE)	// only need to add nodes 1 time
	{
                UA_String folderName = UA_STRING("Airgard Subscribed"); //dataSetReaderConfig.dataSetMetaData.name;
                UA_ObjectAttributes oAttrObj = UA_ObjectAttributes_default;
		UA_VariableAttributes oAttVar = UA_VariableAttributes_default;
                UA_QualifiedName airgardfolderBrowseName, diagnosticfolderBrowseName;

	        //Add a new namespace to the server => Returns the index of the new namespace i.e. namespaceIndex
        	UA_Int16 nsIdx_MKS = UA_Server_addNamespace(uaServer, "virtualskies.com.sg/MKS/");
		size_t namespaceIndex;
		UA_Server_getNamespaceByName(uaServer, UA_STRING("virtualskies.com.sg/MKS/"), &namespaceIndex);

                if (folderName.length > 0)
                {
                        oAttrObj.displayName.locale = UA_STRING("en-US");
                        oAttrObj.displayName.text = folderName;		// actual shown in UAExpert - "en-US", "AirgardSensor MetaData"
                        airgardfolderBrowseName.namespaceIndex = namespaceIndex;	// actual shown in UAExpert - 1, "AirgardSensor MetaData"
                        airgardfolderBrowseName.name = folderName;	// actual shown in UAExpert - 1, "AirgardSensor MetaData"
                }
                else
                {
                        oAttrObj.displayName = UA_LOCALIZEDTEXT("en-US", "AirgardSensor Subscribed");
                        airgardfolderBrowseName.namespaceIndex = namespaceIndex;
			airgardfolderBrowseName.name = UA_STRING("AirgardSensor Subscribed");
                }

		// create a UA structure to receive the incoming mqtt streams
                retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "AirGardSensor Subscribed"), //UA_NODEID_NULL,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), //UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                        airgardfolderBrowseName,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                        oAttrObj, NULL, &airgardfolderId);

		#ifdef DEBUG_MODE
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c :Pass 0 : %s", UA_StatusCode_name(retval));
		#endif
        /* Create a rudimentary objectType
         *
         * airgardfolderId
         * |
         * + (V) SoftwareVersion
         * + (V) DataBlockVersion
         * +-(OT) TimestampType
         * |   + (V) InstrumentTime
         * |   + (V) MeasurementTime
         * +-(OT) StatusType
         * |   + (V) Sensor
         * |   + (V) OperatingTime
         * |   + (V) WarningMessage
         * |   +(OT) InfoType
         * |   |--+ (V) BootStatus
         * |   |  + (V) SnapshotStatus
         * |   |  + (V) SFTPStatus
         * |-(OT) DiagnosticsType
         * |   + (V) IgramPP
         * |   + (V) ...
         * |   + (V) DesiccantCounter
         * |-(OT) DataType (10400)
         * |   + (V) Alarms (20000)
         * |   + (OT) Alarm
         * |   |---+ (V) Tag
         * |   |   + (V) Name
         * |   |   + (V) Probability
         * |   |   + (V) CASnumber
         * |   |   + (V) Concentration
         * |   + (OT) Alarm
         * |   | <repeat>
         * |   + (V) NonAlarms (30000)
	 */

                // Subtree : Airgard->Data       == 
                oAttrObj.displayName = UA_LOCALIZEDTEXT("en-US", "Data");
                retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "Airgard_Data_S"),
                                        airgardfolderId,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                        UA_QUALIFIEDNAME(namespaceIndex, "Data"), //diagnosticfolderBrowseName,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                        oAttrObj, NULL, &datafolderId);
		#ifdef DEBUG_MODE
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c Pass 1 : %s", UA_StatusCode_name(retval));
		#endif

		// Subtree : Airgard->Diagnostics 	== OKAY
		oAttrObj.displayName = UA_LOCALIZEDTEXT("en-US", "Diagnostics");
		retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "Airgard_Diagnostics_S"),
					airgardfolderId,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
					UA_QUALIFIEDNAME(namespaceIndex, "Diagnostic"),
					UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
					oAttrObj, NULL, &diagnosticfolderId);
		#ifdef DEBUG_MODE
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c Pass 2 : %s", UA_StatusCode_name(retval));
		#endif

                // Subtree : Airgard->Methods       ==
                oAttrObj.displayName = UA_LOCALIZEDTEXT("en-US", "Methods");
                retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "Airgard_Methods_S"),
                                        airgardfolderId,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                        UA_QUALIFIEDNAME(namespaceIndex, "Methods"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                        oAttrObj, NULL, &methodsfolderId);
		#ifdef DEBUG_MODE
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c Pass 3 : %s", UA_StatusCode_name(retval));
		#endif

		// Subtree : Airgard->Status	== OKAY
		oAttrObj.displayName = UA_LOCALIZEDTEXT("en-US", "Status");
		retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "AirGard_Status_S"),
					airgardfolderId,
					UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
					UA_QUALIFIEDNAME(namespaceIndex, "Status"),
					UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
					oAttrObj, NULL, &statusfolderId);
		#ifdef DEBUG_MODE
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c Pass 4 : %s", UA_StatusCode_name(retval));
		#endif

		// Subtree : Airgard->Status->Info	== OKAY
		oAttrObj.displayName = UA_LOCALIZEDTEXT("en-US", "Info");
                retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "AirGard_Status_Info_S"),
                                        statusfolderId,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                        UA_QUALIFIEDNAME(namespaceIndex, "Info"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                        oAttrObj, NULL, &statusinfofolderId);
		#ifdef DEBUG_MODE
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c Pass 5 : %s", UA_StatusCode_name(retval));
		#endif

		// Subtree : Airgard->Timestamp		== OKAY
		oAttrObj.displayName = UA_LOCALIZEDTEXT("en-US", "TimeStamp");
		retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "AirGard_TimeStamp_S"),
					airgardfolderId,
					UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
					UA_QUALIFIEDNAME(namespaceIndex, "Timestamp"),
					UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
					oAttrObj, NULL, &timestampfolderId);
		#ifdef DEBUG_MODE
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c Pass 6 : %s", UA_StatusCode_name(retval));
		#endif

		// Create target variables with respect to DataSetMetaDeta fields
		//UA_DataSetReaderConfig dataSetReaderConfig -- global variable
                targetVars = (UA_FieldTargetVariable *) \
						UA_calloc(dataSetReaderConfig.dataSetMetaData.fieldsSize, \
						sizeof(UA_FieldTargetVariable));

		UA_Variant variant_float, variant_int;
		UA_Float data_float = -99.99;
                UA_Int32 data_int = -99;
                UA_String data_string = UA_STRING("Default string");

		newSubscribedNodeId = (UA_NodeId *) calloc(dataSetReaderConfig.dataSetMetaData.fieldsSize, sizeof(dataSetReaderConfig.dataSetMetaData));

                for (size_t i=0; i< dataSetReaderConfig.dataSetMetaData.fieldsSize; i++)
                {
			// Variable to subscribed data */
			UA_VariableAttributes vAttr = UA_VariableAttributes_default;
			UA_LocalizedText_copy(&dataSetReaderConfig.dataSetMetaData.fields[i].description,
						&vAttr.description);
			vAttr.displayName.locale = UA_STRING("en-US");
			vAttr.displayName.text = dataSetReaderConfig.dataSetMetaData.fields[i].name;	// UA_String
/* NodeId dataType */	vAttr.dataType = dataSetReaderConfig.dataSetMetaData.fields[i].dataType;
/* UA_Int32 */		vAttr.valueRank = dataSetReaderConfig.dataSetMetaData.fields[i].valueRank;
			if (vAttr.valueRank > 0)
			{
				vAttr.arrayDimensionsSize = dataSetReaderConfig.dataSetMetaData.fields[i].arrayDimensionsSize;
				vAttr.arrayDimensions = dataSetReaderConfig.dataSetMetaData.fields[i].arrayDimensions;
			}
			else
			{
				vAttr.arrayDimensionsSize = 0;
			}
			if ((i>=14) || (i<=39))
			{
/* UA_Byte */			vAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE | UA_ACCESSLEVELMASK_HISTORYREAD;
/* UA_Byte */			vAttr.userAccessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE | UA_ACCESSLEVELMASK_HISTORYREAD;// needed by UAExpert
/* UA_Boolean */		vAttr.historizing = UA_TRUE;
			}
			else
			{
/* UA_Byte */                   vAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
/* UA_Byte */                   vAttr.userAccessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
/* UA_Boolean */                vAttr.historizing = UA_FALSE;
			}

 			UA_NodeId newNode;
			// changed on 2 Jan 2022
                        if (dataSetReaderConfig.dataSetMetaData.fields[i].builtInType == UA_NS0ID_FLOAT)
                                UA_Variant_setScalar(&vAttr.value, &data_float, &UA_TYPES[UA_TYPES_FLOAT]);
                        else if (dataSetReaderConfig.dataSetMetaData.fields[i].builtInType == UA_NS0ID_INT16)
                                UA_Variant_setScalar(&vAttr.value, &data_int, &UA_TYPES[UA_TYPES_INT16]);
                        else if (dataSetReaderConfig.dataSetMetaData.fields[i].builtInType == UA_NS0ID_STRING)
                                UA_Variant_setScalar(&vAttr.value, &data_string, &UA_TYPES[UA_TYPES_STRING]);

			if ((i==0) || (i==1))
                                retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, (UA_UInt32)i+1+80300),
                                                        airgardfolderId,
                                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                                        UA_QUALIFIEDNAME(namespaceIndex, (char *)dataSetReaderConfig.dataSetMetaData.fields[i].name.data),
                                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                        vAttr, NULL, &newNodeId);

			else if ((i==2) || (i==3))
                                retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, (UA_UInt32)i+1+80300),
                                                        timestampfolderId,
                                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                                        UA_QUALIFIEDNAME(namespaceIndex, (char *)dataSetReaderConfig.dataSetMetaData.fields[i].name.data),
                                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                        vAttr, NULL, &newNodeId);

                        else if ((i==4) || (i==5) || (i==6))
                                retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, (UA_UInt32)i+1+80300),
                                                        statusfolderId,
                                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                                        UA_QUALIFIEDNAME(namespaceIndex, (char *)dataSetReaderConfig.dataSetMetaData.fields[i].name.data),
                                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                        vAttr, NULL, &newNodeId);

                        else if ((i==7) || (i==8) || (i==9) || (i==10) || (i==11) || (i==12)|| (i==13))
                                retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, (UA_UInt32)i+1+80300),
                                                        statusinfofolderId,
                                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                                        UA_QUALIFIEDNAME(namespaceIndex, (char *)dataSetReaderConfig.dataSetMetaData.fields[i].name.data),
                                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                        vAttr, NULL, &newNodeId);


			else //if ((i>=14) || (i<=39))
			{
                                if (dataSetReaderConfig.dataSetMetaData.fields[i].builtInType == UA_NS0ID_FLOAT)
                                        UA_Variant_setScalar(&vAttr.value, &data_float, &UA_TYPES[UA_TYPES_FLOAT]);
                                else if (dataSetReaderConfig.dataSetMetaData.fields[i].builtInType == UA_NS0ID_INT16)
                                        UA_Variant_setScalar(&vAttr.value, &data_int, &UA_TYPES[UA_TYPES_INT16]);
                                else if (dataSetReaderConfig.dataSetMetaData.fields[i].builtInType == UA_NS0ID_STRING)
                                        UA_Variant_setScalar(&vAttr.value, &data_string, &UA_TYPES[UA_TYPES_STRING]);

				retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, (UA_UInt32)i+1+80300),
							diagnosticfolderId,
							UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                           		UA_QUALIFIEDNAME(namespaceIndex, (char *)dataSetReaderConfig.dataSetMetaData.fields[i].name.data),
                                           		UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                           		vAttr, NULL, &newNodeId);
			}

			if (retval == UA_STATUSCODE_GOOD)
     			{
        			//printf("SV_PubSub.c : addSubscribedVariables(): UA_Server_addVariableNode : success : %d, datatype is %s \n", i, vAttr.dataType);
				#ifdef DEBUG_MODE
        			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"SV_PubSub.c :addSubscribedVariables() : success %s", UA_StatusCode_name(retval));
				#endif
     			}
     			else
     			{
        			//printf("SV_PubSub.c : addSubscribedVariables(): UA_Server_addVariableNode : failure \n");
				#ifdef DEBUG_MODE
        			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"SV_PubSub.c : addSubscribedVariables() : failure %s", UA_StatusCode_name(retval));
				#endif
        			exit(EXIT_FAILURE);
     			}
			newSubscribedNodeId[i] = newNodeId;

		}// finished creating the new nodes
	// set flag to UA_FALSE after 1 round of execution
	firsttime_addSubscribedVariableFlag = UA_FALSE;
	}
	else
	{// firsttime_addSubScribedVariableFlag == UA_FALSE;
		//printf("SV_PubSub.c : addSubscribedVariables(): UA_Server_addVariableNode :In 'else firsttime_adSubscribedVariableFlag' section : %d %d \n",
		//			firsttime_addSubscribedVariableFlag, dataSetReaderConfig.dataSetMetaData.fieldsSize);


		// now update the value into the nodes array
		for (size_t i=0; i< dataSetReaderConfig.dataSetMetaData.fieldsSize; i++)
		{
			//printf("SV_PubSub.c 1671: addSubscribedVariables() switch section : (%d) %f\n", i, subscribedIgramPP);
			//break;
			// should be grabbing from the MQTT message instead of dataSetReaderConfig.dataSetMetaData.fields[i].properties->value;
			// variant_value = dataSetReaderConfig.dataSetMetaData.fields[i].properties->value;
			UA_Variant variant_float, variant_string, variant_int;
			//UA_String outputStr;
			int test;

			//UA_NodeId_toString(&newSubscribedNodeId[i], &outputStr); // deprecated

		//UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"SV_PubSub.c : BEFORE SWITCH %d ", i);//, NodeId is %d ", i); //, outputStr.data);
			/* Segmentation fault will ocurr if UAExpert is connected to the OPCUA Server*/
			switch (i)
			{
				case 0 : // 80300 Software Version-----------------General
                                        #ifdef DEBUG_MODE
					printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() SoftwareVersion: %s\n", i, subscribedSoftwareVersion.data);
					#endif
					//outputStr = UA_STRING(subscribedSoftwareVersion);
					UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedSoftwareVersion, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 1 : // 80301 Data Block Version
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Block Version : %s\n", i, subscribedDataBlockVersion.data);
					#endif
					//outputStr = UA_STRING(subscribedDataBlockVersion);
					UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedDataBlockVersion, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 2 : // 80302---------------------Timestamp
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Instrument Time : %s\n", i, subscribedInstrumentTime.data);
					#endif
                                        //outputStr = UA_STRING(subscribedInstrumentTime);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedInstrumentTime, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
					break;

                                case 3 : // 80303
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Measurement Time : %s\n", i, subscribedMeasurementTime.data);
					#endif
                                        //outputStr = UA_STRING(subscribedMeasurementTime);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedMeasurementTime, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;


                                case 4 : // 80304--------------------Status
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Sensor : %s\n", i, subscribedSensor.data);
					#endif
                                        //outputStr = UA_STRING(subscribedSensor);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedSensor, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

				case 5 : // 80305
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() ******Operating Time : %d\n", i, subscribedOperatingTime);
					#endif
                                        //outputStr = UA_STRING(subscribedOperatingTime);
                                        UA_Variant_setScalar(&variant_int, &subscribedOperatingTime, &UA_TYPES[UA_TYPES_INT16]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_int);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 6 : // 80306
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Warning Message : %s\n", i, subscribedWarningMessage.data);
					#endif
                                        //outputStr = UA_STRING(subscribedWarningMessage);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedWarningMessage, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 7 : // 80307---------------------Status(info)
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Boot Status : %s\n", i, subscribedBootStatus.data);
					#endif
                                        //outputStr = UA_STRING(subscribedBootStatus);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedBootStatus, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 8 : // 80308
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Snapshot Status : %s\n", i, subscribedSnapshotStatus.data);
					#endif
                                        //outputStr = UA_STRING(subscribedSnapshotStatus);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedSnapshotStatus, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 9 : // 80309
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() SCP Status : %s\n", i, subscribedSCPStatus.data);
					#endif
                                        //outputStr = UA_STRING(subscribedSCPStatus);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedSCPStatus, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 10 : // 80310
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() SFTP Status : %s\n", i, subscribedSFTPStatus.data);
					#endif
                                        //outputStr = UA_STRING(subscribedSFTPStatus);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedSFTPStatus, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 11 : // 80311
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Runscript Status : %s\n", i, subscribedRunScriptStatus.data);
					#endif
                                        //outputStr = UA_STRING(subscribedRunScriptStatus);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedRunScriptStatus, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 12 : // 80312
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Archive Status : %s\n", i, subscribedArchiveStatus.data);
                                        //outputStr = UA_STRING(subscribedArchiveStatus);
					#endif
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedArchiveStatus, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 13 : // 80313
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Ancillary Sensor Status : %s\n", i, subscribedAncillarySensorStatus.data);
					#endif
                                        //outputStr = UA_STRING(subscribedAncillarySensorStatus);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedAncillarySensorStatus, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

				case 14 : // 80314 IgramPP-------------------------Diagnostics
                                        #ifdef DEBUG_MODE
					printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() IgramPP : %f\n", i, subscribedIgramPP);
					#endif

					UA_Variant_setScalar(&variant_float, &subscribedIgramPP, &UA_TYPES[UA_TYPES_FLOAT]);
					UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is VARIANT

                        		//UA_FieldTargetDataType_init(&targetVars[i].targetVariable);
                        		//targetVars[i].targetVariable.attributeId = UA_ATTRIBUTEID_VALUE;
                        		//targetVars[i].targetVariable.targetNodeId = newSubscribedNodeId[i];

					// update OPCUA address space
					// data = subscribedIgramPP;
					//UA_Variant_setScalar(&varData,&data, &UA_TYPES[UA_TYPES_FLOAT]);
					//UA_Server_writeValue(uaServer, targetVars[i].targetVariable.targetNodeId, variant_float);

					break;

				case 15: // 80315 IgramDC
                                        #ifdef DEBUG_MODE
					printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() IgramDC : %f \n", i, subscribedIgramDC);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedIgramDC, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 16: // 80316 LaserPP
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() LaserPP : %f \n", i, subscribedLaserPP);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedLaserPP, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 17: // 80317 LaserDC
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() LaserDC : %f \n", i, subscribedLaserDC);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedLaserDC, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 18: // 80318 SingleBeamAt900
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() SingleBeamAt900 : %f \n", i, subscribedSingleBeamAt900);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedSingleBeamAt900, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 19: // 80319 SingleBeamAt2500
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() SingleBeamAt2500 : %f \n", i, subscribedSingleBeamAt2500);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedSingleBeamAt2500, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 20: // 80320 SignalToNoiseAt2500
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() ******SignalToNoiseAt2500 : %d \n", i, subscribedSignalToNoiseAt2500);
					#endif

                                        UA_Variant_setScalar(&variant_int, &subscribedSignalToNoiseAt2500, &UA_TYPES[UA_TYPES_INT16]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_int);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 21: // 80321 CenterBurstLocation
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() CenterBurstLocation : %f \n", i, subscribedCenterBurstLocation);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedCenterBurstLocation, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 22: // 80322 DetectorTemp
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() DetectorTemp : %f \n", i, subscribedDetectorTemp);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedDetectorTemp, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 23: // 80323 LaserFrequency
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Laser Frequency : %f \n", i, subscribedLaserFrequency);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedLaserFrequency, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 24: // 80324 HardDriveSpace
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() ******Hard Drive Space : %d \n", i, subscribedHardDriveSpace);
					#endif

                                        UA_Variant_setScalar(&variant_int, &subscribedHardDriveSpace, &UA_TYPES[UA_TYPES_INT16]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_int);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 25: // 80325 Flow
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() ******Flow : %d \n", i, subscribedFlow);
					#endif

                                        UA_Variant_setScalar(&variant_int, &subscribedFlow, &UA_TYPES[UA_TYPES_INT16]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_int);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 26: // 80326 Temperature
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() ******Temperature : %d \n", i, subscribedTemperature);
					#endif

                                        UA_Variant_setScalar(&variant_int, &subscribedTemperature, &UA_TYPES[UA_TYPES_INT16]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_int);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 27: // 80327 Pressure
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Pressure : %f \n", i, subscribedPressure);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedPressure, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 28: // 80328 TempOptics
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() ******Temp Optics : %d \n", i, subscribedTempOptics);
					#endif

                                        UA_Variant_setScalar(&variant_int, &subscribedTempOptics, &UA_TYPES[UA_TYPES_INT16]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_int);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 29: // 80329 BadScanCounter
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() ******BadScan Counter : %d \n", i, subscribedBadScanCounter);
					#endif

                                        UA_Variant_setScalar(&variant_int, &subscribedBadScanCounter, &UA_TYPES[UA_TYPES_INT16]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_int);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 30: // 80330 FreeMemorySpace
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() ******Free Memory Space : %d \n", i, subscribedFreeMemorySpace);
					#endif

                                        UA_Variant_setScalar(&variant_int, &subscribedFreeMemorySpace, &UA_TYPES[UA_TYPES_INT16]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_int);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 31: // 80331
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1687: (%d) addSubscribedVariables() LAB Filename : %s\n", i, subscribedLABFilename.data);
					#endif
                                        //outputStr = UA_STRING(subscribedLABFilename);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedLABFilename, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 32: // 80332
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1687: (%d) addSubscribedVariables() LOG Filename : %s\n", i, subscribedLOGFilename.data);
                                        //outputStr = UA_STRING(subscribedLOGFilename);
					#endif
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedLOGFilename, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 33: // 80333
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1687: (%d) addSubscribedVariables() Lg Filename : %s\n", i, subscribedLgFilename.data);
					#endif
                                        //outputStr = UA_STRING(subscribedLgFilename);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedLgFilename, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 34: // 80334
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1687: (%d) addSubscribedVariables() Second Lg Filename : %s\n", i, subscribedSecondLgFilename.data);
					#endif
                                        //outputStr = UA_STRING(subscribedSecondLgFilename);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedSecondLgFilename, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;


                                case 35: // 80335 SystemCounter
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() System Counter : %f \n", i, subscribedSystemCounter);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedSystemCounter, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 36: // 80336 DetectorCounter
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Detector Counter : %f \n", i, subscribedDetectorCounter);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedDetectorCounter, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 37: // 80337 LaserCounter
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Laser Counter : %f \n", i, subscribedLaserCounter);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedLaserCounter, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 38: // 80338 FlowPumpCounter
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Flow Pump Counter : %f \n", i, subscribedFlowPumpCounter);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedFlowPumpCounter, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 39: // 80339 DesiccantCounter
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Desiccant Counter : %f \n", i, subscribedDesiccantCounter);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedDesiccantCounter, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

				// -----------Data (KIV)

			}

                }

		#ifdef DEBUG_MODE
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : after addTargetVariable (subscriber) loop");
		#endif

		/* no need, since the OPCUA Address are updated with new nodes in the loop above.

                retval = UA_Server_DataSetReader_createTargetVariables(uaServer, dataSetReaderId,
								dataSetReaderConfig.dataSetMetaData.fieldsSize,
								targetVars);
		for (size_t i=0; i< dataSetReaderConfig.dataSetMetaData.fieldsSize; i++)
		{
			if (&targetVars[0].targetVariable)
	                	UA_FieldTargetDataType_clear(&targetVars[0].targetVariable);
		}
		*/

		/*
                if (targetVars)
			UA_free(targetVars);
		if (dataSetReaderConfig.dataSetMetaData.fields)
                	UA_free(dataSetReaderConfig.dataSetMetaData.fields);
		*/
		return retval;
	}
}


/* Periodically refreshes the MQTT stack (sending/receiving) */
//https://github.com/open62541/open62541/blob/master/examples/pubsub/server_pubsub_subscriber_rt_level.c
static void
mqttYieldPollingCallback(UA_Server *uaServer, UA_PubSubConnection *connection)
{
//    connection->channel->yield(connection->channel, (UA_UInt16) 30);	// timeout : 30 seconds
    addSubscribedVariables(uaServer, readerIdentifier);	// repeated update the UA AddressSpace

}


static void
callback(UA_ByteString *encodedBuffer, UA_ByteString *topic)
{
	int i;
     #ifdef DEBUG_MODE
     UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub: Start callback()=======");
     UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "SV_PubSub.c callback() : Message Received in MQTT Broker : %s ", (char*)topic->data);
     #endif

     //printf("before processing: <%s>\n", (char *)encodedBuffer->data);
     // remove unwanted characters in reverse order
     for (i=strlen((char*)encodedBuffer->data); i>0; i--)
     {
	//printf("value of i is %d \n", i);
	if ((char)encodedBuffer->data[i] == '}') break;
     }
     encodedBuffer->data[i+1] ='\0';
     #ifdef DEBUG_MODE
     UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "after processing: \n%s\n", (char *)encodedBuffer->data);
     UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : before new_JSON_checker(20)");
     #endif

     JSON_checker jc = new_JSON_checker(20);	// depth of tree = 20

     #ifdef DEBUG_MODE
     UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : after new_JSON_checker(20)");
     #endif

     if (jc)
     {
     	for (i=0; i < strlen((char*)encodedBuffer->data); i++)
	{
        	int next_char = encodedBuffer->data[i];//getchar();
        	if (next_char <= 0)
            		break;
        	if (!JSON_checker_char(jc, next_char))
		{
            		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : JSON_checker_char(): syntax error");
			JSON_checker_done(jc);	//The JSON_checker object will be deleted by JSON_checker_done.
            		return;
		}
        }
     }
     UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"After new_JSON_checker");

     if (jc)
     {
     	if (!JSON_checker_done(jc))
	{
        	UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : JSON_checker_done(): syntax error\n");
        	return;
     	}
     }
     UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "SV_PubSub.c : callback : JSON_Checker() success");

#ifdef JSON5
	char myCharString[32767];
	json5_object *myjson5_obj = malloc(sizeof(myjson5_obj));

        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "SV_PubSub.c : callback : IN JSON5 section");
	int stringlen = strlen((char*)encodedBuffer->data);
	if (stringlen > 0)
	{
                strncpy(myCharString, (char*)encodedBuffer->data, stringlen);
                myCharString[stringlen]='\0';

		printf("In stringlen > 0 section \n");
		printf("========================\n");
		printf("strlen = %d \n", stringlen);
		printf("string contents is %s \n", myCharString);
		printf("========================\n");

		json5_error err_code = json5_parse(myjson5_obj, myCharString);
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "SV_PubSub.c : json5_object : %d", err_code);

		//sleep(2000);
		free(myjson5_obj);
		return;
	}
#else
     UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "SV_PubSub.c : callback : NOT IN JSON5 section");

     //struct json_object *json_obj;
     struct json_object *value, *valueMessages, *valueEx, *valueBody;
     struct json_object *metadataversion_obj;

     struct json_object *jobj = NULL;
     int stringlen = 0;
     enum json_tokener_error jerr;
     int exists;
     char myCharString[32767];

     UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : Before json_tokener_new()");
     json_tokener *tok= json_tokener_new();
     UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : After json_tokener_new()");

     if (tok == NULL)
     {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Fatal error : Cannot allocate memory for json_tokener_new()");
                return;
     }


	// option 1
    	do
    	{
    		stringlen = strlen((char*)encodedBuffer->data);
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub here 0 : encodedBuffer->data size is %d \n", stringlen);
		//++stringlen;

		if (stringlen > 0)	// jacky added 'if statement' on 3 Apr 2021
		{
			// convert UA_ByteString <encodedBuffer->data> to char*
			//char* myCharString = calloc(stringlen+1, sizeof(char));	// line 2461
			strncpy(myCharString, (char*)encodedBuffer->data, stringlen);
			myCharString[stringlen+1]='\0';


			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : before json_tokener_parse_ex section");
			jobj = json_tokener_parse_ex(tok, myCharString, stringlen);/**** after 10 rounds it crash here ****/

			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub here : trying to print \n");
			if (jobj==NULL)
			{
				if (jerr != json_tokener_continue)
				{
					json_tokener_reset(tok);
					UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Fatal error : Token parsing fail");
					return;
				}
			}
		}
		else
		{
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : *********************** in stringlen <= 0 section");
			goto FreeMemory;
			//return;	// somehow the encodedBuffer->data is empty
		}

		jerr = json_tokener_get_error(tok);
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : here 0.5 : jerror = %d", jerr);

	}
    	while (jerr == json_tokener_continue); // ((jerr = json_tokener_get_error(tok)) == json_tokener_continue);

//goto FreeMemory;	// for testing only
	#ifdef DEBUG
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub here 1");
	#endif
    	if (jerr != json_tokener_success)
	{
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : Error : %s ", json_tokener_error_desc(jerr));
		if (tok != NULL) json_tokener_free(tok);
			return;
	}

	if (json_tokener_get_parse_end(tok) < stringlen)
	{
		goto FreeMemory;
        // Handle extra characters after parsed object as desired.
        // e.g. issue an error, parse another object from that point, etc...
	}
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : callback() ::  Option 1: json_tokener_success");

     	// repeated : option 2 => does not work
     	/*
     	jobj = json_tokener_parse_ex(tok, (char *)encodedBuffer->data, stringlen );
     	if (jobj==NULL){
		if (tok != NULL) json_tokener_free(tok);
		return;
		}
     	printf("Option 2: Received JSON in string format :\n%s \n", json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY)); // json_object_get_string(jobj)); 
     	*/

     // next search for the key "Messages", the return 'contents' are stored in 'value'
     exists = UA_FALSE;
     exists = json_object_object_get_ex(jobj, "MessageId", &value); //struct json_object **
     if (exists==UA_FALSE)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : Warning : Key <MessageId> cannot be found in JSON");
     else
     	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Success : Key <MessageId : %s> found in JSON", json_object_get_string(value));
     json_object_object_del(jobj, "MessageId");

     // next search for the key "MessageType", the return 'contents' are stored in 'value'
     exists = UA_FALSE;
     exists = json_object_object_get_ex(jobj, "MessageType", &value); //struct json_object **
     if (exists==UA_FALSE)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : Warning : Key <MessageType> cannot be found in JSON");
     else
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Success : Key <MessageType : %s> found in JSON", json_object_get_string(value));
     json_object_object_del(jobj, "MessageType");

/* example taken from website
struct json_object_iterator it;
struct json_object_iterator itEnd;

it = json_object_iter_begin(jobj);
itEnd = json_object_iter_end(jobj);
while (!json_object_iter_equal(&it, &itEnd)) {
    printf("%s : %s \n",
           json_object_iter_peek_name(&it),
	   json_object_get_string(json_object_iter_peek_value(&it)) );
    json_object_iter_next(&it);
}
*/

     // next search for the key "Messages", the return 'contents' are in stored in 'valueMessages'
     exists = UA_FALSE;
     exists = json_object_object_get_ex(jobj, "Messages", &valueMessages); //struct json_object **
     if (exists==UA_FALSE) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : Warning : Key <Messages> cannot be found in JSON");
	//if (tok) json_tokener_free(tok);
	//json_object_object_del(jobj, "Messages");
        }
	else UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Success : Key <Messages> found in JSON");
     // get the string from the value of the key "Message"
     struct array_list *myArrayList;
     myArrayList = json_object_get_array(valueMessages); //struct json_object *
//     printf("Length of Array list returned :  array_list_length(length) : %d \n", array_list_length(myArrayList));//json_object_array_length(valueMessages)); //json_object_to_json_string(value));
//     printf("Length of Array list returned :  myArrayList->length : %d \n", myArrayList->length);//json_object_array_length(valueMessages)); //json_object_to_json_string(value));
//     printf("Size of Array list returned : myArrayList->size : %d \n", myArrayList->size);
//     printf("Contents of Key-Messages (Array) returned : %s \n", json_object_to_json_string_ext(valueMessages, JSON_C_TO_STRING_PRETTY));	// returns an arraylist of json-objects

    // https://json-c.github.io/json-c;
    // install in /usr/local/include/

//     printf("First element of the array : %s \n", json_object_to_json_string_ext(json_object_array_get_idx(valueMessages, 0), JSON_C_TO_STRING_PRETTY));	// 
     // OUTPUT : Messages is a big array that starts with a '[' and ends with a ']'
     // First element JSON Object :
     /*	{
    	"DataSetWriterId":2234,
    	"SequenceNumber":0,
    	"MetaDataVersion":{
      	   "MajorVersion":2959993276,
      	   "MinorVersion":2959985426
    	   },
        "Timestamp":2021-02-15T13:57:54.224296Z",
        "Payload":{
           "IgramPP":{
           "Type":10,
           "Body":0
           },
           ....
	"DesiccantCounter":{
           "Type":10,
           "Body":0
           } // DesiccantCounter
          } // Payload
        }
     */

	// create a search object with the search key embeded
	//json_object_array_bsearch(

     // next search for the key "DataSetWriterId", the return 'contents' are stored in 'value'
     exists = UA_FALSE;
     exists = json_object_object_get_ex(json_object_array_get_idx(valueMessages, 0), "DataSetWriterId", &value); //struct json_object **
     if (exists==UA_FALSE)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : Warning : Key <DataSetWriterId> cannot be found in JSON");
     else
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Success : Key <DataSetWriterId : %d> found in JSON", json_object_get_int(value));
     json_object_object_del(jobj, "DataSetWriterId");

     exists = UA_FALSE;
     exists = json_object_object_get_ex(json_object_array_get_idx(valueMessages, 0), "SequenceNumber", &value); //struct json_object **
     if (exists==UA_FALSE)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : Warning : Key <SequenceNumber> cannot be found in JSON");
     else
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Success : Key <SequenceNumber : %d> found in JSON", json_object_get_int(value));
     json_object_object_del(jobj, "SequenceNumber");

     exists = UA_FALSE;
     exists = json_object_object_get_ex(json_object_array_get_idx(valueMessages, 0), "MetaDataVersion", &metadataversion_obj); //struct json_object **	// cannot get the content!
     if (exists==UA_FALSE)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : Warning : Key <MetaDataVersion> cannot be found in JSON");
     else
     {
	char MajorVersion[255], MinorVersion[255];
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Success : Key <MetaDataVersion> found in JSON");	// returns an empty string
	exists = UA_FALSE;
	exists = json_object_object_get_ex(metadataversion_obj, "MajorVersion", &valueEx);
	itoa(json_object_get_int(valueEx), MajorVersion, 10);
     	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<MajorVersion> json_object_get_int(valueEx) returned 	: %d", json_object_get_int(valueEx));
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<MajorVersion> itoa returned 				: %s", MajorVersion);
     	//UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<MajorVersion> returned : %s", itoa(json_object_get_int(valueEx)));
	exists = UA_FALSE;
	exists = json_object_object_get_ex(metadataversion_obj, "MinorVersion", &valueEx);
	itoa(json_object_get_int(valueEx), MinorVersion, 10);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<MinorVersion> json_object_get_int(valueEx) returned 	: %d", json_object_get_int(valueEx));
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<MinorVersion> itoa returned				: %s", MinorVersion);
	//UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<MinorVersion> returned : %s", itoa(json_object_get_int(valueEx)));
     	json_object_object_del(jobj, "MetaDataVersion"); json_object_object_del(jobj, "MajorVersion"); json_object_object_del(jobj, "MinorVersion");
     }

     exists = UA_FALSE;
     exists = json_object_object_get_ex(json_object_array_get_idx(valueMessages, 0), "Timestamp", &value); //struct json_object **
     if (exists==UA_FALSE)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : Key <Timestamp> cannot be found in JSON");
     else
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Success : Key <Timestamp : %s> found in JSON", json_object_get_string(value));
     json_object_object_del(jobj, "Timestamp");

     // next search for the key "Payload", the return 'contents' are stored in 'value'
     exists= UA_FALSE;
     exists = json_object_object_get_ex(json_object_array_get_idx(valueMessages,0), "Payload", &value); //struct json_object **
     if (exists==UA_FALSE)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : Warning : Key <Payload> cannot be found in JSON");
     else
     {
        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "SoftwareVersion", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
	if (json_object_get_string(valueBody) != NULL)
	{
        	//strcpy(subscribedSoftwareVersion, json_object_get_string(valueBody));
		subscribedSoftwareVersion = UA_STRING((char*)json_object_get_string(valueBody));
	}
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<SoftwareVersion:Body> returned : %s", subscribedSoftwareVersion.data );  // json_object_get_string(valueBody));
        json_object_object_del(jobj, "SoftwareVersion");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "DataBlockVersion", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
	if (json_object_get_string(valueBody) != NULL)
	{
        	//strcpy(subscribedDataBlockVersion, json_object_get_string(valueBody));
		subscribedDataBlockVersion = UA_STRING((char*)json_object_get_string(valueBody));
	}
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<BlockVersion:Body> returned : %s", subscribedDataBlockVersion.data );  // json_object_get_string(valueBody));
        json_object_object_del(jobj, "BlockVersion");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "InstrumentTime", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
        if (json_object_get_string(valueBody) != NULL)
	{
                //strcpy(subscribedInstrumentTime, json_object_get_string(valueBody));
		subscribedInstrumentTime = UA_STRING((char*)json_object_get_string(valueBody));
	}
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<InstrumentTime:Body> returned : %s", subscribedInstrumentTime.data );  // json_object_get_string(valu$
        json_object_object_del(jobj, "InstrumentTime");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "MeasurementTime", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
        if (json_object_get_string(valueBody) != NULL)
	{
                //strcpy(subscribedMeasurementTime, json_object_get_string(valueBody));
		subscribedMeasurementTime = UA_STRING((char*)json_object_get_string(valueBody));
	}
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<MeasurementTime:Body> returned : %s", subscribedMeasurementTime.data );  // json_object_get_string($
        json_object_object_del(jobj, "MeasurementTime");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "Sensor", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
        if (json_object_get_string(valueBody) != NULL)
	{
                //strcpy(subscribedSensor, json_object_get_string(valueBody));
		subscribedSensor = UA_STRING((char*)json_object_get_string(valueBody));
	}
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<Sensor:Body> returned : %s", subscribedSensor.data );  // json_object_get_string($
        json_object_object_del(jobj, "Sensor");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "OperatingTime", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
		subscribedOperatingTime = json_object_get_int(value);
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "************<OperatingTime:Body> returned : %d", subscribedOperatingTime );  // json_object_get_string($
        json_object_object_del(jobj, "OperatingTime");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "WarningMessage", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
        if (json_object_get_string(valueBody) != NULL)
	{
                //strcpy(subscribedWarningMessage, json_object_get_string(valueBody));
		subscribedWarningMessage = UA_STRING((char*)json_object_get_string(valueBody));
	}
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<WarningMessage:Body> returned : %s", subscribedWarningMessage.data );  // json_object_get_string($
        json_object_object_del(jobj, "WarningMessage");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "BootStatus", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
        if (json_object_get_string(valueBody) != NULL)
	{
                //strcpy(subscribedBootStatus, json_object_get_string(valueBody));
		subscribedBootStatus = UA_STRING((char*)json_object_get_string(valueBody));
	}
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<BootStatus:Body> returned : %s", subscribedBootStatus.data );  // json_object_get_string($
        json_object_object_del(jobj, "BootStatus");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "SnapshotStatus", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
        if (json_object_get_string(valueBody) != NULL)
	{
                //strcpy(subscribedSnapshotStatus, json_object_get_string(valueBody));
		subscribedSnapshotStatus = UA_STRING((char*)json_object_get_string(valueBody));
	}
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<SnapshotStatus:Body> returned : %s", subscribedSnapshotStatus.data );  // json_object_get_string($
        json_object_object_del(jobj, "SnapshotStatus");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "SCPStatus", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
        if (json_object_get_string(valueBody) != NULL)
	{
                //strcpy(subscribedSCPStatus, json_object_get_string(valueBody));
		subscribedSCPStatus = UA_STRING((char*)json_object_get_string(valueBody));
	}
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<SCPStatus:Body> returned : %s", subscribedSCPStatus.data );  // json_object_get_string($
        json_object_object_del(jobj, "SCPStatus");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "SFTPStatus", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
        if (json_object_get_string(valueBody) != NULL)
	{
                //strcpy(subscribedSFTPStatus, json_object_get_string(valueBody));
		subscribedSFTPStatus = UA_STRING((char*)json_object_get_string(valueBody));
	}
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<SFTPStatus:Body> returned : %s", subscribedSFTPStatus.data );  // json_object_get_string($
        json_object_object_del(jobj, "SFTPStatus");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "RunscriptStatus", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
        if (json_object_get_string(valueBody) != NULL)
	{
                //strcpy(subscribedRunScriptStatus, json_object_get_string(valueBody));
		subscribedRunScriptStatus = UA_STRING((char*)json_object_get_string(valueBody));
	}
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<RunscriptStatus:Body> returned : %s", subscribedRunScriptStatus.data );  // json_object_get_string($
        json_object_object_del(jobj, "RunscriptStatus");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "ArchiveStatus", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
        if (json_object_get_string(valueBody) != NULL)
	{
                //strcpy(subscribedArchiveStatus, json_object_get_string(valueBody));
		subscribedArchiveStatus = UA_STRING((char*)json_object_get_string(valueBody));
	}
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<ArchiveStatus:Body> returned : %s", subscribedArchiveStatus.data );  // json_object_get_string($
        json_object_object_del(jobj, "ArchiveStatus");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "AncillarySensorStatus", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
        if (json_object_get_string(valueBody) != NULL)
	{
                //strcpy(subscribedAncillarySensorStatus, json_object_get_string(valueBody));
		subscribedAncillarySensorStatus = UA_STRING((char*)json_object_get_string(valueBody));
	}
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<AncillarySensorStatus:Body> returned : %s", subscribedAncillarySensorStatus.data );  // json_object_get_string($
        json_object_object_del(jobj, "AncillarySensorStatus");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
     	exists = json_object_object_get_ex(value, "IgramPP", &valueEx);
	exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
		subscribedIgramPP = json_object_get_double(valueBody);
//     	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<IgramPP:Body> returned : %f", subscribedIgramPP );  // json_object_get_double(valueBody));
        json_object_object_del(jobj, "IgramPP");
	json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "IgramDC", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
		subscribedIgramDC = json_object_get_double(valueBody);
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<IgramDC:Body> returned : %f", subscribedIgramDC); // json_object_get_double(valueBody));
        json_object_object_del(jobj, "IgramDC");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "LaserPP", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
		subscribedLaserPP = json_object_get_double(valueBody);
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<LaserPP:Body> returned : %f", subscribedLaserPP); // json_object_get_double(valueBody));
        json_object_object_del(jobj, "LaserPP");
        json_object_object_del(jobj, "Body");

	exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "LaserDC", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
		subscribedLaserDC = json_object_get_double(valueBody);
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<LaserDC:Body> returned : %f", subscribedLaserDC); // json_object_get_double(valueBody));
        json_object_object_del(jobj, "LaserDC");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "SingleBeamAt900", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
		subscribedSingleBeamAt900 = json_object_get_double(valueBody);
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<SingleBeamAt900:Body> returned : %f", subscribedSingleBeamAt900); // json_object_get_double(valueBody));
        json_object_object_del(jobj, "SingleBeamAt900");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "SingleBeamAt2500", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
		subscribedSingleBeamAt2500 = json_object_get_double(valueBody);
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<SingleBeamAt2500:Body> returned : %f", subscribedSingleBeamAt2500); // json_object_get_double(valueBody));
        json_object_object_del(jobj, "SingleBeamAt2500");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "SignalToNoiseAt2500", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
		subscribedSignalToNoiseAt2500 = json_object_get_int(valueBody);
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "************<SignalToNoiseAt2500:Body> returned : %d", subscribedSignalToNoiseAt2500); // json_object_get_int(valueBody));
        json_object_object_del(jobj, "SignalToNoiseAt2500");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "CenterBurstLocation", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
		subscribedCenterBurstLocation = json_object_get_double(valueBody);
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<CenterBurstLocation:Body> returned : %f", subscribedCenterBurstLocation); // json_object_get_double(valueBody));
        json_object_object_del(jobj, "CenterBurstLocation");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "DetectorTemperature", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
		subscribedDetectorTemp = json_object_get_double(valueBody);
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<DetectorTemperature:Body> returned : %f", subscribedDetectorTemp); // json_object_get_double(valueBody));
        json_object_object_del(jobj, "DetectorTemperature");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "LaserFrequency", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
		subscribedLaserFrequency = json_object_get_double(valueBody);
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<LaserFrequency:Body> returned : %f", subscribedLaserFrequency); //json_object_get_double(valueBody));
        json_object_object_del(jobj, "LaserFrequency");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "HardDriveSpace", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
		subscribedHardDriveSpace = json_object_get_int(valueBody);
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "************<HardDriveSpace:Body> returned : %d", subscribedHardDriveSpace); // json_object_get_int(valueBody));
        json_object_object_del(jobj, "HardDriveSpace");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "Flow", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
		subscribedFlow = json_object_get_int(valueBody);
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "************<Flow:Body> returned : %d", subscribedFlow); // json_object_get_double(valueBody));
        json_object_object_del(jobj, "Flow");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "Temperature", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
		subscribedTemperature = json_object_get_int(valueBody);
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "************<Temperature:Body> returned : %d", subscribedTemperature); // json_object_get_double(valueBody));
        json_object_object_del(jobj, "Temperature");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "Pressure", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
		subscribedPressure = json_object_get_double(valueBody);
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<Pressure:Body> returned : %f", subscribedPressure); // json_object_get_double(valueBody));
        json_object_object_del(jobj, "Pressure");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "TempOptics", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
		subscribedTempOptics = json_object_get_int(valueBody);
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "************<TempOptics:Body> returned : %d", subscribedTempOptics); // json_object_get_double(valueBody));
        json_object_object_del(jobj, "TempOptics");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "BadScanCounter", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
		subscribedBadScanCounter = json_object_get_int(valueBody);
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "************<BadScanCounter:Body> returned : %d", subscribedBadScanCounter); // json_object_get_int(valueBody));
        json_object_object_del(jobj, "BadScanCounter");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "FreeMemorySpace", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
		subscribedFreeMemorySpace = json_object_get_int(valueBody);
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "************<FreeMemorySpace:Body> returned : %d", subscribedFreeMemorySpace); // json_object_get_int(valueBody));
        json_object_object_del(jobj, "FreeMemorySpace");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "LABFilename", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
	if (json_object_get_string(valueBody) != NULL)
	{
		//strcpy(subscribedLABFilename, json_object_get_string(valueBody));	// somehow strcpy statement crashed due to unallocated memory
		subscribedLABFilename = UA_STRING((char*)json_object_get_string(valueBody));
	}
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<LABFilename:Body> returned : %s", subscribedLABFilename.data); // json_object_get_string(valueBody));
        json_object_object_del(jobj, "LABFilename");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "LOGFilename", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
	if (json_object_get_string(valueBody) != NULL)
	{
		//strcpy(subscribedLOGFilename, json_object_get_string(valueBody));	// somehow strcpy statement crashed due to unallocated memory
		subscribedLOGFilename = UA_STRING((char*)json_object_get_string(valueBody));
	}
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<LOGFilename:Body> returned : %s", subscribedLOGFilename.data); // json_object_get_string(valueBody));
        json_object_object_del(jobj, "LOGFilename");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "LgFilename", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
	if (json_object_get_string(valueBody) != NULL)
	{
		//strcpy(subscribedLgFilename, json_object_get_string(valueBody));	// somehow strcpy statement crashed due to unallocated memory
		subscribedLgFilename = UA_STRING((char*)json_object_get_string(valueBody));
	}
//        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<LgFilename:Body> returned : %s", subscribedLgFilename.data); // json_object_get_string(valueBody));
        json_object_object_del(jobj, "LgFilename");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "SecondLgFilename", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
	if (json_object_get_string(valueBody) != NULL)
	{
		//strcpy(subscribedSecondLgFilename, json_object_get_string(valueBody));	// somehow strcpy statement crashed due to unallocated memory
		subscribedSecondLgFilename = UA_STRING((char*)json_object_get_string(valueBody));
	}
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<SecondLgFilename:Body> returned : %s", subscribedSecondLgFilename.data); //json_object_get_string(valueBody));
        json_object_object_del(jobj, "SecondLgFilename");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "DetectorCounter", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
       		subscribedDetectorCounter = json_object_get_double(valueBody);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<DetectorCounter:Body> returned : %f", subscribedDetectorCounter); // json_object_get_double(valueBody));
        json_object_object_del(jobj, "DetectorCounter");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "LaserCounter", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
        	subscribedLaserCounter = json_object_get_double(valueBody);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<LaserCounter:Body> returned : %f", subscribedLaserCounter); //json_object_get_double(valueBody));
	json_object_object_del(jobj, "LaserCounter");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "FlowPumpCounter", &valueEx);
        exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
		subscribedFlowPumpCounter = json_object_get_double(valueBody);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<FlowPumpCounter:Body> returned : %f", subscribedFlowPumpCounter); //json_object_get_double(valueBody));
        json_object_object_del(jobj, "FlowPumpCounter");
        json_object_object_del(jobj, "Body");

        exists = UA_FALSE;
        exists = json_object_object_get_ex(value, "DesiccantCounter", &valueEx);
	exists = json_object_object_get_ex(valueEx,"Body", &valueBody);
		subscribedDesiccantCounter = json_object_get_double(valueBody);
	/*
	if (json_object_get_int(valueBody) != NULL)
	{
		int dataType = json_object_get_int(valueBody);
		if (dataType==10) // double
		{
			exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
			if (json_object_get_double(valueBody) != NULL)
        			subscribedDesiccantCounter = json_object_get_double(valueBody);
		}
 		else if (dataType==4) // int
		{
			exists = json_object_object_get_ex(valueEx, "Body", &valueBody);
			if (json_object_get_int(valueBody) != NULL)
				subscribedDesiccantCounter = json_object_get_int(valueBody);
		}
		// else : (dataType=12) fatal error : incorrect type
	}
	*/

        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "<DesiccantCounter:Body> returned : %f", subscribedDesiccantCounter); // json_object_get_double(valueBody));
	json_object_object_del(jobj, "DessiccantCounter");
	//json_object_object_del(jobj, "Type");
        json_object_object_del(jobj, "Body");

     	//json_object_object_del(jobj, "Payload");
     }

FreeMemory:

	// free the memory
	if (tok != NULL) json_tokener_free(tok);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : callback() :: after json_tokener_free(tok)");

     	// DO NOT FREE encodedBuffer : encodedBuffer is passed in by reference
	//if (encodedBuffer != NULL) UA_ByteString_delete(encodedBuffer);

	if (topic != NULL) UA_ByteString_delete(topic);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : callback() :: after UA_ByteString_delete(topic)");

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : callback() :: 1");
	if (jobj) json_object_put(jobj);	// frees the jobj tree recursively
//printf("SV_PubSub.c : callback() :: 2 \n");
//	if (json_obj) json_object_put(json_obj);
//printf("SV_PubSub.c : callback() :: 3 \n");
//     	if (value) json_object_put(value);	// crash here
//printf("SV_PubSub.c : callback() :: 4 \n");
//     	if (valueMessages) json_object_put(valueMessages);
//printf("SV_PubSub.c : callback() :: 5 \n");
//     	if (valueEx) json_object_put(valueEx);
//printf("SV_PubSub.c : callback() :: 6 \n");
//     	if (valueBody) json_object_put(valueBody);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : callback() :: 7");
     	if (metadataversion_obj) json_object_put(metadataversion_obj);

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Leaving SV_PubSub: callback()");

#endif

}


/* Adds a subscription */
//https://github.com/open62541/open62541/blob/master/examples/pubsub/server_pubsub_subscriber_rt_level.c
static void addSubscription(UA_Server *server, UA_PubSubConnection *connection)
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : addSubscription() function");	// UA_PubSubConnection defined in open6541.c : 7013

    // set the PublisherId : this is the id that the subscriber will look for in the broker
    connection->channel->publisherId = PUBLISHERID;
    // set the address
    //UA_String brokerAddress = UA_STRING(BROKER_ADDRESS_URL_MQTT);	// jacky discovered error here : hardcoded broker address 192.168.1.119 :: WRONG!
    UA_String brokerAddress = UA_STRING(connection->config->address.data);
    UA_Variant_setArray(&connection->channel->connectionConfig->address, &brokerAddress, 			// STOP HERE => affects line 1443
			sizeof(brokerAddress), &UA_TYPES[UA_TYPES_STRING]);

    //Register Transport settings
    UA_BrokerWriterGroupTransportDataType brokerTransportSettings;
    memset(&brokerTransportSettings, 0, sizeof(UA_BrokerWriterGroupTransportDataType));

    brokerTransportSettings.queueName = UA_STRING(SUBSCRIBER_TOPIC_MQTT);	// must match PUBLISHER_TOPIC_MQTT
    brokerTransportSettings.resourceUri = UA_STRING_NULL;
    brokerTransportSettings.authenticationProfileUri = UA_STRING_NULL;

    /* QOS */
    brokerTransportSettings.requestedDeliveryGuarantee = UA_BROKERTRANSPORTQUALITYOFSERVICE_EXACTLYONCE; //UA_BROKERTRANSPORTQUALITYOFSERVICE_BESTEFFORT;

    // UA_ExtensionObject defined in open62541.h : 13640
    UA_ExtensionObject transportSettings;
    memset(&transportSettings, 0, sizeof(UA_ExtensionObject));
    transportSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
//    transportSettings.content.decoded.type = &UA_TYPES[UA_TYPES_BROKERWRITERGROUPTRANSPORTDATATYPE];
    transportSettings.content.decoded.type = &UA_TYPES[UA_TYPES_BROKERDATASETREADERTRANSPORTDATATYPE];	// match open62541.c:89311
    transportSettings.content.decoded.data = &brokerTransportSettings;


    // according to OPCFoundation part 14 the filter parameters are:
    // PublisherId
    // WriterGroupId
    // DataSetWriterId
    // DataSetClassId
    // Promoted Fields

    // inspect the contents of UA_PubSubConnection structure (defined in open62541.c:7013)

/*
typedef struct UA_PubSubConnection{
    UA_PubSubComponentEnumType componentType;
    UA_PubSubConnectionConfig *config;
    //internal fields
    UA_PubSubChannel *channel;
    UA_NodeId identifier;
    LIST_HEAD(UA_ListOfWriterGroup, UA_WriterGroup) writerGroups;
    size_t writerGroupsSize;
    LIST_HEAD(UA_ListOfPubSubReaderGroup, UA_ReaderGroup) readerGroups;
    size_t readerGroupsSize;
    TAILQ_ENTRY(UA_PubSubConnection) listEntry;
    UA_UInt16 configurationFreezeCounter;
    // This flag is 'read only' and is set internally based on the PubSub state.
    UA_Boolean configurationFrozen;
} UA_PubSubConnection;

struct UA_PubSubChannel {
    UA_UInt32 publisherId; // unique identifier
    UA_PubSubChannelState state;
    UA_PubSubConnectionConfig *connectionConfig; // link to parent connection config
    UA_SOCKET sockfd;
    void *handle; // implementation specific data
    //@info for handle: each network implementation should provide an structure
    // UA_PubSubChannelData[ImplementationName] This structure can be used by the
    // network implementation to store network implementation specific data.

    // Sending out the content of the buf parameter
    UA_StatusCode (*send)(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettings,
                          const UA_ByteString *buf);

    // Register to an specified message source, e.g. multicast group or topic. Callback is used for mqtt
    UA_StatusCode (*regist)(UA_PubSubChannel * channel, UA_ExtensionObject *transportSettings,
        void (*callback)(UA_ByteString *encodedBuffer, UA_ByteString *topic));

    // Remove subscription to an specified message source, e.g. multicast group or topic
    UA_StatusCode (*unregist)(UA_PubSubChannel * channel, UA_ExtensionObject *transportSettings);

    // Receive messages. A regist to the message source is needed before.
    UA_StatusCode (*receive)(UA_PubSubChannel * channel, UA_ByteString *,
                             UA_ExtensionObject *transportSettings, UA_UInt32 timeout);

    // Closing the connection and implicit free of the channel structures.
    UA_StatusCode (*close)(UA_PubSubChannel *channel);

    // Giving the connection protocoll time to process inbound and outbound traffic.
    UA_StatusCode (*yield)(UA_PubSubChannel *channel, UA_UInt16 timeout);
};


*/
	#ifdef DEBUG_MODE
    	printf("SV_PubSub.c (addSubscription) \n");
	printf("at line 1400 \n");
	printf("connection->channel->publisherId 				= %d\n", connection->channel->publisherId);	// this should be PUBLISHERID (2234)
	printf("connection->channel->state 					= %d\n", connection->channel->state); // UA_PUBSUB_CHANNEL_RDY, UA_PUBSUB_CHANNEL_PUB, UA_PUBSUB_CHANNEL_SUB, UA_PUBSUB_CHANNEL_PUB_SUB, UA_PUBSUB_CHANNEL_ERROR, UA_PUBSUB_CHANNEL_CLOSED
	printf("connection->channel->connectionConfig->name 			= <%s>\n", connection->channel->connectionConfig->name.data);	// defined in open62541.h:30229
	printf("connection->channel->connectionConfig->enabled 			= %d\n", connection->channel->connectionConfig->enabled);
	printf("connection->channel->connectionConfig->transportProfileUri	= %s\n", connection->channel->connectionConfig->transportProfileUri.data); // UA_String

	// convert UA_Variant to UA_String: should not be empty : 1443
	printf("connection->channel->connectionConfig->address 			= %s\n", connection->channel->connectionConfig->address.data);	// this should not be empty : UA_Variant
	printf("connection->channel->connectionConfig->connectionPropertiesSize	= %d\n", connection->channel->connectionConfig->connectionPropertiesSize);
	//sleep(5);

	#endif
/*
struct UA_PubSubConnectionConfig {
    UA_String name;
    UA_Boolean enabled;
    UA_PublisherIdType publisherIdType;
    union { // std: valid types UInt or String
        UA_UInt32 numeric;
        UA_String string;
    } publisherId;
    UA_String transportProfileUri;
    UA_Variant address;
    size_t connectionPropertiesSize;
    UA_KeyValuePair *connectionProperties;
    UA_Variant connectionTransportSettings;
};
*/
	// the following crash
	//UA_String output;
	//UA_NodeId_parse(&connection->identifier, output);
	//printf("connection->identifier = %s\n", output.data);	// should not be empty

	#ifdef DEBUG_MODE
	printf("connection->writerGroupsSize = %d\n", connection->writerGroupsSize);	// how does the system get 2?
	printf("connection->readerGroupsSize = %d\n", connection->readerGroupsSize);	// this should not be 0
	printf("connection->configurationFreezeCounter = %d\n", connection->configurationFreezeCounter);
	#endif

/* runtime values are
connection->channel->publisherId = 0 <should be 2234>
connection->channel->state = 0
connection->channel->connectionConfig->name = <Publisher Connection>
connection->channel->connectionConfig->enabled = 1
connection->channel->connectionConfig->transportProfileUri= http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt
connection->channel->connectionConfig->address = empty <should be opc.mqtt://192.168.1.11>
connection->channel->connectionConfig->connectionPropertiesSize = 3
connection->identifier = empty <???>
connection->writerGroupsSize = 2 <how does the system compute this value>
connection->readerGroupsSize = 0 <dont understand what to set or is this zero>
connection->configurationFreezeCounter = 0
[2021-02-07 17:48:50.160 (UTC+0800)] warn/server        SV_PubSub.c : addSubscription() : register channel failed: BadArgumentsMissing!!!!

*/

	// connection->channel->state = UA_PUBSUB_CHANNEL_ERROR;	// for testing the following
	// confirmed the error mesage came from open62541.c : 89301 : UA_PubSubChannelMQTT_regist()
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : addSubscription() : just about to invoke callback at line 3356");
    UA_StatusCode rv = connection->channel->regist(connection->channel, &transportSettings, &callback);	// line 3356

//******************************//
    // error message received at this juncture :: register channel failed: BadArgumentsMissing!
    if (rv == UA_STATUSCODE_GOOD)
    {
	UA_UInt64 subscriptionCallbackId;
	UA_Server_addRepeatedCallback(server, (UA_ServerCallback)mqttYieldPollingCallback,
                                          connection, PUBLISH_INTERVAL_MQTT / 2, &subscriptionCallbackId);	// 9000 millisecond which is the same as sampling time of airgard
    }
    else
    {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "SV_PubSub.c : addSubscription() : register channel failed: %s!!!!",
                           UA_StatusCode_name(rv));
	//sleep(5);
    }
}


void CreateServerPubSub(UA_Server *uaServer, char* brokeraddress, int port, char* mode)
{
	UA_ServerConfig *config = UA_Server_getConfig(uaServer);
/*
struct UA_ServerConfig {
....
....
    size_t pubsubTransportLayersSize;
    UA_PubSubTransportLayer *pubsubTransportLayers;
    UA_PubSubConfiguration *pubsubConfiguration;
....
}
*/

/*
 struct UA_PubSubConfiguration {

    // Callback for PubSub component state changes:
    //If provided this callback informs the application about PubSub component state changes.
    //E.g. state change from operational to error in case of a DataSetReader MessageReceiveTimeout.
    //The status code provides additional information.
    void (*pubsubStateChangeCallback)(UA_NodeId *Id,
                                      UA_PubSubState state,
                                      UA_StatusCode status);// TODO: maybe status code provides not enough information $
#ifdef UA_ENABLE_PUBSUB_MONITORING
    UA_PubSubMonitoringInterface monitoringInterface;
#endif /* UA_ENABLE_PUBSUB_MONITORING
*/

	UA_StatusCode retval = UA_STATUSCODE_GOOD;
	UA_String transportProfile = UA_STRING("");

	UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING(NETWORKADDRESSURL_PROTOCOL)}; // "opc.udp://224.0.0.22:4840/"
	UA_NetworkAddressUrlDataType networkAddressUrlWss = {UA_STRING_NULL, UA_STRING(NETWORKADDRESSURL_PROTOCOL)};
	// if opc.udp, no need to specify networkinterface
	// if opc.eth then need to specify networkinterface

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : CreateServerPubSub()");

	if ( brokeraddress != NULL)
	{
		char portbuf[5];
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : brokeraddress : %s, port %d", brokeraddress, port);

		MQTT_Port = port;	// save to global variable to be used during the initiatialisation process

		if ( (port == 8883) || (port == 8884) || (port == 8885) )
		{
			char URI_mqtt[100];
			MQTT_Enable = UA_TRUE;
			MQTT_TLS_Enable = UA_TRUE;
			AMQP_Enable = UA_FALSE;

                        //similar to sprintf(URI_mqtt, "opc.mqtt://%s:1883", brokeraddress);
                        strcpy(URI_mqtt, "opc.mqtt://");
                        strcat(URI_mqtt, brokeraddress);
                        strcat(URI_mqtt, ":");
			itoa(port, portbuf, 10);
			portbuf[4] = '\0';
			strcat(URI_mqtt, portbuf);
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : URI_mqtt : <%s>", URI_mqtt);

                        transportProfile = UA_STRING(TRANSPORT_PROFILE_URI_MQTT);
                        networkAddressUrl.url = UA_STRING(URI_mqtt);
		}
		else if ( (port == 1883) || (port == 1884) )
		{
			char URI_mqtt[100];
			// set the MQTT_Enable global flag
			MQTT_Enable = UA_TRUE;
			MQTT_TLS_Enable = UA_FALSE;
			AMQP_Enable = UA_FALSE;

			//similar to sprintf(URI_mqtt, "opc.mqtt://%s:1883", brokeraddress);
			strcpy(URI_mqtt, "opc.mqtt://");
			strcat(URI_mqtt, brokeraddress);
			strcat(URI_mqtt, ":");
			itoa(port, portbuf, 10);
			portbuf[4] = '\0';
			strcat(URI_mqtt, portbuf);
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : URI_mqtt : <%s>", URI_mqtt);

			transportProfile = UA_STRING(TRANSPORT_PROFILE_URI_MQTT);
			networkAddressUrl.url = UA_STRING(URI_mqtt);
		}
		else if (port == 5672)
		{
                        char URI_amqp[100];
                        // set the AMQP_Enable global flag
                        MQTT_Enable = UA_FALSE;
			MQTT_TLS_Enable = UA_FALSE;
                        AMQP_Enable = UA_TRUE;

                        //similar to sprintf(URI_mqtt, "opc.amqp://%s:5672", brokeraddress);
                        strcpy(URI_amqp, "opc.amqp://");
                        strcat(URI_amqp, brokeraddress);
                        strcat(URI_amqp, ":5672");
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : URI_amqp : <%s>", URI_amqp);

                        transportProfile = UA_STRING(TRANSPORT_PROFILE_URI_AMQP);
                        networkAddressUrl.url = UA_STRING(URI_amqp);
		}
	}
	else
	{
		MQTT_Enable = UA_FALSE;
		MQTT_TLS_Enable = UA_FALSE;
		AMQP_Enable = UA_FALSE;
	        if ( strncmp(NETWORKADDRESSURL_PROTOCOL, "opc.udp://", 10) == 0)
        	{
                	transportProfile = UA_STRING(TRANSPORT_PROFILE_URI_UDP);
			networkAddressUrl.url = UA_STRING("opc.udp://224.0.0.22:4840/");
        	}
        	else if ( strncmp(NETWORKADDRESSURL_PROTOCOL, "opc.eth://", 10) == 0)
        	{
                	transportProfile = UA_STRING(TRANSPORT_PROFILE_URI_ETH);
                	networkAddressUrl.url = UA_STRING("opc.eth://224.0.0.22:4840/");
                	//networkAddressUrl.networkInterface = UA_STRING("eth0");        	}
		}
	}

 	networkAddressUrl.networkInterface = UA_STRING("eth0");	// name of the interface defined in /etc/dhcpcd.conf
	//networkAddressUrl.url = UA_STRING(NETWORKADDRESSURL_UDP); // "opc.udp://224.0.0.22:4840/");

	// defunct: config->pubsubTransportLayers = (UA_PubSubTransportLayer *) UA_calloc(3, sizeof(UA_PubSubTransportLayer)); // UDAP, ETHERNET, MQTT, WSS
	// defunct: if(!config->pubsubTransportLayers)
	// defunct: {
	// defunct: 	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"CreateServerPubSub : cannot initialise PubSubTransportLayer");
        // defunct: 	return;
	// defunct: }

	// initiate the PubSub SecurityPolicy - kiv until missing fields are fixed
	config->pubSubConfig.securityPolicies = (UA_PubSubSecurityPolicy *) UA_malloc(sizeof(UA_PubSubSecurityPolicy));
	config->pubSubConfig.securityPoliciesSize = 1;
	UA_PubSubSecurityPolicy_Aes128Ctr(config->pubSubConfig.securityPolicies, &config->logger);

	/* defunct: v1.2.1 */ //config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();	// options: UA_PubSubTransportLayerEthernet(), UA_PubSubTransportLayerMQTT(), 
    	/* defunct: v1.2.1 */ //config->pubsubTransportLayersSize++;
	/* new: v1.2.2 */ UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerUDPMP());

#ifdef UA_ENABLE_PUBSUB_ETH_UADP
	/* defunct: v1.2.1 */ //config->pubsubTransportLayers[1] = UA_PubSubTransportLayerEthernet();
	/* defunct: v1.2.1 */ //config->pubsubTransportLayersSize++;
	/* new: v1.2.2 */ UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerEthernet());
#endif

	if (MQTT_Enable == UA_TRUE && AMQP_Enable == UA_FALSE)
	{
		/* defunct:v1.2.1 */ //config->pubsubTransportLayers[2] = UA_PubSubTransportLayerMQTT();
    		/* defunct:v1.2.1 */ //config->pubsubTransportLayersSize++;
		printf("SV_PubSub.c : CreateServerPubSub() calling UA_PubSubTransportLayerMQTT() now \n"); 
		/* new:v1.2.2 */ UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerMQTT());
	}
	else if (MQTT_Enable == UA_FALSE && AMQP_Enable == UA_TRUE)
	{
		// example can be found in github #3850
		// examples/pubsub/tutorial_pubsub_amqp_publish.c
		// defunct: config->pubsubTransportLayers[2] = UA_PubSubTransportLayerAMQP();
		// defunct: config->pubsubTransportLayersSize++;
		// TODO UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerAMQP());
	}

	#ifdef NOT_READY_WAIT_FOR_OPEN62541 //THIS WILL CAUSE networkAddressUrl.url = (empty) UA_ENABLE_WEBSOCKET_SERVER
	//1. add UA_ServerConfig_addNetworkLayerWS(UA_Server_getConfig(uaServer1), 7681, 0, 0, NULL, NULL); in SV_StartOPCUAServer.c
	//2. gcc -lwebsockets

	    #ifdef UA_ENABLE_JSON_ENCODING
	    {
		// no such thing as UA_PubSubTransportLayerWSS()
		// defunct: config->pubsubTransportLayers[3] = UA_PubSubTransportLayerWS();
		// defunct: config->pubSubTransportLayersSize++;

		// send the JSON payload to port 7681
		// sample in libwebsockets -> https://libwebsockets.org/git/libwebsockets/tree/minimal-examples/ws-client/minimal-ws-client-tx/minimal-ws-client.c
		transportProfile = UA_STRING(TRANSPORT_PROFILE_URI_WSSJSON);
	    }
	    #else
		transportProfile = UA_STRING(TRANSPORT_PROFILE_URI_WSSBIN);
	    #endif

		networkAddressUrlWss.url = UA_STRING("opc.wss://192.168.1.11:7681/");

		retval = CreateServerWebSockets(&publishedDataSetIdentifier, &networkAddressUrlWss);	// inititalise the port and send the data to this port
		if (retval != UA_STATUSCODE_GOOD)
			UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"CreateServerPubSub : cannot initialise websockets port 7681");
		else
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"CreateServerPubSub : Successfully initialise websockets port 7681");
	#endif

/*
	config->pubsubTransportLayers[3] = UA_PubSubTransportLayerAMQP();
    	config->pubsubTransportLayersSize++;
*/

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : In CreateServerPubSub");
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : networkAddressUrl.networkInterface 		: <%s>", networkAddressUrl.networkInterface.data);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : networkAddressUrl.url			: <%s>", networkAddressUrl.url.data);
	//printf("config->pubsubTransportLayers[0]		: %s \n", config->pubsubTransportLayers[0].data);
	// defunct: printf("config->pubsubTransportLayersSize		: %d \n", config->pubsubTransportLayersSize);

	addPubSubConnection(uaServer, &transportProfile, &networkAddressUrl);
	// at this point, the global variable 'PubSubconnectionIdentifier' should be initialised

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : after addPubSubConnection() completed");
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : brokeraddress %s", brokeraddress);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : mode %s", mode);

	if (strlen(mode)!=0)
	{
		if ( (strncmp(mode, "nomode", 6)==0) && (strlen(mode)==6) && (brokeraddress == NULL) )
		{
	                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : mode = UADP");
	                addPublishedDataSet(uaServer);
	                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : addPublishedDataSet() completed");
	                addDataSetField(uaServer);
        	        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : addDataSetField() completed");
                	addWriterGroup(uaServer);
	                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : addWriterGroup() completed");
        	        addDataSetWriter(uaServer);
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : addDataSetWriter() completed");

	                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"CreateServerPubSub : successfully initialise Publisher routine");
        	        return;
		}

		else if ( (strncmp(mode, "pubsub", 6)==0) && (strlen(mode)==6) && (brokeraddress != NULL) )
		{
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : mode = 'pubsub'");
			//UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Warning : currently this is not implementated");
			//UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Warning : please indicate 'pub' or 'sub'");

			// publish
			addPublishedDataSet(uaServer);
			addDataSetField(uaServer);
			addWriterGroup(uaServer);
			addDataSetWriter(uaServer);

			// subscribe
			addReaderGroup(uaServer);
			addDataSetReader(uaServer);
			addSubscribedVariables(uaServer, readerIdentifier);   // line 2395
			UA_PubSubConnection *connection = UA_PubSubConnection_findConnectionbyId(uaServer, PubSubconnectionIdentifier);
			addSubscription(uaServer, connection);
		}

        	else if ( (strncmp(mode, "pub", 3)==0) && (strlen(mode)==3) && (brokeraddress != NULL) )
		{
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : mode = 'pub'");
		 	addPublishedDataSet(uaServer);
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : addPublishedDataSet() completed");
		 	addDataSetField(uaServer);
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : addDataSetField() completed");
			addWriterGroup(uaServer);
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : addWriterGroup() completed");
			addDataSetWriter(uaServer);
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : addDataSetWriter() completed");

			/**
		 	* That's it! You're now publishing the selected fields.
	 		* Open a packet inspection tool of trust e.g. wireshark and take a look on the outgoing packages.
		 	* The following graphic figures out the packages created by this tutorial.
			*/
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"CreateServerPubSub : successfully initialise Publisher routine");
			return;
		}

		else if ( (strncmp(mode, "sub", 3)==0) && (strlen(mode)==3) && (brokeraddress != NULL) )
		{
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : Mode = 'sub'");
       			addReaderGroup(uaServer);
	                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : addReaderGroup() completed");
        	        addDataSetReader(uaServer);
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : addDataSetReader()completed");

	                // create a new OPCUA tree nodes from the subscribed payload - the following statements must come after addSubscription()
        	        addSubscribedVariables(uaServer, readerIdentifier);	// line 2395

		    	// now do Subscriber routine : OPCUAServer 'subscribing' for methodcalls messages submitted by client through MQTT
			// example taken from https://github.com/open62541/open62541/blob/mqtt_demo/examples/pubsub/tutorial_pubsub_mqtt.c
			UA_PubSubConnection *connection = UA_PubSubConnection_findConnectionbyId(uaServer, PubSubconnectionIdentifier); // somehow PubSubconnectionIdentifier is not properly initiaised
	                if(connection != NULL)
			{
                                // initialise a callback for subscription
                                addSubscription(uaServer, connection);

				/*
				typedef struct UA_PubSubConnection{
				    UA_PubSubComponentEnumType componentType;
				    UA_PubSubConnectionConfig *config;
				    //internal fields
				    UA_PubSubChannel *channel;
				    UA_NodeId identifier;
				    LIST_HEAD(UA_ListOfWriterGroup, UA_WriterGroup) writerGroups;
				    size_t writerGroupsSize;
				    LIST_HEAD(UA_ListOfPubSubReaderGroup, UA_ReaderGroup) readerGroups;
				    size_t readerGroupsSize;
				    TAILQ_ENTRY(UA_PubSubConnection) listEntry;
				    UA_UInt16 configurationFreezeCounter;
				    // This flag is 'read only' and is set internally based on the PubSub state.
				    UA_Boolean configurationFrozen;
				} UA_PubSubConnection;
				*/
				//#ifdef DEBUG_MODE
				// inspect the contents of *connection
			        UA_String output, identifier;
        			UA_String_init(&output);
				UA_String_init(&identifier);

	        		UA_NodeId_print(&PubSubconnectionIdentifier, &output);
				UA_NodeId_print(&connection->identifier, &identifier);

        			printf("SV_PubSub.c : CreateServerPubSub() : NodeId : <%s> \n", output.data);
				printf("in CreateServerPubSub()::3590\n");
				printf("connection->componentType 		: %s\n");
				printf("connection->config->name		: %s\n", connection->config->name.data);
				printf("connection->config->enabled 		: %d\n", connection->config->enabled);
				printf("connection->config->transportProfileUri : %s\n", connection->config->transportProfileUri.data);
				printf("connection->config->publisherId->numeric : %d\n", connection->config->publisherId.numeric); //should not be NULL

				// cast UA_Variant to UA_String; example in SV_Historizing.c
				//printf("connection->config->address		: %s\n", *(UA_String *)connection->config->address.data);	// cast UA_Variant to UA_String
				printf("connection->config->connectionPropertiesSize : %d\n", connection->config->connectionPropertiesSize);
				printf("connection->channel			: \n");
				printf("connection->identifier			: %s\n", identifier.data);
				//#endif
				printf("connection->writeGroupsSize		: %d\n", connection->writerGroupsSize);
				printf("connection->readerGroupsSize		: %d\n", connection->readerGroupsSize);
				printf("connection->configurationFreezeCounter	: %d\n", connection->configurationFreezeCounter);
				printf("connection->configurationFrozen		: %d\n", connection->configurationFrozen);

				/* output capture during runtime is
				in CreateServerPubSub()
				connection->componentType               : (null)
				connection->config->name                : Publisher Connection
				connection->config->enabled             : 1
				connection->config->transportProfileUri : http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt
				connection->config->publisherId->numeric : 2234
				connection->config->address             : (null)
				connection->config->connectionPropertiesSize : 3
				connection->channel                     :
				connection->identifier                  : i=50566	// equivalent to NodeId <i=50566>
				*/

				// initialise a callback for subscription - move to front
        	                //addSubscription(uaServer, connection);
				return;
			}
                	else
                        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"CreateServerPubSub : fail to find connectionId");


			/*
		        // provide a callback to get notifications of specific PubSub state changes or timeout (e.g. subscriber MessageReceiveTimeout)
        		config->pubsubConfiguration->pubsubStateChangeCallback = pubSubStateChangeCallback;
	        	//if (*useCustomMonitoring == UA_TRUE)
        		//{
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Use custom monitoring callback implementation");
				UA_PubSubMonitoringInterface monitoringInterface;
				monitoringInterface.createMonitoring = pubSubComponent_createMonitoring;
				monitoringInterface.startMonitoring = pubSubComponent_startMonitoring;
				monitoringInterface.stopMonitoring = pubSubComponent_stopMonitoring;
				monitoringInterface.updateMonitoringInterval = pubSubComponent_updateMonitoringInterval;
				monitoringInterface.deleteMonitoring = pubSubComponent_deleteMonitoring;

				config->pubsubConfiguration->monitoringInterface = monitoringInterface;
        		//}

			*/
			// shift after addDataSetWriter()
			//addReaderGroup(uaServer);
			//printf("CreateServerPubSub : addReaderGroup() completed \n");
			//addDataSetReader(uaServer);
			//printf("CreateServerPubSub : addDataSetReader()completed \n");

			// create a new OPCUA tree nodes from the subscribed payload
			//addSubscribedVariables(uaServer, readerIdentifier);
			//UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"CreateServerPubSub : addSubscribedVariables() completed");
		}
		else
                {
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : <mode> indicated = %s", mode);
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Error: Unknown <mode>");
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Error: please indicate 'pub' or 'sub'");
                        exit(0);
                }

	}
	/*
	else // mode == NULL
	{
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : mode = NULL");
                addPublishedDataSet(uaServer);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : addPublishedDataSet() completed");
                addDataSetField(uaServer);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : addDataSetField() completed");
                addWriterGroup(uaServer);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : addWriterGroup() completed");
                addDataSetWriter(uaServer);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : addDataSetWriter() completed");

                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"CreateServerPubSub : successfully initialise Publisher routine");
		return;
	}
	*/
}
