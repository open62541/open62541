/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 * Copyright (c) 2020 Yannick Wallerer, Siemens AG
 * Copyright (c) 2020 Thomas Fischer, Siemens AG
 * Copyright (c) 2021 Fraunhofer IOSB (Author: Jan Hermes)
 */

#ifndef UA_PUBSUB_H_
#define UA_PUBSUB_H_

#include <open62541/plugin/pubsub.h>
#include <open62541/server.h>
#include <open62541/server_pubsub.h>

#include "open62541_queue.h"
#include "ua_pubsub_networkmessage.h"

/* The public configuration structs are defined in include/ua_plugin_pubsub.h */

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_PUBSUB

struct UA_WriterGroup;
typedef struct UA_WriterGroup UA_WriterGroup;

struct UA_ReaderGroup;
typedef struct UA_ReaderGroup UA_ReaderGroup;

/**********************************************/
/*            PublishedDataSet                */
/**********************************************/

typedef struct UA_PublishedDataSet {
    UA_PublishedDataSetConfig config;
    UA_DataSetMetaDataType dataSetMetaData;
    TAILQ_HEAD(UA_ListOfDataSetField, UA_DataSetField) fields;
    UA_NodeId identifier;
    UA_UInt16 fieldSize;
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

void
UA_PublishedDataSet_clear(UA_Server *server,
                          UA_PublishedDataSet *publishedDataSet);

/**********************************************/
/*               Connection                   */
/**********************************************/

typedef struct UA_PubSubConnection {
    UA_PubSubComponentEnumType componentType;
    UA_PubSubConnectionConfig *config;
    UA_PubSubChannel *channel;
    UA_NodeId identifier;
    LIST_HEAD(UA_ListOfWriterGroup, UA_WriterGroup) writerGroups;
    size_t writerGroupsSize;
    LIST_HEAD(UA_ListOfPubSubReaderGroup, UA_ReaderGroup) readerGroups;
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

void
UA_PubSubConnectionConfig_clear(UA_PubSubConnectionConfig *connectionConfig);

UA_StatusCode
removePubSubConnection(UA_Server *server, const UA_NodeId connection);

void
UA_PubSubConnection_clear(UA_Server *server, UA_PubSubConnection *connection);

/* Register channel for given connectionIdentifier */
UA_StatusCode
UA_PubSubConnection_regist(UA_Server *server, UA_NodeId *connectionIdentifier);

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
removeDataSetWriter(UA_Server *server, const UA_NodeId dsw);

/**********************************************/
/*               WriterGroup                  */
/**********************************************/

struct UA_WriterGroup {
    UA_PubSubComponentEnumType componentType;
    UA_WriterGroupConfig config;
    LIST_ENTRY(UA_WriterGroup) listEntry;
    UA_NodeId identifier;
    UA_PubSubConnection *linkedConnection;
    LIST_HEAD(UA_ListOfDataSetWriter, UA_DataSetWriter) writers;
    UA_UInt32 writersCount;
    UA_UInt64 publishCallbackId;
    UA_Boolean publishCallbackIsRegistered;
    UA_PubSubState state;
    UA_NetworkMessageOffsetBuffer bufferedMessage;
    UA_UInt16 sequenceNumber; /* Increased after every succressuly sent message */
    UA_Boolean configurationFrozen;

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    UA_UInt32 securityTokenId;
    UA_UInt32 nonceSequenceNumber; /* To be part of the MessageNonce */
    void *securityPolicyContext;
#endif
};

UA_StatusCode
removeWriterGroup(UA_Server *server, const UA_NodeId writerGroup);

UA_StatusCode
UA_WriterGroupConfig_copy(const UA_WriterGroupConfig *src,
                          UA_WriterGroupConfig *dst);

UA_WriterGroup *
UA_WriterGroup_findWGbyId(UA_Server *server, UA_NodeId identifier);

UA_StatusCode
UA_WriterGroup_setPubSubState(UA_Server *server,
                              UA_WriterGroup *writerGroup,
                              UA_PubSubState state,
                              UA_StatusCode cause);

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
addDataSetField(UA_Server *server, const UA_NodeId publishedDataSet,
                const UA_DataSetFieldConfig *fieldConfig,
                UA_NodeId *fieldIdentifier);

UA_DataSetFieldResult
removeDataSetField(UA_Server *server, const UA_NodeId dsf);

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
UA_DataSetReader_setPubSubState(UA_Server *server,
                                UA_DataSetReader *dataSetReader,
                                UA_PubSubState state,
                                UA_StatusCode cause);

#ifdef UA_ENABLE_PUBSUB_MONITORING
/* Check if DataSetReader has a message receive timeout */
void
UA_DataSetReader_checkMessageReceiveTimeout(UA_Server *server,
                                            UA_DataSetReader *dataSetReader);

/* DataSetReader MessageReceiveTimeout callback for generic PubSub component
 * timeout handling */
void
UA_DataSetReader_handleMessageReceiveTimeout(UA_Server *server,
                                             void *dataSetReader);
#endif /* UA_ENABLE_PUBSUB_MONITORING */

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

/**********************************************/
/*                ReaderGroup                 */
/**********************************************/

struct UA_ReaderGroup {
    UA_PubSubComponentEnumType componentType;
    UA_ReaderGroupConfig config;
    UA_NodeId identifier;
    UA_NodeId linkedConnection;
    LIST_ENTRY(UA_ReaderGroup) listEntry;
    LIST_HEAD(UA_ListOfPubSubDataSetReader, UA_DataSetReader) readers;
    /* for simplified information access */
    UA_UInt32 readersCount;
    UA_UInt64 subscribeCallbackId;
    UA_PubSubState state;
    UA_Boolean configurationFrozen;

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    UA_UInt32 securityTokenId;
    UA_UInt32 nonceSequenceNumber; /* To be part of the MessageNonce */
    void *securityPolicyContext;
#endif
};

UA_StatusCode
removeReaderGroup(UA_Server *server, UA_NodeId groupIdentifier);

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

#endif /* UA_ENABLE_PUBSUB */

_UA_END_DECLS

#endif /* UA_PUBSUB_H_ */
