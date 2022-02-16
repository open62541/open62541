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
#define APPLICATION_URI "urn:XXX.com.sg"

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
 #define PUBLISHER_TOPIC_EDGE2CLOUD_MQTT        "XXXTopicEdgeToCloud"
 #define PUBLISHER_TOPIC_CLOUD2EDGE_MQTT	"XXXTopicCloudToEdge"
 #define PUBLISHER_QUEUE_EDGE2CLOUD_AMQP	"XXXQueueEdgeToCloud"
 #define PUBLISHER_QUEUE_CLOUD2EDGE_AMQP	"xxxQueueCloudToEdge"
 //#define BROKER_ADDRESS_URL_MQTT           	"opc.mqtt://192.168.1.119:1883/"
 //#define BROKER_ADDRESS_URL_AMQP		"opc.amqp://192.168.1.119:5672/"
 #define USERNAME_OPTION_NAME_MQTT         	"mqttUsername"
 #define USERNAME_OPTION_NAME_AMQP		"amqpUsername"
 #define PASSWORD_OPTION_NAME_MQTT         	"mqttPassword"
 #define PASSWORD_OPTION_NAME_AMQP		"amqpPassword"
 #define USERNAME_MQTT				"myusername" // during execution, mqtt-11 will show : New client connected from 192.168.1.33 as OPCServer-33-Mqtt (c0, k400, u'jackybek')
 #define USERNAME_AMQP				"myusername"
 #define PASSWORD_MQTT                		"mypassword"
 #define PASSWORD_AMQP				"mypassword"

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
#endif

 #define SUBSCRIBER_METADATAUPDATETIME_MQTT      0
 #define SUBSCRIBER_METADATAQUEUENAME_MQTT       "MetaDataTopic"
 #define SUBSCRIBER_TOPIC_MQTT                   "XXXTopic"
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
// to define remaining fields

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
    /*7*///UA_Variant_setScalar(&connectionConfig.connectionTransportSettings,
    /*8*///connectionConfig.connectionPropertiesSize = 3;	// to match connectionOptions[N]
    /*9*///connectionConfig.connectionProperties = (UA_KeyValuePair *)calloc(3, sizeof(UA_KeyValuePair));


    if (MQTT_Enable == UA_TRUE && AMQP_Enable == UA_FALSE)
    {
	#ifdef DEBUG_MODE
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : addPubSubConnection(): MQTT segment \n");
	#endif

	int connectionOptionsCount;
	if (MQTT_TLS_Enable == UA_TRUE)
		connectionOptionsCount = 10;
	else
		connectionOptionsCount = 5;

	size_t connectionOptionsIndex = 0;
	UA_KeyValuePair connectionOptions[connectionOptionsCount];
    size_t namespaceIndex;

    UA_Server_getNamespaceByName(uaServer, UA_STRING("XXX.com.sg"), &namespaceIndex);

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

	connectionOptionsIndex++;	// 3
    	connectionOptions[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, USERNAME_OPTION_NAME_MQTT);	// mqttUsername
    	UA_String mqttUsername = UA_STRING(USERNAME_MQTT);
    	UA_Variant_setScalar(&connectionOptions[connectionOptionsIndex].value, &mqttUsername, &UA_TYPES[UA_TYPES_STRING]);

	connectionOptionsIndex++;	// 4
    	connectionOptions[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, PASSWORD_OPTION_NAME_MQTT);	// mqttPassword
    	UA_String mqttPassword = UA_STRING(PASSWORD_MQTT);
    	UA_Variant_setScalar(&connectionOptions[connectionOptionsIndex].value, &mqttPassword, &UA_TYPES[UA_TYPES_STRING]);

        if (MQTT_TLS_Enable == UA_TRUE)
	{
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

	                connectionOptionsIndex++; // 8
			connectionOptions[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, "mqttClientCertPath");
                	UA_String mqttClientCertPath = UA_STRING(CLIENT_CERT_PATH_MQTT);
	                UA_Variant_setScalar(&connectionOptions[connectionOptionsIndex].value, &mqttClientCertPath, &UA_TYPES[UA_TYPES_STRING]);

        	        connectionOptionsIndex++; // 9
			connectionOptions[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, "mqttClientKeyPath");
	                UA_String mqttClientKeyPath = UA_STRING(CLIENT_KEY_PATH_MQTT);
        	        UA_Variant_setScalar(&connectionOptions[connectionOptionsIndex].value, &mqttClientKeyPath, &UA_TYPES[UA_TYPES_STRING]);
		
	}
   	connectionConfig.connectionProperties = connectionOptions;
    	connectionConfig.connectionPropertiesSize = connectionOptionsIndex+1;	// add 1 because the index start at 0;


	// initiatise the value of PubSubconnectionIdentifier (UA_NodeId) so that we can check later in addSubscription()
	#ifdef DEBUG_MODE
    	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : Before calling UA_Server_addPubSubConnection() : MQTT_Enable == UA_TRUE && AMQP_Enable == UA_FALSE");
	#endif

    	UA_StatusCode retval = UA_Server_addPubSubConnection(uaServer, &connectionConfig, &PubSubconnectionIdentifier);
    	if (retval == UA_STATUSCODE_GOOD)
    	{
		UA_String output;
		UA_String_init(&output);

		UA_NodeId_print(&PubSubconnectionIdentifier, &output);
    	}
    	else
    	{
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"addPubSubConnection : PubSubconnectionIdentifier failure : %s", UA_StatusCode_name(retval));
		exit(EXIT_FAILURE);
    	}
    }
    else if (MQTT_Enable == UA_FALSE && AMQP_Enable == UA_TRUE)
    {
	#ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : addPubSubConnection(): AMQP segment");
	#endif

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
	UA_Server_getNamespaceByName(uaServer, UA_STRING("XXX.com.sg"), &namespaceIndex);

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

    	UA_NodeId connectionIdentifier = UA_NODEID_STRING_ALLOC(namespaceIndex, "ConnectionIdentifier");

	// to extract value from a VARIANT
	//UA_Int16 raw_data = *(UA_Int16 *)varStrNonAlarms->data;

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
    publishedDataSetConfig.name = UA_STRING("XXX PDS");

    /* Create new PublishedDataSet based on the PublishedDataSetConfig. */
    UA_Server_addPublishedDataSet(uaServer, &publishedDataSetConfig, &publishedDataSetIdentifier);


}

/**
 * **DataSetField handling**
 * The DataSetField (DSF) is part of the PDS and describes exactly one published field.
 */
static void
addDataSetField(UA_Server *uaServer)
{


    size_t namespaceIndex;
    UA_Server_getNamespaceByName(uaServer, UA_STRING("XXX.com.sg"), &namespaceIndex);

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

    // repeat the above for the remaining attributes
    

    // added by Jacky on 4/4/2021 to update MQTT payload (MetaDataVersion)
    // the corresponding change has to take place in open62541.c : UA_Server_addDataSetField() - however this change cause publisher to have error
        //UA_open62541/src/pubsub/ua_pubsub_writer.c
        //At line 691
    UA_PublishedDataSet *currentDataSet = UA_PublishedDataSet_findPDSbyId(uaServer, publishedDataSetIdentifier);
    currentDataSet->dataSetMetaData.configurationVersion.majorVersion = MAJOR_SOFTWARE_VERSION;
    currentDataSet->dataSetMetaData.configurationVersion.minorVersion = MINOR_SOFTWARE_VERSION;

    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgSoftwareVersion, NULL); // &f_SoftwareVersion_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgDataBlockVersion, NULL); //&f_DataBlockVersion_Id);
    // repeat the above for the remaining attributes
    
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
    writerGroupConfig.name = UA_STRING("XXX WriterGroup");
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
    dataSetWriterConfig.name = UA_STRING("XXX DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = DATASETWRITERID;
    dataSetWriterConfig.dataSetName = UA_STRING("XXX Dataset");
    dataSetWriterConfig.keyFrameCount = 10;

 	// refer to https://github.com/open62541/open62541/blob/master/tests/pubsub/check_pubsub_encoding_json.c
    	// enable metaDataVersion in UA_DataSetMessageHeader


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
    // repeat the above for rest of the fields
    

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
	dataSetReaderConfig.name = UA_STRING("XXX DataSetReader");
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
	dataSetReaderConfig.securityParameters.securityGroupId = UA_STRING("XXXSecurityGroupId-1");
	//dataSetReaderConfig.securityParameters.keyServersSize = (size_t)1;	// size_t;
	//dataSetReaderConfig.messageSettings = ??		// UA_ExtensionObject
	//dataSetReaderConfig.transportSettings = ??		// UA_ExtensionObject
	dataSetReaderConfig.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;	// UA_SubscribedDataSetEnumType
//	dataSetReaderConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize = 2;	// UA_TargetVariables
//	dataSetReaderConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables.externalDataValue = *MQTT data* 	// UA_FieldTargetVariable .. UA_DataValue**
// the above is not in the sample


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

    readerGroupConfig.name = UA_STRING("XXX ReaderGroup");
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
                UA_String folderName = UA_STRING("XXX Subscribed"); //dataSetReaderConfig.dataSetMetaData.name;
                UA_ObjectAttributes oAttrObj = UA_ObjectAttributes_default;
		UA_VariableAttributes oAttVar = UA_VariableAttributes_default;
                UA_QualifiedName airgardfolderBrowseName, diagnosticfolderBrowseName;

	        //Add a new namespace to the server => Returns the index of the new namespace i.e. namespaceIndex
        	UA_Int16 nsIdx_MKS = UA_Server_addNamespace(uaServer, "XXX.com.sg");
		size_t namespaceIndex;
		UA_Server_getNamespaceByName(uaServer, UA_STRING("XXX.com.sg"), &namespaceIndex);

                if (folderName.length > 0)
                {
                        oAttrObj.displayName.locale = UA_STRING("en-US");
                        oAttrObj.displayName.text = folderName;		// actual shown in UAExpert - "en-US", "XXXSensor MetaData"
                        airgardfolderBrowseName.namespaceIndex = namespaceIndex;	// actual shown in UAExpert - 1, "XXXSensor MetaData"
                        airgardfolderBrowseName.name = folderName;	// actual shown in UAExpert - 1, "XXXSensor MetaData"
                }
                else
                {
                        oAttrObj.displayName = UA_LOCALIZEDTEXT("en-US", "XXXSensor Subscribed");
                        airgardfolderBrowseName.namespaceIndex = namespaceIndex;
			airgardfolderBrowseName.name = UA_STRING("XXXSensor Subscribed");
                }

		// create a UA structure to receive the incoming mqtt streams
                retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "AirGardSensor Subscribed"), //UA_NODEID_NULL,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), //UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                        airgardfolderBrowseName,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                        oAttrObj, NULL, &airgardfolderId);


                // Subtree : XXX->Data       == 
                oAttrObj.displayName = UA_LOCALIZEDTEXT("en-US", "Data");
                retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "XXX_Data_S"),
                                        airgardfolderId,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                        UA_QUALIFIEDNAME(namespaceIndex, "Data"), //diagnosticfolderBrowseName,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                        oAttrObj, NULL, &datafolderId);


		// Subtree : XXX->Diagnostics 	== OKAY
		oAttrObj.displayName = UA_LOCALIZEDTEXT("en-US", "Diagnostics");
		retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "XXX_Diagnostics_S"),
					airgardfolderId,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
					UA_QUALIFIEDNAME(namespaceIndex, "Diagnostic"),
					UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
					oAttrObj, NULL, &diagnosticfolderId);


                // Subtree : XXX->Methods       ==
                oAttrObj.displayName = UA_LOCALIZEDTEXT("en-US", "Methods");
                retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "XXX_Methods_S"),
                                        airgardfolderId,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                        UA_QUALIFIEDNAME(namespaceIndex, "Methods"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                        oAttrObj, NULL, &methodsfolderId);


		// Subtree : XXX->Status	== OKAY
		oAttrObj.displayName = UA_LOCALIZEDTEXT("en-US", "Status");
		retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "XXX_Status_S"),
					airgardfolderId,
					UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
					UA_QUALIFIEDNAME(namespaceIndex, "Status"),
					UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
					oAttrObj, NULL, &statusfolderId);


		// Subtree : XXX->Status->Info	== OKAY
		oAttrObj.displayName = UA_LOCALIZEDTEXT("en-US", "Info");
                retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "XXX_Status_Info_S"),
                                        statusfolderId,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                        UA_QUALIFIEDNAME(namespaceIndex, "Info"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                        oAttrObj, NULL, &statusinfofolderId);


		// Subtree : XXX->Timestamp		== OKAY
		oAttrObj.displayName = UA_LOCALIZEDTEXT("en-US", "TimeStamp");
		retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "XXX_TimeStamp_S"),
					airgardfolderId,
					UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
					UA_QUALIFIEDNAME(namespaceIndex, "Timestamp"),
					UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
					oAttrObj, NULL, &timestampfolderId);


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

			else if ((i==2) || (i==3)) // adapt the above code for different type (numeric, float, string, etc)

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

					//outputStr = UA_STRING(subscribedSoftwareVersion);
					UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedSoftwareVersion, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 1 : // 80301 Data Block Version

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
                        // repeat the above for remaining variables
			}

        }

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
     // remove unwanted characters in reverse order
     for (i=strlen((char*)encodedBuffer->data); i>0; i--)
     {
	//printf("value of i is %d \n", i);
	if ((char)encodedBuffer->data[i] == '}') break;
     }
     encodedBuffer->data[i+1] ='\0';

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
		}

		jerr = json_tokener_get_error(tok);
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : here 0.5 : jerror = %d", jerr);

	}
    	while (jerr == json_tokener_continue); // ((jerr = json_tokener_get_error(tok)) == json_tokener_continue);

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

     // next search for the key "Messages", the return 'contents' are in stored in 'valueMessages'
     exists = UA_FALSE;
     exists = json_object_object_get_ex(jobj, "Messages", &valueMessages); //struct json_object **
     if (exists==UA_FALSE) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : Warning : Key <Messages> cannot be found in JSON");
        }
	else UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Success : Key <Messages> found in JSON");
     // get the string from the value of the key "Message"
     struct array_list *myArrayList;
     myArrayList = json_object_get_array(valueMessages); //struct json_object *

    // https://json-c.github.io/json-c;
    // install in /usr/local/include/
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

    // repeat the above for the remaining attributes

     }

FreeMemory:

	// free the memory
	if (tok != NULL) json_tokener_free(tok);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : callback() :: after json_tokener_free(tok)");

     	// DO NOT FREE encodedBuffer : encodedBuffer is passed in by reference
	//if (encodedBuffer != NULL) UA_ByteString_delete(encodedBuffer);

	if (topic != NULL) UA_ByteString_delete(topic);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : callback() :: after UA_ByteString_delete(topic)");

	if (jobj) json_object_put(jobj);	// frees the jobj tree recursively
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
    transportSettings.content.decoded.type = &UA_TYPES[UA_TYPES_BROKERDATASETREADERTRANSPORTDATATYPE];	// match open62541.c:89311
    transportSettings.content.decoded.data = &brokerTransportSettings;

	// connection->channel->state = UA_PUBSUB_CHANNEL_ERROR;	// for testing the following
	// confirmed the error mesage came from open62541.c : 89301 : UA_PubSubChannelMQTT_regist()
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : addSubscription() : just about to invoke callback at line 3356");
    UA_StatusCode rv = connection->channel->regist(connection->channel, &transportSettings, &callback);	// line 3356

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
	
    }
}

void CreateServerPubSub(UA_Server *uaServer, char* brokeraddress, int port, char* mode)
{
	UA_ServerConfig *config = UA_Server_getConfig(uaServer);
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
		 	addPublishedDataSet(uaServer);
		 	addDataSetField(uaServer);
			addWriterGroup(uaServer);
			addDataSetWriter(uaServer);

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
				return;
			}
            else
                 UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"CreateServerPubSub : fail to find connectionId");
		}
		else
        {
                 UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : <mode> indicated = %s", mode);
                 UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Error: Unknown <mode>");
                 UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Error: please indicate 'pub' or 'sub'");
                 exit(0);
        }
	}
}
