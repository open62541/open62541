/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2022 ISW (for umati and VDW e.V.) (Author: Moritz Walker)
 */

#include <open62541/types.h>
#include <open62541/types_generated_handling.h>

#include "ua_pubsub_networkmessage.h"
#include "ua_types_encoding_json.h"

/* Json keys for DataSetMetaData */
static const char* UA_DECODEKEY_MESSAGEID = "MessageId";
static const char* UA_DECODEKEY_MESSAGETYPE = "MessageType";
static const char* UA_DECODEKEY_PUBLISHERID = "PublisherId";
static const char* UA_DECODEKEY_DATASETWRITERID = "DataSetWriterId";
static const char* UA_DECODEKEY_METADATA = "MetaData";
static const char* UA_DECODEKEY_DATASETWRITERNAME = "DataSetWriterName";

UA_StatusCode
UA_DataSetMetaData_encodeJson_internal(const UA_DataSetMetaData *src, void *c) {
    CtxJson *ctx = (CtxJson *)c;
    status rv = writeJsonObjStart(ctx);

    /* MessageId */
    rv |= writeJsonObjElm(ctx, UA_DECODEKEY_MESSAGEID, &src->messageId,
                          &UA_TYPES[UA_TYPES_STRING]);

    /* MessageType */
    rv |= writeJsonObjElm(ctx, UA_DECODEKEY_MESSAGETYPE, &src->messageType,
                          &UA_TYPES[UA_TYPES_STRING]);

    /* PublisherId */
    switch(src->publisherIdType) {
        case UA_PUBLISHERDATATYPE_UINT32:
            rv = writeJsonKey(ctx, UA_DECODEKEY_PUBLISHERID);
            rv |= encodeJsonInternal(&src->publisherId.publisherIdUInt32,
                                     &UA_TYPES[UA_TYPES_UINT32], ctx);
            break;
        case UA_PUBLISHERDATATYPE_STRING:
            rv = writeJsonKey(ctx, UA_DECODEKEY_PUBLISHERID);
            rv |= encodeJsonInternal(&src->publisherId.publisherIdString,
                                     &UA_TYPES[UA_TYPES_STRING], ctx);
            break;
        default:
            break;
    }

    /* DataSetWriterId */
    rv |= writeJsonObjElm(ctx, UA_DECODEKEY_DATASETWRITERID, &src->dataSetWriterId,
                          &UA_TYPES[UA_TYPES_UINT16]);

    /* DataSetWriterName */
    rv |= writeJsonObjElm(ctx, UA_DECODEKEY_DATASETWRITERNAME, &src->dataSetWriterName,
                          &UA_TYPES[UA_TYPES_STRING]);

    /* DataSetMetaData */
    rv |= writeJsonKey(ctx, UA_DECODEKEY_METADATA);
    rv |= encodeJsonInternal(&src->dataSetMetaData,
                             &UA_TYPES[UA_TYPES_DATASETMETADATATYPE], ctx);
    rv |= writeJsonObjEnd(ctx);
    return rv;
}
