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
#include <open62541/plugin/pubsub.h>
#include <open62541/server.h>
#include <open62541/server_pubsub.h>

#include "open62541_queue.h"
#include "ua_pubsub_networkmessage.h"

#ifdef UA_ENABLE_PUBSUB_SKS
#include <ua_pubsub_keystorage.h>
#endif

/* The public configuration structs are defined in include/ua_plugin_pubsub.h */

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_PUBSUB

struct UA_WriterGroup;
typedef struct UA_WriterGroup UA_WriterGroup;

struct UA_ReaderGroup;
typedef struct UA_ReaderGroup UA_ReaderGroup;

struct UA_SecurityGroup;
typedef struct UA_SecurityGroup UA_SecurityGroup;

/**********************************************/
/*            PublishedDataSet                */
/**********************************************/

typedef struct UA_PublishedDataSet {
    UA_PublishedDataSetConfig config;
    UA_DataSetMetaDataType dataSetMetaData;
    TAILQ_HEAD(, UA_DataSetField) fields;
    UA_UInt16 fieldSize;
    UA_NodeId identifier;
    UA_UInt16 promotedFieldsCount;
    UA_UInt16 configurationFreezeCounter;
    TAILQ_ENTRY(UA_PublishedDataSet) listEntry;
    UA_Boolean configurationFrozen;
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

#define UA_LOG_PDS_INTERNAL(LOGGER, LEVEL, PDS, MSG, ...)               \
    if(UA_LOGLEVEL <= UA_LOGLEVEL_##LEVEL) {                            \
        UA_String idStr = UA_STRING_NULL;                               \
        if(PDS)                                                         \
            UA_NodeId_print(&(PDS)->identifier, &idStr);                \
        UA_LOG_##LEVEL(LOGGER, UA_LOGCATEGORY_PUBSUB,                   \
                       "DataSet %.*s\t| " MSG "%.0s", (int)idStr.length, \
                       (char*)idStr.data, __VA_ARGS__);                 \
        UA_String_clear(&idStr);                                        \
    }

#define UA_LOG_TRACE_DATASET(LOGGER, PDS, ...)                          \
    UA_MACRO_EXPAND(UA_LOG_PDS_INTERNAL(LOGGER, TRACE, PDS, __VA_ARGS__, ""))
#define UA_LOG_DEBUG_DATASET(LOGGER, PDS, ...)                          \
    UA_MACRO_EXPAND(UA_LOG_PDS_INTERNAL(LOGGER, DEBUG, PDS, __VA_ARGS__, ""))
#define UA_LOG_INFO_DATASET(LOGGER, PDS, ...)                           \
    UA_MACRO_EXPAND(UA_LOG_PDS_INTERNAL(LOGGER, INFO, PDS, __VA_ARGS__, ""))
#define UA_LOG_WARNING_DATASET(LOGGER, PDS, ...)                        \
    UA_MACRO_EXPAND(UA_LOG_PDS_INTERNAL(LOGGER, WARNING, PDS, __VA_ARGS__, ""))
#define UA_LOG_ERROR_DATASET(LOGGER, PDS, ...)                          \
    UA_MACRO_EXPAND(UA_LOG_PDS_INTERNAL(LOGGER, ERROR, PDS, __VA_ARGS__, ""))
#define UA_LOG_FATAL_DATASET(LOGGER, PDS, ...)                          \
    UA_MACRO_EXPAND(UA_LOG_PDS_INTERNAL(LOGGER, FATAL, PDS, __VA_ARGS__, ""))

/**********************************************/
/*               Connection                   */
/**********************************************/

typedef struct UA_PubSubConnection {
    UA_PubSubComponentEnumType componentType;
    UA_PubSubConnectionConfig config;
    UA_PubSubChannel *channel;
    UA_NodeId identifier;
    UA_PubSubState state;
    LIST_HEAD(UA_ListOfWriterGroup, UA_WriterGroup) writerGroups;
    size_t writerGroupsSize;

    LIST_HEAD(, UA_ReaderGroup) readerGroups;
    size_t readerGroupsSize;

    TAILQ_ENTRY(UA_PubSubConnection) listEntry;
    UA_UInt16 configurationFreezeCounter;
    UA_Boolean isRegistered; /* Subscriber requires connection channel regist */
    UA_Boolean configurationFrozen;
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

UA_StatusCode
removePubSubConnection(UA_Server *server, const UA_NodeId connection);

void
UA_PubSubConnection_clear(UA_Server *server, UA_PubSubConnection *connection);

/* Register channel for given connectionIdentifier */
UA_StatusCode
UA_PubSubConnection_regist(UA_Server *server, UA_NodeId *connectionIdentifier,
                           const UA_ReaderGroupConfig *readerGroupConfig);

UA_StatusCode
UA_PubSubConnection_setPubSubState(UA_Server *server,
                                UA_PubSubConnection *connection,
                                UA_PubSubState state,
                                UA_StatusCode cause);


#define UA_LOG_CONNECTION_INTERNAL(LOGGER, LEVEL, CONNECTION, MSG, ...) \
    if(UA_LOGLEVEL <= UA_LOGLEVEL_##LEVEL) {                            \
        UA_String idStr = UA_STRING_NULL;                               \
        if(CONNECTION)                                                  \
            UA_NodeId_print(&(CONNECTION)->identifier, &idStr);         \
        UA_LOG_##LEVEL(LOGGER, UA_LOGCATEGORY_PUBSUB,                   \
                       "Connection %.*s\t| " MSG "%.0s", (int)idStr.length, \
                       (char*)idStr.data, __VA_ARGS__);                 \
        UA_String_clear(&idStr);                                        \
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

UA_StatusCode
UA_decodeAndProcessNetworkMessage(UA_Server *server,
                                  UA_PubSubConnection *connection,
                                  UA_ByteString *buf);

/**********************************************/
/*              DataSetWriter                 */
/**********************************************/

#ifdef UA_ENABLE_PUBSUB_DELTAFRAMES
typedef struct UA_DataSetWriterSample {
    UA_Boolean valueChanged;
    UA_DataValue value;
} UA_DataSetWriterSample;
#endif

typedef struct UA_DataSetWriter {
    UA_PubSubComponentEnumType componentType;
    UA_DataSetWriterConfig config;
    LIST_ENTRY(UA_DataSetWriter) listEntry;
    UA_NodeId identifier;
    UA_NodeId linkedWriterGroup;
    UA_NodeId connectedDataSet;
    UA_ConfigurationVersionDataType connectedDataSetVersion;
    UA_PubSubState state;
#ifdef UA_ENABLE_PUBSUB_DELTAFRAMES
    UA_UInt16 deltaFrameCounter; /* count of sent deltaFrames */
    size_t lastSamplesCount;
    UA_DataSetWriterSample *lastSamples;
#endif
    UA_UInt16 actualDataSetMessageSequenceCount;
    UA_Boolean configurationFrozen;
} UA_DataSetWriter;

UA_StatusCode
UA_DataSetWriterConfig_copy(const UA_DataSetWriterConfig *src,
                            UA_DataSetWriterConfig *dst);

UA_DataSetWriter *
UA_DataSetWriter_findDSWbyId(UA_Server *server, UA_NodeId identifier);

UA_StatusCode
UA_DataSetWriter_setPubSubState(UA_Server *server,
                                UA_DataSetWriter *dataSetWriter,
                                UA_PubSubState state,
                                UA_StatusCode cause);

UA_StatusCode
UA_DataSetWriter_generateDataSetMessage(UA_Server *server,
                                        UA_DataSetMessage *dataSetMessage,
                                        UA_DataSetWriter *dataSetWriter);

UA_StatusCode
UA_DataSetWriter_remove(UA_Server *server, UA_WriterGroup *linkedWriterGroup,
                        UA_DataSetWriter *dataSetWriter);

UA_StatusCode
UA_DataSetWriter_create(UA_Server *server,
                        const UA_NodeId writerGroup, const UA_NodeId dataSet,
                        const UA_DataSetWriterConfig *dataSetWriterConfig,
                        UA_NodeId *writerIdentifier);

UA_StatusCode
removeDataSetWriter(UA_Server *server, const UA_NodeId dsw);

#define UA_LOG_WRITER_INTERNAL(LOGGER, LEVEL, WRITER, MSG, ...)         \
    if(UA_LOGLEVEL <= UA_LOGLEVEL_##LEVEL) {                            \
        UA_String idStr = UA_STRING_NULL;                               \
        UA_String groupIdStr = UA_STRING_NULL;                          \
        if(WRITER) {                                                    \
            UA_NodeId_print(&(WRITER)->identifier, &idStr);             \
            UA_NodeId_print(&(WRITER)->linkedWriterGroup, &groupIdStr); \
        }                                                               \
        UA_LOG_##LEVEL(LOGGER, UA_LOGCATEGORY_PUBSUB,                   \
                       "WriterGroup %.*s\t| Writer %.*s\t| " MSG "%.0s", \
                       (int)groupIdStr.length, (char*)groupIdStr.data,  \
                       (int)idStr.length, (char*)idStr.data,            \
                       __VA_ARGS__);                                    \
        UA_String_clear(&idStr);                                        \
        UA_String_clear(&groupIdStr);                                   \
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
    UA_PubSubConnection *linkedConnection;
    LIST_HEAD(, UA_DataSetWriter) writers;
    UA_UInt32 writersCount;
    UA_UInt64 publishCallbackId; /* registered if != 0 */
    UA_PubSubState state;
    UA_NetworkMessageOffsetBuffer bufferedMessage;
    UA_UInt16 sequenceNumber; /* Increased after every succressuly sent message */
    UA_Boolean configurationFrozen;
    UA_DateTime lastPublishTimeStamp;
    UA_PubSubChannel *channel; /* channel used for udp unicast communication */

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
removeWriterGroup(UA_Server *server, const UA_NodeId writerGroup);

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

void
UA_WriterGroup_removePublishCallback(UA_Server *server, UA_WriterGroup *wg);

UA_StatusCode
UA_WriterGroup_setPubSubState(UA_Server *server,
                              UA_WriterGroup *writerGroup,
                              UA_PubSubState state,
                              UA_StatusCode cause);

UA_StatusCode
UA_WriterGroup_updateConfig(UA_Server *server, UA_WriterGroup *wg,
                            const UA_WriterGroupConfig *config);

#define UA_LOG_WRITERGROUP_INTERNAL(LOGGER, LEVEL, WRITERGROUP, MSG, ...) \
    if(UA_LOGLEVEL <= UA_LOGLEVEL_##LEVEL) {                            \
        UA_String idStr = UA_STRING_NULL;                               \
        if(WRITERGROUP)                                                 \
            UA_NodeId_print(&(WRITERGROUP)->identifier, &idStr);        \
        UA_LOG_##LEVEL(LOGGER, UA_LOGCATEGORY_PUBSUB,                   \
                       "WriterGroup %.*s\t| " MSG "%.0s",               \
                       (int)idStr.length, (char*)idStr.data,            \
                       __VA_ARGS__);                                    \
        UA_String_clear(&idStr);                                        \
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
removeDataSetField(UA_Server *server, const UA_NodeId dsf);

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
    UA_NodeId linkedReaderGroup;
    LIST_ENTRY(UA_DataSetReader) listEntry;

    UA_PubSubState state; /* non std */
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
                         UA_ReaderGroup *readerGroup,
                         UA_DataSetReader *dataSetReader,
                         UA_DataSetMessage *dataSetMsg);

UA_StatusCode
UA_DataSetReader_create(UA_Server *server, UA_NodeId readerGroupIdentifier,
                        const UA_DataSetReaderConfig *dataSetReaderConfig,
                        UA_NodeId *readerIdentifier);

UA_StatusCode
removeDataSetReader(UA_Server *server, UA_NodeId readerIdentifier);

/* Copy the configuration of DataSetReader */
UA_StatusCode UA_DataSetReaderConfig_copy(const UA_DataSetReaderConfig *src,
                                          UA_DataSetReaderConfig *dst);

/* Clear the configuration of a DataSetReader */
void UA_DataSetReaderConfig_clear(UA_DataSetReaderConfig *cfg);

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

UA_StatusCode
UA_DataSetReader_setPubSubState(UA_Server *server,
                                UA_DataSetReader *dataSetReader,
                                UA_PubSubState state,
                                UA_StatusCode cause);

UA_StatusCode
UA_DataSetReader_generateNetworkMessage(UA_PubSubConnection *pubSubConnection,
                                        UA_ReaderGroup *readerGroup,
                                        UA_DataSetReader *dataSetReader,
                                        UA_DataSetMessage *dsm, UA_UInt16 *writerId,
                                        UA_Byte dsmCount, UA_NetworkMessage *nm);

UA_StatusCode
UA_DataSetReader_generateDataSetMessage(UA_Server *server,
                                        UA_DataSetMessage *dataSetMessage,
                                        UA_DataSetReader *dataSetReader);

#define UA_LOG_READER_INTERNAL(LOGGER, LEVEL, READER, MSG, ...)         \
    if(UA_LOGLEVEL <= UA_LOGLEVEL_##LEVEL) {                            \
        UA_String idStr = UA_STRING_NULL;                               \
        UA_String groupIdStr = UA_STRING_NULL;                          \
        if(READER) {                                                    \
            UA_NodeId_print(&(READER)->identifier, &idStr);             \
            UA_NodeId_print(&(READER)->linkedReaderGroup, &groupIdStr); \
        }                                                               \
        UA_LOG_##LEVEL(LOGGER, UA_LOGCATEGORY_PUBSUB,                   \
                       "ReaderGroup %.*s\t| Reader %.*s\t| " MSG "%.0s", \
                       (int)groupIdStr.length, (char*)groupIdStr.data,  \
                       (int)idStr.length, (char*)idStr.data,            \
                       __VA_ARGS__);                                    \
        UA_String_clear(&idStr);                                        \
        UA_String_clear(&groupIdStr);                                   \
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
    UA_PubSubConnection *linkedConnection;
    LIST_ENTRY(UA_ReaderGroup) listEntry;
    LIST_HEAD(, UA_DataSetReader) readers;
    /* for simplified information access */
    UA_UInt32 readersCount;
    UA_UInt64 subscribeCallbackId;
    UA_PubSubState state;
    UA_Boolean configurationFrozen;

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
UA_ReaderGroup_create(UA_Server *server, UA_NodeId connectionIdentifier,
                      const UA_ReaderGroupConfig *readerGroupConfig,
                      UA_NodeId *readerGroupIdentifier);

UA_StatusCode
removeReaderGroup(UA_Server *server, UA_NodeId groupIdentifier);

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
UA_ReaderGroup_setPubSubState(UA_Server *server,
                              UA_ReaderGroup *readerGroup,
                              UA_PubSubState state,
                              UA_StatusCode cause);

/*********************************************************/
/*               PublishValues handling                  */
/*********************************************************/

UA_StatusCode
UA_WriterGroup_addPublishCallback(UA_Server *server, UA_WriterGroup *writerGroup);

void
UA_WriterGroup_publishCallback(UA_Server *server, UA_WriterGroup *writerGroup);

/*********************************************************/
/*               SubscribeValues handling                */
/*********************************************************/

UA_StatusCode
UA_ReaderGroup_addSubscribeCallback(UA_Server *server, UA_ReaderGroup *readerGroup);

void
UA_ReaderGroup_removeSubscribeCallback(UA_Server *server, UA_ReaderGroup *readerGroup);

void
UA_ReaderGroup_subscribeCallback(UA_Server *server, UA_ReaderGroup *readerGroup);

#define UA_LOG_READERGROUP_INTERNAL(LOGGER, LEVEL, RG, MSG, ...)        \
    if(UA_LOGLEVEL <= UA_LOGLEVEL_##LEVEL) {                            \
        UA_String idStr = UA_STRING_NULL;                               \
        if(RG)                                                          \
            UA_NodeId_print(&(RG)->identifier, &idStr);                 \
        UA_LOG_##LEVEL(LOGGER, UA_LOGCATEGORY_PUBSUB,                   \
                       "ReaderGroup %.*s\t| " MSG "%.0s", (int)idStr.length, \
                       (char*)idStr.data, __VA_ARGS__);                 \
        UA_String_clear(&idStr);                                        \
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

UA_StatusCode
receiveBufferedNetworkMessage(UA_Server *server, UA_ReaderGroup *readerGroup,
                              UA_PubSubConnection *connection);

/* It serves as the entry point into reader processing and is called when
 * a publish is received, and the topic matches with one or more readers. */
void processMqttSubscriberCallback(UA_Server *server, UA_PubSubConnection *connection,
                                   UA_ByteString *msg);

UA_StatusCode
decodeNetworkMessageJson(UA_Server *server, UA_ByteString *buffer, size_t *pos,
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
UA_PubSubManager_delete(UA_Server *server, UA_PubSubManager *pubSubManager);

#ifndef UA_ENABLE_PUBSUB_INFORMATIONMODEL
void
UA_PubSubManager_generateUniqueNodeId(UA_PubSubManager *psm, UA_NodeId *nodeId);
#endif

UA_Guid
UA_PubSubManager_generateUniqueGuid(UA_Server *server);

UA_UInt32
UA_PubSubConfigurationVersionTimeDifference(void);

UA_PubSubTransportLayer *
UA_getTransportProtocolLayer(const UA_Server *server,
                             const UA_String *transportProfileUri);

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
