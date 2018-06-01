/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#ifndef UA_PUBSUB_H_
#define UA_PUBSUB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "../deps/queue.h"
#include "ua_plugin_pubsub.h"
#include "ua_pubsub_networkmessage.h"
#include "ua_server.h"
#include "ua_server_pubsub.h"

//forward declarations
struct UA_WriterGroup;
typedef struct UA_WriterGroup UA_WriterGroup;

/* The configuration structs (public part of PubSub entities) are defined in include/ua_plugin_pubsub.h */

/**********************************************/
/*            PublishedDataSet                */
/**********************************************/
typedef struct{
    UA_PublishedDataSetConfig config;
    UA_DataSetMetaDataType dataSetMetaData;
    LIST_HEAD(UA_ListOfDataSetField, UA_DataSetField) fields;
    UA_NodeId identifier;
    UA_UInt16 fieldSize;
    UA_UInt16 promotedFieldsCount;
} UA_PublishedDataSet;

UA_StatusCode
UA_PublishedDataSetConfig_copy(const UA_PublishedDataSetConfig *src, UA_PublishedDataSetConfig *dst);
UA_PublishedDataSet *
UA_PublishedDataSet_findPDSbyId(UA_Server *server, UA_NodeId identifier);
void
UA_PublishedDataSet_deleteMembers(UA_Server *server, UA_PublishedDataSet *publishedDataSet);

/**********************************************/
/*               Connection                   */
/**********************************************/
//the connection config (public part of connection) object is defined in include/ua_plugin_pubsub.h
typedef struct{
    UA_PubSubConnectionConfig *config;
    //internal fields
    UA_PubSubChannel *channel;
    UA_NodeId identifier;
    LIST_HEAD(UA_ListOfWriterGroup, UA_WriterGroup) writerGroups;
} UA_PubSubConnection;

UA_StatusCode
UA_PubSubConnectionConfig_copy(const UA_PubSubConnectionConfig *src, UA_PubSubConnectionConfig *dst);
UA_PubSubConnection *
UA_PubSubConnection_findConnectionbyId(UA_Server *server, UA_NodeId connectionIdentifier);
void
UA_PubSubConnectionConfig_deleteMembers(UA_PubSubConnectionConfig *connectionConfig);
void
UA_PubSubConnection_deleteMembers(UA_Server *server, UA_PubSubConnection *connection);

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
#ifdef UA_ENABLE_PUBSUB_DELTAFRAMES
    UA_UInt16 deltaFrameCounter;            //actual count of sent deltaFrames
    size_t lastSamplesCount;
    UA_DataSetWriterSample *lastSamples;
#endif
    UA_UInt16 actualDataSetMessageSequenceCount;
} UA_DataSetWriter;

UA_StatusCode
UA_DataSetWriterConfig_copy(const UA_DataSetWriterConfig *src, UA_DataSetWriterConfig *dst);
UA_DataSetWriter *
UA_DataSetWriter_findDSWbyId(UA_Server *server, UA_NodeId identifier);
void
UA_DataSetWriter_deleteMembers(UA_Server *server, UA_DataSetWriter *dataSetWriter);

/**********************************************/
/*               WriterGroup                  */
/**********************************************/

struct UA_WriterGroup{
    UA_WriterGroupConfig config;
    //internal fields
    LIST_ENTRY(UA_WriterGroup) listEntry;
    UA_NodeId identifier;
    UA_NodeId linkedConnection;
    LIST_HEAD(UA_ListOfDataSetWriter, UA_DataSetWriter) writers;
    UA_UInt32 writersCount;
    UA_UInt64 publishCallbackId;
    UA_Boolean publishCallbackIsRegistered;
};

UA_StatusCode
UA_WriterGroupConfig_copy(const UA_WriterGroupConfig *src, UA_WriterGroupConfig *dst);
UA_WriterGroup *
UA_WriterGroup_findWGbyId(UA_Server *server, UA_NodeId identifier);
void
UA_WriterGroup_deleteMembers(UA_Server *server, UA_WriterGroup *writerGroup);

/**********************************************/
/*               DataSetField                 */
/**********************************************/

typedef struct UA_DataSetField{
    UA_DataSetFieldConfig config;
    //internal fields
    LIST_ENTRY(UA_DataSetField) listEntry;
    UA_NodeId identifier;
    UA_NodeId publishedDataSet;             //ref to parent pds
    UA_FieldMetaData fieldMetaData;
    UA_UInt64 sampleCallbackId;
    UA_Boolean sampleCallbackIsRegistered;
} UA_DataSetField;

UA_StatusCode
UA_DataSetFieldConfig_copy(const UA_DataSetFieldConfig *src, UA_DataSetFieldConfig *dst);
UA_DataSetField *
UA_DataSetField_findDSFbyId(UA_Server *server, UA_NodeId identifier);
void
UA_DataSetField_deleteMembers(UA_DataSetField *field);

/*********************************************************/
/*               PublishValues handling                  */
/*********************************************************/

UA_StatusCode
UA_WriterGroup_addPublishCallback(UA_Server *server, UA_WriterGroup *writerGroup);
void
UA_WriterGroup_publishCallback(UA_Server *server, UA_WriterGroup *writerGroup);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_PUBSUB_H_ */
