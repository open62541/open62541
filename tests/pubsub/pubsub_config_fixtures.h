/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#ifndef OPEN62541_PUBSUB_CONFIG_FIXTURES_H
#define OPEN62541_PUBSUB_CONFIG_FIXTURES_H

/* Fixtures of the configuration update tests. The file elements reference
 * static memory and must not be cleared. */

#include <open62541/server_pubsub.h>

#include "pubsub_test_helpers.h"
#include "pubsub_config_test_helpers.h"
#include "ua_pubsub_internal.h"
#include "ua_server_internal.h"

#include <check.h>
#include <stdio.h>

#define UA_PUBSUBTEST_PROFILE_UDP \
    "http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp"

/* A UInt32 variable in namespace 1 */
static UA_INLINE UA_NodeId
UA_PubSubTest_addVariable(UA_Server *server, UA_UInt32 id, UA_UInt32 value) {
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&vAttr.value, &value, &UA_TYPES[UA_TYPES_UINT32]);
    vAttr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    vAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId varId;
    UA_StatusCode res =
        UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, id),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                  UA_QUALIFIEDNAME(1, "Var"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  vAttr, NULL, &varId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    return varId;
}

typedef struct {
    UA_NodeId varId, pdsId, connId, wgId, dswId, rgId;
} UA_PubSubTestBase;

/* Base configuration via the C API: <prefix>PDS (one field, value 42) and
 * <prefix>Conn (PublisherId 2234) with <prefix>WG (id 100), <prefix>DSW
 * (id 200) and optionally <prefix>RG */
static UA_INLINE void
UA_PubSubTest_addBaseConfig(UA_Server *server, const char *prefix, UA_UInt32 varId,
                            UA_Double publishingInterval, UA_Boolean readerGroup,
                            UA_PubSubTestBase *base) {
    char name[64];
    memset(base, 0, sizeof(UA_PubSubTestBase));
    base->varId = UA_PubSubTest_addVariable(server, varId, 42);

    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(pdsConfig));
    snprintf(name, sizeof(name), "%sPDS", prefix);
    pdsConfig.name = UA_STRING(name);
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    UA_AddPublishedDataSetResult pdsRes =
        UA_Server_addPublishedDataSet(server, &pdsConfig, &base->pdsId);
    ck_assert_int_eq(pdsRes.addResult, UA_STATUSCODE_GOOD);

    UA_DataSetFieldConfig fieldConfig;
    memset(&fieldConfig, 0, sizeof(fieldConfig));
    fieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    fieldConfig.field.variable.fieldNameAlias = UA_STRING("Field1");
    fieldConfig.field.variable.publishParameters.publishedVariable = base->varId;
    fieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_DataSetFieldResult fieldRes =
        UA_Server_addDataSetField(server, base->pdsId, &fieldConfig, NULL);
    ck_assert_int_eq(fieldRes.result, UA_STATUSCODE_GOOD);

    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    snprintf(name, sizeof(name), "%sConn", prefix);
    connectionConfig.name = UA_STRING(name);
    UA_NetworkAddressUrlDataType networkAddressUrl =
        UA_PUBSUB_TEST_NETWORKADDRESSURL(UA_PUBSUB_TEST_UDP_MULTICAST_URL_4801);
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING(UA_PUBSUBTEST_PROFILE_UDP);
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.id.uint16 = 2234;
    UA_StatusCode res =
        UA_Server_addPubSubConnection(server, &connectionConfig, &base->connId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_WriterGroupConfig wgConfig;
    memset(&wgConfig, 0, sizeof(wgConfig));
    snprintf(name, sizeof(name), "%sWG", prefix);
    wgConfig.name = UA_STRING(name);
    wgConfig.writerGroupId = 100;
    wgConfig.publishingInterval = publishingInterval;
    wgConfig.keepAliveTime = 5000.0;
    res = UA_Server_addWriterGroup(server, base->connId, &wgConfig, &base->wgId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_DataSetWriterConfig dswConfig;
    memset(&dswConfig, 0, sizeof(dswConfig));
    snprintf(name, sizeof(name), "%sDSW", prefix);
    dswConfig.name = UA_STRING(name);
    dswConfig.dataSetWriterId = 200;
    res = UA_Server_addDataSetWriter(server, base->wgId, base->pdsId,
                                     &dswConfig, &base->dswId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    if(!readerGroup)
        return;
    UA_ReaderGroupConfig rgConfig;
    memset(&rgConfig, 0, sizeof(rgConfig));
    snprintf(name, sizeof(name), "%sRG", prefix);
    rgConfig.name = UA_STRING(name);
    res = UA_Server_addReaderGroup(server, base->connId, &rgConfig, &base->rgId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
}

static UA_INLINE UA_PubSubConnection *
UA_PubSubTest_findConnection(UA_Server *server, const char *name) {
    return UA_PubSubConnection_findByName(getPSM(server),
                                          UA_STRING((char*)(uintptr_t)name));
}

static UA_INLINE UA_PubSubConfigurationRefDataType
UA_PubSubTest_ref(UA_UInt32 mask, UA_UInt16 elementIndex,
                  UA_UInt16 connectionIndex, UA_UInt16 groupIndex) {
    UA_PubSubConfigurationRefDataType ref;
    UA_PubSubConfigurationRefDataType_init(&ref);
    ref.configurationMask = mask;
    ref.elementIndex = elementIndex;
    ref.connectionIndex = connectionIndex;
    ref.groupIndex = groupIndex;
    return ref;
}

/* File elements. The connections share the address and the PublisherId, the
 * PublishedDataSets share the field. */
static UA_NetworkAddressUrlDataType UA_PubSubTest_fileAddr;
static UA_UInt16 UA_PubSubTest_filePublisherId;
static UA_FieldMetaData UA_PubSubTest_filePdsField;
static UA_PublishedDataItemsDataType UA_PubSubTest_filePdsItems;
static UA_PublishedVariableDataType UA_PubSubTest_filePdsVar;

static UA_INLINE void
UA_PubSubTest_fileConnection(UA_PubSubConnectionDataType *c, const char *name,
                             UA_UInt16 publisherId) {
    UA_PubSubConnectionDataType_init(c);
    c->name = UA_STRING((char*)(uintptr_t)name);
    c->transportProfileUri = UA_STRING(UA_PUBSUBTEST_PROFILE_UDP);
    UA_PubSubTest_initNetworkAddressUrl(&UA_PubSubTest_fileAddr,
        UA_PubSubTest_getUdpMulticastUrl4801());
    UA_ExtensionObject_setValueNoDelete(&c->address, &UA_PubSubTest_fileAddr,
        &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    if(publisherId != 0) {
        UA_PubSubTest_filePublisherId = publisherId;
        UA_Variant_setScalar(&c->publisherId, &UA_PubSubTest_filePublisherId,
                             &UA_TYPES[UA_TYPES_UINT16]);
    }
}

static UA_INLINE void
UA_PubSubTest_fileWriterGroup(UA_WriterGroupDataType *wg, const char *name,
                              UA_UInt16 wgId) {
    UA_WriterGroupDataType_init(wg);
    wg->name = UA_STRING((char*)(uintptr_t)name);
    wg->writerGroupId = wgId;
    wg->publishingInterval = 150.0;
    wg->keepAliveTime = 5000.0;
}

static UA_INLINE void
UA_PubSubTest_fileWriter(UA_DataSetWriterDataType *dsw, const char *name,
                         UA_UInt16 dswId, const char *dataSetName) {
    UA_DataSetWriterDataType_init(dsw);
    dsw->name = UA_STRING((char*)(uintptr_t)name);
    dsw->dataSetWriterId = dswId;
    dsw->dataSetName = UA_STRING((char*)(uintptr_t)dataSetName);
}

/* A PublishedDataSet with one UInt32 field of the variable */
static UA_INLINE void
UA_PubSubTest_filePds(UA_PublishedDataSetDataType *pds, const char *name,
                      UA_NodeId varId) {
    UA_PublishedDataSetDataType_init(pds);
    pds->name = UA_STRING((char*)(uintptr_t)name);
    UA_FieldMetaData_init(&UA_PubSubTest_filePdsField);
    UA_PubSubTest_filePdsField.name = UA_STRING("Field1");
    UA_PubSubTest_filePdsField.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    UA_PubSubTest_filePdsField.builtInType = UA_NS0ID_UINT32;
    UA_PubSubTest_filePdsField.valueRank = -1;
    pds->dataSetMetaData.name = pds->name;
    pds->dataSetMetaData.fields = &UA_PubSubTest_filePdsField;
    pds->dataSetMetaData.fieldsSize = 1;
    UA_PublishedVariableDataType_init(&UA_PubSubTest_filePdsVar);
    UA_PubSubTest_filePdsVar.publishedVariable = varId;
    UA_PubSubTest_filePdsVar.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_PublishedDataItemsDataType_init(&UA_PubSubTest_filePdsItems);
    UA_PubSubTest_filePdsItems.publishedData = &UA_PubSubTest_filePdsVar;
    UA_PubSubTest_filePdsItems.publishedDataSize = 1;
    UA_ExtensionObject_setValueNoDelete(&pds->dataSetSource,
        &UA_PubSubTest_filePdsItems, &UA_TYPES[UA_TYPES_PUBLISHEDDATAITEMSDATATYPE]);
}

#endif /* OPEN62541_PUBSUB_CONFIG_FIXTURES_H */
