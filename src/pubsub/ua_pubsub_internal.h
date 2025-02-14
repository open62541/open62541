/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2019 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 * Copyright (c) 2020 Yannick Wallerer, Siemens AG
 * Copyright (c) 2020, 2022 Thomas Fischer, Siemens AG
 * Copyright (c) 2021 Fraunhofer IOSB (Author: Jan Hermes)
 * Copyright (c) 2022 Siemens AG (Author: Thomas Fischer)
 * Copyright (c) 2022 Fraunhofer IOSB (Author: Noel Graf)
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#ifndef UA_PUBSUB_INTERNAL_H_
#define UA_PUBSUB_INTERNAL_H_

#define UA_INTERNAL
#include <open62541/server.h>
#include <open62541/server_pubsub.h>

#include "mp_printf.h"
#include "ua_pubsub_networkmessage.h"
#include "../server/ua_server_internal.h"

/**
 * PubSub State Machine
 * --------------------
 * 
 * The following table described the behaviour of components expected during
 * state changes and also the integration which is expected between the
 * components.
 *
 * We distinguish between `enabled` and `disabled` states. The disabled states
 * or `Disabled` and `Error`. The difference is that disabled states need to
 * manually enabled (via the _enable method call). The other states are either
 * Operational or return automatically to the Operational state once the
 * prerequisites are met.
 * 
 * +----------------+-------+--------------------+----------------+--------------------+----------------+----------------+
 * |**Component**   |       |**Disabled**        |**Paused**      |**Pre-Operational** |**Operational** |**Error**       |
 * +----------------+-------+--------------------+----------------+--------------------+----------------+----------------+
 * |PubSubConnection|Trigger|Manual disable      |Connection      |Manual enable ||    |Pre-Operational |Unrecoverable   |
 * |                |       |                    |enabled but the |Recoverable abort of|&& Connected    |abort of the    |
 * |                |       |                    |server is not   |EventLoop connection|EventLoop       |EventLoop       |
 * |                |       |                    |running.        |                    |connection      |connection ||   |
 * |                |       |                    |                |                    |                |Internal Error  |
 * |                +-------+--------------------+----------------+--------------------+----------------+----------------+
 * |                |Action |The underlying      |                |Start the async     |                |Same as the     |
 * |                |       |connection is closed|                |opening of the      |                |Disabled case   |
 * |                |       |(async). Immediately|                |underlying EventLoop|                |                |
 * |                |       |set the EventLoop   |                |connection.         |                |                |
 * |                |       |connection context  |                |Automatically switch|                |                |
 * |                |       |pointer to NULL. So |                |to operational when |                |                |
 * |                |       |that the            |                |the EventLoop       |                |                |
 * |                |       |PubSubConnection can|                |connection is fully |                |                |
 * |                |       |be freed without    |                |open. This can only |                |                |
 * |                |       |waiting for the     |                |be signaled by the  |                |                |
 * |                |       |EventLoop connection|                |underlying EventLoop|                |                |
 * |                |       |to finish closing.  |                |connection in the   |                |                |
 * +----------------+-------+--------------------+----------------+--------------------+----------------+----------------+
 * |WriterGroup     |Trigger|Manual disable      |WG is enabled &&|WG is enabled &&    |WG is enabled &&|Internal error  |
 * |                |       |                    |PubSubConnection|PubSubConnection    |PubSubConnection|                |
 * |                |       |                    |not enabled     |Pre-Operational     |Operational     |                |
 * |                +-------+--------------------+----------------+--------------------+----------------+----------------+
 * |                |Action |Publish callback    |Publish callback|Publish callback    |Publish callback|Publish callback|
 * |                |       |deregistered        |deregistered    |deregistered        |registered      |deregistered    |
 * +----------------+-------+--------------------+----------------+--------------------+----------------+----------------+
 * |DataSetWriter   |Trigger|Manual disable      |DSW enabled &&  |DSW enabled && WG   |DSW enabled &&  |Internal error  |
 * |                |       |                    |WG is not       |Pre-Operational     |WG is           |                |
 * |                |       |                    |enabled         |                    |Operational     |                |
 * +----------------+-------+--------------------+----------------+--------------------+----------------+----------------+
 * |ReaderGroup     |Trigger|Manual disable      |RG enabled &&   |RG enabled &&       |RG enabled &&   |Internal error  |
 * |                |       |                    |PubSubConnection|(PubSubConnection   |PubSubConnection|                |
 * |                |       |                    |not enabled     |Pre-Operational ||  |Operational &&  |                |
 * |                |       |                    |                |RG-connection not   |RG-connection   |                |
 * |                |       |                    |                |fully established)  |established     |                |
 * |                +-------+--------------------+----------------+--------------------+----------------+----------------+
 * |                |Action |RG connection       |RG connection   |RG connection       |RG connection   |RG connection   |
 * |                |       |disconnected        |disconnected    |connected           |connected       |disconnected    |
 * +----------------+-------+--------------------+----------------+--------------------+----------------+----------------+
 * |DataSetReader   |Trigger|Manual disable      |DSR enabled &&  |DSR enabled && RG   |DSR enabled &&  |Internal error  |
 * |                |       |                    |RG not enabled  |Pre-Operational     |RG Operational  |                |
 * +----------------+-------+--------------------+----------------+--------------------+----------------+----------------+
 */

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_PUBSUB

/* Max number of underlying sockets for sending and receiving for every
 * PubSubConnection. Note that a PubSubConnection may have WriterGroups with
 * dedicated sockets. Because for UDP unicast only the WriterGroup has the
 * target host information. */
#define UA_PUBSUB_MAXCHANNELS 8

#define UA_PUBSUB_PROFILES_SIZE 4

struct UA_WriterGroup;
typedef struct UA_WriterGroup UA_WriterGroup;

struct UA_ReaderGroup;
typedef struct UA_ReaderGroup UA_ReaderGroup;

struct UA_SecurityGroup;
typedef struct UA_SecurityGroup UA_SecurityGroup;

struct UA_DataSetReader;
typedef struct UA_DataSetReader UA_DataSetReader;

struct UA_PubSubManager;
typedef struct UA_PubSubManager UA_PubSubManager;

struct UA_PubSubKeyStorage;
typedef struct UA_PubSubKeyStorage UA_PubSubKeyStorage;

/* Get the matching ConnectionManager from the EventLoop */
UA_ConnectionManager *
getCM(UA_EventLoop *el, UA_String protocol);

const char *
UA_PubSubState_name(UA_PubSubState state);

/* A component is considered enabled if it is not in the DISABLED or ERROR
 * state. All other states (also PAUSED) can lead to automatic recovery into
 * OPERATIONAL. */
static UA_INLINE UA_Boolean
UA_PubSubState_isEnabled(UA_PubSubState state) {
    return (state != UA_PUBSUBSTATE_DISABLED &&
            state != UA_PUBSUBSTATE_ERROR);
}

/* All PubSubComponents share the same header structure */

typedef enum  {
    UA_PUBSUBCOMPONENT_CONNECTION  = 0,
    UA_PUBSUBCOMPONENT_WRITERGROUP  = 1,
    UA_PUBSUBCOMPONENT_DATASETWRITER  = 2,
    UA_PUBSUBCOMPONENT_READERGROUP  = 3,
    UA_PUBSUBCOMPONENT_DATASETREADER  = 4,
    UA_PUBSUBCOMPONENT_PUBLISHEDDATASET  = 5,
    UA_PUBSUBCOMPONENT_SUBSCRIBEDDDATASET = 6,
} UA_PubSubComponentType;

typedef struct {
    UA_NodeId identifier;
    UA_PubSubComponentType componentType;
    UA_PubSubState state;
    UA_String logIdString; /* Precomputed logging prefix */
    UA_Boolean transientState; /* We are in the middle of a state update */
} UA_PubSubComponentHead;

#define UA_LOG_PUBSUB_INTERNAL(LOGGER, LEVEL, COMPONENT, MSG, ...)      \
    if(UA_LOGLEVEL <= UA_LOGLEVEL_##LEVEL) {                            \
        UA_LOG_##LEVEL(LOGGER, UA_LOGCATEGORY_PUBSUB, "%S" MSG "%.0s",  \
                       (COMPONENT)->head.logIdString, __VA_ARGS__);     \
    }

#define UA_LOG_TRACE_PUBSUB(LOGGER, COMPONENT, ...)                          \
    UA_MACRO_EXPAND(UA_LOG_PUBSUB_INTERNAL(LOGGER, TRACE, COMPONENT, __VA_ARGS__, ""))
#define UA_LOG_DEBUG_PUBSUB(LOGGER, COMPONENT, ...)                          \
    UA_MACRO_EXPAND(UA_LOG_PUBSUB_INTERNAL(LOGGER, DEBUG, COMPONENT, __VA_ARGS__, ""))
#define UA_LOG_INFO_PUBSUB(LOGGER, COMPONENT, ...)                           \
    UA_MACRO_EXPAND(UA_LOG_PUBSUB_INTERNAL(LOGGER, INFO, COMPONENT, __VA_ARGS__, ""))
#define UA_LOG_WARNING_PUBSUB(LOGGER, COMPONENT, ...)                        \
    UA_MACRO_EXPAND(UA_LOG_PUBSUB_INTERNAL(LOGGER, WARNING, COMPONENT, __VA_ARGS__, ""))
#define UA_LOG_ERROR_PUBSUB(LOGGER, COMPONENT, ...)                          \
    UA_MACRO_EXPAND(UA_LOG_PUBSUB_INTERNAL(LOGGER, ERROR, COMPONENT, __VA_ARGS__, ""))
#define UA_LOG_FATAL_PUBSUB(LOGGER, COMPONENT, ...)                          \
    UA_MACRO_EXPAND(UA_LOG_PUBSUB_INTERNAL(LOGGER, FATAL, COMPONENT, __VA_ARGS__, ""))

void
UA_PubSubComponentHead_clear(UA_PubSubComponentHead *psch);

/**********************************************/
/*            PublishedDataSet                */
/**********************************************/

typedef struct UA_PublishedDataSet {
    UA_PubSubComponentHead head;
    TAILQ_ENTRY(UA_PublishedDataSet) listEntry;
    TAILQ_HEAD(, UA_DataSetField) fields;
    UA_PublishedDataSetConfig config;
    UA_DataSetMetaDataType dataSetMetaData;
    UA_UInt16 fieldSize;
    UA_UInt16 promotedFieldsCount;

    /* The counter is required because the PDS has not state.
     * Check if it is actively used when changes are introduced. */
    UA_UInt16 configurationFreezeCounter;
} UA_PublishedDataSet;

UA_StatusCode
UA_PublishedDataSetConfig_copy(const UA_PublishedDataSetConfig *src,
                               UA_PublishedDataSetConfig *dst);

UA_PublishedDataSet *
UA_PublishedDataSet_find(UA_PubSubManager *psm, const UA_NodeId id);

UA_PublishedDataSet *
UA_PublishedDataSet_findByName(UA_PubSubManager *psm, const UA_String name);

UA_AddPublishedDataSetResult
UA_PublishedDataSet_create(UA_PubSubManager *psm,
                           const UA_PublishedDataSetConfig *publishedDataSetConfig,
                           UA_NodeId *pdsIdentifier);

UA_StatusCode
UA_PublishedDataSet_remove(UA_PubSubManager *psm, UA_PublishedDataSet *pds);

/*********************/
/* SubscribedDataSet */
/*********************/

typedef struct UA_SubscribedDataSet {
    UA_PubSubComponentHead head;
    TAILQ_ENTRY(UA_SubscribedDataSet) listEntry;
    UA_SubscribedDataSetConfig config;
    UA_DataSetReader *connectedReader;
} UA_SubscribedDataSet;

UA_StatusCode
UA_SubscribedDataSetConfig_copy(const UA_SubscribedDataSetConfig *src,
                                UA_SubscribedDataSetConfig *dst);

UA_SubscribedDataSet *
UA_SubscribedDataSet_find(UA_PubSubManager *psm, const UA_NodeId id);

UA_SubscribedDataSet *
UA_SubscribedDataSet_findByName(UA_PubSubManager *psm, const UA_String name);

void
UA_SubscribedDataSet_remove(UA_PubSubManager *psm, UA_SubscribedDataSet *sds);

/**********************************************/
/*               Connection                   */
/**********************************************/

typedef struct UA_PubSubConnection {
    UA_PubSubComponentHead head;
    TAILQ_ENTRY(UA_PubSubConnection) listEntry;

    /* The send/recv connections are only opened if the state is operational */
    UA_PubSubConnectionConfig config;
    UA_Boolean json; /* Extracted from the TransportProfileUrl */

    /* Channels belonging to the PubSubConnection. Send channels belong to
     * WriterGroups, recv channels belong to ReaderGroups. We only open channels 
     * if there is at least one WriterGroup/ReaderGroup respectively.
     *
     * Some channels belong exclusively to just one WriterGroup/ReaderGroup that
     * defines additional connection properties. For example an MQTT topic name
     * or QoS parameters. In that case a dedicated NetworkCallback is used that
     * takes this ReaderGroup/WriterGroup directly as context. */
    UA_ConnectionManager *cm;
    uintptr_t recvChannels[UA_PUBSUB_MAXCHANNELS];
    size_t recvChannelsSize;
    uintptr_t sendChannel;

    size_t writerGroupsSize;
    LIST_HEAD(, UA_WriterGroup) writerGroups;

    size_t readerGroupsSize;
    LIST_HEAD(, UA_ReaderGroup) readerGroups;

    UA_DateTime silenceErrorUntil; /* Avoid generating too many logs */

    UA_Boolean deleteFlag; /* To be deleted - in addition to the PubSubState */
    UA_DelayedCallback dc; /* For delayed freeing */
} UA_PubSubConnection;

UA_StatusCode
UA_PubSubConnectionConfig_copy(const UA_PubSubConnectionConfig *src,
                               UA_PubSubConnectionConfig *dst);

UA_PubSubConnection *
UA_PubSubConnection_find(UA_PubSubManager *psm, const UA_NodeId id);

UA_StatusCode
UA_PubSubConnection_create(UA_PubSubManager *psm,
                           const UA_PubSubConnectionConfig *connectionConfig,
                           UA_NodeId *connectionIdentifier);

void
UA_PubSubConnectionConfig_clear(UA_PubSubConnectionConfig *connectionConfig);

void
UA_PubSubConnection_delete(UA_PubSubManager *psm, UA_PubSubConnection *c);

UA_StatusCode
UA_PubSubConnection_setPubSubState(UA_PubSubManager *psm, UA_PubSubConnection *c,
                                   UA_PubSubState targetState);

/* Also used by the ReaderGroup ... */
UA_StatusCode
UA_PubSubConnection_decodeNetworkMessage(UA_PubSubManager *psm,
                                         UA_PubSubConnection *connection,
                                         UA_ByteString buffer,
                                         UA_NetworkMessage *nm);

/**********************************************/
/*              DataSetWriter                 */
/**********************************************/

typedef struct UA_DataSetWriterSample {
    UA_Boolean valueChanged;
    UA_DataValue value;
} UA_DataSetWriterSample;

typedef struct UA_DataSetWriter {
    UA_PubSubComponentHead head;
    LIST_ENTRY(UA_DataSetWriter) listEntry;

    UA_DataSetWriterConfig config;
    UA_WriterGroup *linkedWriterGroup;
    UA_PublishedDataSet *connectedDataSet;
    UA_ConfigurationVersionDataType connectedDataSetVersion;

    /* Deltaframes */
    UA_UInt16 deltaFrameCounter; /* count of sent deltaFrames */
    size_t lastSamplesCount;
    UA_DataSetWriterSample *lastSamples;

    UA_UInt16 actualDataSetMessageSequenceCount;
    UA_Boolean configurationFrozen;
    UA_UInt64 pubSubStateTimerId;
} UA_DataSetWriter;

UA_StatusCode
UA_DataSetWriterConfig_copy(const UA_DataSetWriterConfig *src,
                            UA_DataSetWriterConfig *dst);

UA_DataSetWriter *
UA_DataSetWriter_find(UA_PubSubManager *psm, const UA_NodeId id);

UA_StatusCode
UA_DataSetWriter_setPubSubState(UA_PubSubManager *psm, UA_DataSetWriter *dsw,
                                UA_PubSubState targetState);

UA_StatusCode
UA_DataSetWriter_generateDataSetMessage(UA_PubSubManager *psm,
                                        UA_DataSetWriter *dsw,
                                        UA_DataSetMessage *dsm);

UA_StatusCode
UA_DataSetWriter_create(UA_PubSubManager *psm,
                        const UA_NodeId writerGroup, const UA_NodeId dataSet,
                        const UA_DataSetWriterConfig *dataSetWriterConfig,
                        UA_NodeId *writerIdentifier);

UA_StatusCode
UA_DataSetWriter_remove(UA_PubSubManager *psm, UA_DataSetWriter *dsw);

/**********************************************/
/*               WriterGroup                  */
/**********************************************/

struct UA_WriterGroup {
    UA_PubSubComponentHead head;
    LIST_ENTRY(UA_WriterGroup) listEntry;

    UA_WriterGroupConfig config;

    LIST_HEAD(, UA_DataSetWriter) writers;
    UA_UInt32 writersCount;

    UA_UInt64 publishCallbackId; /* registered if != 0 */
    UA_UInt16 sequenceNumber; /* Increased after every sent message */
    UA_DateTime lastPublishTimeStamp;

    /* The ConnectionManager pointer is stored in the Connection. The channels
     * are either stored here or in the Connection, but never both. */
    UA_PubSubConnection *linkedConnection;
    uintptr_t sendChannel;
    UA_Boolean deleteFlag;

    UA_UInt32 securityTokenId;
    UA_UInt32 nonceSequenceNumber; /* To be part of the MessageNonce */
    void *securityPolicyContext;
#ifdef UA_ENABLE_PUBSUB_SKS
    UA_PubSubKeyStorage *keyStorage; /* non-owning pointer to keyStorage*/
#endif
};

UA_StatusCode
UA_WriterGroup_create(UA_PubSubManager *psm, const UA_NodeId connection,
                      const UA_WriterGroupConfig *writerGroupConfig,
                      UA_NodeId *writerGroupIdentifier);

void
UA_WriterGroup_remove(UA_PubSubManager *psm, UA_WriterGroup *wg);

/* Exposed so we can change the publish interval without having to stop */
UA_StatusCode
UA_WriterGroup_addPublishCallback(UA_PubSubManager *psm, UA_WriterGroup *wg);

void
UA_WriterGroup_removePublishCallback(UA_PubSubManager *psm, UA_WriterGroup *wg);

UA_StatusCode
UA_WriterGroup_setEncryptionKeys(UA_PubSubManager *psm, UA_WriterGroup *wg,
                                 UA_UInt32 securityTokenId,
                                 const UA_ByteString signingKey,
                                 const UA_ByteString encryptingKey,
                                 const UA_ByteString keyNonce);

UA_StatusCode
UA_WriterGroupConfig_copy(const UA_WriterGroupConfig *src,
                          UA_WriterGroupConfig *dst);

UA_WriterGroup *
UA_WriterGroup_find(UA_PubSubManager *psm, const UA_NodeId id);

UA_StatusCode
UA_WriterGroup_setPubSubState(UA_PubSubManager *psm, UA_WriterGroup *wg,
                              UA_PubSubState targetState);

void
UA_WriterGroup_publishCallback(UA_PubSubManager *psm, UA_WriterGroup *wg);

/**********************************************/
/*               DataSetField                 */
/**********************************************/

typedef struct UA_DataSetField {
    UA_DataSetFieldConfig config;
    TAILQ_ENTRY(UA_DataSetField) listEntry;
    UA_NodeId identifier;
    UA_NodeId publishedDataSet;     /* parent pds */
    UA_FieldMetaData fieldMetaData; /* contains the dataSetFieldId */
    UA_UInt64 sampleCallbackId;
    UA_Boolean sampleCallbackIsRegistered;
} UA_DataSetField;

UA_StatusCode
UA_DataSetFieldConfig_copy(const UA_DataSetFieldConfig *src,
                           UA_DataSetFieldConfig *dst);

UA_DataSetField *
UA_DataSetField_find(UA_PubSubManager *psm, const UA_NodeId id);

UA_DataSetFieldResult
UA_DataSetField_remove(UA_PubSubManager *psm, UA_DataSetField *currentField);

UA_DataSetFieldResult
UA_DataSetField_create(UA_PubSubManager *psm, const UA_NodeId publishedDataSet,
                       const UA_DataSetFieldConfig *fieldConfig,
                       UA_NodeId *fieldIdentifier);

void
UA_PubSubDataSetField_sampleValue(UA_PubSubManager *psm,
                                  UA_DataSetField *field,
                                  UA_DataValue *value);

/**********************************************/
/*               DataSetReader                */
/**********************************************/

struct UA_DataSetReader {
    UA_PubSubComponentHead head;
    LIST_ENTRY(UA_DataSetReader) listEntry;

    UA_DataSetReaderConfig config;
    UA_ReaderGroup *linkedReaderGroup;

    /* MessageReceiveTimeout handling */
    UA_UInt64 msgRcvTimeoutTimerId;
};

UA_DataSetReader *
UA_DataSetReader_find(UA_PubSubManager *psm, const UA_NodeId id);

/* Process Network Message using DataSetReader */
void
UA_DataSetReader_process(UA_PubSubManager *psm,
                         UA_DataSetReader *dataSetReader,
                         UA_DataSetMessage *dataSetMsg);

UA_StatusCode
UA_DataSetReader_checkIdentifier(UA_PubSubManager *psm, UA_DataSetReader *dsr,
                                 UA_NetworkMessage *msg);

UA_StatusCode
UA_DataSetReader_create(UA_PubSubManager *psm, UA_NodeId readerGroupIdentifier,
                        const UA_DataSetReaderConfig *dataSetReaderConfig,
                        UA_NodeId *readerIdentifier);

UA_StatusCode
UA_DataSetReader_remove(UA_PubSubManager *psm, UA_DataSetReader *dsr);

UA_StatusCode
DataSetReader_createTargetVariables(UA_PubSubManager *psm, UA_DataSetReader *dsr,
                                    size_t targetsSize, const UA_FieldTargetDataType *targets);

/* Returns an error reason if the target state is `Error` */
void
UA_DataSetReader_setPubSubState(UA_PubSubManager *psm, UA_DataSetReader *dsr,
                                UA_PubSubState targetState, UA_StatusCode errorReason);

/**********************************************/
/*                ReaderGroup                 */
/**********************************************/

struct UA_ReaderGroup {
    UA_PubSubComponentHead head;
    LIST_ENTRY(UA_ReaderGroup) listEntry;

    UA_ReaderGroupConfig config;

    LIST_HEAD(, UA_DataSetReader) readers;
    UA_UInt32 readersCount;

    UA_Boolean hasReceived; /* Received a message since the last _connect */

    /* The ConnectionManager pointer is stored in the Connection. The channels 
     * are either stored here or in the Connection, but never both. */
    UA_PubSubConnection *linkedConnection;
    uintptr_t recvChannels[UA_PUBSUB_MAXCHANNELS];
    size_t recvChannelsSize;
    UA_Boolean deleteFlag;

    UA_UInt32 securityTokenId;
    UA_UInt32 nonceSequenceNumber; /* To be part of the MessageNonce */
    void *securityPolicyContext;
#ifdef UA_ENABLE_PUBSUB_SKS
    UA_PubSubKeyStorage *keyStorage;
#endif
};

UA_StatusCode
UA_ReaderGroup_create(UA_PubSubManager *psm, UA_NodeId connectionId,
                      const UA_ReaderGroupConfig *rgc,
                      UA_NodeId *readerGroupId);

void
UA_ReaderGroup_remove(UA_PubSubManager *psm, UA_ReaderGroup *rg);

UA_StatusCode
UA_ReaderGroup_connect(UA_PubSubManager *psm, UA_ReaderGroup *rg,
                       UA_Boolean validate);

UA_Boolean
UA_ReaderGroup_canConnect(UA_ReaderGroup *rg);

void
UA_ReaderGroup_disconnect(UA_ReaderGroup *rg);

UA_StatusCode
UA_ReaderGroup_setEncryptionKeys(UA_PubSubManager *psm, UA_ReaderGroup *rg,
                                 UA_UInt32 securityTokenId,
                                 const UA_ByteString signingKey,
                                 const UA_ByteString encryptingKey,
                                 const UA_ByteString keyNonce);

UA_StatusCode
UA_ReaderGroupConfig_copy(const UA_ReaderGroupConfig *src,
                          UA_ReaderGroupConfig *dst);

UA_ReaderGroup *
UA_ReaderGroup_find(UA_PubSubManager *psm, const UA_NodeId id);

UA_StatusCode
UA_ReaderGroup_setPubSubState(UA_PubSubManager *psm, UA_ReaderGroup *rg,
                              UA_PubSubState targetState);

UA_Boolean
UA_ReaderGroup_process(UA_PubSubManager *psm, UA_ReaderGroup *rg,
                       UA_NetworkMessage *nm);

/* The buffer is the entire message. The ctx->pos points after the decoded
 * header. The ctx->end is modified to remove padding, etc. */
UA_StatusCode
verifyAndDecryptNetworkMessage(const UA_Logger *logger, UA_ByteString buffer,
                               Ctx *ctx, UA_NetworkMessage *nm,
                               UA_ReaderGroup *rg);

#ifdef UA_ENABLE_PUBSUB_SKS

/*********************************************************/
/*                    SecurityGroup                      */
/*********************************************************/

struct UA_SecurityGroup {
    UA_String securityGroupId;
    UA_SecurityGroupConfig config;
    UA_PubSubKeyStorage *keyStorage;
    UA_NodeId securityGroupNodeId;
    UA_UInt64 callbackId;
    UA_DateTime baseTime;
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    UA_NodeId securityGroupFolderId;
#endif
    TAILQ_ENTRY(UA_SecurityGroup) listEntry;
};

UA_StatusCode
UA_SecurityGroupConfig_copy(const UA_SecurityGroupConfig *src,
                            UA_SecurityGroupConfig *dst);

UA_SecurityGroup *
UA_SecurityGroup_findByName(UA_PubSubManager *psm, const UA_String name);

UA_SecurityGroup *
UA_SecurityGroup_find(UA_PubSubManager *psm, const UA_NodeId id);

void
UA_SecurityGroup_remove(UA_PubSubManager *psm, UA_SecurityGroup *sg);

#endif /* UA_ENABLE_PUBSUB_SKS */

/******************/
/* PubSub Manager */
/******************/

typedef enum {
    UA_WRITER_GROUP = 0,
    UA_DATA_SET_WRITER = 1,
} UA_ReserveIdType;

typedef struct UA_ReserveId {
    UA_UInt16 id;
    UA_ReserveIdType reserveIdType;
    UA_String transportProfileUri;
    UA_NodeId sessionId;
    ZIP_ENTRY(UA_ReserveId) treeEntry;
} UA_ReserveId;

typedef ZIP_HEAD(UA_ReserveIdTree, UA_ReserveId) UA_ReserveIdTree;

struct UA_PubSubManager {
    UA_ServerComponent sc;

    UA_Logger *logging; /* shortcut to sc->server.logging */

    UA_UInt64 defaultPublisherId;
    /* Connections and PublishedDataSets can exist alone (own lifecycle) -> top
     * level components */
    size_t connectionsSize;
    TAILQ_HEAD(, UA_PubSubConnection) connections;

    size_t publishedDataSetsSize;
    TAILQ_HEAD(, UA_PublishedDataSet) publishedDataSets;

    size_t subscribedDataSetsSize;
    TAILQ_HEAD(, UA_SubscribedDataSet) subscribedDataSets;

    size_t reserveIdsSize;
    UA_ReserveIdTree reserveIds;

#ifdef UA_ENABLE_PUBSUB_SKS
    LIST_HEAD(, UA_PubSubKeyStorage) pubSubKeyList;

    size_t securityGroupsSize;
    TAILQ_HEAD(, UA_SecurityGroup) securityGroups;
#endif

#ifndef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    UA_UInt32 uniqueIdCount;
#endif
};

static UA_INLINE UA_PubSubManager *
getPSM(UA_Server *server) {
    return (UA_PubSubManager*)getServerComponentByName(server, UA_STRING("pubsub"));
}

UA_StatusCode
UA_PubSubManager_clear(UA_PubSubManager *psm);

void
UA_PubSubManager_setState(UA_PubSubManager *psm,
                          UA_LifecycleState state);

UA_StatusCode
UA_PubSubManager_reserveIds(UA_PubSubManager *psm, UA_NodeId sessionId,
                            UA_UInt16 numRegWriterGroupIds,
                            UA_UInt16 numRegDataSetWriterIds,
                            UA_String transportProfileUri, UA_UInt16 **writerGroupIds,
                            UA_UInt16 **dataSetWriterIds);

void
UA_PubSubManager_freeIds(UA_PubSubManager *psm);

#ifndef UA_ENABLE_PUBSUB_INFORMATIONMODEL
void
UA_PubSubManager_generateUniqueNodeId(UA_PubSubManager *psm, UA_NodeId *nodeId);
#endif

UA_Guid
UA_PubSubManager_generateUniqueGuid(UA_PubSubManager *psm);

UA_UInt32
UA_PubSubConfigurationVersionTimeDifference(UA_DateTime now);

/************************************/
/* Information Model Representation */
/************************************/

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL /* conditional compilation */

UA_StatusCode
initPubSubNS0(UA_Server *server);

UA_StatusCode
addPubSubConnectionRepresentation(UA_Server *server, UA_PubSubConnection *connection);

UA_StatusCode
addWriterGroupRepresentation(UA_Server *server, UA_WriterGroup *writerGroup);

UA_StatusCode
addReaderGroupRepresentation(UA_Server *server, UA_ReaderGroup *readerGroup);

UA_StatusCode
addDataSetWriterRepresentation(UA_Server *server, UA_DataSetWriter *dataSetWriter);

UA_StatusCode
addPublishedDataItemsRepresentation(UA_Server *server, UA_PublishedDataSet *publishedDataSet);

UA_StatusCode
addSubscribedDataSetRepresentation(UA_Server *server, UA_SubscribedDataSet *subscribedDataSet);

UA_StatusCode
addDataSetReaderRepresentation(UA_Server *server, UA_DataSetReader *dataSetReader);

UA_StatusCode
connectDataSetReaderToDataSet(UA_Server *server, UA_NodeId dsrId, UA_NodeId sdsId);

#ifdef UA_ENABLE_PUBSUB_SKS
UA_StatusCode
addSecurityGroupRepresentation(UA_Server *server, UA_SecurityGroup *securityGroup);
#endif /* UA_ENABLE_PUBSUB_SKS */

#endif /* UA_ENABLE_PUBSUB_INFORMATIONMODEL */

#endif /* UA_ENABLE_PUBSUB */

_UA_END_DECLS

#endif /* UA_PUBSUB_INTERNAL_H_ */
