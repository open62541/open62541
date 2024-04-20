/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2022 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 * Copyright (c) 2021 Fraunhofer IOSB (Author: Jan Hermes)
 * Copyright (c) 2022 Siemens AG (Author: Thomas Fischer)
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#ifndef UA_SERVER_PUBSUB_H
#define UA_SERVER_PUBSUB_H

#include <open62541/common.h>
#include <open62541/util.h>
#include <open62541/client.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/eventloop.h>

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_PUBSUB

/**
 * .. _pubsub:
 *
 * PubSub
 * ======
 *
 * In PubSub the participating OPC UA Applications take their roles as
 * Publishers and Subscribers. Publishers are the sources of data, while
 * Subscribers consume that data. Communication in PubSub is message-based.
 * Publishers send messages to a Message Oriented Middleware, without knowledge
 * of what, if any, Subscribers there may be. Similarly, Subscribers express
 * interest in specific types of data, and process messages that contain this
 * data, without knowledge of what Publishers there are.
 *
 * Message Oriented Middleware is software or hardware infrastructure that
 * supports sending and receiving messages between distributed systems. OPC UA
 * PubSub supports two different Message Oriented Middleware variants, namely
 * the broker-less form and broker-based form. A broker-less form is where the
 * Message Oriented Middleware is the network infrastructure that is able to
 * route datagram-based messages. Subscribers and Publishers use datagram
 * protocols like UDP. In a broker-based form, the core component of the Message
 * Oriented Middleware is a message Broker. Subscribers and Publishers use
 * standard messaging protocols like AMQP or MQTT to communicate with the
 * Broker.
 *
 * This makes PubSub suitable for applications where location independence
 * and/or scalability are required.
 *
 * The Publish/Subscribe (PubSub) extension for OPC UA enables fast and
 * efficient 1:m communication. The PubSub extension is protocol agnostic and
 * can be used with broker based protocols like MQTT and AMQP or brokerless
 * implementations like UDP-Multicasting.
 *
 * The configuration model for PubSub uses the following components: */

typedef enum  {
    UA_PUBSUB_COMPONENT_CONNECTION,
    UA_PUBSUB_COMPONENT_WRITERGROUP,
    UA_PUBSUB_COMPONENT_DATASETWRITER,
    UA_PUBSUB_COMPONENT_READERGROUP,
    UA_PUBSUB_COMPONENT_DATASETREADER
} UA_PubSubComponentEnumType;

/**
 * The following figure shows how the PubSub components are related.
 * The PubSub Tutorials have more examples about the API usage::
 *
 *  +--------+
 *  | Server |
 *  +--------+
 *    |  |
 *    |  |  +------------------------+
 *    |  +--> PubSubPublishedDataSet <----------+
 *    |     +------------------------+          |
 *    |       |                                 |
 *    |       |    +--------------+             |
 *    |       +----> DataSetField |             |
 *    |            +--------------+             |
 *    |                                         |
 *    |     +------------------+                |
 *    +-----> PubSubConnection |                |
 *          +------------------+                |
 *            |  |                              |
 *            |  |    +-------------+           |
 *            |  +----> WriterGroup |           |
 *            |       +-------------+           |
 *            |         |                       |
 *            |         |    +---------------+  |
 *            |         +----> DataSetWriter <--+
 *            |              +---------------+
 *            |
 *            |       +-------------+
 *            +-------> ReaderGroup |
 *                    +-------------+
 *                      |
 *                      |    +---------------+
 *                      +----> DataSetReader |
 *                           +---------------+
 *                             |
 *                             |    +-------------------+
 *                             +----> SubscribedDataSet |
 *                                  +-------------------+
 *                                    |
 *                                    |    +-------------------------+
 *                                    +----> TargetVariablesDataType |
 *                                    |    +-------------------------+
 *                                    |
 *                                    |    +---------------------------------+
 *                                    +----> SubscribedDataSetMirrorDataType |
 *                                         +---------------------------------+
 *
 * PubSub Information Model Representation
 * ---------------------------------------
 * .. _pubsub_informationmodel:
 *
 * The complete PubSub configuration is available inside the information model.
 * The entry point is the node 'PublishSubscribe', located under the Server
 * node.
 * The standard defines for PubSub no new Service set. The configuration can
 * optionally be done over methods inside the information model.
 * The information model representation of the current PubSub configuration is
 * generated automatically. This feature can be enabled/disabled by changing the
 * UA_ENABLE_PUBSUB_INFORMATIONMODEL option.
 *
 * Connections
 * -----------
 * The PubSub connections are the abstraction between the concrete transport protocol
 * and the PubSub functionality. It is possible to create multiple connections with
 * different transport protocols at runtime.
 */

/* Valid PublisherId types are defined in Part 14, 7.2.2.2.2 NetworkMessage
 * Layout (bit range 0-2).
 *
 * - 000 Byte (default value if ExtendedFlags1 is omitted)
 * - 001 UInt16
 * - 010 UInt32
 * - 011 UInt64
 * - 100 String */

typedef enum {
    UA_PUBLISHERIDTYPE_BYTE   = 0,
    UA_PUBLISHERIDTYPE_UINT16 = 1,
    UA_PUBLISHERIDTYPE_UINT32 = 2,
    UA_PUBLISHERIDTYPE_UINT64 = 3,
    UA_PUBLISHERIDTYPE_STRING = 4
} UA_PublisherIdType;

typedef struct {
    UA_PublisherIdType idType;
    union {
        UA_Byte byte;
        UA_UInt16 uint16;
        UA_UInt32 uint32;
        UA_UInt64 uint64;
        UA_String string;
    } id;
} UA_PublisherId;

UA_EXPORT UA_StatusCode
UA_PublisherId_copy(const UA_PublisherId *src, UA_PublisherId *dst);

UA_EXPORT void
UA_PublisherId_clear(UA_PublisherId *p);

/* The variant must contain a scalar of the five possible identifier types */
UA_EXPORT UA_StatusCode
UA_PublisherId_fromVariant(UA_PublisherId *p, const UA_Variant *src);

/* Makes a shallow copy (no malloc) in the variant */
UA_EXPORT void
UA_PublisherId_toVariant(const UA_PublisherId *p, UA_Variant *dst);

typedef struct {
    UA_String name;
    UA_Boolean enabled;
    UA_PublisherId publisherId;
    UA_String transportProfileUri;
    UA_Variant address;
    UA_KeyValueMap connectionProperties;
    UA_Variant connectionTransportSettings;

    UA_EventLoop *eventLoop; /* Use an external EventLoop (use the EventLoop of
                              * the server if this is NULL). Propagates to the
                              * ReaderGroup/WriterGroup attached to the
                              * Connection. */
} UA_PubSubConnectionConfig;

#ifdef UA_ENABLE_PUBSUB_MONITORING

typedef enum {
    UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT
    // extend as needed
} UA_PubSubMonitoringType;

/* PubSub monitoring interface */
typedef struct {
    UA_StatusCode (*createMonitoring)(UA_Server *server, UA_NodeId Id,
                                      UA_PubSubComponentEnumType eComponentType,
                                      UA_PubSubMonitoringType eMonitoringType,
                                      void *data, UA_ServerCallback callback);
    UA_StatusCode (*startMonitoring)(UA_Server *server, UA_NodeId Id,
                                     UA_PubSubComponentEnumType eComponentType,
                                     UA_PubSubMonitoringType eMonitoringType, void *data);
    UA_StatusCode (*stopMonitoring)(UA_Server *server, UA_NodeId Id,
                                    UA_PubSubComponentEnumType eComponentType,
                                    UA_PubSubMonitoringType eMonitoringType, void *data);
    UA_StatusCode (*updateMonitoringInterval)(UA_Server *server, UA_NodeId Id,
                                              UA_PubSubComponentEnumType eComponentType,
                                              UA_PubSubMonitoringType eMonitoringType,
                                              void *data);
    UA_StatusCode (*deleteMonitoring)(UA_Server *server, UA_NodeId Id,
                                      UA_PubSubComponentEnumType eComponentType,
                                      UA_PubSubMonitoringType eMonitoringType, void *data);
} UA_PubSubMonitoringInterface;

#endif /* UA_ENABLE_PUBSUB_MONITORING */

/* General PubSub configuration */
struct UA_PubSubConfiguration {
    /* Callback for PubSub component state changes: If provided this callback
     * informs the application about PubSub component state changes. E.g. state
     * change from operational to error in case of a DataSetReader
     * MessageReceiveTimeout. The status code provides additional
     * information. */
    void (*stateChangeCallback)(UA_Server *server, UA_NodeId *id,
                                UA_PubSubState state, UA_StatusCode status);

    UA_Boolean enableDeltaFrames;

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    UA_Boolean enableInformationModelMethods;
#endif

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    /* PubSub security policies */
    size_t securityPoliciesSize;
    UA_PubSubSecurityPolicy *securityPolicies;
#endif

#ifdef UA_ENABLE_PUBSUB_MONITORING
    UA_PubSubMonitoringInterface monitoringInterface;
#endif
};

/* Add a new PubSub connection to the given server and open it.
 * @param server The server to add the connection to.
 * @param connectionConfig The configuration for the newly added connection.
 * @param connectionIdentifier If not NULL will be set to the identifier of the
 *        newly added connection.
 * @return UA_STATUSCODE_GOOD if connection was successfully added, otherwise an
 *         error code. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_addPubSubConnection(UA_Server *server,
                              const UA_PubSubConnectionConfig *connectionConfig,
                              UA_NodeId *connectionId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_enablePubSubConnection(UA_Server *server,
                                 const UA_NodeId connectionId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_disablePubSubConnection(UA_Server *server,
                                  const UA_NodeId connectionId);

/* Returns a deep copy of the config */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_getPubSubConnectionConfig(UA_Server *server,
                                    const UA_NodeId connectionId,
                                    UA_PubSubConnectionConfig *config);

/* Remove Connection, identified by the NodeId. Deletion of Connection
 * removes all contained WriterGroups and Writers. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_removePubSubConnection(UA_Server *server, const UA_NodeId connectionId);

/**
 * PublishedDataSets
 * -----------------
 * The PublishedDataSets (PDS) are containers for the published information. The
 * PDS contain the published variables and meta information. The metadata is
 * commonly autogenerated or given as constant argument as part of the template
 * functions. The template functions are standard defined and intended for
 * configuration tools. You should normally create an empty PDS and call the
 * functions to add new fields. */

/* The UA_PUBSUB_DATASET_PUBLISHEDITEMS has currently no additional members and
 * thus no dedicated config structure. */

typedef enum {
    UA_PUBSUB_DATASET_PUBLISHEDITEMS,
    UA_PUBSUB_DATASET_PUBLISHEDEVENTS,
    UA_PUBSUB_DATASET_PUBLISHEDITEMS_TEMPLATE,
    UA_PUBSUB_DATASET_PUBLISHEDEVENTS_TEMPLATE,
} UA_PublishedDataSetType;

typedef struct {
    UA_DataSetMetaDataType metaData;
    size_t variablesToAddSize;
    UA_PublishedVariableDataType *variablesToAdd;
} UA_PublishedDataItemsTemplateConfig;

typedef struct {
    UA_NodeId eventNotfier;
    UA_ContentFilter filter;
} UA_PublishedEventConfig;

typedef struct {
    UA_DataSetMetaDataType metaData;
    UA_NodeId eventNotfier;
    size_t selectedFieldsSize;
    UA_SimpleAttributeOperand *selectedFields;
    UA_ContentFilter filter;
} UA_PublishedEventTemplateConfig;

/* Configuration structure for PublishedDataSet */
typedef struct {
    UA_String name;
    UA_PublishedDataSetType publishedDataSetType;
    union {
        /* The UA_PUBSUB_DATASET_PUBLISHEDITEMS has currently no additional members
         * and thus no dedicated config structure.*/
        UA_PublishedDataItemsTemplateConfig itemsTemplate;
        UA_PublishedEventConfig event;
        UA_PublishedEventTemplateConfig eventTemplate;
    } config;
} UA_PublishedDataSetConfig;

void UA_EXPORT
UA_PublishedDataSetConfig_clear(UA_PublishedDataSetConfig *pdsConfig);

typedef struct {
    UA_StatusCode addResult;
    size_t fieldAddResultsSize;
    UA_StatusCode *fieldAddResults;
    UA_ConfigurationVersionDataType configurationVersion;
} UA_AddPublishedDataSetResult;

UA_EXPORT UA_AddPublishedDataSetResult UA_THREADSAFE
UA_Server_addPublishedDataSet(UA_Server *server,
                              const UA_PublishedDataSetConfig *publishedDataSetConfig,
                              UA_NodeId *pdsId);

/* Returns a deep copy of the config */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_getPublishedDataSetConfig(UA_Server *server, const UA_NodeId pdsId,
                                    UA_PublishedDataSetConfig *config);

/* Returns a deep copy of the DataSetMetaData for an specific PDS */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_getPublishedDataSetMetaData(UA_Server *server, const UA_NodeId pdsId,
                                      UA_DataSetMetaDataType *metaData);

/* Remove PublishedDataSet, identified by the NodeId. Deletion of PDS removes
 * all contained and linked PDS Fields. Connected WriterGroups will be also
 * removed. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_removePublishedDataSet(UA_Server *server, const UA_NodeId pdsId);

/**
 * DataSetFields
 * -------------
 * The description of published variables is named DataSetField. Each
 * DataSetField contains the selection of one information model node. The
 * DataSetField has additional parameters for the publishing, sampling and error
 * handling process. */

typedef struct{
    UA_ConfigurationVersionDataType configurationVersion;
    UA_String fieldNameAlias;
    UA_Boolean promotedField;
    UA_PublishedVariableDataType publishParameters;

    /* non std. field */
    struct {
        UA_Boolean rtFieldSourceEnabled;
        /* If the rtInformationModelNode is set, the nodeid in publishParameter must point
         * to a node with external data source backend defined
         * */
        UA_Boolean rtInformationModelNode;
        //TODO -> decide if suppress C++ warnings and use 'UA_DataValue * * const staticValueSource;'
        UA_DataValue ** staticValueSource;
    } rtValueSource;
    UA_UInt32 maxStringLength;

} UA_DataSetVariableConfig;

typedef enum {
    UA_PUBSUB_DATASETFIELD_VARIABLE,
    UA_PUBSUB_DATASETFIELD_EVENT
} UA_DataSetFieldType;

typedef struct {
    UA_DataSetFieldType dataSetFieldType;
    union {
        /* events need other config later */
        UA_DataSetVariableConfig variable;
    } field;
} UA_DataSetFieldConfig;

void UA_EXPORT
UA_DataSetFieldConfig_clear(UA_DataSetFieldConfig *dataSetFieldConfig);

typedef struct {
    UA_StatusCode result;
    UA_ConfigurationVersionDataType configurationVersion;
} UA_DataSetFieldResult;

UA_EXPORT UA_DataSetFieldResult UA_THREADSAFE
UA_Server_addDataSetField(UA_Server *server,
                          const UA_NodeId publishedDataSet,
                          const UA_DataSetFieldConfig *fieldConfig,
                          UA_NodeId *fieldId);

/* Returns a deep copy of the config */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_getDataSetFieldConfig(UA_Server *server, const UA_NodeId dsfId,
                                UA_DataSetFieldConfig *config);

UA_EXPORT UA_DataSetFieldResult UA_THREADSAFE
UA_Server_removeDataSetField(UA_Server *server, const UA_NodeId dsfId);

/**
 * Custom Callback Implementation
 * ------------------------------
 * The user can use his own callback implementation for publishing
 * and subscribing. The user must take care of the callback to call for
 * every publishing or subscibing interval */

typedef struct {
    /* User's callback implementation. The user configured base time and timer policy
     * will be provided as an argument to this callback so that the user can
     * implement his callback (thread) considering base time and timer policies */
    UA_StatusCode (*addCustomCallback)(UA_Server *server, UA_NodeId identifier,
                                       UA_ServerCallback callback,
                                       void *data, UA_Double interval_ms,
                                       UA_DateTime *baseTime, UA_TimerPolicy timerPolicy,
                                       UA_UInt64 *callbackId);

    UA_StatusCode (*changeCustomCallback)(UA_Server *server, UA_NodeId identifier,
                                          UA_UInt64 callbackId, UA_Double interval_ms,
                                          UA_DateTime *baseTime, UA_TimerPolicy timerPolicy);

    void (*removeCustomCallback)(UA_Server *server, UA_NodeId identifier, UA_UInt64 callbackId);

} UA_PubSub_CallbackLifecycle;

/**
 * WriterGroup
 * -----------
 * All WriterGroups are created within a PubSubConnection and automatically
 * deleted if the connection is removed. The WriterGroup is primary used as
 * container for :ref:`dsw` and network message settings. The WriterGroup can be
 * imagined as producer of the network messages. The creation of network
 * messages is controlled by parameters like the publish interval, which is e.g.
 * contained in the WriterGroup. */

typedef enum {
    UA_PUBSUB_ENCODING_UADP = 0,
    UA_PUBSUB_ENCODING_JSON = 1,
    UA_PUBSUB_ENCODING_BINARY = 2
} UA_PubSubEncodingType;

/**
 * WriterGroup
 * -----------
 * The message publishing can be configured for realtime requirements. The RT-levels
 * go along with different requirements. The below listed levels can be configured:
 *
 * UA_PUBSUB_RT_NONE -
 * ---> Description: Default "none-RT" Mode
 * ---> Requirements: -
 * ---> Restrictions: -
 * UA_PUBSUB_RT_DIRECT_VALUE_ACCESS (Preview - not implemented)
 * ---> Description: Normally, the latest value for each DataSetField is read out of the information model. Within this RT-mode, the
 * value source of each field configured as static pointer to an DataValue. The publish cycle won't use call the server read function.
 * ---> Requirements: All fields must be configured with a 'staticValueSource'.
 * ---> Restrictions: -
 * UA_PUBSUB_RT_FIXED_LENGTH (Preview - not implemented)
 * ---> Description: All DataSetFields have a known, non-changing length. The server will pre-generate some
 * buffers and use only memcopy operations to generate requested PubSub packages.
 * ---> Requirements: DataSetFields with variable size cannot be used within this mode.
 * ---> Restrictions: The configuration must be frozen and changes are not allowed while the WriterGroup is 'Operational'.
 * UA_PUBSUB_RT_DETERMINISTIC (Preview - not implemented)
 * ---> Description: -
 * ---> Requirements: -
 * ---> Restrictions: -
 *
 * WARNING! For hard real time requirements the underlying system must be rt-capable.
 *
 */
typedef enum {
    UA_PUBSUB_RT_NONE = 0,
    UA_PUBSUB_RT_DIRECT_VALUE_ACCESS = 1,
    UA_PUBSUB_RT_FIXED_SIZE = 2,
    UA_PUBSUB_RT_DETERMINISTIC = 4,
} UA_PubSubRTLevel;

typedef struct {
    UA_String name;
    UA_Boolean enabled;
    UA_UInt16 writerGroupId;
    UA_Duration publishingInterval;
    UA_Double keepAliveTime;
    UA_Byte priority;
    UA_ExtensionObject transportSettings;
    UA_ExtensionObject messageSettings;
    UA_KeyValueMap groupProperties;
    UA_PubSubEncodingType encodingMimeType;
    /* PubSub Manager Callback */
    UA_PubSub_CallbackLifecycle pubsubManagerCallback;
    /* non std. config parameter. maximum count of embedded DataSetMessage in
     * one NetworkMessage */
    UA_UInt16 maxEncapsulatedDataSetMessageCount;
    /* non std. field */
    UA_PubSubRTLevel rtLevel;

    /* Message are encrypted if a SecurityPolicy is configured and the
     * securityMode set accordingly. The symmetric key is a runtime information
     * and has to be set via UA_Server_setWriterGroupEncryptionKey. */
    UA_MessageSecurityMode securityMode; /* via the UA_WriterGroupDataType */
#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    UA_PubSubSecurityPolicy *securityPolicy;
    UA_String securityGroupId;
#endif
} UA_WriterGroupConfig;

void UA_EXPORT
UA_WriterGroupConfig_clear(UA_WriterGroupConfig *writerGroupConfig);

/* Add a new WriterGroup to an existing Connection */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_addWriterGroup(UA_Server *server, const UA_NodeId connection,
                         const UA_WriterGroupConfig *writerGroupConfig,
                         UA_NodeId *wgId);

/* Returns a deep copy of the config */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_getWriterGroupConfig(UA_Server *server, const UA_NodeId wgId,
                               UA_WriterGroupConfig *config);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_updateWriterGroupConfig(UA_Server *server, const UA_NodeId wgId,
                                  const UA_WriterGroupConfig *config);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_WriterGroup_getState(UA_Server *server, const UA_NodeId wgId,
                               UA_PubSubState *state);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_WriterGroup_publish(UA_Server *server, const UA_NodeId wgId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_WriterGroup_lastPublishTimestamp(UA_Server *server, const UA_NodeId wgId,
                                    UA_DateTime *timestamp);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_removeWriterGroup(UA_Server *server, const UA_NodeId wgId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_freezeWriterGroupConfiguration(UA_Server *server,
                                         const UA_NodeId wgId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_unfreezeWriterGroupConfiguration(UA_Server *server,
                                           const UA_NodeId wgId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_enableWriterGroup(UA_Server *server, const UA_NodeId wgId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_disableWriterGroup(UA_Server *server, const UA_NodeId wgId);

#define UA_Server_setWriterGroupOperational(server, wgId)   \
    UA_Server_enableWriterGroup(server, wgId)

#define UA_Server_setWriterGroupDisabled(server, wgId)          \
    UA_Server_disableWriterGroup(server, wgId)

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
/* Set the group key for the message encryption */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_setWriterGroupEncryptionKeys(UA_Server *server, const UA_NodeId wgId,
                                       UA_UInt32 securityTokenId,
                                       const UA_ByteString signingKey,
                                       const UA_ByteString encryptingKey,
                                       const UA_ByteString keyNonce);
#endif

/**
 * .. _dsw:
 *
 * DataSetWriter
 * -------------
 * The DataSetWriters are the glue between the WriterGroups and the
 * PublishedDataSets. The DataSetWriter contain configuration parameters and
 * flags which influence the creation of DataSet messages. These messages are
 * encapsulated inside the network message. The DataSetWriter must be linked
 * with an existing PublishedDataSet and be contained within a WriterGroup. */

typedef struct {
    UA_String name;
    UA_UInt16 dataSetWriterId;
    UA_DataSetFieldContentMask dataSetFieldContentMask;
    UA_UInt32 keyFrameCount;
    UA_ExtensionObject messageSettings;
    UA_ExtensionObject transportSettings;
    UA_String dataSetName;
    UA_KeyValueMap dataSetWriterProperties;
} UA_DataSetWriterConfig;

void UA_EXPORT
UA_DataSetWriterConfig_clear(UA_DataSetWriterConfig *pdsConfig);

/* Add a new DataSetWriter to an existing WriterGroup. The DataSetWriter must be
 * coupled with a PublishedDataSet on creation.
 *
 * Part 14, 7.1.5.2.1 defines: The link between the PublishedDataSet and
 * DataSetWriter shall be created when an instance of the DataSetWriterType is
 * created. */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_addDataSetWriter(UA_Server *server,
                           const UA_NodeId writerGroup, const UA_NodeId dataSet,
                           const UA_DataSetWriterConfig *dataSetWriterConfig,
                           UA_NodeId *dswId);

/* Returns a deep copy of the config */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_getDataSetWriterConfig(UA_Server *server, const UA_NodeId dswId,
                                 UA_DataSetWriterConfig *config);

UA_EXPORT UA_StatusCode  UA_THREADSAFE
UA_Server_enableDataSetWriter(UA_Server *server, const UA_NodeId dswId);

UA_EXPORT UA_StatusCode  UA_THREADSAFE
UA_Server_disableDataSetWriter(UA_Server *server, const UA_NodeId dswId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_DataSetWriter_getState(UA_Server *server, const UA_NodeId dswId,
                                 UA_PubSubState *state);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_removeDataSetWriter(UA_Server *server, const UA_NodeId dswId);

/**
 * SubscribedDataSet
 * -----------------
 * SubscribedDataSet describes the processing of the received DataSet.
 * SubscribedDataSet defines which field in the DataSet is mapped to which
 * Variable in the OPC UA Application. SubscribedDataSet has two sub-types
 * called the TargetVariablesType and SubscribedDataSetMirrorType.
 * SubscribedDataSetMirrorType is currently not supported. SubscribedDataSet is
 * set to TargetVariablesType and then the list of target Variables are created
 * in the Subscriber AddressSpace. TargetVariables are a list of variables that
 * are to be added in the Subscriber AddressSpace. It defines a list of Variable
 * mappings between received DataSet fields and added Variables in the
 * Subscriber AddressSpace. */

/* SubscribedDataSetDataType Definition */
typedef enum {
    UA_PUBSUB_SDS_TARGET,
    UA_PUBSUB_SDS_MIRROR
} UA_SubscribedDataSetEnumType;

typedef struct {
    /* Standard-defined FieldTargetDataType */
    UA_FieldTargetDataType targetVariable;

    /* If realtime-handling is required, set this pointer non-NULL and it will be used
     * to memcpy the value instead of using the Write service.
     * If the beforeWrite method pointer is set, it will be called before a memcpy update
     * to the value. But param externalDataValue already contains the new value.
     * If the afterWrite method pointer is set, it will be called after a memcpy update
     * to the value. */
    UA_DataValue **externalDataValue;
    void *targetVariableContext; /* user-defined pointer */
    void (*beforeWrite)(UA_Server *server,
                        const UA_NodeId *readerIdentifier,
                        const UA_NodeId *readerGroupIdentifier,
                        const UA_NodeId *targetVariableIdentifier,
                        void *targetVariableContext,
                        UA_DataValue **externalDataValue);
    void (*afterWrite)(UA_Server *server,
                       const UA_NodeId *readerIdentifier,
                       const UA_NodeId *readerGroupIdentifier,
                       const UA_NodeId *targetVariableIdentifier,
                       void *targetVariableContext,
                       UA_DataValue **externalDataValue);
} UA_FieldTargetVariable;

typedef struct {
    size_t targetVariablesSize;
    UA_FieldTargetVariable *targetVariables;
} UA_TargetVariables;

/* Return Status Code after creating TargetVariables in Subscriber AddressSpace */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_DataSetReader_createTargetVariables(UA_Server *server, const UA_NodeId dsrId,
                                              size_t targetVariablesSize,
                                              const UA_FieldTargetVariable *targetVariables);

/* To Do:Implementation of SubscribedDataSetMirrorType
 * UA_StatusCode
 * A_PubSubDataSetReader_createDataSetMirror(UA_Server *server, UA_NodeId dataSetReaderIdentifier,
 * UA_SubscribedDataSetMirrorDataType* mirror) */

/**
 * DataSetReader
 * -------------
 * DataSetReader can receive NetworkMessages with the DataSetMessage
 * of interest sent by the Publisher. DataSetReaders represent
 * the configuration necessary to receive and process DataSetMessages
 * on the Subscriber side. DataSetReader must be linked with a
 * SubscribedDataSet and be contained within a ReaderGroup. */

typedef enum {
    UA_PUBSUB_RT_UNKNOWN = 0,
    UA_PUBSUB_RT_VARIANT = 1,
    UA_PUBSUB_RT_DATA_VALUE = 2,
    UA_PUBSUB_RT_RAW = 4,
} UA_PubSubRtEncoding;

/* Parameters for PubSub DataSetReader Configuration */
typedef struct {
    UA_String name;
    UA_PublisherId publisherId;
    UA_UInt16 writerGroupId;
    UA_UInt16 dataSetWriterId;
    UA_DataSetMetaDataType dataSetMetaData;
    UA_DataSetFieldContentMask dataSetFieldContentMask;
    UA_Double messageReceiveTimeout;
    UA_ExtensionObject messageSettings;
    UA_ExtensionObject transportSettings;
    UA_SubscribedDataSetEnumType subscribedDataSetType;
    /* TODO UA_SubscribedDataSetMirrorDataType subscribedDataSetMirror */
    union {
        UA_TargetVariables subscribedDataSetTarget;
        // UA_SubscribedDataSetMirrorDataType subscribedDataSetMirror;
    } subscribedDataSet;
    /* non std. fields */
    UA_String linkedStandaloneSubscribedDataSetName;
    UA_PubSubRtEncoding expectedEncoding;
} UA_DataSetReaderConfig;

UA_EXPORT UA_StatusCode
UA_DataSetReaderConfig_copy(const UA_DataSetReaderConfig *src,
                            UA_DataSetReaderConfig *dst);

UA_EXPORT void
UA_DataSetReaderConfig_clear(UA_DataSetReaderConfig *cfg);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_DataSetReader_updateConfig(UA_Server *server, const UA_NodeId dsrId,
                                     UA_NodeId readerGroupIdentifier,
                                     const UA_DataSetReaderConfig *config);

/* Get the configuration (deep copy) of the DataSetReader */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_DataSetReader_getConfig(UA_Server *server, const UA_NodeId dsrId,
                                  UA_DataSetReaderConfig *config);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_DataSetReader_getState(UA_Server *server, UA_NodeId dsrId,
                                 UA_PubSubState *state);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_enableDataSetReader(UA_Server *server, const UA_NodeId dsrId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_disableDataSetReader(UA_Server *server, const UA_NodeId dsrId);

typedef struct {
    UA_String name;
    UA_SubscribedDataSetEnumType subscribedDataSetType;
    union {
        /* datasetmirror is currently not implemented */
        UA_TargetVariablesDataType target;
    } subscribedDataSet;
    UA_DataSetMetaDataType dataSetMetaData;
    UA_Boolean isConnected;
} UA_StandaloneSubscribedDataSetConfig;

void
UA_StandaloneSubscribedDataSetConfig_clear(UA_StandaloneSubscribedDataSetConfig *sdsConfig);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_addStandaloneSubscribedDataSet(UA_Server *server,
                                         const UA_StandaloneSubscribedDataSetConfig *sdsConfig,
                                         UA_NodeId *sdsId);

/* Remove StandaloneSubscribedDataSet, identified by the NodeId. */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_removeStandaloneSubscribedDataSet(UA_Server *server,
                                            const UA_NodeId sdsId);

/**
 * ReaderGroup
 * -----------
 *
 * ReaderGroup is used to group a list of DataSetReaders. All ReaderGroups are
 * created within a PubSubConnection and automatically deleted if the connection
 * is removed. All network message related filters are only available in the
 * DataSetReader.
 *
 * The RT-levels go along with different requirements. The below listed levels
 * can be configured for a ReaderGroup.
 *
 * - UA_PUBSUB_RT_NONE: RT applied to this level
 * - PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS: Extends PubSub RT functionality and
 *   implements fast path message decoding in the Subscriber. Uses a buffered
 *   network message and only decodes the necessary offsets stored in an offset
 *   buffer. */

/* ReaderGroup configuration */
typedef struct {
    UA_String name;

    /* non std. field */
    UA_PubSubRTLevel rtLevel;
    UA_KeyValueMap groupProperties;
    UA_PubSubEncodingType encodingMimeType;
    UA_ExtensionObject transportSettings;

    /* Messages are decrypted if a SecurityPolicy is configured and the
     * securityMode set accordingly. The symmetric key is a runtime information
     * and has to be set via UA_Server_setReaderGroupEncryptionKey. */
    UA_MessageSecurityMode securityMode;
#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    UA_PubSubSecurityPolicy *securityPolicy;
    UA_String securityGroupId;
#endif
} UA_ReaderGroupConfig;

void UA_EXPORT
UA_ReaderGroupConfig_clear(UA_ReaderGroupConfig *readerGroupConfig);

/* Add DataSetReader to the ReaderGroup */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_addDataSetReader(UA_Server *server, UA_NodeId readerGroupIdentifier,
                           const UA_DataSetReaderConfig *dataSetReaderConfig,
                           UA_NodeId *readerIdentifier);

/* Remove DataSetReader from ReaderGroup */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_removeDataSetReader(UA_Server *server, UA_NodeId readerIdentifier);

/* To Do: Update Configuration of ReaderGroup
 * UA_StatusCode UA_EXPORT
 * UA_Server_ReaderGroup_updateConfig(UA_Server *server, UA_NodeId readerGroupIdentifier,
 *                                    const UA_ReaderGroupConfig *config);
 */

/* Get configuration of ReaderGroup (deep copy) */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_ReaderGroup_getConfig(UA_Server *server, const UA_NodeId rgId,
                                UA_ReaderGroupConfig *config);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_ReaderGroup_getState(UA_Server *server, const UA_NodeId rgId,
                               UA_PubSubState *state);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_addReaderGroup(UA_Server *server, const UA_NodeId connectionId,
                         const UA_ReaderGroupConfig *readerGroupConfig,
                         UA_NodeId *readerGroupIdentifier);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_removeReaderGroup(UA_Server *server, const UA_NodeId rgId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_freezeReaderGroupConfiguration(UA_Server *server, const UA_NodeId rgId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_unfreezeReaderGroupConfiguration(UA_Server *server, const UA_NodeId rgId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_enableReaderGroup(UA_Server *server, const UA_NodeId rgId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_disableReaderGroup(UA_Server *server, const UA_NodeId rgId);

#define UA_Server_setReaderGroupOperational(server, rgId) \
    UA_Server_enableReaderGroup(server, rgId)

#define UA_Server_setReaderGroupDisabled(server, rgId) \
    UA_Server_disableReaderGroup(server, rgId)

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
/* Set the group key for the message encryption */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_setReaderGroupEncryptionKeys(UA_Server *server, UA_NodeId readerGroup,
                                       UA_UInt32 securityTokenId,
                                       UA_ByteString signingKey,
                                       UA_ByteString encryptingKey,
                                       UA_ByteString keyNonce);
#endif

#ifdef UA_ENABLE_PUBSUB_SKS

/**
 * SecurityGroup
 * -------------
 *
 * A SecurityGroup is an abstraction that represents the message security settings and
 * security keys for a subset of NetworkMessages exchanged between Publishers and
 * Subscribers. The SecurityGroup objects are created on a Security Key Service (SKS). The
 * SKS manages the access to the keys based on the role permission for a user assigned to
 * a SecurityGroup Object. A SecurityGroup is identified with a unique identifier called
 * the SecurityGroupId. It is unique within the SKS.
 *
 * .. note:: The access to the SecurityGroup and therefore the securitykeys managed by SKS
 *           requires management of Roles and Permissions in the SKS. The Role Permission
 *           model is not supported at the time of writing. However, the access control plugin can
 *           be used to create and manage role permission on SecurityGroup object.
 */

typedef struct {
    UA_String securityGroupName;
    UA_Duration keyLifeTime;
    UA_String securityPolicyUri;
    UA_UInt32 maxFutureKeyCount;
    UA_UInt32 maxPastKeyCount;
} UA_SecurityGroupConfig;

/**
 * @brief Creates a SecurityGroup object and add it to the list in PubSub Manager. If the
 * information model is enabled then the SecurityGroup object Node is also created in the
 * server. A keyStorage with initial list of keys is created with a SecurityGroup. A
 * callback is added to new SecurityGroup which updates the keys periodically at each
 * KeyLifeTime expire.
 *
 * @param server The server instance
 * @param securityGroupFolderNodeId The parent node of the SecurityGroup. It must be of
 * SecurityGroupFolderType
 * @param securityGroupConfig The security settings of a SecurityGroup
 * @param securityGroupNodeId The output nodeId of the new SecurityGroup
 * @return UA_StatusCode The return status code
 */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_addSecurityGroup(UA_Server *server, UA_NodeId securityGroupFolderNodeId,
                           const UA_SecurityGroupConfig *securityGroupConfig,
                           UA_NodeId *securityGroupNodeId);

/**
 * @brief Removes the SecurityGroup from PubSub Manager. It removes the KeyStorage
 * associated with the SecurityGroup from the server.
 *
 * @param server The server instance
 * @param securityGroup The nodeId of the securityGroup to be removed
 * @return UA_StatusCode The returned status code.
 */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_removeSecurityGroup(UA_Server *server, const UA_NodeId securityGroup);

/**
 * @brief This is a repeated callback which is triggered on each iteration of SKS Pull request.
 * The server uses this callback to notify user about the status of current Pull request iteration.
 * The period is calculated based on the KeylifeTime of specified in the SecurityGroup object node on
 * the SKS server.
 *
 * @param server The server instance managing the publisher/subscriber.
 * @param sksPullRequestStatus The current status of sks pull request.
 * @param context The pointer to user defined data passed to this callback.
 */
typedef void
(*UA_Server_sksPullRequestCallback)(UA_Server *server, UA_StatusCode sksPullRequestStatus, void* context);

/**
 * @brief Sets the SKS client config used to call the GetSecurityKeys Method on SKS and get the
 * initial set of keys for a SecurityGroupId and adds timedCallback for the next GetSecurityKeys
 * method Call. This uses async Client API for SKS Pull request. The SKS Client instance is created and destroyed at
 * runtime on each iteration of SKS Pull request by the server. The key Rollover mechanism will check if the new
 * keys are needed then it will call the getSecurityKeys Method on SKS Server. At the end of SKS Pull request
 * iteration, the sks client will be deleted by a delayed callback (in next server iteration).
 *
 * @note It is be called before setting Reader/Writer Group into Operational because this also allocates
 * a channel context for the pubsub security policy.
 *
 * @note the stateCallback of sksClientConfig will be overwritten by an internal callback.
 *
 * @param server the server instance
 * @param clientConfig holds the required configuration to make encrypted connection with
 * SKS Server. The input client config takes the lifecycle as long as SKS request are made.
 * It is deleted with its plugins when the server is deleted or the last Reader/Writer
 * Group of the securityGroupId is deleted. The input config is copied to an internal
 * config object and the content of input config object will be reset to zero.
 * @param endpointUrl holds the endpointUrl of the SKS server
 * @param securityGroupId the SecurityGroupId of the securityGroup on SKS and
 * reader/writergroups
 * @param callback the user defined callback to notify the user about the status of SKS
 * Pull request.
 * @param context passed to the callback function
 * @return UA_StatusCode the retuned status
 */
UA_StatusCode UA_EXPORT
UA_Server_setSksClient(UA_Server *server, UA_String securityGroupId,
                       UA_ClientConfig *clientConfig, const char *endpointUrl,
                       UA_Server_sksPullRequestCallback callback, void *context);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_setReaderGroupActivateKey(UA_Server *server, const UA_NodeId readerGroupId);

UA_EXPORT UA_StatusCode  UA_THREADSAFE
UA_Server_setWriterGroupActivateKey(UA_Server *server, const UA_NodeId writerGroup);

#endif /* UA_ENABLE_PUBSUB_SKS */

#endif /* UA_ENABLE_PUBSUB */

_UA_END_DECLS

#endif /* UA_SERVER_PUBSUB_H */
