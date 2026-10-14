/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "gds_receiver_internal.h"

#ifdef UA_ENABLE_DRIVER_GDS_RECEIVER

#include <open62541/driver/file_transfer.h>

/* The TrustList Objects (Part 12, 7.8.2) are served by file transfer drivers.
 * The driver provides the FileType Methods and Properties; the backend below
 * encodes a snapshot of the TrustList for each read and stages the bytes of a
 * write until CloseAndUpdate. */

static const struct {
    UA_UInt32 objectId;
    UA_UInt32 groupId;
    UA_UInt32 lastModifiedTimeId;
    UA_UInt32 maxByteStringLengthId;
} gdsTrustListNodes[UA_GDSRECEIVER_TRUSTLISTS] = {
    {UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST,
     UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP,
     UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_LASTMODIFIEDTIME,
     UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_MAXBYTESTRINGLENGTH},
    {UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST,
     UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP,
     UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_LASTMODIFIEDTIME,
     UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_MAXBYTESTRINGLENGTH}
};

void
UA_GDSReceiver_initTrustLists(UA_GDSReceiverContext *ctx) {
    UA_UtcTime now = UA_DateTime_now();
    for(size_t i = 0; i < UA_GDSRECEIVER_TRUSTLISTS; i++) {
        UA_GDSTrustList *tl = &ctx->trustLists[i];
        tl->receiver = ctx;
        tl->objectId = UA_NODEID_NUMERIC(0, gdsTrustListNodes[i].objectId);
        tl->certificateGroupId = UA_NODEID_NUMERIC(0, gdsTrustListNodes[i].groupId);
        tl->lastUpdateTime = now;
        LIST_INIT(&tl->handles);
    }
}

UA_GDSTrustList *
UA_GDSReceiver_getTrustList(UA_GDSReceiverContext *ctx, const UA_NodeId *id) {
    for(size_t i = 0; i < UA_GDSRECEIVER_TRUSTLISTS; i++) {
        UA_GDSTrustList *tl = &ctx->trustLists[i];
        if(UA_NodeId_equal(id, &tl->objectId) ||
           UA_NodeId_equal(id, &tl->certificateGroupId))
            return tl;
    }
    return NULL;
}

UA_GDSTrustListHandle *
UA_GDSTrustList_findHandle(UA_GDSTrustList *tl, UA_UInt32 id) {
    UA_GDSTrustListHandle *h;
    LIST_FOREACH(h, &tl->handles, entry) {
        if(h->id == id)
            return h;
    }
    return NULL;
}

UA_Boolean
UA_GDSReceiver_trustListOpenForWrite(UA_GDSReceiverContext *ctx) {
    for(size_t i = 0; i < UA_GDSRECEIVER_TRUSTLISTS; i++) {
        UA_GDSTrustListHandle *h;
        LIST_FOREACH(h, &ctx->trustLists[i].handles, entry) {
            if(h->mode != UA_OPENFILEMODE_READ)
                return true;
        }
    }
    return false;
}

static void
gdsTrustListRemoveHandle(UA_GDSTrustList *tl, UA_GDSTrustListHandle *h) {
    LIST_REMOVE(h, entry);
    tl->handlesSize--;
    UA_ByteString_clear(&h->data);
    UA_free(h);
}

/**************************************
 * File Transfer Backend
 **************************************/

#define GDS_TRUSTLIST_WRITEMODE \
    (UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING)

/* Default max-read-length of the file transfer driver */
#define GDS_TRUSTLIST_READLENGTH_DEFAULT (1u << 20)

/* The TrustList is opened for the Session of a Method call. OpenWithMasks
 * passes the masks as the openContext. */
static UA_StatusCode
gdsTrustListOpen(UA_FileTransferFileBackend *b, const UA_String path, UA_Byte mode,
                 const UA_FileTransferOpenContext *context, UA_UInt32 *handle) {
    UA_GDSTrustList *tl = (UA_GDSTrustList*)b->context;
    UA_GDSReceiverContext *ctx = tl->receiver;
    if(!context || !context->sessionId)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    const UA_NodeId *sessionId = context->sessionId;
    UA_UInt32 masks = context->openContext ?
        *(const UA_UInt32*)context->openContext : UA_TRUSTLISTMASKS_ALL;
    if(path.length > 0)
        return UA_STATUSCODE_BADNOTFOUND;
    if(ctx->drv.state != UA_LIFECYCLESTATE_STARTED)
        return UA_STATUSCODE_BADINVALIDSTATE;
    /* A TrustList is read or replaced completely (Part 12, 7.8.2.2) */
    if(mode != UA_OPENFILEMODE_READ && mode != GDS_TRUSTLIST_WRITEMODE)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    /* Only an Open for writing from the Session of the transaction continues
     * it (Part 12, 7.8.2.2), unless ApplyChanges is already queued */
    UA_GDSTransaction *transaction = &ctx->transaction;
    if(transaction->state == UA_GDSTRANSACTIONSTATE_PENDING &&
       (mode != GDS_TRUSTLIST_WRITEMODE || transaction->applyChangesQueued ||
        !UA_NodeId_equal(&transaction->sessionId, sessionId)))
        return UA_STATUSCODE_BADTRANSACTIONPENDING;

    UA_GDSTrustListHandle *h = (UA_GDSTrustListHandle*)
        UA_calloc(1, sizeof(UA_GDSTrustListHandle));
    if(!h)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(mode == UA_OPENFILEMODE_READ) {
        /* The reads of a handle see the TrustList at the time of the Open */
        UA_CertificateGroup *certGroup = getCertGroup(ctx->drv.server, &tl->objectId);
        UA_TrustListDataType trustList;
        UA_TrustListDataType_init(&trustList);
        trustList.specifiedLists = masks;
        res = certGroup ? certGroup->getTrustList(certGroup, &trustList) :
            UA_STATUSCODE_BADINTERNALERROR;
        if(res == UA_STATUSCODE_GOOD)
            res = UA_encodeBinary(&trustList, &UA_TYPES[UA_TYPES_TRUSTLISTDATATYPE],
                                  &h->data, NULL);
        UA_TrustListDataType_clear(&trustList);
    } else if(transaction->state == UA_GDSTRANSACTIONSTATE_FRESH) {
        /* Writing starts a transaction. A failed open closes the handle again,
         * which ends the transaction without changes. */
        res = UA_GDSTransaction_init(transaction, ctx->drv.server, *sessionId);
        if(res == UA_STATUSCODE_GOOD)
            res = UA_GDSReceiver_watchTransaction(ctx);
    }
    if(res != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&h->data);
        UA_free(h);
        return res;
    }

    do {
        tl->nextHandle++;
        if(tl->nextHandle == 0)
            tl->nextHandle = 1;
    } while(UA_GDSTrustList_findHandle(tl, tl->nextHandle));
    h->id = tl->nextHandle;
    h->mode = mode;
    LIST_INSERT_HEAD(&tl->handles, h, entry);
    tl->handlesSize++;
    *handle = h->id;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
gdsTrustListClose(UA_FileTransferFileBackend *b, UA_UInt32 handle) {
    UA_GDSTrustList *tl = (UA_GDSTrustList*)b->context;
    UA_GDSTrustListHandle *h = UA_GDSTrustList_findHandle(tl, handle);
    if(!h)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Close discards the written data (Part 12, 7.8.2.1). It is not staged in
     * the transaction yet. Other changes of the transaction are kept; a
     * transaction without changes ends with its last TrustList open for
     * writing. */
    UA_Boolean written = (h->mode == GDS_TRUSTLIST_WRITEMODE);
    gdsTrustListRemoveHandle(tl, h);
    UA_GDSTransaction *transaction = &tl->receiver->transaction;
    if(written && transaction->state == UA_GDSTRANSACTIONSTATE_PENDING &&
       transaction->certGroupSize == 0 && transaction->certificateInfosSize == 0 &&
       !UA_GDSReceiver_trustListOpenForWrite(tl->receiver))
        UA_GDSTransaction_clear(transaction);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
gdsTrustListRead(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                 UA_Int32 length, UA_ByteString *out) {
    UA_GDSTrustList *tl = (UA_GDSTrustList*)b->context;
    UA_GDSTrustListHandle *h = UA_GDSTrustList_findHandle(tl, handle);
    if(!h || length < 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    size_t remaining = h->data.length - h->position;
    size_t count = ((size_t)length < remaining) ? (size_t)length : remaining;
    if(count == 0)
        return UA_STATUSCODE_GOOD;
    UA_StatusCode res = UA_ByteString_allocBuffer(out, count);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    memcpy(out->data, h->data.data + h->position, count);
    h->position += count;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
gdsTrustListWrite(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                  const UA_ByteString data) {
    UA_GDSTrustList *tl = (UA_GDSTrustList*)b->context;
    UA_GDSTrustListHandle *h = UA_GDSTrustList_findHandle(tl, handle);
    if(!h)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(data.length == 0)
        return UA_STATUSCODE_GOOD;

    /* Abort when the TrustList would exceed the maximum size (0 = unlimited) */
    UA_ServerConfig *sc = UA_Server_getConfig(tl->receiver->drv.server);
    size_t end = h->position + data.length;
    size_t length = (end > h->data.length) ? end : h->data.length;
    if(sc->maxTrustListSize != 0 && length > sc->maxTrustListSize) {
        UA_LOG_WARNING(sc->logging, UA_LOGCATEGORY_SERVER,
                       "Write on trust list exceeds limit");
        return UA_STATUSCODE_BADREQUESTTOOLARGE;
    }
    if(length > h->data.length) {
        UA_Byte *grown = (UA_Byte*)UA_realloc(h->data.data, length);
        if(!grown)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        h->data.data = grown;
        h->data.length = length;
    }
    memcpy(h->data.data + h->position, data.data, data.length);
    h->position = end;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
gdsTrustListGetPosition(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                        UA_UInt64 *outPosition) {
    UA_GDSTrustList *tl = (UA_GDSTrustList*)b->context;
    UA_GDSTrustListHandle *h = UA_GDSTrustList_findHandle(tl, handle);
    if(!h)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    *outPosition = h->position;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
gdsTrustListSetPosition(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                        UA_UInt64 position) {
    UA_GDSTrustList *tl = (UA_GDSTrustList*)b->context;
    UA_GDSTrustListHandle *h = UA_GDSTrustList_findHandle(tl, handle);
    if(!h)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    h->position = (position < h->data.length) ? (size_t)position : h->data.length;
    return UA_STATUSCODE_GOOD;
}

/* The Size of a TrustList has no meaning (Part 12, 7.8.2.1). LastUpdateTime
 * is the time of the last change. */
static UA_StatusCode
gdsTrustListGetInfo(UA_FileTransferFileBackend *b, const UA_String path,
                    UA_FileTransferFileInfo *outInfo) {
    UA_GDSTrustList *tl = (UA_GDSTrustList*)b->context;
    if(path.length > 0)
        return UA_STATUSCODE_BADNOTFOUND;
    memcpy(outInfo->name, "TrustList", sizeof("TrustList"));
    outInfo->sizeUnknown = true;
    outInfo->lastModified = tl->lastUpdateTime;
    outInfo->accessRights = UA_FILEACCESS_READ | UA_FILEACCESS_WRITE;
    return UA_STATUSCODE_GOOD;
}

/* The driver closes all handles before it is freed */
static void
gdsTrustListClear(UA_FileTransferFileBackend *b) {
    UA_GDSTrustList *tl = (UA_GDSTrustList*)b->context;
    UA_GDSTrustListHandle *h, *tmp;
    LIST_FOREACH_SAFE(h, &tl->handles, entry, tmp)
        gdsTrustListRemoveHandle(tl, h);
    tl->driver = NULL;
}

/**************************************
 * File Transfer Drivers
 **************************************/

/* The optional FileType Properties of the TrustList Objects are not in the
 * Namespace Zero nodeset. They are created once with their standard NodeIds
 * and the AccessRestrictions of the Object. */
static UA_StatusCode
gdsTrustListAddProperty(UA_Server *server, const UA_NodeId objectId,
                        UA_UInt32 propertyId, char *name, const void *value,
                        const UA_DataType *type) {
    UA_NodeId id = UA_NODEID_NUMERIC(0, propertyId);
    UA_NodeClass nodeClass;
    if(UA_Server_readNodeClass(server, id, &nodeClass) == UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_GOOD;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", name);
    attr.dataType = type->typeId;
    attr.valueRank = UA_VALUERANK_SCALAR;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    UA_Variant_setScalar(&attr.value, (void*)(uintptr_t)value, type);
    UA_StatusCode res =
        UA_Server_addVariableNode(server, id, objectId, UA_NS0ID(HASPROPERTY),
                                  UA_QUALIFIEDNAME(0, name), UA_NS0ID(PROPERTYTYPE),
                                  attr, NULL, NULL);
    UA_AccessRestrictionType restrictions = 0;
    if(res == UA_STATUSCODE_GOOD &&
       UA_Server_readAccessRestrictions(server, objectId, &restrictions) ==
       UA_STATUSCODE_GOOD && restrictions != 0)
        res = UA_Server_writeAccessRestrictions(server, id, restrictions);
    return res;
}

static UA_StatusCode
gdsTrustListStartDriver(UA_GDSTrustList *tl, size_t index) {
    UA_Server *server = tl->receiver->drv.server;
    UA_DateTime lastModified = tl->lastUpdateTime;
    UA_UInt32 maxLength = 0;
    UA_StatusCode res =
        gdsTrustListAddProperty(server, tl->objectId,
                                gdsTrustListNodes[index].lastModifiedTimeId,
                                "LastModifiedTime", &lastModified,
                                &UA_TYPES[UA_TYPES_DATETIME]);
    if(res == UA_STATUSCODE_GOOD)
        res = gdsTrustListAddProperty(server, tl->objectId,
                                      gdsTrustListNodes[index].maxByteStringLengthId,
                                      "MaxByteStringLength", &maxLength,
                                      &UA_TYPES[UA_TYPES_UINT32]);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_FileTransferFileBackend backend;
    memset(&backend, 0, sizeof(backend));
    backend.context = tl;
    backend.open = gdsTrustListOpen;
    backend.close = gdsTrustListClose;
    backend.read = gdsTrustListRead;
    backend.write = gdsTrustListWrite;
    backend.getPosition = gdsTrustListGetPosition;
    backend.setPosition = gdsTrustListSetPosition;
    backend.getInfo = gdsTrustListGetInfo;
    backend.clear = gdsTrustListClear;
    UA_FileTransferNodeDescription description;
    memset(&description, 0, sizeof(description));
    description.nodeId = tl->objectId;
    UA_Driver *drv = NULL;
    res = UA_FileTransferDriver_newFile(server, &backend, UA_STRING_NULL,
                                        &description, NULL, &drv);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    /* A TrustList up to MaxTrustListSize fits into one Write */
    UA_ServerConfig *sc = UA_Server_getConfig(server);
    if(sc->maxTrustListSize > GDS_TRUSTLIST_READLENGTH_DEFAULT)
        res = UA_KeyValueMap_setScalar(&drv->params, UA_QUALIFIEDNAME(0, "max-read-length"),
                                       &sc->maxTrustListSize, &UA_TYPES[UA_TYPES_UINT32]);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_Server_addDriver(server, drv);
    if(res != UA_STATUSCODE_GOOD) {
        drv->free(drv);
        return res;
    }
    tl->driver = drv;

    /* The startup of the server does not start the drivers that other
     * drivers add during their start */
    if(drv->state != UA_LIFECYCLESTATE_STARTED)
        res = drv->start(drv);
    return res;
}

UA_StatusCode
UA_GDSReceiver_startTrustLists(UA_GDSReceiverContext *ctx) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < UA_GDSRECEIVER_TRUSTLISTS && res == UA_STATUSCODE_GOOD; i++)
        res = gdsTrustListStartDriver(&ctx->trustLists[i], i);
    if(res != UA_STATUSCODE_GOOD)
        UA_GDSReceiver_stopTrustLists(ctx);
    return res;
}

/* The file transfer drivers were added after the receiver and are before it in
 * the list of the server. Removing them during the stop of the receiver keeps
 * the iteration of the server over the drivers intact. */
void
UA_GDSReceiver_stopTrustLists(UA_GDSReceiverContext *ctx) {
    UA_Server *server = ctx->drv.server;
    for(size_t i = 0; i < UA_GDSRECEIVER_TRUSTLISTS; i++) {
        UA_Driver *drv = ctx->trustLists[i].driver;
        if(!drv)
            continue;
        if(drv->state != UA_LIFECYCLESTATE_STOPPED)
            drv->stop(drv);
        UA_Server_removeDriver(server, drv);
        drv->free(drv);
        ctx->trustLists[i].driver = NULL;
    }
}

/**************************************
 * TrustListType Methods
 **************************************/

/* OpenWithMasks opens the TrustList for reading with a snapshot of the lists
 * in the masks (Part 12, 7.8.2.3) */
UA_StatusCode
UA_GDSReceiver_openTrustListWithMask(UA_GDSReceiverContext *ctx, UA_GDSTrustList *tl,
                                    const UA_NodeId *sessionId, void *sessionContext,
                                    UA_UInt32 mask, UA_Variant *output) {
    if(ctx->transaction.state == UA_GDSTRANSACTIONSTATE_PENDING)
        return UA_STATUSCODE_BADTRANSACTIONPENDING;
    if(!tl->driver)
        return UA_STATUSCODE_BADINVALIDSTATE;
    UA_UInt32 fileHandle = 0;
    UA_StatusCode res =
        UA_FileTransferDriver_openHandle(tl->driver, tl->objectId, sessionId,
                                         sessionContext, UA_OPENFILEMODE_READ,
                                         &mask, &fileHandle, NULL);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = UA_Variant_setScalarCopy(output, &fileHandle, &UA_TYPES[UA_TYPES_UINT32]);
    if(res != UA_STATUSCODE_GOOD)
        UA_FileTransferDriver_closeHandle(tl->driver, tl->objectId, sessionId,
                                          fileHandle);
    return res;
}

#endif /* UA_ENABLE_DRIVER_GDS_RECEIVER */
