/* ========================================================================
 * Copyright (c) 2005-2021 The OPC Foundation, Inc. All rights reserved.
 *
 * OPC Foundation MIT License 1.00
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * The complete license agreement can be found here:
 * http://opcfoundation.org/License/MIT/1.00/
 * ======================================================================*/

#ifndef _OpcUa_ServerApi_H_
#define _OpcUa_ServerApi_H_ 1
#ifdef OPCUA_HAVE_SERVERAPI

#include <opcua_types.h>
#include <opcua_endpoint.h>

OPCUA_BEGIN_EXTERN_C

#ifndef OPCUA_EXCLUDE_FindServers
/*============================================================================
 * Synchronously calls the FindServers service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_FindServers(
    OpcUa_Endpoint                 hEndpoint,
    OpcUa_Handle                   hContext,
    const OpcUa_RequestHeader*     pRequestHeader,
    const OpcUa_String*            pEndpointUrl,
    OpcUa_Int32                    nNoOfLocaleIds,
    const OpcUa_String*            pLocaleIds,
    OpcUa_Int32                    nNoOfServerUris,
    const OpcUa_String*            pServerUris,
    OpcUa_ResponseHeader*          pResponseHeader,
    OpcUa_Int32*                   pNoOfServers,
    OpcUa_ApplicationDescription** pServers);

/*============================================================================
 * Begins processing of a FindServers service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginFindServers(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_FindServersOnNetwork
/*============================================================================
 * Synchronously calls the FindServersOnNetwork service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_FindServersOnNetwork(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_UInt32               nStartingRecordId,
    OpcUa_UInt32               nMaxRecordsToReturn,
    OpcUa_Int32                nNoOfServerCapabilityFilter,
    const OpcUa_String*        pServerCapabilityFilter,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_DateTime*            pLastCounterResetTime,
    OpcUa_Int32*               pNoOfServers,
    OpcUa_ServerOnNetwork**    pServers);

/*============================================================================
 * Begins processing of a FindServersOnNetwork service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginFindServersOnNetwork(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_GetEndpoints
/*============================================================================
 * Synchronously calls the GetEndpoints service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_GetEndpoints(
    OpcUa_Endpoint              hEndpoint,
    OpcUa_Handle                hContext,
    const OpcUa_RequestHeader*  pRequestHeader,
    const OpcUa_String*         pEndpointUrl,
    OpcUa_Int32                 nNoOfLocaleIds,
    const OpcUa_String*         pLocaleIds,
    OpcUa_Int32                 nNoOfProfileUris,
    const OpcUa_String*         pProfileUris,
    OpcUa_ResponseHeader*       pResponseHeader,
    OpcUa_Int32*                pNoOfEndpoints,
    OpcUa_EndpointDescription** pEndpoints);

/*============================================================================
 * Begins processing of a GetEndpoints service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginGetEndpoints(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_RegisterServer
/*============================================================================
 * Synchronously calls the RegisterServer service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_RegisterServer(
    OpcUa_Endpoint                hEndpoint,
    OpcUa_Handle                  hContext,
    const OpcUa_RequestHeader*    pRequestHeader,
    const OpcUa_RegisteredServer* pServer,
    OpcUa_ResponseHeader*         pResponseHeader);

/*============================================================================
 * Begins processing of a RegisterServer service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginRegisterServer(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_RegisterServer2
/*============================================================================
 * Synchronously calls the RegisterServer2 service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_RegisterServer2(
    OpcUa_Endpoint                hEndpoint,
    OpcUa_Handle                  hContext,
    const OpcUa_RequestHeader*    pRequestHeader,
    const OpcUa_RegisteredServer* pServer,
    OpcUa_Int32                   nNoOfDiscoveryConfiguration,
    const OpcUa_ExtensionObject*  pDiscoveryConfiguration,
    OpcUa_ResponseHeader*         pResponseHeader,
    OpcUa_Int32*                  pNoOfConfigurationResults,
    OpcUa_StatusCode**            pConfigurationResults,
    OpcUa_Int32*                  pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**        pDiagnosticInfos);

/*============================================================================
 * Begins processing of a RegisterServer2 service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginRegisterServer2(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_CreateSession
/*============================================================================
 * Synchronously calls the CreateSession service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_CreateSession(
    OpcUa_Endpoint                      hEndpoint,
    OpcUa_Handle                        hContext,
    const OpcUa_RequestHeader*          pRequestHeader,
    const OpcUa_ApplicationDescription* pClientDescription,
    const OpcUa_String*                 pServerUri,
    const OpcUa_String*                 pEndpointUrl,
    const OpcUa_String*                 pSessionName,
    const OpcUa_ByteString*             pClientNonce,
    const OpcUa_ByteString*             pClientCertificate,
    OpcUa_Double                        nRequestedSessionTimeout,
    OpcUa_UInt32                        nMaxResponseMessageSize,
    OpcUa_ResponseHeader*               pResponseHeader,
    OpcUa_NodeId*                       pSessionId,
    OpcUa_NodeId*                       pAuthenticationToken,
    OpcUa_Double*                       pRevisedSessionTimeout,
    OpcUa_ByteString*                   pServerNonce,
    OpcUa_ByteString*                   pServerCertificate,
    OpcUa_Int32*                        pNoOfServerEndpoints,
    OpcUa_EndpointDescription**         pServerEndpoints,
    OpcUa_Int32*                        pNoOfServerSoftwareCertificates,
    OpcUa_SignedSoftwareCertificate**   pServerSoftwareCertificates,
    OpcUa_SignatureData*                pServerSignature,
    OpcUa_UInt32*                       pMaxRequestMessageSize);

/*============================================================================
 * Begins processing of a CreateSession service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginCreateSession(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_ActivateSession
/*============================================================================
 * Synchronously calls the ActivateSession service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_ActivateSession(
    OpcUa_Endpoint                         hEndpoint,
    OpcUa_Handle                           hContext,
    const OpcUa_RequestHeader*             pRequestHeader,
    const OpcUa_SignatureData*             pClientSignature,
    OpcUa_Int32                            nNoOfClientSoftwareCertificates,
    const OpcUa_SignedSoftwareCertificate* pClientSoftwareCertificates,
    OpcUa_Int32                            nNoOfLocaleIds,
    const OpcUa_String*                    pLocaleIds,
    const OpcUa_ExtensionObject*           pUserIdentityToken,
    const OpcUa_SignatureData*             pUserTokenSignature,
    OpcUa_ResponseHeader*                  pResponseHeader,
    OpcUa_ByteString*                      pServerNonce,
    OpcUa_Int32*                           pNoOfResults,
    OpcUa_StatusCode**                     pResults,
    OpcUa_Int32*                           pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**                 pDiagnosticInfos);

/*============================================================================
 * Begins processing of a ActivateSession service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginActivateSession(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_CloseSession
/*============================================================================
 * Synchronously calls the CloseSession service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_CloseSession(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Boolean              bDeleteSubscriptions,
    OpcUa_ResponseHeader*      pResponseHeader);

/*============================================================================
 * Begins processing of a CloseSession service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginCloseSession(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_Cancel
/*============================================================================
 * Synchronously calls the Cancel service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_Cancel(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_UInt32               nRequestHandle,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_UInt32*              pCancelCount);

/*============================================================================
 * Begins processing of a Cancel service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginCancel(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_AddNodes
/*============================================================================
 * Synchronously calls the AddNodes service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_AddNodes(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Int32                nNoOfNodesToAdd,
    const OpcUa_AddNodesItem*  pNodesToAdd,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_Int32*               pNoOfResults,
    OpcUa_AddNodesResult**     pResults,
    OpcUa_Int32*               pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     pDiagnosticInfos);

/*============================================================================
 * Begins processing of a AddNodes service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginAddNodes(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_AddReferences
/*============================================================================
 * Synchronously calls the AddReferences service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_AddReferences(
    OpcUa_Endpoint                 hEndpoint,
    OpcUa_Handle                   hContext,
    const OpcUa_RequestHeader*     pRequestHeader,
    OpcUa_Int32                    nNoOfReferencesToAdd,
    const OpcUa_AddReferencesItem* pReferencesToAdd,
    OpcUa_ResponseHeader*          pResponseHeader,
    OpcUa_Int32*                   pNoOfResults,
    OpcUa_StatusCode**             pResults,
    OpcUa_Int32*                   pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**         pDiagnosticInfos);

/*============================================================================
 * Begins processing of a AddReferences service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginAddReferences(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_DeleteNodes
/*============================================================================
 * Synchronously calls the DeleteNodes service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_DeleteNodes(
    OpcUa_Endpoint               hEndpoint,
    OpcUa_Handle                 hContext,
    const OpcUa_RequestHeader*   pRequestHeader,
    OpcUa_Int32                  nNoOfNodesToDelete,
    const OpcUa_DeleteNodesItem* pNodesToDelete,
    OpcUa_ResponseHeader*        pResponseHeader,
    OpcUa_Int32*                 pNoOfResults,
    OpcUa_StatusCode**           pResults,
    OpcUa_Int32*                 pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**       pDiagnosticInfos);

/*============================================================================
 * Begins processing of a DeleteNodes service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginDeleteNodes(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_DeleteReferences
/*============================================================================
 * Synchronously calls the DeleteReferences service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_DeleteReferences(
    OpcUa_Endpoint                    hEndpoint,
    OpcUa_Handle                      hContext,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_Int32                       nNoOfReferencesToDelete,
    const OpcUa_DeleteReferencesItem* pReferencesToDelete,
    OpcUa_ResponseHeader*             pResponseHeader,
    OpcUa_Int32*                      pNoOfResults,
    OpcUa_StatusCode**                pResults,
    OpcUa_Int32*                      pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**            pDiagnosticInfos);

/*============================================================================
 * Begins processing of a DeleteReferences service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginDeleteReferences(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_Browse
/*============================================================================
 * Synchronously calls the Browse service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_Browse(
    OpcUa_Endpoint                 hEndpoint,
    OpcUa_Handle                   hContext,
    const OpcUa_RequestHeader*     pRequestHeader,
    const OpcUa_ViewDescription*   pView,
    OpcUa_UInt32                   nRequestedMaxReferencesPerNode,
    OpcUa_Int32                    nNoOfNodesToBrowse,
    const OpcUa_BrowseDescription* pNodesToBrowse,
    OpcUa_ResponseHeader*          pResponseHeader,
    OpcUa_Int32*                   pNoOfResults,
    OpcUa_BrowseResult**           pResults,
    OpcUa_Int32*                   pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**         pDiagnosticInfos);

/*============================================================================
 * Begins processing of a Browse service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginBrowse(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_BrowseNext
/*============================================================================
 * Synchronously calls the BrowseNext service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_BrowseNext(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Boolean              bReleaseContinuationPoints,
    OpcUa_Int32                nNoOfContinuationPoints,
    const OpcUa_ByteString*    pContinuationPoints,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_Int32*               pNoOfResults,
    OpcUa_BrowseResult**       pResults,
    OpcUa_Int32*               pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     pDiagnosticInfos);

/*============================================================================
 * Begins processing of a BrowseNext service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginBrowseNext(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_TranslateBrowsePathsToNodeIds
/*============================================================================
 * Synchronously calls the TranslateBrowsePathsToNodeIds service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_TranslateBrowsePathsToNodeIds(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Int32                nNoOfBrowsePaths,
    const OpcUa_BrowsePath*    pBrowsePaths,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_Int32*               pNoOfResults,
    OpcUa_BrowsePathResult**   pResults,
    OpcUa_Int32*               pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     pDiagnosticInfos);

/*============================================================================
 * Begins processing of a TranslateBrowsePathsToNodeIds service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginTranslateBrowsePathsToNodeIds(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_RegisterNodes
/*============================================================================
 * Synchronously calls the RegisterNodes service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_RegisterNodes(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Int32                nNoOfNodesToRegister,
    const OpcUa_NodeId*        pNodesToRegister,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_Int32*               pNoOfRegisteredNodeIds,
    OpcUa_NodeId**             pRegisteredNodeIds);

/*============================================================================
 * Begins processing of a RegisterNodes service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginRegisterNodes(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_UnregisterNodes
/*============================================================================
 * Synchronously calls the UnregisterNodes service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_UnregisterNodes(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Int32                nNoOfNodesToUnregister,
    const OpcUa_NodeId*        pNodesToUnregister,
    OpcUa_ResponseHeader*      pResponseHeader);

/*============================================================================
 * Begins processing of a UnregisterNodes service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginUnregisterNodes(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_QueryFirst
/*============================================================================
 * Synchronously calls the QueryFirst service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_QueryFirst(
    OpcUa_Endpoint                   hEndpoint,
    OpcUa_Handle                     hContext,
    const OpcUa_RequestHeader*       pRequestHeader,
    const OpcUa_ViewDescription*     pView,
    OpcUa_Int32                      nNoOfNodeTypes,
    const OpcUa_NodeTypeDescription* pNodeTypes,
    const OpcUa_ContentFilter*       pFilter,
    OpcUa_UInt32                     nMaxDataSetsToReturn,
    OpcUa_UInt32                     nMaxReferencesToReturn,
    OpcUa_ResponseHeader*            pResponseHeader,
    OpcUa_Int32*                     pNoOfQueryDataSets,
    OpcUa_QueryDataSet**             pQueryDataSets,
    OpcUa_ByteString*                pContinuationPoint,
    OpcUa_Int32*                     pNoOfParsingResults,
    OpcUa_ParsingResult**            pParsingResults,
    OpcUa_Int32*                     pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**           pDiagnosticInfos,
    OpcUa_ContentFilterResult*       pFilterResult);

/*============================================================================
 * Begins processing of a QueryFirst service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginQueryFirst(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_QueryNext
/*============================================================================
 * Synchronously calls the QueryNext service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_QueryNext(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Boolean              bReleaseContinuationPoint,
    const OpcUa_ByteString*    pContinuationPoint,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_Int32*               pNoOfQueryDataSets,
    OpcUa_QueryDataSet**       pQueryDataSets,
    OpcUa_ByteString*          pRevisedContinuationPoint);

/*============================================================================
 * Begins processing of a QueryNext service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginQueryNext(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_Read
/*============================================================================
 * Synchronously calls the Read service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_Read(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Double               nMaxAge,
    OpcUa_TimestampsToReturn   eTimestampsToReturn,
    OpcUa_Int32                nNoOfNodesToRead,
    const OpcUa_ReadValueId*   pNodesToRead,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_Int32*               pNoOfResults,
    OpcUa_DataValue**          pResults,
    OpcUa_Int32*               pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     pDiagnosticInfos);

/*============================================================================
 * Begins processing of a Read service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginRead(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_HistoryRead
/*============================================================================
 * Synchronously calls the HistoryRead service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_HistoryRead(
    OpcUa_Endpoint                  hEndpoint,
    OpcUa_Handle                    hContext,
    const OpcUa_RequestHeader*      pRequestHeader,
    const OpcUa_ExtensionObject*    pHistoryReadDetails,
    OpcUa_TimestampsToReturn        eTimestampsToReturn,
    OpcUa_Boolean                   bReleaseContinuationPoints,
    OpcUa_Int32                     nNoOfNodesToRead,
    const OpcUa_HistoryReadValueId* pNodesToRead,
    OpcUa_ResponseHeader*           pResponseHeader,
    OpcUa_Int32*                    pNoOfResults,
    OpcUa_HistoryReadResult**       pResults,
    OpcUa_Int32*                    pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**          pDiagnosticInfos);

/*============================================================================
 * Begins processing of a HistoryRead service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginHistoryRead(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_Write
/*============================================================================
 * Synchronously calls the Write service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_Write(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Int32                nNoOfNodesToWrite,
    const OpcUa_WriteValue*    pNodesToWrite,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_Int32*               pNoOfResults,
    OpcUa_StatusCode**         pResults,
    OpcUa_Int32*               pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     pDiagnosticInfos);

/*============================================================================
 * Begins processing of a Write service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginWrite(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_HistoryUpdate
/*============================================================================
 * Synchronously calls the HistoryUpdate service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_HistoryUpdate(
    OpcUa_Endpoint               hEndpoint,
    OpcUa_Handle                 hContext,
    const OpcUa_RequestHeader*   pRequestHeader,
    OpcUa_Int32                  nNoOfHistoryUpdateDetails,
    const OpcUa_ExtensionObject* pHistoryUpdateDetails,
    OpcUa_ResponseHeader*        pResponseHeader,
    OpcUa_Int32*                 pNoOfResults,
    OpcUa_HistoryUpdateResult**  pResults,
    OpcUa_Int32*                 pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**       pDiagnosticInfos);

/*============================================================================
 * Begins processing of a HistoryUpdate service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginHistoryUpdate(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_Call
/*============================================================================
 * Synchronously calls the Call service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_Call(
    OpcUa_Endpoint                 hEndpoint,
    OpcUa_Handle                   hContext,
    const OpcUa_RequestHeader*     pRequestHeader,
    OpcUa_Int32                    nNoOfMethodsToCall,
    const OpcUa_CallMethodRequest* pMethodsToCall,
    OpcUa_ResponseHeader*          pResponseHeader,
    OpcUa_Int32*                   pNoOfResults,
    OpcUa_CallMethodResult**       pResults,
    OpcUa_Int32*                   pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**         pDiagnosticInfos);

/*============================================================================
 * Begins processing of a Call service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginCall(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_CreateMonitoredItems
/*============================================================================
 * Synchronously calls the CreateMonitoredItems service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_CreateMonitoredItems(
    OpcUa_Endpoint                          hEndpoint,
    OpcUa_Handle                            hContext,
    const OpcUa_RequestHeader*              pRequestHeader,
    OpcUa_UInt32                            nSubscriptionId,
    OpcUa_TimestampsToReturn                eTimestampsToReturn,
    OpcUa_Int32                             nNoOfItemsToCreate,
    const OpcUa_MonitoredItemCreateRequest* pItemsToCreate,
    OpcUa_ResponseHeader*                   pResponseHeader,
    OpcUa_Int32*                            pNoOfResults,
    OpcUa_MonitoredItemCreateResult**       pResults,
    OpcUa_Int32*                            pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**                  pDiagnosticInfos);

/*============================================================================
 * Begins processing of a CreateMonitoredItems service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginCreateMonitoredItems(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_ModifyMonitoredItems
/*============================================================================
 * Synchronously calls the ModifyMonitoredItems service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_ModifyMonitoredItems(
    OpcUa_Endpoint                          hEndpoint,
    OpcUa_Handle                            hContext,
    const OpcUa_RequestHeader*              pRequestHeader,
    OpcUa_UInt32                            nSubscriptionId,
    OpcUa_TimestampsToReturn                eTimestampsToReturn,
    OpcUa_Int32                             nNoOfItemsToModify,
    const OpcUa_MonitoredItemModifyRequest* pItemsToModify,
    OpcUa_ResponseHeader*                   pResponseHeader,
    OpcUa_Int32*                            pNoOfResults,
    OpcUa_MonitoredItemModifyResult**       pResults,
    OpcUa_Int32*                            pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**                  pDiagnosticInfos);

/*============================================================================
 * Begins processing of a ModifyMonitoredItems service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginModifyMonitoredItems(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_SetMonitoringMode
/*============================================================================
 * Synchronously calls the SetMonitoringMode service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_SetMonitoringMode(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_UInt32               nSubscriptionId,
    OpcUa_MonitoringMode       eMonitoringMode,
    OpcUa_Int32                nNoOfMonitoredItemIds,
    const OpcUa_UInt32*        pMonitoredItemIds,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_Int32*               pNoOfResults,
    OpcUa_StatusCode**         pResults,
    OpcUa_Int32*               pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     pDiagnosticInfos);

/*============================================================================
 * Begins processing of a SetMonitoringMode service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginSetMonitoringMode(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_SetTriggering
/*============================================================================
 * Synchronously calls the SetTriggering service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_SetTriggering(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_UInt32               nSubscriptionId,
    OpcUa_UInt32               nTriggeringItemId,
    OpcUa_Int32                nNoOfLinksToAdd,
    const OpcUa_UInt32*        pLinksToAdd,
    OpcUa_Int32                nNoOfLinksToRemove,
    const OpcUa_UInt32*        pLinksToRemove,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_Int32*               pNoOfAddResults,
    OpcUa_StatusCode**         pAddResults,
    OpcUa_Int32*               pNoOfAddDiagnosticInfos,
    OpcUa_DiagnosticInfo**     pAddDiagnosticInfos,
    OpcUa_Int32*               pNoOfRemoveResults,
    OpcUa_StatusCode**         pRemoveResults,
    OpcUa_Int32*               pNoOfRemoveDiagnosticInfos,
    OpcUa_DiagnosticInfo**     pRemoveDiagnosticInfos);

/*============================================================================
 * Begins processing of a SetTriggering service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginSetTriggering(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_DeleteMonitoredItems
/*============================================================================
 * Synchronously calls the DeleteMonitoredItems service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_DeleteMonitoredItems(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_UInt32               nSubscriptionId,
    OpcUa_Int32                nNoOfMonitoredItemIds,
    const OpcUa_UInt32*        pMonitoredItemIds,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_Int32*               pNoOfResults,
    OpcUa_StatusCode**         pResults,
    OpcUa_Int32*               pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     pDiagnosticInfos);

/*============================================================================
 * Begins processing of a DeleteMonitoredItems service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginDeleteMonitoredItems(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_CreateSubscription
/*============================================================================
 * Synchronously calls the CreateSubscription service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_CreateSubscription(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Double               nRequestedPublishingInterval,
    OpcUa_UInt32               nRequestedLifetimeCount,
    OpcUa_UInt32               nRequestedMaxKeepAliveCount,
    OpcUa_UInt32               nMaxNotificationsPerPublish,
    OpcUa_Boolean              bPublishingEnabled,
    OpcUa_Byte                 nPriority,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_UInt32*              pSubscriptionId,
    OpcUa_Double*              pRevisedPublishingInterval,
    OpcUa_UInt32*              pRevisedLifetimeCount,
    OpcUa_UInt32*              pRevisedMaxKeepAliveCount);

/*============================================================================
 * Begins processing of a CreateSubscription service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginCreateSubscription(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_ModifySubscription
/*============================================================================
 * Synchronously calls the ModifySubscription service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_ModifySubscription(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_UInt32               nSubscriptionId,
    OpcUa_Double               nRequestedPublishingInterval,
    OpcUa_UInt32               nRequestedLifetimeCount,
    OpcUa_UInt32               nRequestedMaxKeepAliveCount,
    OpcUa_UInt32               nMaxNotificationsPerPublish,
    OpcUa_Byte                 nPriority,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_Double*              pRevisedPublishingInterval,
    OpcUa_UInt32*              pRevisedLifetimeCount,
    OpcUa_UInt32*              pRevisedMaxKeepAliveCount);

/*============================================================================
 * Begins processing of a ModifySubscription service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginModifySubscription(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_SetPublishingMode
/*============================================================================
 * Synchronously calls the SetPublishingMode service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_SetPublishingMode(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Boolean              bPublishingEnabled,
    OpcUa_Int32                nNoOfSubscriptionIds,
    const OpcUa_UInt32*        pSubscriptionIds,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_Int32*               pNoOfResults,
    OpcUa_StatusCode**         pResults,
    OpcUa_Int32*               pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     pDiagnosticInfos);

/*============================================================================
 * Begins processing of a SetPublishingMode service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginSetPublishingMode(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_Publish
/*============================================================================
 * Synchronously calls the Publish service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_Publish(
    OpcUa_Endpoint                           hEndpoint,
    OpcUa_Handle                             hContext,
    const OpcUa_RequestHeader*               pRequestHeader,
    OpcUa_Int32                              nNoOfSubscriptionAcknowledgements,
    const OpcUa_SubscriptionAcknowledgement* pSubscriptionAcknowledgements,
    OpcUa_ResponseHeader*                    pResponseHeader,
    OpcUa_UInt32*                            pSubscriptionId,
    OpcUa_Int32*                             pNoOfAvailableSequenceNumbers,
    OpcUa_UInt32**                           pAvailableSequenceNumbers,
    OpcUa_Boolean*                           pMoreNotifications,
    OpcUa_NotificationMessage*               pNotificationMessage,
    OpcUa_Int32*                             pNoOfResults,
    OpcUa_StatusCode**                       pResults,
    OpcUa_Int32*                             pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**                   pDiagnosticInfos);

/*============================================================================
 * Begins processing of a Publish service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginPublish(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_Republish
/*============================================================================
 * Synchronously calls the Republish service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_Republish(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_UInt32               nSubscriptionId,
    OpcUa_UInt32               nRetransmitSequenceNumber,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_NotificationMessage* pNotificationMessage);

/*============================================================================
 * Begins processing of a Republish service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginRepublish(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_TransferSubscriptions
/*============================================================================
 * Synchronously calls the TransferSubscriptions service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_TransferSubscriptions(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Int32                nNoOfSubscriptionIds,
    const OpcUa_UInt32*        pSubscriptionIds,
    OpcUa_Boolean              bSendInitialValues,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_Int32*               pNoOfResults,
    OpcUa_TransferResult**     pResults,
    OpcUa_Int32*               pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     pDiagnosticInfos);

/*============================================================================
 * Begins processing of a TransferSubscriptions service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginTransferSubscriptions(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

#ifndef OPCUA_EXCLUDE_DeleteSubscriptions
/*============================================================================
 * Synchronously calls the DeleteSubscriptions service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_DeleteSubscriptions(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Int32                nNoOfSubscriptionIds,
    const OpcUa_UInt32*        pSubscriptionIds,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_Int32*               pNoOfResults,
    OpcUa_StatusCode**         pResults,
    OpcUa_Int32*               pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     pDiagnosticInfos);

/*============================================================================
 * Begins processing of a DeleteSubscriptions service request.
 *===========================================================================*/
OPCUA_EXPORT_SYNC_SERVER_API OpcUa_StatusCode OpcUa_Server_BeginDeleteSubscriptions(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType);
#endif

OPCUA_END_EXTERN_C

#endif /* OPCUA_HAVE_SERVERAPI */
#endif /* _OpcUa_ServerApi_H_ */
/* This is the last line of an autogenerated file. */
