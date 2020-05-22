/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 */

#ifndef UA_PUBSUB_H_
#define UA_PUBSUB_H_

#include <open62541/plugin/pubsub.h>
#include <open62541/server.h>
#include <open62541/server_pubsub.h>

#include "open62541_queue.h"
#include "ua_pubsub_networkmessage.h"

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

/* forward declarations */
struct UA_WriterGroup;
typedef struct UA_WriterGroup UA_WriterGroup;

/* Declaration for ReaderGroup */
struct UA_ReaderGroup;
typedef struct UA_ReaderGroup UA_ReaderGroup;

/* The configuration structs (public part of PubSub entities) are defined in include/ua_plugin_pubsub.h */

/**********************************************/
/*            PublishedDataSet                */
/**********************************************/
typedef struct UA_PublishedDataSet{
    UA_PublishedDataSetConfig config;
    UA_DataSetMetaDataType dataSetMetaData;
    TAILQ_HEAD(UA_ListOfDataSetField, UA_DataSetField) fields;
    UA_NodeId identifier;
    UA_UInt16 fieldSize;
    UA_UInt16 promotedFieldsCount;
    UA_UInt16 configurationFreezeCounter;
    TAILQ_ENTRY(UA_PublishedDataSet) listEntry;
    /* This flag is 'read only' and is set internally based on the PubSub state. */
    UA_Boolean configurationFrozen;
} UA_PublishedDataSet;

UA_StatusCode
UA_PublishedDataSetConfig_copy(const UA_PublishedDataSetConfig *src, UA_PublishedDataSetConfig *dst);
UA_PublishedDataSet *
UA_PublishedDataSet_findPDSbyId(UA_Server *server, UA_NodeId identifier);
void
UA_PublishedDataSet_clear(UA_Server *server, UA_PublishedDataSet *publishedDataSet);

/**********************************************/
/*               Connection                   */
/**********************************************/
//the connection config (public part of connection) object is defined in include/ua_plugin_pubsub.h
typedef struct UA_PubSubConnection{
    UA_PubSubConnectionConfig *config;
    //internal fields
    UA_PubSubChannel *channel;
    UA_NodeId identifier;
    LIST_HEAD(UA_ListOfWriterGroup, UA_WriterGroup) writerGroups;
    LIST_HEAD(UA_ListOfPubSubReaderGroup, UA_ReaderGroup) readerGroups;
    size_t readerGroupsSize;
    TAILQ_ENTRY(UA_PubSubConnection) listEntry;
    UA_UInt16 configurationFreezeCounter;
    /* This flag is 'read only' and is set internally based on the PubSub state. */
    UA_Boolean configurationFrozen;
} UA_PubSubConnection;

UA_StatusCode
UA_PubSubConnectionConfig_copy(const UA_PubSubConnectionConfig *src, UA_PubSubConnectionConfig *dst);
UA_PubSubConnection *
UA_PubSubConnection_findConnectionbyId(UA_Server *server, UA_NodeId connectionIdentifier);
void
UA_PubSubConnectionConfig_clear(UA_PubSubConnectionConfig *connectionConfig);
void
UA_PubSubConnection_clear(UA_Server *server, UA_PubSubConnection *connection);
/* Register channel for given connectionIdentifier */
UA_StatusCode
UA_PubSubConnection_regist(UA_Server *server, UA_NodeId *connectionIdentifier);

/**********************************************/
/*              DataSetWriter                 */
/**********************************************/

#ifdef UA_ENABLE_PUBSUB_DELTAFRAMES
typedef struct UA_DataSetWriterSample{
    UA_Boolean valueChanged;
    UA_DataValue value;
} UA_DataSetWriterSample;
#endif

typedef struct UA_DataSetWriter{
    UA_DataSetWriterConfig config;
    //internal fields
    LIST_ENTRY(UA_DataSetWriter) listEntry;
    UA_NodeId identifier;
    UA_NodeId linkedWriterGroup;
    UA_NodeId connectedDataSet;
    UA_ConfigurationVersionDataType connectedDataSetVersion;
    UA_PubSubState state;
#ifdef UA_ENABLE_PUBSUB_DELTAFRAMES
    UA_UInt16 deltaFrameCounter;            //actual count of sent deltaFrames
    size_t lastSamplesCount;
    UA_DataSetWriterSample *lastSamples;
#endif
    UA_UInt16 actualDataSetMessageSequenceCount;
    /* This flag is 'read only' and is set internally based on the PubSub state. */
    UA_Boolean configurationFrozen;
} UA_DataSetWriter;

UA_StatusCode
UA_DataSetWriterConfig_copy(const UA_DataSetWriterConfig *src, UA_DataSetWriterConfig *dst);
UA_DataSetWriter *
UA_DataSetWriter_findDSWbyId(UA_Server *server, UA_NodeId identifier);
UA_StatusCode
UA_DataSetWriter_setPubSubState(UA_Server *server, UA_PubSubState state, UA_DataSetWriter *dataSetWriter);

/**********************************************/
/*               WriterGroup                  */
/**********************************************/

struct UA_WriterGroup{
    UA_WriterGroupConfig config;
    //internal fields
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
    /* This flag is 'read only' and is set internally based on the PubSub state. */
    UA_Boolean configurationFrozen;
};

UA_StatusCode
UA_WriterGroupConfig_copy(const UA_WriterGroupConfig *src, UA_WriterGroupConfig *dst);
UA_WriterGroup *
UA_WriterGroup_findWGbyId(UA_Server *server, UA_NodeId identifier);
UA_StatusCode
UA_WriterGroup_setPubSubState(UA_Server *server, UA_PubSubState state, UA_WriterGroup *writerGroup);

/**********************************************/
/*               DataSetField                 */
/**********************************************/

typedef struct UA_DataSetField{
    UA_DataSetFieldConfig config;
    //internal fields
    TAILQ_ENTRY(UA_DataSetField) listEntry;
    UA_NodeId identifier;
    UA_NodeId publishedDataSet;             //ref to parent pds
    UA_FieldMetaData fieldMetaData;
    UA_UInt64 sampleCallbackId;
    UA_Boolean sampleCallbackIsRegistered;
    /* This flag is 'read only' and is set internally based on the PubSub state. */
    UA_Boolean configurationFrozen;
} UA_DataSetField;

UA_StatusCode
UA_DataSetFieldConfig_copy(const UA_DataSetFieldConfig *src, UA_DataSetFieldConfig *dst);
UA_DataSetField *
UA_DataSetField_findDSFbyId(UA_Server *server, UA_NodeId identifier);

/**********************************************/
/*               DataSetReader                */
/**********************************************/

/* SubscribedDataSetDataType Definition */
typedef enum {
    UA_PUBSUB_SDS_TARGET,
    UA_PUBSUB_SDS_MIRROR
}UA_SubscribedDataSetEnumType;

/* DataSetReader Type definition */
typedef struct UA_DataSetReader {
    UA_DataSetReaderConfig config;
    /* implementation defined fields */
    UA_NodeId identifier;
    UA_NodeId linkedReaderGroup;
    LIST_ENTRY(UA_DataSetReader) listEntry;
    UA_SubscribedDataSetEnumType subscribedDataSetType;
    UA_TargetVariablesDataType subscribedDataSetTarget;
    /* TODO UA_SubscribedDataSetMirrorDataType subscribedDataSetMirror */
    /* non std */
    UA_PubSubState state;
    /* This flag is 'read only' and is set internally based on the PubSub state. */
    UA_Boolean configurationFrozen;
    UA_NetworkMessageOffsetBuffer bufferedMessage;
}UA_DataSetReader;

/* Process Network Message using DataSetReader */
void UA_Server_DataSetReader_process(UA_Server *server, UA_DataSetReader *dataSetReader, UA_DataSetMessage* dataSetMsg);

/* Copy the configuration of DataSetReader */
UA_StatusCode UA_DataSetReaderConfig_copy(const UA_DataSetReaderConfig *src, UA_DataSetReaderConfig *dst);

/* Add TargetVariables */
UA_StatusCode
UA_Server_DataSetReader_addTargetVariables(UA_Server* server, UA_NodeId* parentNode, UA_NodeId dataSetReaderIdentifier, UA_SubscribedDataSetEnumType sdsType);

UA_StatusCode
UA_DataSetReader_setPubSubState(UA_Server *server, UA_PubSubState state, UA_DataSetReader *dataSetReader);

/**********************************************/
/*                ReaderGroup                 */
/**********************************************/
/* ReaderGroup Type Definition*/

struct UA_ReaderGroup {
    UA_ReaderGroupConfig config;
    UA_NodeId identifier;
    UA_NodeId linkedConnection;
    LIST_ENTRY(UA_ReaderGroup) listEntry;
    LIST_HEAD(UA_ListOfPubSubDataSetReader, UA_DataSetReader) readers;
    /* for simplified information access */
    UA_UInt32 readersCount;
    UA_UInt64 subscribeCallbackId;
    UA_Boolean subscribeCallbackIsRegistered;
    UA_PubSubState state;
    /* This flag is 'read only' and is set internally based on the PubSub state. */
    UA_Boolean configurationFrozen;
};

/* Copy configuration of ReaderGroup */
UA_StatusCode
UA_ReaderGroupConfig_copy(const UA_ReaderGroupConfig *src, UA_ReaderGroupConfig *dst);

/* Process Network Message */
UA_StatusCode
UA_Server_processNetworkMessage(UA_Server *server, UA_NetworkMessage* pMsg, UA_PubSubConnection *pConnection);

/* Prototypes for internal util functions - some functions maybe removed later
 *(currently moved from public to internal)*/
UA_ReaderGroup *UA_ReaderGroup_findRGbyId(UA_Server *server, UA_NodeId identifier);
UA_DataSetReader *UA_ReaderGroup_findDSRbyId(UA_Server *server, UA_NodeId identifier);
UA_StatusCode
UA_ReaderGroup_setPubSubState(UA_Server *server, UA_PubSubState state, UA_ReaderGroup *readerGroup);

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
UA_ReaderGroup_subscribeCallback(UA_Server *server, UA_ReaderGroup *readerGroup);

#endif /* UA_ENABLE_PUBSUB */

_UA_END_DECLS

#endif /* UA_PUBSUB_H_ */
