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

#ifndef UA_PUBSUB_H_
#define UA_PUBSUB_H_

#define UA_INTERNAL
#include <open62541/server.h>
#include <open62541/server_pubsub.h>

#include "open62541_queue.h"
#include "ziptree.h"
#include "mp_printf.h"
#include "ua_pubsub_networkmessage.h"

#ifdef UA_ENABLE_PUBSUB_SKS
#include <ua_pubsub_keystorage.h>
#endif

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
 * |PubSubConnection|Trigger|Manual disable      |Not available   |Manual enable ||    |Pre-Operational |Unrecoverable   |
 * |                |       |                    |                |Recoverable abort of|&& Connected    |abort of the    |
 * |                |       |                    |                |EventLoop connection|EventLoop       |EventLoop       |
 * |                |       |                    |                |                    |connection      |connection ||   |
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

/* Max number of underlying for sending and receiving */
#define UA_PUBSUB_MAXCHANNELS 8

struct UA_WriterGroup;
typedef struct UA_WriterGroup UA_WriterGroup;

struct UA_ReaderGroup;
typedef struct UA_ReaderGroup UA_ReaderGroup;

struct UA_SecurityGroup;
typedef struct UA_SecurityGroup UA_SecurityGroup;

const char *
UA_PubSubState_name(UA_PubSubState state);

/**********************************************/
/*            PublishedDataSet                */
/**********************************************/

typedef struct UA_PublishedDataSet {
    TAILQ_ENTRY(UA_PublishedDataSet) listEntry;
    TAILQ_HEAD(, UA_DataSetField) fields;
    UA_NodeId identifier;
    UA_String logIdString;
    UA_PublishedDataSetConfig config;
    UA_DataSetMetaDataType dataSetMetaData;
    UA_UInt16 fieldSize;
    UA_UInt16 promotedFieldsCount;
    UA_UInt16 configurationFreezeCounter;
} UA_PublishedDataSet;

UA_StatusCode
UA_PublishedDataSetConfig_copy(const UA_PublishedDataSetConfig *src,
                               UA_PublishedDataSetConfig *dst);

UA_PublishedDataSet *
UA_PublishedDataSet_findPDSbyId(UA_Server *server, UA_NodeId identifier);

UA_PublishedDataSet *
UA_PublishedDataSet_findPDSbyName(UA_Server *server, UA_String name);

UA_AddPublishedDataSetResult
UA_PublishedDataSet_create(UA_Server *server,
                           const UA_PublishedDataSetConfig *publishedDataSetConfig,
                           UA_NodeId *pdsIdentifier);

void
UA_PublishedDataSet_clear(UA_Server *server,
                          UA_PublishedDataSet *publishedDataSet);

UA_StatusCode
UA_PublishedDataSet_remove(UA_Server *server, UA_PublishedDataSet *publishedDataSet);

UA_StatusCode
getPublishedDataSetConfig(UA_Server *server, const UA_NodeId pds,
                          UA_PublishedDataSetConfig *config);

typedef struct UA_StandaloneSubscribedDataSet{
    UA_StandaloneSubscribedDataSetConfig config;
    UA_NodeId identifier;
    TAILQ_ENTRY(UA_StandaloneSubscribedDataSet) listEntry;
    UA_NodeId connectedReader;
} UA_StandaloneSubscribedDataSet;

UA_StatusCode
UA_StandaloneSubscribedDataSetConfig_copy(const UA_StandaloneSubscribedDataSetConfig *src,
                                          UA_StandaloneSubscribedDataSetConfig *dst);
UA_StandaloneSubscribedDataSet *
UA_StandaloneSubscribedDataSet_findSDSbyId(UA_Server *server, UA_NodeId identifier);
UA_StandaloneSubscribedDataSet *
UA_StandaloneSubscribedDataSet_findSDSbyName(UA_Server *server, UA_String identifier);
void
UA_StandaloneSubscribedDataSet_clear(UA_Server *server,
                                     UA_StandaloneSubscribedDataSet *subscribedDataSet);

#define UA_LOG_DATASET_INTERNAL(LOGGER, LEVEL, PDS, MSG, ...)           \
    if(UA_LOGLEVEL <= UA_LOGLEVEL_##LEVEL) {                            \
        UA_LOG_##LEVEL(LOGGER, UA_LOGCATEGORY_PUBSUB, "%.*s" MSG "%.0s", \
                       (int)(PDS)->logIdString.length,                  \
                       (char*)(PDS)->logIdString.data, __VA_ARGS__);    \
    }

#define UA_LOG_TRACE_DATASET(LOGGER, PDS, ...)                          \
    UA_MACRO_EXPAND(UA_LOG_DATASET_INTERNAL(LOGGER, TRACE, PDS, __VA_ARGS__, ""))
#define UA_LOG_DEBUG_DATASET(LOGGER, PDS, ...)                          \
    UA_MACRO_EXPAND(UA_LOG_DATASET_INTERNAL(LOGGER, DEBUG, PDS, __VA_ARGS__, ""))
#define UA_LOG_INFO_DATASET(LOGGER, PDS, ...)                           \
    UA_MACRO_EXPAND(UA_LOG_DATASET_INTERNAL(LOGGER, INFO, PDS, __VA_ARGS__, ""))
#define UA_LOG_WARNING_DATASET(LOGGER, PDS, ...)                        \
    UA_MACRO_EXPAND(UA_LOG_DATASET_INTERNAL(LOGGER, WARNING, PDS, __VA_ARGS__, ""))
#define UA_LOG_ERROR_DATASET(LOGGER, PDS, ...)                          \
    UA_MACRO_EXPAND(UA_LOG_DATASET_INTERNAL(LOGGER, ERROR, PDS, __VA_ARGS__, ""))
#define UA_LOG_FATAL_DATASET(LOGGER, PDS, ...)                          \
    UA_MACRO_EXPAND(UA_LOG_DATASET_INTERNAL(LOGGER, FATAL, PDS, __VA_ARGS__, ""))

/**********************************************/
/*               Connection                   */
/**********************************************/

typedef struct UA_PubSubConnection {
    UA_PubSubComponentEnumType componentType;

    TAILQ_ENTRY(UA_PubSubConnection) listEntry;
    UA_NodeId identifier;
    UA_String logIdString;

    /* The send/recv connections are only opened if the state is operational */
    UA_PubSubState state;
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

    UA_UInt16 configurationFreezeCounter;

    UA_Boolean deleteFlag; /* To be deleted - in addition to the PubSubState */
    UA_DelayedCallback dc; /* For delayed freeing */
} UA_PubSubConnection;

UA_StatusCode
UA_PubSubConnectionConfig_copy(const UA_PubSubConnectionConfig *src,
                               UA_PubSubConnectionConfig *dst);

UA_PubSubConnection *
UA_PubSubConnection_findConnectionbyId(UA_Server *server,
                                       UA_NodeId connectionIdentifier);

UA_StatusCode
UA_PubSubConnection_create(UA_Server *server,
                           const UA_PubSubConnectionConfig *connectionConfig,
                           UA_NodeId *connectionIdentifier);

void
UA_PubSubConnectionConfig_clear(UA_PubSubConnectionConfig *connectionConfig);

void
UA_PubSubConnection_delete(UA_Server *server, UA_PubSubConnection *c);

UA_StatusCode
UA_PubSubConnection_connect(UA_Server *server, UA_PubSubConnection *c,
                            UA_Boolean validate);

void
UA_PubSubConnection_process(UA_Server *server, UA_PubSubConnection *c,
                            UA_ByteString msg);


void
UA_PubSubConnection_disconnect(UA_PubSubConnection *c);

/* Returns either the eventloop configured in the connection or, in its absence,
 * for the server */
UA_EventLoop *
UA_PubSubConnection_getEL(UA_Server *server, UA_PubSubConnection *c);

UA_StatusCode
UA_PubSubConnection_setPubSubState(UA_Server *server,
                                   UA_PubSubConnection *connection,
                                   UA_PubSubState targetState);

#define UA_LOG_CONNECTION_INTERNAL(LOGGER, LEVEL, CONNECTION, MSG, ...) \
    if(UA_LOGLEVEL <= UA_LOGLEVEL_##LEVEL) {                            \
        UA_LOG_##LEVEL(LOGGER, UA_LOGCATEGORY_PUBSUB, "%.*s" MSG "%.0s", \
                       (int)(CONNECTION)->logIdString.length,           \
                       (char*)(CONNECTION)->logIdString.data, __VA_ARGS__); \
    }

#define UA_LOG_TRACE_CONNECTION(LOGGER, CONNECTION, ...)                \
    UA_MACRO_EXPAND(UA_LOG_CONNECTION_INTERNAL(LOGGER, TRACE, CONNECTION, __VA_ARGS__, ""))
#define UA_LOG_DEBUG_CONNECTION(LOGGER, CONNECTION, ...)                \
    UA_MACRO_EXPAND(UA_LOG_CONNECTION_INTERNAL(LOGGER, DEBUG, CONNECTION, __VA_ARGS__, ""))
#define UA_LOG_INFO_CONNECTION(LOGGER, CONNECTION, ...)                 \
    UA_MACRO_EXPAND(UA_LOG_CONNECTION_INTERNAL(LOGGER, INFO, CONNECTION, __VA_ARGS__, ""))
#define UA_LOG_WARNING_CONNECTION(LOGGER, CONNECTION, ...)              \
    UA_MACRO_EXPAND(UA_LOG_CONNECTION_INTERNAL(LOGGER, WARNING, CONNECTION, __VA_ARGS__, ""))
#define UA_LOG_ERROR_CONNECTION(LOGGER, CONNECTION, ...)                \
    UA_MACRO_EXPAND(UA_LOG_CONNECTION_INTERNAL(LOGGER, ERROR, CONNECTION, __VA_ARGS__, ""))
#define UA_LOG_FATAL_CONNECTION(LOGGER, CONNECTION, ...)                \
    UA_MACRO_EXPAND(UA_LOG_CONNECTION_INTERNAL(LOGGER, FATAL, CONNECTION, __VA_ARGS__, ""))

/**********************************************/
/*              DataSetWriter                 */
/**********************************************/

typedef struct UA_DataSetWriterSample {
    UA_Boolean valueChanged;
    UA_DataValue value;
} UA_DataSetWriterSample;

typedef struct UA_DataSetWriter {
    UA_PubSubComponentEnumType componentType;
    UA_DataSetWriterConfig config;
    LIST_ENTRY(UA_DataSetWriter) listEntry;
    UA_NodeId identifier;
    UA_String logIdString;
    UA_WriterGroup *linkedWriterGroup;
    UA_NodeId connectedDataSet;
    UA_ConfigurationVersionDataType connectedDataSetVersion;
    UA_PubSubState state;

    /* Deltaframes */
    UA_UInt16 deltaFrameCounter; /* count of sent deltaFrames */
    size_t lastSamplesCount;
    UA_DataSetWriterSample *lastSamples;

    UA_UInt16 actualDataSetMessageSequenceCount;
    UA_Boolean configurationFrozen;
    UA_UInt64  pubSubStateTimerId;
} UA_DataSetWriter;

UA_StatusCode
UA_DataSetWriterConfig_copy(const UA_DataSetWriterConfig *src,
                            UA_DataSetWriterConfig *dst);

UA_DataSetWriter *
UA_DataSetWriter_findDSWbyId(UA_Server *server, UA_NodeId identifier);

UA_StatusCode
UA_DataSetWriter_setPubSubState(UA_Server *server,
                                UA_DataSetWriter *dataSetWriter,
                                UA_PubSubState targetState);

UA_StatusCode
UA_DataSetWriter_generateDataSetMessage(UA_Server *server,
                                        UA_DataSetMessage *dataSetMessage,
                                        UA_DataSetWriter *dataSetWriter);

UA_StatusCode
UA_DataSetWriter_prepareDataSet(UA_Server *server, UA_DataSetWriter *dsw,
                                UA_DataSetMessage *dsm);

void
UA_DataSetWriter_freezeConfiguration(UA_Server *server, UA_DataSetWriter *dsw);

void
UA_DataSetWriter_unfreezeConfiguration(UA_Server *server, UA_DataSetWriter *dsw);

UA_StatusCode
UA_DataSetWriter_create(UA_Server *server,
                        const UA_NodeId writerGroup, const UA_NodeId dataSet,
                        const UA_DataSetWriterConfig *dataSetWriterConfig,
                        UA_NodeId *writerIdentifier);


UA_StatusCode
UA_DataSetWriter_remove(UA_Server *server, UA_DataSetWriter *dataSetWriter);

#define UA_LOG_WRITER_INTERNAL(LOGGER, LEVEL, WRITER, MSG, ...)         \
    if(UA_LOGLEVEL <= UA_LOGLEVEL_##LEVEL) {                            \
        UA_LOG_##LEVEL(LOGGER, UA_LOGCATEGORY_PUBSUB, "%.*s" MSG "%.0s", \
                       (int)(WRITER)->logIdString.length,               \
                       (char*)(WRITER)->logIdString.data,               \
                       __VA_ARGS__);                                    \
    }

#define UA_LOG_TRACE_WRITER(LOGGER, WRITER, ...)                        \
    UA_MACRO_EXPAND(UA_LOG_WRITER_INTERNAL(LOGGER, TRACE, WRITER, __VA_ARGS__, ""))
#define UA_LOG_DEBUG_WRITER(LOGGER, WRITER, ...)                        \
    UA_MACRO_EXPAND(UA_LOG_WRITER_INTERNAL(LOGGER, DEBUG, WRITER, __VA_ARGS__, ""))
#define UA_LOG_INFO_WRITER(LOGGER, WRITER, ...)                         \
    UA_MACRO_EXPAND(UA_LOG_WRITER_INTERNAL(LOGGER, INFO, WRITER, __VA_ARGS__, ""))
#define UA_LOG_WARNING_WRITER(LOGGER, WRITER, ...)                      \
    UA_MACRO_EXPAND(UA_LOG_WRITER_INTERNAL(LOGGER, WARNING, WRITER, __VA_ARGS__, ""))
#define UA_LOG_ERROR_WRITER(LOGGER, WRITER, ...)                        \
    UA_MACRO_EXPAND(UA_LOG_WRITER_INTERNAL(LOGGER, ERROR, WRITER, __VA_ARGS__, ""))
#define UA_LOG_FATAL_WRITER(LOGGER, WRITER, ...)                        \
    UA_MACRO_EXPAND(UA_LOG_WRITER_INTERNAL(LOGGER, FATAL, WRITER, __VA_ARGS__, ""))

/**********************************************/
/*               WriterGroup                  */
/**********************************************/

struct UA_WriterGroup {
    UA_PubSubComponentEnumType componentType;
    UA_WriterGroupConfig config;
    LIST_ENTRY(UA_WriterGroup) listEntry;
    UA_NodeId identifier;
    UA_String logIdString;

    LIST_HEAD(, UA_DataSetWriter) writers;
    UA_UInt32 writersCount;

    UA_UInt64 publishCallbackId; /* registered if != 0 */
    UA_PubSubState state;
    UA_NetworkMessageOffsetBuffer bufferedMessage;
    UA_UInt16 sequenceNumber; /* Increased after every succressuly sent message */
    UA_Boolean configurationFrozen;
    UA_DateTime lastPublishTimeStamp;

    /* The ConnectionManager pointer is stored in the Connection. The channels
     * are either stored here or in the Connection, but never both. */
    UA_PubSubConnection *linkedConnection;
    uintptr_t sendChannel;
    UA_Boolean deleteFlag;

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    UA_UInt32 securityTokenId;
    UA_UInt32 nonceSequenceNumber; /* To be part of the MessageNonce */
    void *securityPolicyContext;
#ifdef UA_ENABLE_PUBSUB_SKS
    UA_PubSubKeyStorage *keyStorage; /* non-owning pointer to keyStorage*/
#endif
#endif
};

UA_StatusCode
UA_WriterGroup_create(UA_Server *server, const UA_NodeId connection,
                      const UA_WriterGroupConfig *writerGroupConfig,
                      UA_NodeId *writerGroupIdentifier);

UA_StatusCode
UA_WriterGroup_remove(UA_Server *server, UA_WriterGroup *wg);

void
UA_WriterGroup_disconnect(UA_WriterGroup *wg);

UA_StatusCode
UA_WriterGroup_connect(UA_Server *server, UA_WriterGroup *wg,
                       UA_Boolean validate);

UA_Boolean
UA_WriterGroup_canConnect(UA_WriterGroup *wg);

UA_StatusCode
setWriterGroupEncryptionKeys(UA_Server *server, const UA_NodeId writerGroup,
                             UA_UInt32 securityTokenId,
                             const UA_ByteString signingKey,
                             const UA_ByteString encryptingKey,
                             const UA_ByteString keyNonce);

UA_StatusCode
UA_WriterGroupConfig_copy(const UA_WriterGroupConfig *src,
                          UA_WriterGroupConfig *dst);

UA_WriterGroup *
UA_WriterGroup_findWGbyId(UA_Server *server, UA_NodeId identifier);

UA_StatusCode
UA_WriterGroup_freezeConfiguration(UA_Server *server, UA_WriterGroup *wg);

UA_StatusCode
UA_WriterGroup_unfreezeConfiguration(UA_Server *server, UA_WriterGroup *wg);

UA_StatusCode
UA_WriterGroup_setPubSubState(UA_Server *server,
                              UA_WriterGroup *writerGroup,
                              UA_PubSubState targetState);
UA_StatusCode
UA_WriterGroup_addPublishCallback(UA_Server *server, UA_WriterGroup *writerGroup);

void
UA_WriterGroup_publishCallback(UA_Server *server,
                               UA_WriterGroup *writerGroup);

UA_StatusCode
UA_WriterGroup_updateConfig(UA_Server *server, UA_WriterGroup *wg,
                            const UA_WriterGroupConfig *config);

UA_StatusCode
UA_WriterGroup_enableWriterGroup(UA_Server *server,
                                 const UA_NodeId writerGroup);

#define UA_LOG_WRITERGROUP_INTERNAL(LOGGER, LEVEL, WG, MSG, ...) \
    if(UA_LOGLEVEL <= UA_LOGLEVEL_##LEVEL) {                            \
        UA_LOG_##LEVEL(LOGGER, UA_LOGCATEGORY_PUBSUB, "%.*s" MSG "%.0s", \
                       (int)(WG)->logIdString.length,                   \
                       (char*)(WG)->logIdString.data, __VA_ARGS__);     \
    }

#define UA_LOG_TRACE_WRITERGROUP(LOGGER, WRITERGROUP, ...)              \
    UA_MACRO_EXPAND(UA_LOG_WRITERGROUP_INTERNAL(LOGGER, TRACE, WRITERGROUP, __VA_ARGS__, ""))
#define UA_LOG_DEBUG_WRITERGROUP(LOGGER, WRITERGROUP, ...)              \
    UA_MACRO_EXPAND(UA_LOG_WRITERGROUP_INTERNAL(LOGGER, DEBUG, WRITERGROUP, __VA_ARGS__, ""))
#define UA_LOG_INFO_WRITERGROUP(LOGGER, WRITERGROUP, ...)               \
    UA_MACRO_EXPAND(UA_LOG_WRITERGROUP_INTERNAL(LOGGER, INFO, WRITERGROUP, __VA_ARGS__, ""))
#define UA_LOG_WARNING_WRITERGROUP(LOGGER, WRITERGROUP, ...)            \
    UA_MACRO_EXPAND(UA_LOG_WRITERGROUP_INTERNAL(LOGGER, WARNING, WRITERGROUP, __VA_ARGS__, ""))
#define UA_LOG_ERROR_WRITERGROUP(LOGGER, WRITERGROUP, ...)              \
    UA_MACRO_EXPAND(UA_LOG_WRITERGROUP_INTERNAL(LOGGER, ERROR, WRITERGROUP, __VA_ARGS__, ""))
#define UA_LOG_FATAL_WRITERGROUP(LOGGER, WRITERGROUP, ...)              \
    UA_MACRO_EXPAND(UA_LOG_WRITERGROUP_INTERNAL(LOGGER, FATAL, WRITERGROUP, __VA_ARGS__, ""))

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
    UA_Boolean configurationFrozen;
} UA_DataSetField;

UA_StatusCode
UA_DataSetFieldConfig_copy(const UA_DataSetFieldConfig *src,
                           UA_DataSetFieldConfig *dst);

UA_DataSetField *
UA_DataSetField_findDSFbyId(UA_Server *server, UA_NodeId identifier);

UA_DataSetFieldResult
UA_DataSetField_remove(UA_Server *server, UA_DataSetField *currentField);

UA_DataSetFieldResult
UA_DataSetField_create(UA_Server *server, const UA_NodeId publishedDataSet,
                       const UA_DataSetFieldConfig *fieldConfig,
                       UA_NodeId *fieldIdentifier);

void
UA_PubSubDataSetField_sampleValue(UA_Server *server, UA_DataSetField *field,
                                  UA_DataValue *value);

/**********************************************/
/*               DataSetReader                */
/**********************************************/

/* DataSetReader Type definition */
typedef struct UA_DataSetReader {
    UA_PubSubComponentEnumType componentType;
    UA_DataSetReaderConfig config;
    UA_NodeId identifier;
    UA_String logIdString;
    UA_ReaderGroup *linkedReaderGroup;
    LIST_ENTRY(UA_DataSetReader) listEntry;

    UA_PubSubState state;
    UA_Boolean configurationFrozen;
    UA_NetworkMessageOffsetBuffer bufferedMessage;

#ifdef UA_ENABLE_PUBSUB_MONITORING
    /* MessageReceiveTimeout handling */
    UA_ServerCallback msgRcvTimeoutTimerCallback;
    UA_UInt64 msgRcvTimeoutTimerId;
    UA_Boolean msgRcvTimeoutTimerRunning;
#endif
    UA_DateTime lastHeartbeatReceived;
} UA_DataSetReader;

/* Process Network Message using DataSetReader */
void
UA_DataSetReader_process(UA_Server *server,
                         UA_DataSetReader *dataSetReader,
                         UA_DataSetMessage *dataSetMsg);

UA_StatusCode
UA_DataSetReader_checkIdentifier(UA_Server *server, UA_NetworkMessage *msg,
                                 UA_DataSetReader *reader,
                                 UA_ReaderGroupConfig readerGroupConfig);

UA_StatusCode
UA_DataSetReader_create(UA_Server *server, UA_NodeId readerGroupIdentifier,
                        const UA_DataSetReaderConfig *dataSetReaderConfig,
                        UA_NodeId *readerIdentifier);

UA_StatusCode
UA_DataSetReader_prepareOffsetBuffer(UA_Server *server, UA_DataSetReader *reader,
                                     UA_ByteString *buf, size_t *pos);

void
UA_DataSetReader_decodeAndProcessRT(UA_Server *server, UA_DataSetReader *dsr,
                                    UA_ByteString *buf);

UA_StatusCode
UA_DataSetReader_remove(UA_Server *server, UA_DataSetReader *dsr);

/* Copy the configuration of Target Variables */
UA_StatusCode UA_TargetVariables_copy(const UA_TargetVariables *src,
                                      UA_TargetVariables *dst);

/* Clear the Target Variables configuration */
void UA_TargetVariables_clear(UA_TargetVariables *subscribedDataSetTarget);

/* Copy the configuration of Field Target Variables */
UA_StatusCode UA_FieldTargetVariable_copy(const UA_FieldTargetVariable *src,
                                          UA_FieldTargetVariable *dst);

UA_StatusCode
DataSetReader_createTargetVariables(UA_Server *server, UA_DataSetReader *dsr,
                                    size_t targetVariablesSize,
                                    const UA_FieldTargetVariable *targetVariables);

/* Returns an error reason if the target state is `Error` */
UA_StatusCode
UA_DataSetReader_setPubSubState(UA_Server *server, UA_DataSetReader *dsr,
                                UA_PubSubState targetState);

#define UA_LOG_READER_INTERNAL(LOGGER, LEVEL, READER, MSG, ...)         \
    if(UA_LOGLEVEL <= UA_LOGLEVEL_##LEVEL) {                            \
        UA_LOG_##LEVEL(LOGGER, UA_LOGCATEGORY_PUBSUB, "%.*s" MSG "%.0s", \
                       (int)(READER)->logIdString.length,               \
                       (char*)(READER)->logIdString.data,               \
                       __VA_ARGS__);                                    \
    }

#define UA_LOG_TRACE_READER(LOGGER, READER, ...)                        \
    UA_MACRO_EXPAND(UA_LOG_READER_INTERNAL(LOGGER, TRACE, READER, __VA_ARGS__, ""))
#define UA_LOG_DEBUG_READER(LOGGER, READER, ...)                        \
    UA_MACRO_EXPAND(UA_LOG_READER_INTERNAL(LOGGER, DEBUG, READER, __VA_ARGS__, ""))
#define UA_LOG_INFO_READER(LOGGER, READER, ...)                         \
    UA_MACRO_EXPAND(UA_LOG_READER_INTERNAL(LOGGER, INFO, READER, __VA_ARGS__, ""))
#define UA_LOG_WARNING_READER(LOGGER, READER, ...)                      \
    UA_MACRO_EXPAND(UA_LOG_READER_INTERNAL(LOGGER, WARNING, READER, __VA_ARGS__, ""))
#define UA_LOG_ERROR_READER(LOGGER, READER, ...)                        \
    UA_MACRO_EXPAND(UA_LOG_READER_INTERNAL(LOGGER, ERROR, READER, __VA_ARGS__, ""))
#define UA_LOG_FATAL_READER(LOGGER, READER, ...)                        \
    UA_MACRO_EXPAND(UA_LOG_READER_INTERNAL(LOGGER, FATAL, READER, __VA_ARGS__, ""))

/**********************************************/
/*                ReaderGroup                 */
/**********************************************/

struct UA_ReaderGroup {
    UA_PubSubComponentEnumType componentType;
    UA_ReaderGroupConfig config;
    UA_NodeId identifier;
    UA_String logIdString;
    LIST_ENTRY(UA_ReaderGroup) listEntry;

    LIST_HEAD(, UA_DataSetReader) readers;
    UA_UInt32 readersCount;

    UA_PubSubState state;
    UA_Boolean configurationFrozen;
    UA_Boolean hasReceived; /* Received a message since the last _connect */

    /* The ConnectionManager pointer is stored in the Connection. The channels 
     * are either stored here or in the Connection, but never both. */
    UA_PubSubConnection *linkedConnection;
    uintptr_t recvChannels[UA_PUBSUB_MAXCHANNELS];
    size_t recvChannelsSize;
    UA_Boolean deleteFlag;

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    UA_UInt32 securityTokenId;
    UA_UInt32 nonceSequenceNumber; /* To be part of the MessageNonce */
    void *securityPolicyContext;
#ifdef UA_ENABLE_PUBSUB_SKS
    UA_PubSubKeyStorage *keyStorage;
#endif
#endif
};

UA_StatusCode
UA_ReaderGroup_create(UA_Server *server, UA_NodeId connectionId,
                      const UA_ReaderGroupConfig *rgc,
                      UA_NodeId *readerGroupId);

UA_StatusCode
UA_ReaderGroup_remove(UA_Server *server, UA_ReaderGroup *rg);

UA_StatusCode
UA_ReaderGroup_connect(UA_Server *server, UA_ReaderGroup *rg, UA_Boolean validate);

void
UA_ReaderGroup_disconnect(UA_ReaderGroup *rg);

UA_StatusCode
setReaderGroupEncryptionKeys(UA_Server *server, const UA_NodeId readerGroup,
                             UA_UInt32 securityTokenId,
                             const UA_ByteString signingKey,
                             const UA_ByteString encryptingKey,
                             const UA_ByteString keyNonce);

UA_StatusCode
UA_ReaderGroupConfig_copy(const UA_ReaderGroupConfig *src,
                          UA_ReaderGroupConfig *dst);

/* Prototypes for internal util functions - some functions maybe removed later
 * (currently moved from public to internal) */
UA_ReaderGroup *
UA_ReaderGroup_findRGbyId(UA_Server *server, UA_NodeId identifier);

UA_DataSetReader *
UA_ReaderGroup_findDSRbyId(UA_Server *server, UA_NodeId identifier);

UA_StatusCode
UA_ReaderGroup_freezeConfiguration(UA_Server *server, UA_ReaderGroup *rg);

UA_StatusCode
UA_ReaderGroup_unfreezeConfiguration(UA_Server *server, UA_ReaderGroup *rg);

UA_StatusCode
UA_ReaderGroup_setPubSubState(UA_Server *server, UA_ReaderGroup *rg,
                              UA_PubSubState targetState);

UA_Boolean
UA_ReaderGroup_decodeAndProcessRT(UA_Server *server, UA_ReaderGroup *readerGroup,
                                    UA_ByteString *buf);

UA_Boolean
UA_ReaderGroup_process(UA_Server *server, UA_ReaderGroup *readerGroup,
                       UA_NetworkMessage *nm);

#define UA_LOG_READERGROUP_INTERNAL(LOGGER, LEVEL, RG, MSG, ...)        \
    if(UA_LOGLEVEL <= UA_LOGLEVEL_##LEVEL) {                            \
        UA_LOG_##LEVEL(LOGGER, UA_LOGCATEGORY_PUBSUB, "%.*s" MSG "%.0s", \
                       (int)(RG)->logIdString.length,                   \
                       (char*)(RG)->logIdString.data,                   \
                       __VA_ARGS__);                                    \
    }

#define UA_LOG_TRACE_READERGROUP(LOGGER, READERGROUP, ...)              \
    UA_MACRO_EXPAND(UA_LOG_READERGROUP_INTERNAL(LOGGER, TRACE, READERGROUP, __VA_ARGS__, ""))
#define UA_LOG_DEBUG_READERGROUP(LOGGER, READERGROUP, ...)              \
    UA_MACRO_EXPAND(UA_LOG_READERGROUP_INTERNAL(LOGGER, DEBUG, READERGROUP, __VA_ARGS__, ""))
#define UA_LOG_INFO_READERGROUP(LOGGER, READERGROUP, ...)               \
    UA_MACRO_EXPAND(UA_LOG_READERGROUP_INTERNAL(LOGGER, INFO, READERGROUP, __VA_ARGS__, ""))
#define UA_LOG_WARNING_READERGROUP(LOGGER, READERGROUP, ...)            \
    UA_MACRO_EXPAND(UA_LOG_READERGROUP_INTERNAL(LOGGER, WARNING, READERGROUP, __VA_ARGS__, ""))
#define UA_LOG_ERROR_READERGROUP(LOGGER, READERGROUP, ...)              \
    UA_MACRO_EXPAND(UA_LOG_READERGROUP_INTERNAL(LOGGER, ERROR, READERGROUP, __VA_ARGS__, ""))
#define UA_LOG_FATAL_READERGROUP(LOGGER, READERGROUP, ...)              \
    UA_MACRO_EXPAND(UA_LOG_READERGROUP_INTERNAL(LOGGER, FATAL, READERGROUP, __VA_ARGS__, ""))

/*********************************************************/
/*               Reading Message handling                */
/*********************************************************/

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
UA_StatusCode
verifyAndDecrypt(const UA_Logger *logger, UA_ByteString *buffer,
                 const size_t *currentPosition, const UA_NetworkMessage *nm,
                 UA_Boolean doValidate, UA_Boolean doDecrypt,
                 void *channelContext, UA_PubSubSecurityPolicy *securityPolicy);

UA_StatusCode
verifyAndDecryptNetworkMessage(const UA_Logger *logger, UA_ByteString *buffer,
                               size_t *currentPosition, UA_NetworkMessage *nm,
                               UA_ReaderGroup *readerGroup);
#endif

/* Takes a value (and not a pointer) to the buffer. The original buffer is
   const. Internally we may adjust the length during decryption. */
UA_StatusCode
decodeNetworkMessage(UA_Server *server, UA_ByteString *buffer, size_t *pos,
                     UA_NetworkMessage *nm, UA_PubSubConnection *connection);

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

/* finds the SecurityGroup within the server by SecurityGroup Name/Id*/
UA_SecurityGroup *
UA_SecurityGroup_findSGbyName(UA_Server *server, UA_String securityGroupName);

/* finds the SecurityGroup within the server by NodeId*/
UA_SecurityGroup *
UA_SecurityGroup_findSGbyId(UA_Server *server, UA_NodeId identifier);

void
UA_SecurityGroup_delete(UA_SecurityGroup *securityGroup);

void
removeSecurityGroup(UA_Server *server, UA_SecurityGroup *securityGroup);

#endif /* UA_ENABLE_PUBSUB_SKS */

/******************/
/* PubSub Manager */
/******************/

typedef struct UA_TopicAssign {
    UA_ReaderGroup *rgIdentifier;
    UA_String topic;
    TAILQ_ENTRY(UA_TopicAssign) listEntry;
} UA_TopicAssign;

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

typedef struct UA_PubSubManager {
    UA_UInt64 defaultPublisherId;
    /* Connections and PublishedDataSets can exist alone (own lifecycle) -> top
     * level components */
    size_t connectionsSize;
    TAILQ_HEAD(, UA_PubSubConnection) connections;

    size_t publishedDataSetsSize;
    TAILQ_HEAD(, UA_PublishedDataSet) publishedDataSets;

    size_t subscribedDataSetsSize;
    TAILQ_HEAD(, UA_StandaloneSubscribedDataSet) subscribedDataSets;

    size_t topicAssignSize;
    TAILQ_HEAD(, UA_TopicAssign) topicAssign;

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
} UA_PubSubManager;

UA_StatusCode
UA_PubSubManager_addPubSubTopicAssign(UA_Server *server, UA_ReaderGroup *readerGroup,
                                      UA_String topic);

UA_StatusCode
UA_PubSubManager_reserveIds(UA_Server *server, UA_NodeId sessionId, UA_UInt16 numRegWriterGroupIds,
                            UA_UInt16 numRegDataSetWriterIds, UA_String transportProfileUri,
                            UA_UInt16 **writerGroupIds, UA_UInt16 **dataSetWriterIds);

void
UA_PubSubManager_freeIds(UA_Server *server);

void
UA_PubSubManager_init(UA_Server *server, UA_PubSubManager *pubSubManager);

void
UA_PubSubManager_shutdown(UA_Server *server, UA_PubSubManager *pubSubManager);

void
UA_PubSubManager_delete(UA_Server *server, UA_PubSubManager *pubSubManager);

#ifndef UA_ENABLE_PUBSUB_INFORMATIONMODEL
void
UA_PubSubManager_generateUniqueNodeId(UA_PubSubManager *psm, UA_NodeId *nodeId);
#endif

#ifdef UA_ENABLE_PUBSUB_FILE_CONFIG
/* Decodes the information from the ByteString. If the decoded content is a
 * PubSubConfiguration in a UABinaryFileDataType-object. It will overwrite the
 * current PubSub configuration from the server. */
UA_StatusCode
UA_PubSubManager_loadPubSubConfigFromByteString(UA_Server *server,
                                                const UA_ByteString buffer);

/* Saves the current PubSub configuration of a server in a byteString. */
UA_StatusCode
UA_PubSubManager_getEncodedPubSubConfiguration(UA_Server *server,
                                               UA_ByteString *buffer);
#endif

UA_Guid
UA_PubSubManager_generateUniqueGuid(UA_Server *server);

UA_UInt32
UA_PubSubConfigurationVersionTimeDifference(UA_DateTime now);

/*************************************************/
/*      PubSub component monitoring              */
/*************************************************/

#ifdef UA_ENABLE_PUBSUB_MONITORING

UA_StatusCode
UA_PubSubManager_setDefaultMonitoringCallbacks(UA_PubSubMonitoringInterface *monitoringInterface);

#endif /* UA_ENABLE_PUBSUB_MONITORING */

#endif /* UA_ENABLE_PUBSUB */

_UA_END_DECLS

#endif /* UA_PUBSUB_H_ */
