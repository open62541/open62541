/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Markus Karch, Fraunhofer IOSB
 */

#ifndef OPEN62541_UA_RECORD_DATATYPE_H
#define OPEN62541_UA_RECORD_DATATYPE_H

#include "ua_types.h"
#include "ua_types_generated.h"

/**
 * ApplicationRecordDataType
 * ^^^^^^^^^^^^^^^^^^^^^^^^^
 */
typedef struct {
    UA_NodeId applicationId;
    UA_String applicationUri;
    UA_ApplicationType applicationType;
    size_t applicationNamesSize;
    UA_LocalizedText *applicationNames;
    UA_String productUri;
    size_t discoveryUrlsSize;
    UA_String *discoveryUrls;
    size_t serverCapabilitiesSize;
    UA_String *serverCapabilities;
} UA_ApplicationRecordDataType;

/* ApplicationRecordDataType */
static UA_DataTypeMember ApplicationRecordDataType_members[7] = {
        {
                UA_TYPENAME("ApplicationId") /* .memberName */
                UA_TYPES_NODEID, /* .memberTypeIndex */
                0, /* .padding */
                true, /* .namespaceZero */
                false /* .isArray */
        },
        {
                UA_TYPENAME("ApplicationUri") /* .memberName */
                UA_TYPES_STRING, /* .memberTypeIndex */
                offsetof(UA_ApplicationRecordDataType, applicationUri) - offsetof(UA_ApplicationRecordDataType, applicationId) - sizeof(UA_NodeId), /* .padding */
                true, /* .namespaceZero */
                false /* .isArray */
        },
        {
                UA_TYPENAME("ApplicationType") /* .memberName */
                UA_TYPES_APPLICATIONTYPE, /* .memberTypeIndex */
                offsetof(UA_ApplicationRecordDataType, applicationType) - offsetof(UA_ApplicationRecordDataType, applicationUri) - sizeof(UA_String), /* .padding */
                true, /* .namespaceZero */
                false /* .isArray */
        },
        {
                UA_TYPENAME("ApplicationNames") /* .memberName */
                UA_TYPES_LOCALIZEDTEXT, /* .memberTypeIndex */
                offsetof(UA_ApplicationRecordDataType, applicationNamesSize) - offsetof(UA_ApplicationRecordDataType, applicationType) - sizeof(UA_ApplicationType), /* .padding */
                true, /* .namespaceZero */
                true /* .isArray */
        },
        {
                UA_TYPENAME("ProductUri") /* .memberName */
                UA_TYPES_STRING, /* .memberTypeIndex */
                offsetof(UA_ApplicationRecordDataType, productUri) - offsetof(UA_ApplicationRecordDataType, applicationNames) - sizeof(void*), /* .padding */
                true, /* .namespaceZero */
                false /* .isArray */
        },
        {
                UA_TYPENAME("DiscoveryUrls") /* .memberName */
                UA_TYPES_STRING, /* .memberTypeIndex */
                offsetof(UA_ApplicationRecordDataType, discoveryUrlsSize) - offsetof(UA_ApplicationRecordDataType, productUri) - sizeof(UA_String), /* .padding */
                true, /* .namespaceZero */
                true /* .isArray */
        },
        {
                UA_TYPENAME("ServerCapabilities") /* .memberName */
                UA_TYPES_STRING, /* .memberTypeIndex */
                offsetof(UA_ApplicationRecordDataType, serverCapabilitiesSize) - offsetof(UA_ApplicationRecordDataType, discoveryUrls) - sizeof(void*), /* .padding */
                true, /* .namespaceZero */
                true /* .isArray */
        }};


static const UA_DataType ApplicationRecordDataType =
        {
                UA_TYPENAME("ApplicationRecordDataType") /* .typeName */
                {2, UA_NODEIDTYPE_NUMERIC, {1}}, /* .typeId */
                sizeof(UA_ApplicationRecordDataType), /* .memSize */
                1, /* .typeIndex */
                7, /* .membersSize */
                false, /* .builtin */
                false, /* .pointerFree */
                false, /* .overlayable */
                134, /* .binaryEncodingId */
                ApplicationRecordDataType_members /* .members */
        };

#endif //OPEN62541_UA_RECORD_DATATYPE_H
