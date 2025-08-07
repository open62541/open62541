/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2025 Fraunhofer IOSB (Author: Andreas Ebner)
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
 * The figure below shows how the PubSub components are related.
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
 * PublisherId
 * -----------
 * Valid PublisherId types are defined in Part 14, 7.2.2.2.2 NetworkMessage. The
 * PublisherId is sometimes encoded as a variant in the standard (e.g. in some
 * configuration structures). We do however use our own tagged-union structure
 * where we can. */

typedef enum {
    UA_PUBLISHERIDTYPE_BYTE   = 0, /* 000 Byte (default) */
    UA_PUBLISHERIDTYPE_UINT16 = 1, /* 001 UInt16 */
    UA_PUBLISHERIDTYPE_UINT32 = 2, /* 010 UInt32 */
    UA_PUBLISHERIDTYPE_UINT64 = 3, /* 011 UInt64 */
    UA_PUBLISHERIDTYPE_STRING = 4  /* 100 String */
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

/**
 * PubSub Components
 * -----------------
 * A PubSubComponent is either a PubSubConnection, DataSetReader, ReaderGroup,
 * DataSetWriter, WriterGroup, PublishedDataSet or SubscribedDataSet. The
 * PubSubComponents are represented in the information model (if that is enabled
 * for the server). In the C-API they are identified by a unique NodeId that is
 * assigned during creation. */

typedef enum  {
    UA_PUBSUBCOMPONENT_CONNECTION  = 0,
    UA_PUBSUBCOMPONENT_WRITERGROUP  = 1,
    UA_PUBSUBCOMPONENT_DATASETWRITER  = 2,
    UA_PUBSUBCOMPONENT_READERGROUP  = 3,
    UA_PUBSUBCOMPONENT_DATASETREADER  = 4,
    UA_PUBSUBCOMPONENT_PUBLISHEDDATASET  = 5,
    UA_PUBSUBCOMPONENT_SUBSCRIBEDDDATASET = 6,
} UA_PubSubComponentType;

/**
 * The datasets are static "configuration containers". The other
 * PubSubComponents are active and have a state machine governing their runtime
 * behavior and state transitions. The state machine API (in C and in the
 * information model) exposes only ``_enable`` and ``_disable`` methods for the
 * different PubSubComponents. Their actual state is more detailed and emerges
 * from the internal behavior. This ``UA_PubSubState`` defines five possible
 * states, part 14 contains a diagram for the possible state transitions:
 *
 * - DISABLED
 * - PAUSED
 * - OPERATIONAL
 * - ERROR
 * - PREOPERATIONAL
 *
 * In open62541 we classify all PubSubStates as either *enabled* or *disabled*.
 * The disabled states are DISABLED and ERROR. These need to be manually enabled
 * to trigger a state change. All other states are enabled and "want to become
 * OPERATIONAL". The state machine triggers internally to automatically reach
 * the OPERATIONAL state when the external conditions allow it. The
 * PREOPERATIONAL state indicates that necessary measures to become OPERATIONAL
 * have been taken, but the OPERATIONAL state has not yet been achieved. For
 * example, a ReaderGroup only becomes OPERATIONAL, once the first message for
 * it has been received.
 *
 * Notably, the state machines of the PubSubComponents are cascading. That is,
 * the state depends on the state of the parent component. For example, an
 * OPERATIONAL WriterGroup becomes PAUSED when its parent PubSubConnection goes
 * to DISABLED. The PAUSED WriterGroup automatically returns to OPERATIONAL once
 * the parent PubSubConnection becomes OPERATIONAL again. */

/* Enable all PubSub components. They are triggered in the following order:
 * DataSetWriter, WriterGroups, DataSetReader, ReaderGroups, PubSubConnections.
 * Returns the ORed statuscodes from enabling the individual components. */
UA_EXPORT UA_StatusCode
UA_Server_enableAllPubSubComponents(UA_Server *server);

/* Disable all PubSubComponents (same order as for _enableAll) */
UA_EXPORT void
UA_Server_disableAllPubSubComponents(UA_Server *server);

/**
 * The default implementation of the state machines manages resources via the
 * configured EventLoop. Most notably these are connections (sockets) and timed
 * callbacks. A *custom state machine* can be configured when these resources
 * needto be managed outside of the EventLoop. An example are realtime-capable
 * connections that should not end up in the same ``select`` syscall as the
 * server TCP connections. Custom state machines are set in the configuration
 * structure of the individual PubSubComponents. The custom state machine only
 * needs to handle the state of its "local" PubSubComponent. The open62541
 * implementation automatically triggers the child PubSubComponents after a
 * state change.
 *
 * The following definitions are part of the configuration for all active
 * PubSubComponents (not the dataset PubSubComponents). */

#define UA_PUBSUBCOMPONENT_COMMON                                     \
    UA_String name; /* For logging and in the information model */    \
    void *context;  /* Custom pointer forwarded to callbacks */       \
    UA_Boolean enabled; /* Component is auto-enabled at creation */   \
                                                                      \
    /* The custom state machine callback is optional (can be */       \
    /* NULL). It gets called with a request to change the state to */ \
    /* targetState. The state pointer has the old (and afterwards */  \
    /* the new) state. When the state machine returns a bad */        \
    /* statuscode, the state must be set to ERROR before. */          \
    UA_StatusCode (*customStateMachine)(UA_Server *server,            \
                                        const UA_NodeId componentId,  \
                                        void *componentContext,       \
                                        UA_PubSubState *state,        \
                                        UA_PubSubState targetState);  \

/**
 * Global PubSub Configuration
 * ---------------------------
 * The following PubSub configuration structure is part of the server-config.
 * It configures behavior that is valid for all PubSubComponents. */

typedef struct {
    /* Notify the application when a new PubSubComponent is added or removed.
     * That way it is possible to keep track of the changes from the methods in
     * the information model.
     *
     * When the return StatusCode is not good, then adding/removing the
     * component is aborted. When a component is added, it is possible to call
     * the public API _getConfig and _updateConfig methods on it from within the
     * lifecycle callback. */
    UA_StatusCode
    (*componentLifecycleCallback)(UA_Server *server, const UA_NodeId id,
                                  const UA_PubSubComponentType componentType,
                                  UA_Boolean remove);

    /* The callback is executed first thing in the state machine. The component
     * config can be modified from within the beforeStateChangeCallback if the
     * component is not enabled. Also the TargetState can be changed. For
     * example to prevent a component that is not ready from getting enabled. */
    void (*beforeStateChangeCallback)(UA_Server *server, const UA_NodeId id,
                                      UA_PubSubState *targetState);

    /* Callback to notify the application about PubSub component state changes.
     * The status code provides additional information. */
    void (*stateChangeCallback)(UA_Server *server, const UA_NodeId id,
                                UA_PubSubState state, UA_StatusCode status);

    UA_Boolean enableDeltaFrames;

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    UA_Boolean enableInformationModelMethods;
#endif

    /* PubSub security policies */
    size_t securityPoliciesSize;
    UA_PubSubSecurityPolicy *securityPolicies;
} UA_PubSubConfiguration;

/**
 * PubSubConnection
 * ----------------
 * PubSubConnections are the abstraction between the concrete transport protocol
 * and the PubSub functionality. It is possible to create multiple
 * PubSubConnections with (possibly) different transport protocols at
 * runtime. */

typedef struct {
    UA_PUBSUBCOMPONENT_COMMON
    /* Configuration parameters from PubSubConnectionDataType */
    UA_PublisherId publisherId;
    UA_String transportProfileUri;
    UA_Variant address;
    UA_KeyValueMap connectionProperties;
    UA_Variant connectionTransportSettings;
} UA_PubSubConnectionConfig;

UA_EXPORT UA_StatusCode
UA_PubSubConnectionConfig_copy(const UA_PubSubConnectionConfig *src,
                               UA_PubSubConnectionConfig *dst);

UA_EXPORT void
UA_PubSubConnectionConfig_clear(UA_PubSubConnectionConfig *cfg);

/* Add a PubSub connection to the server. The connectionId can be NULL. If
 * defined, it is set to the NodeId of the created PubSubConnection. */
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

/* Manually "inject" a packet as if it had been received by the
 * PubSubConnection. This is intended to be used in combination with a custom
 * state machine where sockets (connections) are handled by user code. */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_processPubSubConnectionReceive(UA_Server *server,
                                         const UA_NodeId connectionId,
                                         const UA_ByteString packet);

/* Returns a deep copy of the config */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_getPubSubConnectionConfig(UA_Server *server,
                                    const UA_NodeId connectionId,
                                    UA_PubSubConnectionConfig *config);

/* The PubSubConnection must be disabled to update the config */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_updatePubSubConnectionConfig(UA_Server *server,
                                       const UA_NodeId connectionId,
                                       const UA_PubSubConnectionConfig *config);

/* Deletion of a PubSubConnection removes all "below" WriterGroups and
 * ReaderGroups. This can fail if the PubSubConnection is enabled. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_removePubSubConnection(UA_Server *server,
                                 const UA_NodeId connectionId);

/**
 * PublishedDataSet
 * ----------------
 * The PublishedDataSets (PDS) are containers for the published information. The
 * PDS contain the published variables and meta information. The metadata is
 * commonly autogenerated or given as constant argument as part of the template
 * functions. The template functions are standard defined and intended for
 * configuration tools. You should normally create an empty PDS and call the
 * functions to add new fields. */

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
        /* The UA_PUBSUB_DATASET_PUBLISHEDITEMS has currently no additional
         * members and thus no dedicated config structure.*/
        UA_PublishedDataItemsTemplateConfig itemsTemplate;
        UA_PublishedEventConfig event;
        UA_PublishedEventTemplateConfig eventTemplate;
    } config;

    void *context; /* Context Configuration (PublishedDataSet has no state
                    * machine) */
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
                              const UA_PublishedDataSetConfig *pdsConfig,
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
 * DataSetField
 * ------------
 * The description of published variables is named DataSetField. Each
 * DataSetField contains the selection of one information model node. The
 * DataSetField has additional parameters for the publishing, sampling and error
 * handling process. */

typedef struct {
    UA_ConfigurationVersionDataType configurationVersion;
    UA_String fieldNameAlias;
    UA_Boolean promotedField;
    UA_PublishedVariableDataType publishParameters;

    UA_UInt32 maxStringLength;
    UA_LocalizedText description;
    /* If dataSetFieldId is not set, the GUID will be generated on adding the
     * field */
    UA_Guid dataSetFieldId;
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
    UA_PUBSUB_ENCODING_JSON
} UA_PubSubEncodingType;

typedef struct {
    UA_PUBSUBCOMPONENT_COMMON
    UA_UInt16 writerGroupId;
    UA_Duration publishingInterval;
    UA_Double keepAliveTime;
    UA_Byte priority;
    UA_ExtensionObject transportSettings;
    UA_ExtensionObject messageSettings;
    UA_KeyValueMap groupProperties;
    UA_PubSubEncodingType encodingMimeType;

    /* non std. config parameter. maximum count of embedded DataSetMessage in
     * one NetworkMessage */
    UA_UInt16 maxEncapsulatedDataSetMessageCount;

    /* Security Configuration
     * Message are encrypted if a SecurityPolicy is configured and the
     * securityMode set accordingly. The symmetric key is a runtime information
     * and has to be set via UA_Server_setWriterGroupEncryptionKey. */
    UA_MessageSecurityMode securityMode; /* via the UA_WriterGroupDataType */
    UA_PubSubSecurityPolicy *securityPolicy;
    UA_String securityGroupId;
} UA_WriterGroupConfig;

void UA_EXPORT
UA_WriterGroupConfig_clear(UA_WriterGroupConfig *writerGroupConfig);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_addWriterGroup(UA_Server *server, const UA_NodeId connection,
                         const UA_WriterGroupConfig *writerGroupConfig,
                         UA_NodeId *wgId);

/* Returns a deep copy of the config */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_getWriterGroupConfig(UA_Server *server, const UA_NodeId wgId,
                               UA_WriterGroupConfig *config);

/* The WriterGroup must be disabled to update the config */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_updateWriterGroupConfig(UA_Server *server, const UA_NodeId wgId,
                                  const UA_WriterGroupConfig *config);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_getWriterGroupState(UA_Server *server, const UA_NodeId wgId,
                              UA_PubSubState *state);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_triggerWriterGroupPublish(UA_Server *server,
                                    const UA_NodeId wgId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_getWriterGroupLastPublishTimestamp(UA_Server *server,
                                             const UA_NodeId wgId,
                                             UA_DateTime *timestamp);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_removeWriterGroup(UA_Server *server, const UA_NodeId wgId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_enableWriterGroup(UA_Server *server, const UA_NodeId wgId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_disableWriterGroup(UA_Server *server, const UA_NodeId wgId);

/* Set the group key for the message encryption */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_setWriterGroupEncryptionKeys(UA_Server *server, const UA_NodeId wgId,
                                       UA_UInt32 securityTokenId,
                                       const UA_ByteString signingKey,
                                       const UA_ByteString encryptingKey,
                                       const UA_ByteString keyNonce);

/* Legacy API */
#define UA_Server_setWriterGroupOperational(server, wgId) \
    UA_Server_enableWriterGroup(server, wgId)
#define UA_Server_setWriterGroupDisabled(server, wgId) \
    UA_Server_disableWriterGroup(server, wgId)
#define UA_Server_WriterGroup_getState(server, wgId, state) \
    UA_Server_getWriterGroupState(server, wgId, state)
#define UA_WriterGroup_lastPublishTimestamp(server, wgId, timestamp) \
    UA_Server_getWriterGroupLastPublishTimestamp(server, wgId, timestamp)
#define UA_Server_WriterGroup_publish(server, wgId) \
    UA_Server_triggerWriterGroupPublish(server, wgId)

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
    UA_PUBSUBCOMPONENT_COMMON
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

/* The DataSetWriter must be disabled to update the config */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_updateDataSetWriterConfig(UA_Server *server, const UA_NodeId dswId,
                                    const UA_DataSetWriterConfig *config);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_enableDataSetWriter(UA_Server *server, const UA_NodeId dswId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_disableDataSetWriter(UA_Server *server, const UA_NodeId dswId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_getDataSetWriterState(UA_Server *server, const UA_NodeId dswId,
                                UA_PubSubState *state);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_removeDataSetWriter(UA_Server *server, const UA_NodeId dswId);

/* Legacy API */
#define UA_Server_DataSetWriter_getState(server, dswId, state) \
    UA_Server_getDataSetWriterState(server, dswId, state)

/**
 * SubscribedDataSet
 * -----------------
 * With the OPC UA Part 14 1.0.5, the concept of StandaloneSubscribedDataSet
 * (SSDS) was introduced. The SSDS is the counterpart to the PublishedDataSet
 * and has its own lifecycle. The SSDS can be connected to exactly one
 * DataSetReader. In general, the SSDS is optional and a DataSetReader can still
 * be defined without referencing a SSDS.
 *
 * The SubscribedDataSet has two sub-types called the TargetVariablesType and
 * SubscribedDataSetMirrorType. SubscribedDataSetMirrorType is currently not
 * supported. SubscribedDataSet is set to TargetVariablesType and then the list
 * of target Variables are created in the Subscriber AddressSpace.
 * TargetVariables are a list of variables that are to be added in the
 * Subscriber AddressSpace. It defines a list of Variable mappings between
 * received DataSet fields and added Variables in the Subscriber
 * AddressSpace. */

typedef enum {
    UA_PUBSUB_SDS_TARGET,
    UA_PUBSUB_SDS_MIRROR
} UA_SubscribedDataSetType;

typedef struct {
    UA_String name;
    UA_SubscribedDataSetType subscribedDataSetType;
    union {
        /* DataSetMirror is currently not implemented */
        UA_TargetVariablesDataType target;
    } subscribedDataSet;
    UA_DataSetMetaDataType dataSetMetaData;

    void *context; /* Context Configuration (SubscribedDataSet has no state
                    * machine) */
} UA_SubscribedDataSetConfig;

UA_EXPORT void
UA_SubscribedDataSetConfig_clear(UA_SubscribedDataSetConfig *sdsConfig);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_addSubscribedDataSet(UA_Server *server,
                               const UA_SubscribedDataSetConfig *sdsConfig,
                               UA_NodeId *sdsId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_removeSubscribedDataSet(UA_Server *server, const UA_NodeId sdsId);

/* TODO: Implementation of SubscribedDataSetMirrorType */

/**
 * DataSetReader
 * -------------
 * DataSetReader can receive NetworkMessages with the DataSetMessage
 * of interest sent by the Publisher. DataSetReaders represent
 * the configuration necessary to receive and process DataSetMessages
 * on the Subscriber side. DataSetReader must be linked with a
 * SubscribedDataSet and be contained within a ReaderGroup. */

typedef struct {
    UA_PUBSUBCOMPONENT_COMMON
    UA_PublisherId publisherId;
    UA_UInt16 writerGroupId;
    UA_UInt16 dataSetWriterId;
    UA_DataSetMetaDataType dataSetMetaData;
    UA_DataSetFieldContentMask dataSetFieldContentMask;
    UA_Double messageReceiveTimeout; /* The maximum interval (in milliseconds)
                                      * after which we want to receive a
                                      * message. Gets reset after every received
                                      * message. If <= 0.0, then no timeout is
                                      * configured. */
    UA_ExtensionObject messageSettings;
    UA_ExtensionObject transportSettings;
    UA_SubscribedDataSetType subscribedDataSetType;
    union {
        /* TODO: UA_SubscribedDataSetMirrorDataType subscribedDataSetMirror */
        UA_TargetVariablesDataType target;
    } subscribedDataSet;
    /* non std. fields */
    UA_String linkedStandaloneSubscribedDataSetName;
} UA_DataSetReaderConfig;

UA_EXPORT UA_StatusCode
UA_DataSetReaderConfig_copy(const UA_DataSetReaderConfig *src,
                            UA_DataSetReaderConfig *dst);

UA_EXPORT void
UA_DataSetReaderConfig_clear(UA_DataSetReaderConfig *cfg);

/* Get the configuration (deep copy) of the DataSetReader */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_getDataSetReaderConfig(UA_Server *server, const UA_NodeId dsrId,
                                 UA_DataSetReaderConfig *config);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_getDataSetReaderState(UA_Server *server, const UA_NodeId dsrId,
                                UA_PubSubState *state);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_addDataSetReader(UA_Server *server, UA_NodeId readerGroupId,
                           const UA_DataSetReaderConfig *config,
                           UA_NodeId *dsrId);

/* The DataSetReader must be disabled to update the config */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_updateDataSetReaderConfig(UA_Server *server,
                                    const UA_NodeId dsrId,
                                    const UA_DataSetReaderConfig *config);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_removeDataSetReader(UA_Server *server, const UA_NodeId dsrId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_enableDataSetReader(UA_Server *server, const UA_NodeId dsrId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_disableDataSetReader(UA_Server *server, const UA_NodeId dsrId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_setDataSetReaderTargetVariables(
    UA_Server *server, const UA_NodeId dsrId,
    size_t targetVariablesSize,
    const UA_FieldTargetDataType *targetVariables);

/* Legacy API */
#define UA_Server_DataSetReader_getConfig(server, dsrId, config) \
    UA_Server_getDataSetReaderConfig(server, dsrId, config)
#define UA_Server_DataSetReader_getState(server, dsrId, state) \
    UA_Server_getDataSetReaderState(server, dsrId, state)
#define UA_Server_DataSetReader_createTargetVariables(server, dsrId, \
                                                      tvsSize, tvs)  \
    UA_Server_setDataSetReaderTargetVariables(server, dsrId, tvsSize, tvs)

/**
 * ReaderGroup
 * -----------
 * ReaderGroups contain a list of DataSetReaders. All ReaderGroups are
 * created within a PubSubConnection and automatically deleted if the connection
 * is removed. All network message related filters are only available in the
 * DataSetReader.
 *
 * The RT-levels go along with different requirements. The below listed levels
 * can be configured for a ReaderGroup. */

typedef struct {
    UA_PUBSUBCOMPONENT_COMMON

    /* non std. fields */
    UA_KeyValueMap groupProperties;
    UA_PubSubEncodingType encodingMimeType;
    UA_ExtensionObject transportSettings;

    /* Messages are decrypted if a SecurityPolicy is configured and the
     * securityMode set accordingly. The symmetric key is a runtime information
     * and has to be set via UA_Server_setReaderGroupEncryptionKey. */
    UA_MessageSecurityMode securityMode;
    UA_PubSubSecurityPolicy *securityPolicy;
    UA_String securityGroupId;
} UA_ReaderGroupConfig;

void UA_EXPORT
UA_ReaderGroupConfig_clear(UA_ReaderGroupConfig *readerGroupConfig);

/* Get configuration of ReaderGroup (deep copy) */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_getReaderGroupConfig(UA_Server *server, const UA_NodeId rgId,
                               UA_ReaderGroupConfig *config);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_getReaderGroupState(UA_Server *server, const UA_NodeId rgId,
                              UA_PubSubState *state);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_addReaderGroup(UA_Server *server, const UA_NodeId connectionId,
                         const UA_ReaderGroupConfig *config,
                         UA_NodeId *rgId);

/* The ReaderGroup must be disabled to update the config */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_updateReaderGroupConfig(UA_Server *server,
                                  const UA_NodeId rgId,
                                  const UA_ReaderGroupConfig *config);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_removeReaderGroup(UA_Server *server, const UA_NodeId rgId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_enableReaderGroup(UA_Server *server, const UA_NodeId rgId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_disableReaderGroup(UA_Server *server, const UA_NodeId rgId);

/* Set the group key for the message encryption */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_setReaderGroupEncryptionKeys(UA_Server *server,
                                       const UA_NodeId rgId,
                                       UA_UInt32 securityTokenId,
                                       const UA_ByteString signingKey,
                                       const UA_ByteString encryptingKey,
                                       const UA_ByteString keyNonce);

#ifdef UA_ENABLE_PUBSUB_FILE_CONFIG

/* Decodes the information from the ByteString. If the decoded content is a
 * PubSubConfiguration in a UABinaryFileDataType-object. It will overwrite the
 * current PubSub configuration from the server. The added components are
 * enabled automatically if their enabled-flag is set in the config.
 * Child-components are enabled first.
 *
 * Note that you need to disable all components with
 * UA_Server_disableAllPubSubComponents before loading the config. */
UA_EXPORT UA_StatusCode
UA_Server_loadPubSubConfigFromByteString(UA_Server *server,
                                         const UA_ByteString buffer);

/* Saves the current PubSub configuration of a server in a byteString. */
UA_EXPORT UA_StatusCode
UA_Server_writePubSubConfigurationToByteString(UA_Server *server,
                                               UA_ByteString *buffer);
#endif

/* Legacy API */
#define UA_Server_ReaderGroup_getConfig(server, rgId, config) \
    UA_Server_getReaderGroupConfig(server, rgId, config)
#define UA_Server_ReaderGroup_getState(server, rgId, state) \
    UA_Server_getReaderGroupState(server, rgId, state)
#define UA_Server_setReaderGroupOperational(server, rgId) \
    UA_Server_enableReaderGroup(server, rgId)
#define UA_Server_setReaderGroupDisabled(server, rgId) \
    UA_Server_disableReaderGroup(server, rgId)

#ifdef UA_ENABLE_PUBSUB_SKS

/**
 * SecurityGroup
 * -------------
 * A SecurityGroup is an abstraction that represents the message security
 * settings and security keys for a subset of NetworkMessages exchanged between
 * Publishers and Subscribers. The SecurityGroup objects are created on a
 * Security Key Service (SKS). The SKS manages the access to the keys based on
 * the role permission for a user assigned to a SecurityGroup Object. A
 * SecurityGroup is identified with a unique identifier called the
 * SecurityGroupId. It is unique within the SKS.
 *
 * .. note:: The access to the SecurityGroup and therefore the securitykeys
 *           managed by SKS requires management of Roles and Permissions in the
 *           SKS. The Role Permission model is not supported at the time of
 *           writing. However, the access control plugin can be used to create
 *           and manage role permission on SecurityGroup object. */
typedef struct {
    UA_String securityGroupName;
    UA_Duration keyLifeTime;
    UA_String securityPolicyUri;
    UA_UInt32 maxFutureKeyCount;
    UA_UInt32 maxPastKeyCount;
} UA_SecurityGroupConfig;

/* Creates a SecurityGroup object and add it to the list in PubSub Manager. If
 * the information model is enabled then the SecurityGroup object Node is also
 * created in the server. A keyStorage with initial list of keys is created with
 * a SecurityGroup. A callback is added to new SecurityGroup which updates the
 * keys periodically at each KeyLifeTime expire.
 *
 * @param server The server instance
 * @param securityGroupFolderNodeId The parent node of the SecurityGroup. It
 *        must be of SecurityGroupFolderType
 * @param securityGroupConfig The security settings of a SecurityGroup
 * @param securityGroupNodeId The output nodeId of the new SecurityGroup
 * @return UA_StatusCode The return status code */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_addSecurityGroup(UA_Server *server,
                           UA_NodeId securityGroupFolderNodeId,
                           const UA_SecurityGroupConfig *securityGroupConfig,
                           UA_NodeId *securityGroupNodeId);

/* Removes the SecurityGroup from PubSub Manager. It removes the KeyStorage
 * associated with the SecurityGroup from the server.
 *
 * @param server The server instance
 * @param securityGroup The nodeId of the securityGroup to be removed
 * @return UA_StatusCode The returned status code. */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_removeSecurityGroup(UA_Server *server,
                              const UA_NodeId securityGroup);

/* This is a repeated callback which is triggered on each iteration of SKS Pull
 * request. The server uses this callback to notify user about the status of
 * current Pull request iteration. The period is calculated based on the
 * KeylifeTime of specified in the SecurityGroup object node on the SKS server.
 *
 * @param server The server instance managing the publisher/subscriber.
 * @param sksPullRequestStatus The current status of sks pull request.
 * @param context The pointer to user defined data passed to this callback. */
typedef void
(*UA_Server_sksPullRequestCallback)(UA_Server *server,
                                    UA_StatusCode sksPullRequestStatus,
                                    void* context);

/* Sets the SKS client config used to call the GetSecurityKeys Method on SKS and
 * get the initial set of keys for a SecurityGroupId and adds timedCallback for
 * the next GetSecurityKeys method Call. This uses async Client API for SKS Pull
 * request. The SKS Client instance is created and destroyed at runtime on each
 * iteration of SKS Pull request by the server. The key Rollover mechanism will
 * check if the new keys are needed then it will call the getSecurityKeys Method
 * on SKS Server. At the end of SKS Pull request iteration, the sks client will
 * be deleted by a delayed callback (in next server iteration).
 *
 * Note: It is be called before setting Reader/Writer Group into Operational
 * because this also allocates a channel context for the pubsub security policy.
 *
 * Note: The stateCallback of sksClientConfig will be overwritten by an internal
 * callback.
 *
 * @param server the server instance
 * @param clientConfig holds the required configuration to make encrypted
 *        connection with SKS Server. The input client config takes the
 *        lifecycle as long as SKS request are made. It is deleted with its
 *        plugins when the server is deleted or the last Reader/Writer Group of
 *        the securityGroupId is deleted. The input config is copied to an
 *        internal config object and the content of input config object will be
 *        reset to zero.
 * @param endpointUrl holds the endpointUrl of the SKS server
 * @param securityGroupId the SecurityGroupId of the securityGroup on SKS and
 *        reader/writergroups
 * @param callback the user defined callback to notify the user about the status
 *        of SKS Pull request.
 * @param context passed to the callback function
 * @return UA_StatusCode the retuned status */
UA_StatusCode UA_EXPORT
UA_Server_setSksClient(UA_Server *server, UA_String securityGroupId,
                       UA_ClientConfig *clientConfig, const char *endpointUrl,
                       UA_Server_sksPullRequestCallback callback, void *context);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_setReaderGroupActivateKey(UA_Server *server,
                                    const UA_NodeId readerGroupId);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_setWriterGroupActivateKey(UA_Server *server,
                                    const UA_NodeId writerGroup);

#endif /* UA_ENABLE_PUBSUB_SKS */

/**
 * Offset Table
 * ------------
 * When the content of a PubSub Networkmessage has a fixed length, then only a
 * few "content bytes" at known locations within the NetworkMessage change
 * between publish cycles. The so-called offset table exposes this to enable
 * fast-path implementations for realtime applications. */

typedef enum {
    UA_PUBSUBOFFSETTYPE_NETWORKMESSAGE_GROUPVERSION,   /* UInt32 */
    UA_PUBSUBOFFSETTYPE_NETWORKMESSAGE_SEQUENCENUMBER, /* UInt16 */
    UA_PUBSUBOFFSETTYPE_NETWORKMESSAGE_TIMESTAMP,      /* DateTime */
    UA_PUBSUBOFFSETTYPE_NETWORKMESSAGE_PICOSECONDS,    /* UInt16 */
    UA_PUBSUBOFFSETTYPE_DATASETMESSAGE, /* no content, marks the DSM beginning */
    UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_SEQUENCENUMBER, /* UInt16 */
    UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_STATUS,         /* UInt16 */
    UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_TIMESTAMP,      /* DateTime */
    UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_PICOSECONDS,    /* UInt16 */
    UA_PUBSUBOFFSETTYPE_DATASETFIELD_DATAVALUE,
    UA_PUBSUBOFFSETTYPE_DATASETFIELD_VARIANT,
    UA_PUBSUBOFFSETTYPE_DATASETFIELD_RAW
} UA_PubSubOffsetType;

typedef struct {
    UA_PubSubOffsetType offsetType; /* Content type at the offset */
    size_t offset;                  /* Offset in the NetworkMessage */

    /* The PubSub component that originates / receives the offset content.
     * - For NetworkMessage-offsets this is the ReaderGroup / WriterGroup.
     * - For DataSetMessage-offsets this is DataSetReader / DataSetWriter.
     * - For DataSetFields this is the NodeId associated with the field:
     *   - For Writers the NodeId of the DataSetField (in a PublishedDataSet).
     *   - For Readers the TargetNodeId of the FieldTargetDataType (this can
     *     come from a SubscribedDataSet or a StandaloneSubscribedDataSets).
     *     Access more metadata from the FieldTargetVariable by counting the
     *     index of the current DataSetField-offset within the DataSetMessage
     *     and use that index for the lookup in the DataSetReader configuration. */
    UA_NodeId component;
} UA_PubSubOffset;

typedef struct {
    UA_PubSubOffset *offsets;      /* Array of offset entries */
    size_t offsetsSize;            /* Number of entries */
    UA_ByteString networkMessage;  /* Current NetworkMessage in binary encoding */
} UA_PubSubOffsetTable;

UA_EXPORT void
UA_PubSubOffsetTable_clear(UA_PubSubOffsetTable *ot);

/* Compute the offset table for a WriterGroup */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_computeWriterGroupOffsetTable(UA_Server *server,
                                        const UA_NodeId writerGroupId,
                                        UA_PubSubOffsetTable *ot);

/**
 * For ReaderGroups we cannot compute the offset table up front, because it is
 * not ensured that all Readers end up with their DataSetMessage in the same
 * NetworkMessage. Furthermore the ReaderGroup might receive messages from
 * multiple different publishers.
 *
 * Instead the offset tables are computed beforehand for each DataSetReader. At
 * runtime, use UA_NetworkMessage_decodeBinaryHeaders to decode the
 * NetworkMessage headers. The information therein (e.g. the MessageCount and
 * and the DataSetWriterIds) can then be used to iterate over the
 * DataSetMessages in the payload with their respective offset tables. */

/* The offsets begin at zero for the DataSetMessage */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_computeDataSetReaderOffsetTable(UA_Server *server,
                                          const UA_NodeId dataSetReaderId,
                                          UA_PubSubOffsetTable *ot);

#endif /* UA_ENABLE_PUBSUB */

_UA_END_DECLS

#endif /* UA_SERVER_PUBSUB_H */
