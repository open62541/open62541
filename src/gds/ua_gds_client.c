/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Markus Karch, Fraunhofer IOSB
 */

#include <ua_client_internal.h>
#include <ua_client_config.h>
#include "ua_gds_client.h"
#include "ua_client_highlevel.h"
#include "ua_util.h"
#include "ua_log_stdout.h"

#ifdef UA_ENABLE_GDS_CLIENT

UA_StatusCode
UA_GDS_call_findApplication(UA_Client *client,
                            UA_String uri,
                            size_t *length,
                            UA_ApplicationRecordDataType *records) {
    UA_Variant input;
    UA_Variant_setScalarCopy(&input, &uri, &UA_TYPES[UA_TYPES_STRING]);
    size_t outputSize;
    UA_Variant *output;
    UA_StatusCode retval = UA_Client_call(client, UA_NODEID_NUMERIC(2, 141),
                                          UA_NODEID_NUMERIC(2, 143), 1, &input, &outputSize, &output);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_ExtensionObject *eo = (UA_ExtensionObject*) output->data;
        if (eo != NULL) {
            *length = output->arrayLength;
            //TODO copy records (unnecessary for now)
            //UA_ApplicationRecordDataType *record3 = (UA_ApplicationRecordDataType *) eo->content.decoded.data;
        }
        else {
            *length = 0;
        }
        UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Method call was unsuccessful, and %x returned values available.\n", retval);
    }
    UA_Variant_deleteMembers(&input);
    return retval;
}


UA_StatusCode
UA_GDS_call_registerApplication(UA_Client *client,
                                UA_ApplicationRecordDataType *record,
                                UA_NodeId *newNodeId) {

    UA_Variant input;
    UA_Variant_setScalarCopy(&input, record, &ApplicationRecordDataType);
    size_t outputSize;
    UA_Variant *output;
    UA_StatusCode  retval = UA_Client_call(client, UA_NODEID_NUMERIC(2, 141),
                                           UA_NODEID_NUMERIC(2, 146), 1, &input, &outputSize, &output);
    if(retval == UA_STATUSCODE_GOOD) {
        *newNodeId =  *((UA_NodeId*)output[0].data);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"ApplicationID: " UA_PRINTF_GUID_FORMAT "",
                    UA_PRINTF_GUID_DATA(newNodeId->identifier.guid));
        UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Method call was unsuccessful, and %x returned values available.\n", retval);
    }

    UA_Variant_deleteMembers(&input);
    return retval;
}

UA_StatusCode
UA_GDS_call_unregisterApplication(UA_Client *client,
                                  UA_NodeId *newNodeId) {
    UA_Variant input;
    UA_Variant_setScalarCopy(&input, newNodeId, &UA_TYPES[UA_TYPES_NODEID]);
    size_t outputSize;
    UA_Variant *output;
    UA_StatusCode  retval = UA_Client_call(client, UA_NODEID_NUMERIC(2, 141),
                                           UA_NODEID_NUMERIC(2, 149), 1, &input, &outputSize, &output);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Method call was unsuccessful, and %x returned values available.\n", retval);
    }
    UA_Variant_deleteMembers(&input);
    return retval;
}

UA_StatusCode
UA_GDS_call_startNewKeyPairRequest(UA_Client *client,
                                   UA_NodeId *applicationId,
                                   const UA_NodeId *certificateGroupId,
                                   const UA_NodeId *certificateTypeId,
                                   UA_String *subjectName,
                                   size_t domainNameLength,
                                   UA_String *domainNames,
                                   const UA_String *privateKeyFormat,
                                   const UA_String *privateKeyPassword,
                                   UA_NodeId *requestId) {
    UA_Variant input[7];

    UA_Variant_setScalarCopy(&input[0], applicationId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalarCopy(&input[1], certificateGroupId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalarCopy(&input[2], certificateTypeId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalarCopy(&input[3], subjectName, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setArrayCopy(&input[4], domainNames, 3, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalarCopy(&input[5], privateKeyFormat, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalarCopy(&input[6], privateKeyPassword, &UA_TYPES[UA_TYPES_STRING]);
    size_t outputSize;
    UA_Variant *output;
    UA_StatusCode retval = UA_Client_call(client, UA_NODEID_NUMERIC(2, 141),
                                          UA_NODEID_NUMERIC(2, 154), 7, input, &outputSize, &output);
    if(retval == UA_STATUSCODE_GOOD) {
        *requestId =  *((UA_NodeId*)output[0].data);
        // printf("RequestID: " UA_PRINTF_GUID_FORMAT "\n",UA_PRINTF_GUID_DATA(requestId->identifier.guid));
        UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    } else {
        printf("Method call was unsuccessful, and %x returned values available.\n", retval);
    }

    for(size_t i = 0; i < 7; i++)
        UA_Variant_deleteMembers(&input[i]);;

    return retval;
}

UA_StatusCode
UA_GDS_call_startSigningRequest(UA_Client *client,
                                UA_NodeId *applicationId,
                                const UA_NodeId *certificateGroupId,
                                const UA_NodeId *certificateTypeId,
                                UA_ByteString *csr,
                                UA_NodeId *requestId) {
    UA_Variant input[4];

    UA_Variant_setScalarCopy(&input[0], applicationId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalarCopy(&input[1], certificateGroupId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalarCopy(&input[2], certificateTypeId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalarCopy(&input[3], csr, &UA_TYPES[UA_TYPES_BYTESTRING]);
    size_t outputSize;
    UA_Variant *output;
    UA_StatusCode  retval = UA_Client_call(client, UA_NODEID_NUMERIC(2, 141),
                                           UA_NODEID_NUMERIC(2, 157), 4, input, &outputSize, &output);
    if(retval == UA_STATUSCODE_GOOD) {
        *requestId =  *((UA_NodeId*)output[0].data);
        //printf("RequestID: " UA_PRINTF_GUID_FORMAT "\n", UA_PRINTF_GUID_DATA(requestId->identifier.guid));
        UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Method call was unsuccessful, and %x returned values available.\n", retval);
    }

    for(size_t i = 0; i < 4; i++)
        UA_Variant_deleteMembers(&input[i]);;

    return retval;
}

UA_StatusCode
UA_GDS_call_finishRequest(UA_Client *client,
                          UA_NodeId *applicationId,
                          UA_NodeId *requestId,
                          UA_ByteString *certificate,
                          UA_ByteString *privateKey,
                          UA_ByteString *issuerCertificate){
    UA_Variant input[2];
    UA_Variant_setScalarCopy(&input[0], applicationId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalarCopy(&input[1], requestId, &UA_TYPES[UA_TYPES_NODEID]);
    size_t outputSize;
    UA_Variant *output;
    UA_StatusCode retval = UA_Client_call(client, UA_NODEID_NUMERIC(2, 141),
                                          UA_NODEID_NUMERIC(2, 163), 2, input, &outputSize, &output);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_ByteString *cert = (UA_ByteString *) output[0].data;
        UA_ByteString_allocBuffer(certificate, cert->length);
        memcpy(certificate->data, cert->data, cert->length);

        UA_ByteString *privKey = (UA_ByteString *) output[1].data;
        if (privKey != NULL) {
            UA_ByteString_allocBuffer(privateKey, privKey->length);
            memcpy(privateKey->data, privKey->data, privKey->length);
        }

        UA_ByteString *issuer = (UA_ByteString *) output[2].data;
        UA_ByteString_allocBuffer(issuerCertificate, issuer->length);
        memcpy(issuerCertificate[0].data, issuer[0].data, issuer->length);

        UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Certificate received");
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Method call was unsuccessful, and %x returned values available.\n", retval);
    }

    for(size_t i = 0; i < 2; i++)
        UA_Variant_deleteMembers(&input[i]);

    return retval;
}

UA_StatusCode
UA_GDS_call_getCertificateGroups(UA_Client *client,
                                        UA_NodeId *applicationId,
                                        size_t *cg_size,
                                        UA_NodeId **certificateGroups) {
    UA_Variant input;
    UA_Variant_setScalarCopy(&input, applicationId, &UA_TYPES[UA_TYPES_NODEID]);
    size_t outputSize;
    UA_Variant *output;
    UA_StatusCode  retval = UA_Client_call(client, UA_NODEID_NUMERIC(2, 141),
                                           UA_NODEID_NUMERIC(2, 508), 1, &input, &outputSize, &output);
    if(retval == UA_STATUSCODE_GOOD) {
        *cg_size = output->arrayLength;
        if (output->arrayLength > 0) {
            *certificateGroups = (UA_NodeId *) UA_calloc (output->arrayLength, sizeof(UA_NodeId));
            memcpy(*certificateGroups, output->data, output->arrayLength * sizeof(UA_NodeId));
        }
        UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Method call was unsuccessful, and %x returned values available.\n", retval);
    }
    UA_Variant_deleteMembers(&input);
    return retval;
}

UA_StatusCode
UA_GDS_call_getTrustList(UA_Client *client,
                                UA_NodeId *applicationId,
                                const UA_NodeId *certificateGroupId,
                                UA_NodeId *trustListId) {

    UA_Variant input[2];
    UA_Variant_setScalarCopy(&input[0], applicationId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalarCopy(&input[1], certificateGroupId, &UA_TYPES[UA_TYPES_NODEID]);
    size_t outputSize;
    UA_Variant *output;
    UA_StatusCode retval = UA_Client_call(client, UA_NODEID_NUMERIC(2, 141),
                                          UA_NODEID_NUMERIC(2, 204), 2, input, &outputSize, &output);
    if(retval == UA_STATUSCODE_GOOD) {
        *trustListId =  *((UA_NodeId*)output[0].data);
        UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Method call was unsuccessful, and %x returned values available.\n", retval);
    }

    for(size_t i = 0; i < 2; i++)
        UA_Variant_deleteMembers(&input[i]);

    return retval;
}

UA_StatusCode
UA_GDS_call_openTrustList(UA_Client *client,
                                 UA_Byte *mode,
                                 UA_UInt32 *fileHandle) {

    UA_Variant input;
    UA_Variant_setScalarCopy(&input, mode, &UA_TYPES[UA_TYPES_BYTE]);
    size_t outputSize;
    UA_Variant *output;
    UA_StatusCode retval = UA_Client_call(client, UA_NODEID_NUMERIC(2, 616),
                                          UA_NODEID_NUMERIC(0, 11580), 1, &input, &outputSize, &output);
    if(retval == UA_STATUSCODE_GOOD) {
        *fileHandle = *(UA_UInt32 *) output->data;
        UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Method call was unsuccessful, and %x returned values available.\n", retval);
    }

    UA_Variant_deleteMembers(&input);

    return retval;
}

UA_StatusCode
UA_GDS_call_readTrustList(UA_Client *client,
                                 UA_UInt32 *fileHandle,
                                 UA_Int32 *length,
                                 UA_TrustListDataType *trustList) {

    UA_Variant input[2];
    UA_Variant_setScalarCopy(&input[0], fileHandle, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalarCopy(&input[1], length, &UA_TYPES[UA_TYPES_INT32]);
    size_t outputSize;
    UA_Variant *output;;
    UA_StatusCode retval = UA_Client_call(client, UA_NODEID_NUMERIC(2, 616),
                                          UA_NODEID_NUMERIC(0, 11585), 2, input, &outputSize, &output);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_TrustListDataType *tmp =  (UA_TrustListDataType *) output->data;
        if (tmp != NULL) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Received TrustList (TrustListSize: %lu, TrustedCRLSize: %lu)",
                        tmp->trustedCertificatesSize, tmp->trustedCrlsSize);

            size_t index = 0;
            trustList->trustedCertificatesSize = tmp->trustedCertificatesSize;
            trustList->trustedCertificates =
                    (UA_ByteString *) UA_malloc (sizeof(UA_ByteString) * tmp->trustedCertificatesSize);
            while (index < tmp->trustedCertificatesSize) {
                UA_ByteString_allocBuffer(&trustList->trustedCertificates[index],
                                          tmp->trustedCertificates[index].length);
                memcpy(trustList->trustedCertificates[index].data,
                       tmp->trustedCertificates[index].data,
                       tmp->trustedCertificates[index].length);
                index++;
            }
            index = 0;
            trustList->trustedCrlsSize = tmp->trustedCrlsSize;
            trustList->trustedCrls =
                    (UA_ByteString *) UA_malloc (sizeof(UA_ByteString) * tmp->trustedCrlsSize);
            while (index < tmp->trustedCrlsSize) {
                UA_ByteString_allocBuffer(&trustList->trustedCrls[index],
                                          tmp->trustedCrls[index].length);
                memcpy(trustList->trustedCrls[index].data,
                       tmp->trustedCrls[index].data,
                       tmp->trustedCrls[index].length);
                index++;
            }
            UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
        }
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Method call was unsuccessful, and %x returned values available.\n", retval);
    }

    for(size_t i = 0; i < 2; i++)
        UA_Variant_deleteMembers(&input[i]);

    return retval;
}

UA_StatusCode
UA_GDS_call_closeTrustList(UA_Client *client,
                           UA_UInt32 *fileHandle) {

    UA_Variant input;
    UA_Variant_setScalarCopy(&input, fileHandle, &UA_TYPES[UA_TYPES_UINT32]);
    UA_StatusCode retval = UA_Client_call(client, UA_NODEID_NUMERIC(2, 616),
                                          UA_NODEID_NUMERIC(0, 11583), 1, &input, NULL, NULL);
    if(retval) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Method call was unsuccessful, and %x returned values available.\n", retval);
    }
    UA_Variant_deleteMembers(&input);
    return retval;
}


static
UA_DataTypeArray *UA_GDS_Client_DataTypeArray;

UA_StatusCode
UA_GDS_Client_deinit(UA_Client *client) {
    client->config.customDataTypes = UA_GDS_Client_DataTypeArray->next;
    UA_free(UA_GDS_Client_DataTypeArray);
    return UA_STATUSCODE_GOOD;
}
UA_StatusCode
UA_GDS_Client_init(UA_Client *client) {
    UA_DataTypeArray init = { client->config.customDataTypes, 1, &ApplicationRecordDataType};
    UA_GDS_Client_DataTypeArray = (UA_DataTypeArray*) UA_malloc(sizeof(UA_DataTypeArray));
    memcpy(UA_GDS_Client_DataTypeArray, &init, sizeof(init));
    client->config.customDataTypes = UA_GDS_Client_DataTypeArray;
    return UA_STATUSCODE_GOOD;
}
#endif
