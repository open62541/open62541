/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/plugin/certificategroup_default.h>
#include "gds_receiver_internal.h"

#ifdef UA_ENABLE_DRIVER_GDS_RECEIVER

#include <open62541/driver/file_transfer.h>

#define UA_SHA1_LENGTH 20
#define GDS_RECEIVER_MAX_ENDPOINTS 32
#define STATIC_NS0ID(ID) {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_##ID}}

typedef enum UA_GDSTransactionChanges {
    UA_GDSTRANSACTIONCHANGES_NOTHING = 0,
    UA_GDSTRANSACTIONCHANGES_TRUSTLIST,
    UA_GDSTRANSACTIONCHANGES_CERTIFICATE,
    UA_GDSTRANSACTIONCHANGES_BOTH,
} UA_GDSTransactionChanges;

static UA_StatusCode
UA_GDSReceiver_applyChanges(UA_GDSReceiverContext *ctx);

/* AddCertificate and RemoveCertificate cannot be called while the TrustList is
 * open (Part 12, 7.8.2.6 and 7.8.2.7) */
static UA_StatusCode
checkTrustListClosed(UA_GDSTrustList *tl) {
    UA_GDSTrustListHandle *h;
    LIST_FOREACH(h, &tl->handles, entry) {
        if(h->mode == (UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING))
            return UA_STATUSCODE_BADINVALIDSTATE;
    }
    if(tl->handlesSize > 0)
        return UA_STATUSCODE_BADNOTWRITABLE;
    return UA_STATUSCODE_GOOD;
}

/********************/
/* GDS Transaction  */
/********************/

UA_StatusCode
UA_GDSTransaction_init(UA_GDSTransaction *transaction, UA_Server *server,
                       const UA_NodeId sessionId) {
    if(!transaction || !server)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString csr = UA_BYTESTRING_NULL;
    if(transaction->localCsrCertificate.length > 0)
        csr = transaction->localCsrCertificate;

    memset(transaction, 0, sizeof(UA_GDSTransaction));

    UA_StatusCode res = UA_NodeId_copy(&sessionId, &transaction->sessionId);
    if(res != UA_STATUSCODE_GOOD) {
        transaction->localCsrCertificate = csr;
        return res;
    }
    transaction->state = UA_GDSTRANSACTIONSTATE_PENDING;
    transaction->server = server;
    transaction->localCsrCertificate = csr;
    transaction->applyChangesQueued = false;

    return UA_STATUSCODE_GOOD;
}

/* Returns the appropriate CertificateGroup from the transaction.
 * If the CertificateGroup does not exist in the transaction, it will be created. */
static UA_CertificateGroup*
UA_GDSTransaction_getCertificateGroup(UA_GDSTransaction *transaction,
                                      const UA_CertificateGroup *certGroup) {
    if(!transaction || !certGroup)
        return NULL;

    /* Check if transaction was initialized */
    if(transaction->state != UA_GDSTRANSACTIONSTATE_PENDING)
        return NULL;

    for(size_t i = 0; i < transaction->certGroupSize; i++) {
        UA_CertificateGroup *group = &transaction->certGroups[i];
        if(UA_NodeId_equal(&group->certificateGroupId, &certGroup->certificateGroupId))
            return group;
    }

    /* If the certGroup does not exist, create a new one */
    UA_CertificateGroup *newGroups = (UA_CertificateGroup*)
        UA_realloc(transaction->certGroups,
                   (transaction->certGroupSize + 1) * sizeof(UA_CertificateGroup));
    if(!newGroups)
        return NULL;
    transaction->certGroups = newGroups;

    transaction->certGroupSize++;

    UA_CertificateGroup* newGroup = (UA_CertificateGroup*)
        &transaction->certGroups[transaction->certGroupSize-1];
    memset(newGroup, 0, sizeof(UA_CertificateGroup));

    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;
    UA_StatusCode res = certGroup->getTrustList(
        (UA_CertificateGroup*)(uintptr_t)certGroup, &trustList);
    if(res != UA_STATUSCODE_GOOD) {
        UA_TrustListDataType_clear(&trustList);
        transaction->certGroupSize--;
        return NULL;
    }

    /* Set up the parameters */
    UA_KeyValuePair params[1] = {{{0, UA_STRING_STATIC("max-trust-listsize")}, {0}}};
    UA_KeyValueMap paramsMap = {1, params};
    UA_ServerConfig *config = UA_Server_getConfig(transaction->server);
    UA_Variant_setScalar(&params[0].value, &config->maxTrustListSize,
                         &UA_TYPES[UA_TYPES_UINT32]);

    /* Initialize the CertificateGroup */
    res = UA_CertificateGroup_Memorystore(
        newGroup, (UA_NodeId*)(uintptr_t)&certGroup->certificateGroupId,
        &trustList, certGroup->logging, &paramsMap);

    UA_TrustListDataType_clear(&trustList);
    if(res != UA_STATUSCODE_GOOD) {
        transaction->certGroupSize--;
        return NULL;
    }
    return newGroup;
}

static UA_StatusCode
UA_GDSTransaction_addCertificateInfo(UA_GDSTransaction *transaction,
                                     const UA_NodeId certificateGroupId,
                                     const UA_NodeId certificateTypeId,
                                     const UA_ByteString *certificate,
                                     const UA_ByteString *privateKey) {
    if(!transaction || !certificate)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Check if transaction was initialized */
    if(transaction->state != UA_GDSTRANSACTIONSTATE_PENDING)
        return UA_STATUSCODE_BADINVALIDSTATE;

    /* Check if an entry with certificateGroupId and certificateTypeId already exists */
    for(size_t i = 0; i < transaction->certificateInfosSize; i++) {
        UA_GDSCertificateInfo *certInfo = &transaction->certificateInfos[i];
        if(!UA_NodeId_equal(&certInfo->certificateGroup, &certificateGroupId) ||
           !UA_NodeId_equal(&certInfo->certificateType, &certificateTypeId))
            continue;

        UA_ByteString newCertificate = UA_BYTESTRING_NULL;
        UA_ByteString newPrivateKey = UA_BYTESTRING_NULL;
        UA_StatusCode res = UA_ByteString_copy(certificate, &newCertificate);
        if(privateKey)
            res |= UA_ByteString_copy(privateKey, &newPrivateKey);
        if(res != UA_STATUSCODE_GOOD) {
            UA_ByteString_clear(&newCertificate);
            UA_ByteString_memZero(&newPrivateKey);
            UA_ByteString_clear(&newPrivateKey);
            return res;
        }
        UA_ByteString_clear(&certInfo->certificate);
        UA_ByteString_memZero(&certInfo->privateKey);
        UA_ByteString_clear(&certInfo->privateKey);
        certInfo->certificate = newCertificate;
        certInfo->privateKey = newPrivateKey;
        return UA_STATUSCODE_GOOD;
    }

    UA_GDSCertificateInfo stagedInfo;
    memset(&stagedInfo, 0, sizeof(stagedInfo));
    UA_StatusCode res = UA_ByteString_copy(certificate, &stagedInfo.certificate);
    res |= UA_NodeId_copy(&certificateGroupId, &stagedInfo.certificateGroup);
    res |= UA_NodeId_copy(&certificateTypeId, &stagedInfo.certificateType);
    if(privateKey)
        res |= UA_ByteString_copy(privateKey, &stagedInfo.privateKey);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&stagedInfo.certificate);
        UA_ByteString_memZero(&stagedInfo.privateKey);
        UA_ByteString_clear(&stagedInfo.privateKey);
        UA_NodeId_clear(&stagedInfo.certificateGroup);
        UA_NodeId_clear(&stagedInfo.certificateType);
        return res;
    }

    UA_GDSCertificateInfo *newCertInfos = (UA_GDSCertificateInfo *)
        UA_realloc(transaction->certificateInfos,
                   (transaction->certificateInfosSize + 1) * sizeof(UA_GDSCertificateInfo));
    if(!newCertInfos) {
        UA_ByteString_clear(&stagedInfo.certificate);
        UA_ByteString_memZero(&stagedInfo.privateKey);
        UA_ByteString_clear(&stagedInfo.privateKey);
        UA_NodeId_clear(&stagedInfo.certificateGroup);
        UA_NodeId_clear(&stagedInfo.certificateType);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    transaction->certificateInfos = newCertInfos;

    transaction->certificateInfos[transaction->certificateInfosSize] = stagedInfo;
    transaction->certificateInfosSize++;

    return UA_STATUSCODE_GOOD;
}

void
UA_GDSTransaction_clear(UA_GDSTransaction *transaction) {
    if(!transaction)
        return;

    transaction->state = UA_GDSTRANSACTIONSTATE_FRESH;
    transaction->server = NULL;
    UA_NodeId_clear(&transaction->sessionId);
    UA_ByteString_clear(&transaction->localCsrCertificate);

    if(transaction->certGroups) {
        for(size_t i = 0; i < transaction->certGroupSize; i++) {
            transaction->certGroups[i].clear(&transaction->certGroups[i]);
        }
        UA_free(transaction->certGroups);
        transaction->certGroupSize = 0;
        transaction->certGroups = NULL;
    }

    if(transaction->certificateInfos) {
        for(size_t i = 0; i < transaction->certificateInfosSize; i++) {
            UA_ByteString_clear(&transaction->certificateInfos[i].certificate);
            UA_ByteString_memZero(&transaction->certificateInfos[i].privateKey);
            UA_ByteString_clear(&transaction->certificateInfos[i].privateKey);
            UA_NodeId_clear(&transaction->certificateInfos[i].certificateGroup);
            UA_NodeId_clear(&transaction->certificateInfos[i].certificateType);
        }
        UA_free(transaction->certificateInfos);
        transaction->certificateInfosSize = 0;
        transaction->certificateInfos = NULL;
    }
}

/********************/
/*   GDS Manager    */
/********************/

UA_Boolean
UA_GDSReceiver_transactionPending(UA_GDSReceiverContext *ctx) {
    return ctx->transaction.state != UA_GDSTRANSACTIONSTATE_FRESH;
}

UA_StatusCode
UA_GDSReceiver_applyChangesForSession(UA_GDSReceiverContext *ctx,
                                     const UA_NodeId *sessionId) {
    UA_GDSTransaction *transaction = &ctx->transaction;
    /* A fresh transaction has no owning session. Check the state first, so
     * that the session comparison does not hide Bad_NothingToDo. */
    if(transaction->state == UA_GDSTRANSACTIONSTATE_FRESH)
        return UA_STATUSCODE_BADNOTHINGTODO;
    if(!UA_NodeId_equal(&transaction->sessionId, sessionId))
        return UA_STATUSCODE_BADUSERACCESSDENIED;
    return UA_GDSReceiver_applyChanges(ctx);
}

UA_StatusCode
UA_GDSReceiver_closeAndUpdateTrustList(UA_GDSReceiverContext *ctx,
                                      UA_GDSTrustList *tl,
                                      const UA_NodeId *sessionId,
                                      UA_UInt32 fileHandle,
                                      UA_Variant *output) {
    if(!tl->driver)
        return UA_STATUSCODE_BADINVALIDSTATE;
    UA_Byte mode = 0;
    UA_UInt32 backendHandle = 0;
    UA_StatusCode retval =
        UA_FileTransferDriver_getHandleInfo(tl->driver, tl->objectId, sessionId,
                                            fileHandle, &mode, &backendHandle);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(mode != (UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING))
        return UA_STATUSCODE_BADINVALIDSTATE;
    /* A write handle belongs to the transaction of its Session */
    if(ctx->transaction.state != UA_GDSTRANSACTIONSTATE_PENDING ||
       !UA_NodeId_equal(&ctx->transaction.sessionId, sessionId))
        return UA_STATUSCODE_BADINVALIDSTATE;
    UA_GDSTrustListHandle *h = UA_GDSTrustList_findHandle(tl, backendHandle);
    UA_CertificateGroup *certGroup = getCertGroup(ctx->drv.server, &tl->objectId);
    if(!h || !certGroup)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Decode first, a failure stages nothing in the transaction */
    UA_TrustListDataType trustList;
    retval = UA_decodeBinary(&h->data, &trustList,
                             &UA_TYPES[UA_TYPES_TRUSTLISTDATATYPE], NULL);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_TrustListDataType_clear(&trustList);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_CertificateGroup *transactionCertGroup =
        UA_GDSTransaction_getCertificateGroup(&ctx->transaction, certGroup);
    if(!transactionCertGroup) {
        UA_TrustListDataType_clear(&trustList);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    retval = transactionCertGroup->setTrustList(transactionCertGroup, &trustList);
    UA_TrustListDataType_clear(&trustList);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* The staged TrustList keeps the transaction when the handle closes */
    retval = UA_FileTransferDriver_closeHandle(tl->driver, tl->objectId,
                                               sessionId, fileHandle);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Output arg, indicates that the ApplyChanges Method shall be called before
     * the new trust list will be used. */
    UA_Boolean applyChangesRequired = true;
    return UA_Variant_setScalarCopy(output, &applyChangesRequired,
                                    &UA_TYPES[UA_TYPES_BOOLEAN]);
}

/* Verify the certificate in a copy of the CertificateGroup. The copy keeps
 * failed certificates out of the rejected list of the CertificateGroup. */
static UA_StatusCode
verifyInTrustListCopy(UA_CertificateGroup *certGroup,
                      const UA_ByteString *certificate) {
    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;
    UA_StatusCode res = certGroup->getTrustList(certGroup, &trustList);
    if(res != UA_STATUSCODE_GOOD) {
        UA_TrustListDataType_clear(&trustList);
        return res;
    }

    UA_CertificateGroup copy;
    memset(&copy, 0, sizeof(UA_CertificateGroup));
    res = UA_CertificateGroup_Memorystore(&copy, &certGroup->certificateGroupId,
                                          &trustList, certGroup->logging, NULL);
    UA_TrustListDataType_clear(&trustList);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    res = copy.verifyCertificate(&copy, certificate);
    copy.clear(&copy);
    return res;
}

/* TODO: Handle isTrustedCertificate */
UA_StatusCode
UA_GDSReceiver_addCertificate(UA_GDSReceiverContext *ctx,
                             UA_CertificateGroup *certGroup,
                             UA_ByteString *certificate,
                             const UA_Boolean *isTrustedCertificate) {
    UA_assert(certGroup != NULL);
    UA_Server *server = ctx->drv.server;
    UA_ServerConfig *sc = UA_Server_getConfig(server);

    /* This method cannot be called if the containing TrustList Object is open */
    UA_GDSTrustList *tl =
        UA_GDSReceiver_getTrustList(ctx, &certGroup->certificateGroupId);
    if(!tl)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_StatusCode res = checkTrustListClosed(tl);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    if(UA_GDSReceiver_transactionPending(ctx))
        return UA_STATUSCODE_BADTRANSACTIONPENDING;

    /* CA certificates cannot be added using this method because it does not
     * support adding CRLs */
    if(UA_CertificateUtils_checkCA(certificate) == UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(sc->logging, UA_LOGCATEGORY_SERVER,
                     "The certificate could not be added because it is a CA certificate. "
                     "CA certificates must be added using the FileType methods.");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    res = verifyInTrustListCopy(certGroup, certificate);
    if(res == UA_STATUSCODE_BADCERTIFICATEUNTRUSTED) {
        /* The chain is complete, but no certificate in it is trusted. Adding
         * the certificate makes it trusted. */
        res = UA_STATUSCODE_GOOD;
    } else if(res == UA_STATUSCODE_BADCERTIFICATETIMEINVALID ||
              res == UA_STATUSCODE_BADCERTIFICATEISSUERTIMEINVALID) {
        /* Suppressible errors are ignored. The validity period is checked
         * after all other steps, so the other steps passed. */
        UA_LOG_WARNING(sc->logging, UA_LOGCATEGORY_SERVER,
                       "AddCertificate: Ignoring the suppressible error %s",
                       UA_StatusCode_name(res));
        res = UA_STATUSCODE_GOOD;
    }
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(sc->logging, UA_LOGCATEGORY_SERVER,
                     "AddCertificate: The certificate is not valid (%s)",
                     UA_StatusCode_name(res));
        return res;
    }

    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    trustList.trustedCertificates = certificate;
    trustList.trustedCertificatesSize = 1;

    res = certGroup->addToTrustList(certGroup, &trustList);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Updating LastUpdateTime Variable in the information model */
    tl->lastUpdateTime = UA_DateTime_now();
    writeLastUpdateVariable(ctx, tl);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_GDSReceiver_stageCertificateUpdate(UA_GDSReceiverContext *ctx,
                                const UA_NodeId *sessionId,
                                const UA_NodeId *certificateGroupId,
                                const UA_NodeId *certificateTypeId,
                                const UA_ByteString *certificate,
                                const UA_String *privateKeyFormat,
                                const UA_ByteString *privateKey) {
    /* The server currently only supports the DefaultApplicationGroup. A null
     * CertificateGroupId selects it (Part 12, 7.10.5). */
    static UA_NodeId defaultApplicationGroup =
        STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    if(UA_NodeId_isNull(certificateGroupId))
        certificateGroupId = &defaultApplicationGroup;
    if(!UA_NodeId_equal(certificateGroupId, &defaultApplicationGroup))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* The certificate type must be one of the CertificateTypes of the group */
    UA_ServerConfig *sc = UA_Server_getConfig(ctx->drv.server);
    if(!UA_GDSReceiver_certificateTypeSupported(sc, certificateTypeId))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Verify that the privateKey is in a supported format and
     * that it matches the specified certificate */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(privateKey && privateKey->length > 0) {
        const UA_String pemFormat = UA_STRING("PEM");
        const UA_String derFormat = UA_STRING("DER");
        if(!UA_String_equal(&pemFormat, privateKeyFormat) &&
           !UA_String_equal(&derFormat, privateKeyFormat))
            return UA_STATUSCODE_BADNOTSUPPORTED;
        retval = UA_CertificateUtils_checkKeyPair(certificate, privateKey);
        if(retval != UA_STATUSCODE_GOOD)
            return UA_STATUSCODE_BADNOTSUPPORTED;
    } else {
        /* Validate the certificate before staging a keyless update. */
        size_t keySize = 0;
        if(UA_CertificateUtils_getKeySize((UA_ByteString*)(uintptr_t)certificate,
                                          &keySize) != UA_STATUSCODE_GOOD)
            return UA_STATUSCODE_BADCERTIFICATEINVALID;
    }

    /* The transaction is discarded once the queued ApplyChanges has run.
     * Refuse new changes instead of silently dropping them. */
    UA_GDSTransaction *transaction = &ctx->transaction;
    if(transaction->applyChangesQueued)
        return UA_STATUSCODE_BADTRANSACTIONPENDING;
    if(transaction->state == UA_GDSTRANSACTIONSTATE_FRESH) {
        retval = UA_GDSTransaction_init(transaction, ctx->drv.server, *sessionId);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    if(!UA_NodeId_equal(&transaction->sessionId, sessionId))
        return UA_STATUSCODE_BADTRANSACTIONPENDING;

    return UA_GDSTransaction_addCertificateInfo(transaction, *certificateGroupId,
                                                *certificateTypeId, certificate,
                                                privateKey);
}

/* TODO: Expose implementation in core open62541 */

static UA_Byte
lc(UA_Byte c) {
    if(((int)c) - 'A' < 26) return c | 32;
    return c;
}
static int
case_cmp(const UA_Byte *l, const UA_Byte *r, size_t n) {
    if(!n--) return 0;
    for(; *l && *r && n && (*l == *r || lc(*l) == lc(*r)); l++, r++, n--);
    return lc(*l) - lc(*r);
}
static UA_Boolean
String_equal_ignorecase(const UA_String *s1, const UA_String *s2) {
    if(s1->length != s2->length)
        return false;
    if(s1->length == 0)
        return true;
    if(s2->data == NULL)
        return false;
    return case_cmp(s1->data, s2->data, s1->length) == 0;
}

UA_StatusCode
UA_GDSReceiver_removeCertificate(UA_GDSReceiverContext *ctx,
                                UA_CertificateGroup *certGroup,
                                const UA_NodeId *sessionId,
                                const UA_String *thumbprint,
                                const UA_Boolean *isTrustedCertificate) {
    UA_assert(certGroup != NULL);
    UA_Server *server = ctx->drv.server;
    UA_ServerConfig *sc = UA_Server_getConfig(server);

    /* This Method cannot be called if the containing TrustList Object is open */
    UA_GDSTrustList *tl =
        UA_GDSReceiver_getTrustList(ctx, &certGroup->certificateGroupId);
    if(!tl)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_StatusCode retval = checkTrustListClosed(tl);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_GDSTransaction *transaction = &ctx->transaction;
    if(transaction->state != UA_GDSTRANSACTIONSTATE_FRESH)
        return UA_STATUSCODE_BADTRANSACTIONPENDING;

    /* When a certificate is removed, a transaction is created which is then
     * executed directly. No apply changes is required */
    retval = UA_GDSTransaction_init(transaction, server, *sessionId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_CertificateGroup *transactionCG =
        UA_GDSTransaction_getCertificateGroup(transaction, certGroup);
    if(!transactionCG) {
        UA_GDSTransaction_clear(transaction);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;

    UA_ByteString *certificates;
    size_t certificatesSize = 0;
    retval = certGroup->getTrustList(certGroup, &trustList);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_TrustListDataType_clear(&trustList);
        UA_GDSTransaction_clear(transaction);
        return retval;
    }

    if(*isTrustedCertificate) {
        certificates = trustList.trustedCertificates;
        certificatesSize = trustList.trustedCertificatesSize;
    } else {
        certificates = trustList.issuerCertificates;
        certificatesSize = trustList.issuerCertificatesSize;
    }

    UA_TrustListDataType list;
    UA_TrustListDataType_init(&list);

    UA_ByteString *crls = NULL;
    size_t crlsSize = 0;
    UA_ByteString certificate = UA_BYTESTRING_NULL;

    UA_Byte buf[UA_SHA1_LENGTH * 2];
    UA_String thumbpr = {UA_SHA1_LENGTH * 2, buf};
    for(size_t i = 0; i < certificatesSize; i++) {
        /* Compare thumbprint */
        certificate = certificates[i];
        retval = UA_CertificateUtils_getThumbprint(&certificate, &thumbpr);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
        if(!String_equal_ignorecase(thumbprint, &thumbpr))
            continue;

        retval = certGroup->getCertificateCrls(certGroup, &certificate,
                                               *isTrustedCertificate,
                                               &crls, &crlsSize);

        /* Tolerate "Bad_NoMatch" to support removing CA certificates that do
         * not have an associated CRL. */
        if(retval != UA_STATUSCODE_GOOD && retval != UA_STATUSCODE_BADNOMATCH)
            goto cleanup;

        if(*isTrustedCertificate) {
            list.specifiedLists =
                UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES | UA_TRUSTLISTMASKS_TRUSTEDCRLS;
            list.trustedCertificates = &certificate;
            list.trustedCertificatesSize = 1;
            list.trustedCrls = crls;
            list.trustedCrlsSize = crlsSize;
        } else {
            list.specifiedLists =
                UA_TRUSTLISTMASKS_ISSUERCERTIFICATES | UA_TRUSTLISTMASKS_ISSUERCRLS;
            list.issuerCertificates = &certificate;
            list.issuerCertificatesSize = 1;
            list.issuerCrls = crls;
            list.issuerCrlsSize = crlsSize;
        }
        break;
    }

    /* Thumbprint not found */
    if(list.specifiedLists == UA_TRUSTLISTMASKS_NONE) {
        UA_LOG_INFO(sc->logging, UA_LOGCATEGORY_SERVER,
                    "The certificate to remove was not found");
        retval = UA_STATUSCODE_BADINVALIDARGUMENT;
        goto cleanup;
    }

    /* Add to the transaction */
    retval = transactionCG->removeFromTrustList(transactionCG, &list);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Apply */
    retval = UA_GDSReceiver_applyChanges(ctx);

cleanup:
    UA_TrustListDataType_clear(&trustList);
    UA_Array_delete(crls, crlsSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    if(retval != UA_STATUSCODE_GOOD)
        UA_GDSTransaction_clear(transaction);
    return retval;
}

UA_StatusCode
UA_GDSReceiver_getRejectedList(UA_GDSReceiverContext *ctx, size_t outputSize,
                              UA_Variant *output) {
    if(outputSize < 1)
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    if(outputSize > 1)
        return UA_STATUSCODE_BADTOOMANYARGUMENTS;

    UA_Server *server = ctx->drv.server;
    UA_ServerConfig *sc = UA_Server_getConfig(server);

    size_t rejectedListSize = 0;

    /* DefaultApplicationGroup */
    UA_CertificateGroup *certGroup = &sc->secureChannelPKI;
    UA_ByteString *rejectedListSecureChannel = NULL;
    size_t rejectedListSecureChannelSize = 0;
    UA_StatusCode res = certGroup->getRejectedList(
        certGroup, &rejectedListSecureChannel, &rejectedListSecureChannelSize);
    if(res != UA_STATUSCODE_GOOD) {
        UA_Array_delete(rejectedListSecureChannel,
                        rejectedListSecureChannelSize,
                        &UA_TYPES[UA_TYPES_BYTESTRING]);
        return res;
    }
    rejectedListSize += rejectedListSecureChannelSize;

    /* DefaultUserTokenGroup */
    certGroup = &sc->sessionPKI;
    UA_ByteString *rejectedListSession = NULL;
    size_t rejectedListSessionSize = 0;
    res = certGroup->getRejectedList(certGroup, &rejectedListSession,
                                     &rejectedListSessionSize);
    if(res != UA_STATUSCODE_GOOD) {
        UA_Array_delete(rejectedListSecureChannel,
                        rejectedListSecureChannelSize,
                        &UA_TYPES[UA_TYPES_BYTESTRING]);
        UA_Array_delete(rejectedListSession, rejectedListSessionSize,
                        &UA_TYPES[UA_TYPES_BYTESTRING]);
        return res;
    }
    rejectedListSize += rejectedListSessionSize;

    if(rejectedListSize == 0) {
        UA_Variant_setArray(&output[0], NULL, 0, &UA_TYPES[UA_TYPES_BYTESTRING]);
        UA_Array_delete(rejectedListSecureChannel,
                        rejectedListSecureChannelSize,
                        &UA_TYPES[UA_TYPES_BYTESTRING]);
        UA_Array_delete(rejectedListSession, rejectedListSessionSize,
                        &UA_TYPES[UA_TYPES_BYTESTRING]);
        return UA_STATUSCODE_GOOD;
    }

    /* Create a temp array (shallow) */
    UA_ByteString *rejectedList = (UA_ByteString*)
        UA_Array_new(rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    if(!rejectedList) {
        UA_Array_delete(rejectedListSecureChannel,
                        rejectedListSecureChannelSize,
                        &UA_TYPES[UA_TYPES_BYTESTRING]);
        UA_Array_delete(rejectedListSession,
                        rejectedListSessionSize,
                        &UA_TYPES[UA_TYPES_BYTESTRING]);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    memcpy(rejectedList, rejectedListSecureChannel,
           rejectedListSecureChannelSize * sizeof(UA_ByteString));
    memcpy(rejectedList + rejectedListSecureChannelSize,
           rejectedListSession, rejectedListSessionSize * sizeof(UA_ByteString));

    /* Set the array in the output */
    res = UA_Variant_setArrayCopy(&output[0], rejectedList, rejectedListSize,
                                  &UA_TYPES[UA_TYPES_BYTESTRING]);

    /* Clean up */
    UA_Array_delete(rejectedListSecureChannel, rejectedListSecureChannelSize,
                    &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Array_delete(rejectedListSession, rejectedListSessionSize,
                    &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_free(rejectedList);
    return res;
}

static void
secureChannel_delayedClose(void *application, void *context) {
    UA_GDSReceiverContext *ctx = (UA_GDSReceiverContext*)context;
    UA_Server *server = ctx->drv.server;
    UA_ServerConfig *sc = UA_Server_getConfig(server);
    UA_GDSTransactionChanges *changes = (UA_GDSTransactionChanges*)application;

    ChannelMetadata *cm;
    UA_CertificateGroup *certGroup = &sc->secureChannelPKI;
    switch(*changes) {
    case UA_GDSTRANSACTIONCHANGES_NOTHING:
        break;

    case UA_GDSTRANSACTIONCHANGES_BOTH:
    case UA_GDSTRANSACTIONCHANGES_CERTIFICATE:
        /* Shutdown SecureChannels that use an updated certificate type. */
        LIST_FOREACH(cm, &ctx->secureChannels, pointers) {
            UA_Boolean certificateTypeUpdated = false;
            for(size_t i = 0; i < ctx->transaction.certificateInfosSize; i++) {
                UA_GDSCertificateInfo *certInfo =
                    &ctx->transaction.certificateInfos[i];
                if(UA_NodeId_equal(&cm->certificateTypeId,
                                   &certInfo->certificateType)) {
                    certificateTypeUpdated = true;
                    break;
                }
            }
            if(!certificateTypeUpdated)
                continue;
            UA_Server_closeSecureChannel(server, cm->channelId,
                                         UA_SHUTDOWNREASON_CLOSE);
        }
        break;

    default:
        /* Re-verify remote certificates. Close the SecureChannel on failure. */
        LIST_FOREACH(cm, &ctx->secureChannels, pointers) {
            UA_StatusCode res =
                certGroup->verifyCertificate(certGroup, &cm->certificate);
            if(res != UA_STATUSCODE_GOOD)
                UA_Server_closeSecureChannel(server, cm->channelId,
                                             UA_SHUTDOWNREASON_CLOSE);
        }
        break;
    }

    UA_free(changes);
    ctx->transaction.applyChangesQueued = false;
    UA_GDSTransaction_clear(&ctx->transaction);
    UA_assert(ctx->pendingDelayedCallbacks > 0);
    ctx->pendingDelayedCallbacks--;
    if(ctx->pendingDelayedCallbacks == 0 &&
       ctx->drv.state == UA_LIFECYCLESTATE_STOPPING)
        ctx->drv.state = UA_LIFECYCLESTATE_STOPPED;
}

static UA_SecurityPolicy *
getSecPolicyByUri(UA_ServerConfig *sc, const UA_String *securityPolicyUri) {
    for(size_t i = 0; i < sc->securityPoliciesSize; i++) {
        UA_SecurityPolicy *sp = &sc->securityPolicies[i];
        if(UA_String_equal(securityPolicyUri, &sp->policyUri))
            return sp;
    }
    return NULL;
}

/* The receiver updates the RSA application certificates of the
 * DefaultApplicationGroup. A certificate type is supported only if an endpoint
 * uses a SecurityPolicy with that type, so that an update takes effect. The
 * supported types are the CertificateTypes of the group (Part 12 §7.10.5). */
UA_Boolean
UA_GDSReceiver_certificateTypeSupported(UA_ServerConfig *sc,
                                        const UA_NodeId *certificateTypeId) {
    static UA_NodeId defaultApplicationGroup =
        STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    static UA_NodeId certTypRsaMin = STATIC_NS0ID(RSAMINAPPLICATIONCERTIFICATETYPE);
    static UA_NodeId certTypRsaSha256 = STATIC_NS0ID(RSASHA256APPLICATIONCERTIFICATETYPE);
    if(!UA_NodeId_equal(certificateTypeId, &certTypRsaSha256) &&
       !UA_NodeId_equal(certificateTypeId, &certTypRsaMin))
        return false;

    for(size_t i = 0; i < sc->endpointsSize; i++) {
        UA_SecurityPolicy *sp =
            getSecPolicyByUri(sc, &sc->endpoints[i].securityPolicyUri);
        if(sp && UA_NodeId_equal(&sp->certificateTypeId, certificateTypeId) &&
           UA_NodeId_equal(&sp->certificateGroupId, &defaultApplicationGroup))
            return true;
    }
    return false;
}

/* Update every SecurityPolicy and endpoint that uses the certificate type.
 * First resolve all endpoint policies and allocate the replacement endpoint
 * certificates. This ensures configuration and allocation errors are reported
 * before any live policy is changed. A SecurityPolicy referenced by multiple
 * endpoints is updated only once. After all policy updates succeed, commit the
 * preallocated endpoint certificates without further fallible operations.
 *
 * SecurityPolicy implementations update their private state directly and do
 * not expose the previous private key for rollback. An unexpected failure from
 * a policy after an earlier policy succeeded is therefore logged explicitly. */
static UA_StatusCode
applyCertificateToPolicies(UA_ServerConfig *sc,
                           const UA_NodeId *certificateTypeId,
                           const UA_ByteString certificate,
                           const UA_ByteString privateKey) {
    if(sc->endpointsSize > GDS_RECEIVER_MAX_ENDPOINTS) {
        UA_LOG_ERROR(sc->logging, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Cannot update the certificate for more than %u endpoints",
                     (unsigned)GDS_RECEIVER_MAX_ENDPOINTS);
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    UA_SecurityPolicy *policies[GDS_RECEIVER_MAX_ENDPOINTS];
    size_t policiesSize = 0;
    UA_ByteString endpointCertificates[GDS_RECEIVER_MAX_ENDPOINTS] = {0};
    UA_Boolean updateEndpoint[GDS_RECEIVER_MAX_ENDPOINTS] = {0};

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < sc->endpointsSize; i++) {
        UA_EndpointDescription *ed = &sc->endpoints[i];
        UA_SecurityPolicy *sp = getSecPolicyByUri(sc, &ed->securityPolicyUri);
        if(!sp) {
            res = UA_STATUSCODE_BADINTERNALERROR;
            goto cleanup;
        }
        if(!UA_NodeId_equal(&sp->certificateTypeId, certificateTypeId))
            continue;

        res = UA_ByteString_copy(&certificate, &endpointCertificates[i]);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
        updateEndpoint[i] = true;

        size_t j = 0;
        for(; j < policiesSize; j++) {
            if(policies[j] == sp)
                break;
        }
        if(j == policiesSize)
            policies[policiesSize++] = sp;
    }

    /* No endpoint uses the certificate type. The update would have no
     * effect. */
    if(policiesSize == 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* A SecurityPolicy#None endpoint presents the certificate of the None
     * SecurityPolicy, which is usually the replaced certificate. Update it as
     * well, so that all endpoints present the same certificate. */
    for(size_t i = 0; i < sc->endpointsSize; i++) {
        if(updateEndpoint[i])
            continue;
        UA_SecurityPolicy *sp =
            getSecPolicyByUri(sc, &sc->endpoints[i].securityPolicyUri);
        if(!sp || sp->policyType != UA_SECURITYPOLICYTYPE_NONE ||
           sp->localCertificate.length == 0)
            continue;

        UA_Boolean replaced = false;
        size_t j = 0;
        for(; j < policiesSize; j++) {
            if(policies[j] == sp)
                break;
            if(UA_ByteString_equal(&sp->localCertificate,
                                   &policies[j]->localCertificate))
                replaced = true;
        }
        if(j == policiesSize && !replaced)
            continue;

        res = UA_ByteString_copy(&certificate, &endpointCertificates[i]);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
        updateEndpoint[i] = true;
        if(j == policiesSize)
            policies[policiesSize++] = sp;
    }

    /* Endpoint resolution and allocations cannot fail from here onwards. */
    for(size_t i = 0; i < policiesSize; i++) {
        res = policies[i]->updateCertificate(policies[i], certificate,
                                              privateKey);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(sc->logging, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Updating the certificate failed after %u of %u "
                         "SecurityPolicies were updated",
                         (unsigned)i, (unsigned)policiesSize);
            goto cleanup;
        }
    }

    /* Commit the preallocated endpoint certificates without further errors. */
    for(size_t i = 0; i < sc->endpointsSize; i++) {
        if(!updateEndpoint[i])
            continue;
        UA_ByteString_clear(&sc->endpoints[i].serverCertificate);
        sc->endpoints[i].serverCertificate = endpointCertificates[i];
        endpointCertificates[i] = UA_BYTESTRING_NULL;
    }

cleanup:
    for(size_t i = 0; i < sc->endpointsSize; i++)
        UA_ByteString_clear(&endpointCertificates[i]);
    return res;
}

typedef struct {
    UA_DelayedCallback dc;
    UA_GDSReceiverContext *ctx;
    UA_NodeId certificateTypeId;
} CloseChannelsCallback;

static void
closeChannelsAfterCertificateUpdate(void *application, void *context) {
    CloseChannelsCallback *ccb = (CloseChannelsCallback*)context;
    UA_GDSReceiverContext *ctx = ccb->ctx;
    ChannelMetadata *cm, *cmTmp;
    LIST_FOREACH_SAFE(cm, &ctx->secureChannels, pointers, cmTmp) {
        if(!UA_NodeId_equal(&cm->certificateTypeId, &ccb->certificateTypeId))
            continue;
        UA_Server_closeSecureChannel(ctx->drv.server, cm->channelId,
                                     UA_SHUTDOWNREASON_CLOSE);
    }
    UA_assert(ctx->pendingDelayedCallbacks > 0);
    ctx->pendingDelayedCallbacks--;
    if(ctx->pendingDelayedCallbacks == 0 &&
       ctx->drv.state == UA_LIFECYCLESTATE_STOPPING)
        ctx->drv.state = UA_LIFECYCLESTATE_STOPPED;
    UA_NodeId_clear(&ccb->certificateTypeId);
    UA_free(ccb);
}

static UA_StatusCode
updateCertificateLocked(UA_GDSReceiver *receiver,
                        const UA_NodeId certificateGroupId,
                        const UA_NodeId certificateTypeId,
                        const UA_ByteString certificate,
                        const UA_ByteString *privateKey) {
    UA_GDSReceiverContext *ctx = (UA_GDSReceiverContext*)receiver;
    if(ctx->drv.state != UA_LIFECYCLESTATE_STARTED)
        return UA_STATUSCODE_BADINVALIDSTATE;
    if(ctx->transaction.state == UA_GDSTRANSACTIONSTATE_PENDING)
        return UA_STATUSCODE_BADTRANSACTIONPENDING;

    static UA_NodeId defaultApplicationGroup =
        STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    UA_NodeId certGroupId = certificateGroupId;
    if(UA_NodeId_isNull(&certGroupId))
        certGroupId = defaultApplicationGroup;
    if(!UA_NodeId_equal(&certGroupId, &defaultApplicationGroup))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    static UA_NodeId certTypRsaSha256 =
        STATIC_NS0ID(RSASHA256APPLICATIONCERTIFICATETYPE);
    if(!UA_NodeId_equal(&certificateTypeId, &certTypRsaSha256))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_ByteString newPrivateKey = UA_BYTESTRING_NULL;
    if(privateKey) {
        if(UA_CertificateUtils_checkKeyPair(&certificate, privateKey) !=
           UA_STATUSCODE_GOOD)
            return UA_STATUSCODE_BADNOTSUPPORTED;
        newPrivateKey = *privateKey;
    } else {
        size_t keySize = 0;
        if(UA_CertificateUtils_getKeySize((UA_ByteString*)(uintptr_t)&certificate,
                                          &keySize) != UA_STATUSCODE_GOOD)
            return UA_STATUSCODE_BADCERTIFICATEINVALID;
    }

    UA_Server *server = receiver->drv.server;
    UA_ServerConfig *sc = UA_Server_getConfig(server);
    CloseChannelsCallback *ccb =
        (CloseChannelsCallback*)UA_calloc(1, sizeof(CloseChannelsCallback));
    if(!ccb)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    ccb->ctx = ctx;
    UA_StatusCode res =
        UA_NodeId_copy(&certificateTypeId, &ccb->certificateTypeId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(ccb);
        return res;
    }

    res = applyCertificateToPolicies(sc, &certificateTypeId, certificate,
                                     newPrivateKey);
    if(res != UA_STATUSCODE_GOOD) {
        UA_NodeId_clear(&ccb->certificateTypeId);
        UA_free(ccb);
        return res;
    }

    ccb->dc.callback = closeChannelsAfterCertificateUpdate;
    ccb->dc.context = ccb;
    ctx->pendingDelayedCallbacks++;
    sc->eventLoop->addDelayedCallback(sc->eventLoop, &ccb->dc);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_GDSReceiver_updateCertificate(UA_GDSReceiver *receiver,
                                 const UA_NodeId certificateGroupId,
                                 const UA_NodeId certificateTypeId,
                                 const UA_ByteString certificate,
                                 const UA_ByteString *privateKey) {
    if(!receiver || certificate.length == 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(!receiver->drv.server)
        return UA_STATUSCODE_BADINVALIDSTATE;

    UA_ServerConfig *sc = UA_Server_getConfig(receiver->drv.server);
    sc->eventLoop->lock(sc->eventLoop);
    UA_StatusCode res = updateCertificateLocked(receiver, certificateGroupId,
                                                certificateTypeId, certificate,
                                                privateKey);
    sc->eventLoop->unlock(sc->eventLoop);
    return res;
}

static UA_StatusCode
createSigningRequestLocked(UA_GDSReceiver *receiver,
                           const UA_NodeId certificateGroupId,
                           const UA_NodeId certificateTypeId,
                           const UA_String *subjectName,
                           const UA_Boolean *regenerateKey,
                           const UA_ByteString *nonce,
                           UA_ByteString *csr) {
    UA_GDSReceiverContext *ctx = (UA_GDSReceiverContext*)receiver;
    if(ctx->drv.state != UA_LIFECYCLESTATE_STARTED)
        return UA_STATUSCODE_BADINVALIDSTATE;

    static UA_NodeId defaultApplicationGroup =
        STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    UA_NodeId certGroupId = certificateGroupId;
    if(UA_NodeId_isNull(&certGroupId))
        certGroupId = defaultApplicationGroup;
    if(!UA_NodeId_equal(&certGroupId, &defaultApplicationGroup))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Without a SecurityPolicy for the certificate type, no CSR would be
     * created */
    UA_ServerConfig *sc = UA_Server_getConfig(receiver->drv.server);
    if(!UA_GDSReceiver_certificateTypeSupported(sc, &certificateTypeId))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    if(!UA_NodeId_equal(&sc->secureChannelPKI.certificateGroupId,
                        &defaultApplicationGroup))
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ByteString *newPrivateKey = NULL;
    if(regenerateKey && *regenerateKey) {
        newPrivateKey = UA_ByteString_new();
        if(!newPrivateKey)
            return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    for(size_t i = 0; i < sc->endpointsSize; i++) {
        UA_SecurityPolicy *sp =
            getSecPolicyByUri(sc, &sc->endpoints[i].securityPolicyUri);
        if(!sp) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            goto cleanup;
        }
        if(sp->policyType == UA_SECURITYPOLICYTYPE_NONE)
            continue;
        if(!UA_NodeId_equal(&certificateTypeId, &sp->certificateTypeId) ||
           !UA_NodeId_equal(&certGroupId, &sp->certificateGroupId))
            continue;

        retval = sp->createSigningRequest(sp, subjectName, nonce,
                                          &UA_KEYVALUEMAP_NULL, csr,
                                          newPrivateKey);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    UA_ByteString localCsrCertificate = UA_BYTESTRING_NULL;
    retval = UA_ByteString_copy(csr, &localCsrCertificate);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&ctx->transaction.localCsrCertificate);
        ctx->transaction.localCsrCertificate = localCsrCertificate;
    }

cleanup:
    if(newPrivateKey) {
        UA_ByteString_memZero(newPrivateKey);
        UA_ByteString_delete(newPrivateKey);
    }
    return retval;
}

UA_StatusCode
UA_GDSReceiver_createSigningRequest(UA_GDSReceiver *receiver,
                                    const UA_NodeId certificateGroupId,
                                    const UA_NodeId certificateTypeId,
                                    const UA_String *subjectName,
                                    const UA_Boolean *regenerateKey,
                                    const UA_ByteString *nonce,
                                    UA_ByteString *csr) {
    if(!receiver || !csr)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(!receiver->drv.server)
        return UA_STATUSCODE_BADINVALIDSTATE;

    UA_ServerConfig *sc = UA_Server_getConfig(receiver->drv.server);
    sc->eventLoop->lock(sc->eventLoop);
    UA_StatusCode res = createSigningRequestLocked(
        receiver, certificateGroupId, certificateTypeId, subjectName,
        regenerateKey, nonce, csr);
    sc->eventLoop->unlock(sc->eventLoop);
    return res;
}

static UA_StatusCode
UA_GDSReceiver_applyChanges(UA_GDSReceiverContext *ctx) {
    UA_Server *server = ctx->drv.server;
    UA_ServerConfig *sc = UA_Server_getConfig(server);
    UA_GDSTransaction *transaction = &ctx->transaction;
    if(transaction->applyChangesQueued)
        return UA_STATUSCODE_BADINVALIDSTATE;

    /* Check if a TrustList is still open */
    for(size_t i = 0; i < transaction->certGroupSize; i++) {
        UA_CertificateGroup *certGroup = &transaction->certGroups[i];
        UA_GDSTrustList *tl =
            UA_GDSReceiver_getTrustList(ctx, &certGroup->certificateGroupId);
        if(!tl)
            return UA_STATUSCODE_BADINTERNALERROR;
        if(tl->handlesSize > 0)
            return UA_STATUSCODE_BADINVALIDSTATE;
    }

    /* No changes are applied while any TrustList is open for writing (Part 12
     * §7.10.9). The transaction may also hold a certificate update only. */
    if(UA_GDSReceiver_trustListOpenForWrite(ctx))
        return UA_STATUSCODE_BADINVALIDSTATE;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_GDSTransactionChanges *changes = (UA_GDSTransactionChanges*)
        UA_calloc(1, sizeof(UA_GDSTransactionChanges));
    if(!changes)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    size_t groupsSize = transaction->certGroupSize;
    UA_CertificateGroup **groups = NULL;
    UA_TrustListDataType *newTrustLists = NULL;
    UA_TrustListDataType *oldTrustLists = NULL;
    if(groupsSize > 0) {
        groups = (UA_CertificateGroup**)UA_calloc(groupsSize, sizeof(void*));
        newTrustLists = (UA_TrustListDataType*)
            UA_calloc(groupsSize, sizeof(UA_TrustListDataType));
        oldTrustLists = (UA_TrustListDataType*)
            UA_calloc(groupsSize, sizeof(UA_TrustListDataType));
        if(!groups || !newTrustLists || !oldTrustLists) {
            retval = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
    }

    /* Resolve every live group and snapshot both the staged and current trust
     * lists before mutating configuration. The snapshots permit rolling back
     * all trust-list changes if any subsequent operation fails. */
    for(size_t i = 0; i < groupsSize; i++) {
        UA_CertificateGroup *stagedGroup = &transaction->certGroups[i];
        groups[i] = getCertGroup(server, &stagedGroup->certificateGroupId);
        if(!groups[i]) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            goto cleanup;
        }
        newTrustLists[i].specifiedLists = UA_TRUSTLISTMASKS_ALL;
        oldTrustLists[i].specifiedLists = UA_TRUSTLISTMASKS_ALL;
        retval = stagedGroup->getTrustList(stagedGroup, &newTrustLists[i]);
        if(retval == UA_STATUSCODE_GOOD)
            retval = groups[i]->getTrustList(groups[i], &oldTrustLists[i]);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    size_t appliedGroups = 0;
    for(size_t i = 0; i < groupsSize; i++) {
        /* Include the current group in rollback even if its plugin reports an
         * error after partially changing its internal state. */
        appliedGroups = i + 1;
        retval = groups[i]->setTrustList(groups[i], &newTrustLists[i]);
        if(retval != UA_STATUSCODE_GOOD)
            goto rollback;
    }

    /* Apply Server certificate changes */
    for(size_t i = 0; i < transaction->certificateInfosSize; i++) {
        UA_GDSCertificateInfo certInfo = transaction->certificateInfos[i];
        UA_NodeId certTypeId = certInfo.certificateType;
        UA_ByteString certificate = certInfo.certificate;
        UA_ByteString privateKey = certInfo.privateKey;

        retval = applyCertificateToPolicies(sc, &certTypeId, certificate,
                                            privateKey);
        if(retval != UA_STATUSCODE_GOOD)
            goto rollback;
    }

    if(groupsSize > 0 && transaction->certificateInfosSize > 0)
        *changes = UA_GDSTRANSACTIONCHANGES_BOTH;
    else if(groupsSize > 0)
        *changes = UA_GDSTRANSACTIONCHANGES_TRUSTLIST;
    else if(transaction->certificateInfosSize > 0)
        *changes = UA_GDSTRANSACTIONCHANGES_CERTIFICATE;

    /* Publish metadata only after the complete transaction has succeeded. */
    for(size_t i = 0; i < groupsSize; i++) {
        UA_GDSTrustList *tl =
            UA_GDSReceiver_getTrustList(ctx, &groups[i]->certificateGroupId);
        UA_assert(tl != NULL);
        tl->lastUpdateTime = UA_DateTime_now();
        writeLastUpdateVariable(ctx, tl);
    }

    /* Add to the delayed callback list. Will be cleaned up in the next
     * eventloop iteration. This is required so that the apply function can
     * return a statuscode before the SecureChannel is closed. */
    UA_DelayedCallback *dc = &transaction->dc;
    dc->callback = secureChannel_delayedClose;
    dc->application = changes;
    dc->context = ctx;

    UA_EventLoop *el = sc->eventLoop;
    transaction->applyChangesQueued = true;
    ctx->pendingDelayedCallbacks++;
    el->addDelayedCallback(el, dc);
    for(size_t i = 0; i < groupsSize; i++) {
        UA_TrustListDataType_clear(&newTrustLists[i]);
        UA_TrustListDataType_clear(&oldTrustLists[i]);
    }
    UA_free(groups);
    UA_free(newTrustLists);
    UA_free(oldTrustLists);
    return UA_STATUSCODE_GOOD;

rollback:
    while(appliedGroups > 0) {
        appliedGroups--;
        UA_StatusCode rollbackRes = groups[appliedGroups]->setTrustList(
            groups[appliedGroups], &oldTrustLists[appliedGroups]);
        if(rollbackRes != UA_STATUSCODE_GOOD)
            UA_LOG_ERROR(sc->logging, UA_LOGCATEGORY_SERVER,
                         "Could not roll back a GDS Receiver trust-list update");
    }

cleanup:
    if(newTrustLists) {
        for(size_t i = 0; i < groupsSize; i++)
            UA_TrustListDataType_clear(&newTrustLists[i]);
    }
    if(oldTrustLists) {
        for(size_t i = 0; i < groupsSize; i++)
            UA_TrustListDataType_clear(&oldTrustLists[i]);
    }
    UA_free(groups);
    UA_free(newTrustLists);
    UA_free(oldTrustLists);
    UA_GDSTransaction_clear(transaction);
    UA_free(changes);
    return retval;
}

static void
receiverNotificationCallback(UA_Driver *drv,
                             UA_ApplicationNotificationType type,
                             const UA_KeyValueMap payload) {
    UA_GDSReceiverContext *ctx = (UA_GDSReceiverContext*)drv;
    if(type == UA_APPLICATIONNOTIFICATIONTYPE_SESSION_CLOSED) {
        /* A closed or abandoned Session discards its transaction (Part 12,
         * 7.10.9). The file transfer drivers close its TrustList handles. */
        const UA_NodeId *sessionId = (const UA_NodeId*)
            UA_KeyValueMap_getScalar(&payload, UA_QUALIFIEDNAME(0, "session-id"),
                                     &UA_TYPES[UA_TYPES_NODEID]);
        UA_GDSTransaction *transaction = &ctx->transaction;
        if(!sessionId || transaction->state != UA_GDSTRANSACTIONSTATE_PENDING ||
           transaction->applyChangesQueued ||
           !UA_NodeId_equal(&transaction->sessionId, sessionId))
            return;
        UA_ServerConfig *sc = UA_Server_getConfig(drv->server);
        UA_LOG_INFO(sc->logging, UA_LOGCATEGORY_SERVER,
                    "Session with an open transaction has ended. "
                    "The transaction has been discarded.");
        UA_GDSTransaction_clear(transaction);
    } else if(type == UA_APPLICATIONNOTIFICATIONTYPE_SECURECHANNEL_OPENED) {
        UA_UInt32 channelId = *(const UA_UInt32*)
            UA_KeyValueMap_getScalar(&payload,
                                     UA_QUALIFIEDNAME(0, "securechannel-id"),
                                     &UA_TYPES[UA_TYPES_UINT32]);
        UA_ByteString certificate = *(const UA_ByteString*)
            UA_KeyValueMap_getScalar(&payload,
                                     UA_QUALIFIEDNAME(0, "remote-certificate"),
                                     &UA_TYPES[UA_TYPES_BYTESTRING]);
        const UA_NodeId *certificateTypeId = (const UA_NodeId*)
            UA_KeyValueMap_getScalar(&payload,
                                     UA_QUALIFIEDNAME(0, "certificate-type-id"),
                                     &UA_TYPES[UA_TYPES_NODEID]);
        if(!certificateTypeId)
            return;

        ChannelMetadata *cm = (ChannelMetadata*)UA_calloc(1, sizeof(ChannelMetadata));
        if(!cm) {
            UA_ServerConfig *sc = UA_Server_getConfig(drv->server);
            UA_LOG_WARNING(sc->logging, UA_LOGCATEGORY_SERVER,
                           "Could not register SecureChannel (oom)");
            return;
        }
        cm->channelId = channelId;
        UA_StatusCode res = UA_NodeId_copy(certificateTypeId,
                                           &cm->certificateTypeId);
        res |= UA_ByteString_copy(&certificate, &cm->certificate);
        if(res != UA_STATUSCODE_GOOD) {
            UA_NodeId_clear(&cm->certificateTypeId);
            UA_ByteString_clear(&cm->certificate);
            UA_free(cm);
            return;
        }
        LIST_INSERT_HEAD(&ctx->secureChannels, cm, pointers);
    } else if(type == UA_APPLICATIONNOTIFICATIONTYPE_SECURECHANNEL_CLOSED) {
        UA_UInt32 channelId = *(const UA_UInt32*)
            UA_KeyValueMap_getScalar(&payload,
                                     UA_QUALIFIEDNAME(0, "securechannel-id"),
                                     &UA_TYPES[UA_TYPES_UINT32]);
        ChannelMetadata *cm;
        LIST_FOREACH(cm, &ctx->secureChannels, pointers) {
            if(cm->channelId != channelId)
                continue;
            LIST_REMOVE(cm, pointers);
            UA_NodeId_clear(&cm->certificateTypeId);
            UA_ByteString_clear(&cm->certificate);
            UA_free(cm);
            break;
        }
    }
}

static UA_StatusCode
UA_GDSReceiver_start(UA_Driver *drv) {
    UA_GDSReceiverContext *ctx = (UA_GDSReceiverContext*)drv;

    /* Initialize ns0 entries only once */
    if(!ctx->initialized) {
        UA_StatusCode res = initNS0PushManagement(ctx);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        res = UA_GDSReceiver_startTrustLists(ctx);
        if(res != UA_STATUSCODE_GOOD) {
            clearNS0PushManagement(ctx);
            return res;
        }
#ifdef UA_ENABLE_RBAC
        /* Restrict the GDS methods to the SecurityAdmin role. Runs after the
         * ns0 GDS nodes have been created so the permissions can be applied.
         * The AccessControl plugin can restrict the methods as well. So a
         * failure is only logged. */
        res = initGDSRolePermissions(drv->server);
        if(res != UA_STATUSCODE_GOOD) {
            UA_ServerConfig *sc = UA_Server_getConfig(drv->server);
            UA_LOG_WARNING(sc->logging, UA_LOGCATEGORY_SERVER,
                           "Could not restrict the PushManagement methods to "
                           "the SecurityAdmin role (%s). Only the AccessControl "
                           "restricts them.", UA_StatusCode_name(res));
        }
#endif
        ctx->initialized = true;
    }

    drv->state = UA_LIFECYCLESTATE_STARTED;

    return UA_STATUSCODE_GOOD;
}

static void
UA_GDSReceiver_stop(UA_Driver *drv) {
    UA_GDSReceiverContext *ctx = (UA_GDSReceiverContext*)drv;
    if(ctx->initialized) {
        UA_GDSReceiver_stopTrustLists(ctx);
        clearNS0PushManagement(ctx);
        ctx->initialized = false;
    }
    if(ctx->pendingDelayedCallbacks > 0)
        drv->state = UA_LIFECYCLESTATE_STOPPING;
    else
        drv->state = UA_LIFECYCLESTATE_STOPPED;
}

static UA_StatusCode
UA_GDSReceiver_free(UA_Driver *drv) {
    if(drv->state != UA_LIFECYCLESTATE_STOPPED) {
        if(drv->server) {
            UA_ServerConfig *sc = UA_Server_getConfig(drv->server);
            UA_LOG_ERROR(sc->logging, UA_LOGCATEGORY_SERVER,
                         "Cannot delete the GDS Receiver driver because "
                         "it is not stopped");
        }
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_GDSReceiverContext *ctx = (UA_GDSReceiverContext*)drv;
    UA_GDSReceiver_stopTrustLists(ctx);
    UA_GDSTransaction_clear(&ctx->transaction);
    UA_KeyValueMap_clear(&drv->params);

    /* Free SecureChannel Metadata */
    ChannelMetadata *cm, *cm_tmp;
    LIST_FOREACH_SAFE(cm, &ctx->secureChannels, pointers, cm_tmp) {
        LIST_REMOVE(cm, pointers);
        UA_NodeId_clear(&cm->certificateTypeId);
        UA_ByteString_clear(&cm->certificate);
        UA_free(cm);
    }

    UA_free(drv);
    return UA_STATUSCODE_GOOD;
}

UA_GDSReceiver *
UA_GDSReceiver_new(void) {
    UA_GDSReceiverContext *ctx = (UA_GDSReceiverContext*)UA_calloc(1, sizeof(UA_GDSReceiverContext));
    if(!ctx)
        return NULL;

    ctx->drv.driverType = UA_DRIVERTYPE_GDS_RECEIVER;
    ctx->drv.name = UA_STRING("gds-receiver");
    ctx->drv.start = UA_GDSReceiver_start;
    ctx->drv.stop = UA_GDSReceiver_stop;
    ctx->drv.free = UA_GDSReceiver_free;

    ctx->drv.notificationCallback = receiverNotificationCallback;
    ctx->drv.notificationFilter = UA_APPLICATIONNOTIFICATIONTYPE_SECURECHANNEL |
        UA_APPLICATIONNOTIFICATIONTYPE_SESSION;
    UA_GDSReceiver_initTrustLists(ctx);

    return (UA_GDSReceiver*)ctx;
}

#endif
