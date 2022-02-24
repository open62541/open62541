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

#ifndef _OpcUa_ClientApi_H_
#define _OpcUa_ClientApi_H_ 1
#ifdef OPCUA_HAVE_CLIENTAPI

#include <opcua_types.h>
#include <opcua_channel.h>

OPCUA_BEGIN_EXTERN_C

#ifndef OPCUA_EXCLUDE_FindServers
/*============================================================================
 * Synchronously calls the FindServers service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_FindServers(
    OpcUa_Channel                  hChannel,
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
 * Asynchronously calls the FindServers service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginFindServers(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    const OpcUa_String*               pEndpointUrl,
    OpcUa_Int32                       nNoOfLocaleIds,
    const OpcUa_String*               pLocaleIds,
    OpcUa_Int32                       nNoOfServerUris,
    const OpcUa_String*               pServerUris,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_FindServersOnNetwork
/*============================================================================
 * Synchronously calls the FindServersOnNetwork service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_FindServersOnNetwork(
    OpcUa_Channel              hChannel,
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
 * Asynchronously calls the FindServersOnNetwork service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginFindServersOnNetwork(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_UInt32                      nStartingRecordId,
    OpcUa_UInt32                      nMaxRecordsToReturn,
    OpcUa_Int32                       nNoOfServerCapabilityFilter,
    const OpcUa_String*               pServerCapabilityFilter,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_GetEndpoints
/*============================================================================
 * Synchronously calls the GetEndpoints service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_GetEndpoints(
    OpcUa_Channel               hChannel,
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
 * Asynchronously calls the GetEndpoints service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginGetEndpoints(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    const OpcUa_String*               pEndpointUrl,
    OpcUa_Int32                       nNoOfLocaleIds,
    const OpcUa_String*               pLocaleIds,
    OpcUa_Int32                       nNoOfProfileUris,
    const OpcUa_String*               pProfileUris,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_RegisterServer
/*============================================================================
 * Synchronously calls the RegisterServer service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_RegisterServer(
    OpcUa_Channel                 hChannel,
    const OpcUa_RequestHeader*    pRequestHeader,
    const OpcUa_RegisteredServer* pServer,
    OpcUa_ResponseHeader*         pResponseHeader);

/*============================================================================
 * Asynchronously calls the RegisterServer service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginRegisterServer(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    const OpcUa_RegisteredServer*     pServer,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_RegisterServer2
/*============================================================================
 * Synchronously calls the RegisterServer2 service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_RegisterServer2(
    OpcUa_Channel                 hChannel,
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
 * Asynchronously calls the RegisterServer2 service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginRegisterServer2(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    const OpcUa_RegisteredServer*     pServer,
    OpcUa_Int32                       nNoOfDiscoveryConfiguration,
    const OpcUa_ExtensionObject*      pDiscoveryConfiguration,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_CreateSession
/*============================================================================
 * Synchronously calls the CreateSession service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_CreateSession(
    OpcUa_Channel                       hChannel,
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
 * Asynchronously calls the CreateSession service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginCreateSession(
    OpcUa_Channel                       hChannel,
    const OpcUa_RequestHeader*          pRequestHeader,
    const OpcUa_ApplicationDescription* pClientDescription,
    const OpcUa_String*                 pServerUri,
    const OpcUa_String*                 pEndpointUrl,
    const OpcUa_String*                 pSessionName,
    const OpcUa_ByteString*             pClientNonce,
    const OpcUa_ByteString*             pClientCertificate,
    OpcUa_Double                        nRequestedSessionTimeout,
    OpcUa_UInt32                        nMaxResponseMessageSize,
    OpcUa_Channel_PfnRequestComplete*   pCallback,
    OpcUa_Void*                         pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_ActivateSession
/*============================================================================
 * Synchronously calls the ActivateSession service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_ActivateSession(
    OpcUa_Channel                          hChannel,
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
 * Asynchronously calls the ActivateSession service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginActivateSession(
    OpcUa_Channel                          hChannel,
    const OpcUa_RequestHeader*             pRequestHeader,
    const OpcUa_SignatureData*             pClientSignature,
    OpcUa_Int32                            nNoOfClientSoftwareCertificates,
    const OpcUa_SignedSoftwareCertificate* pClientSoftwareCertificates,
    OpcUa_Int32                            nNoOfLocaleIds,
    const OpcUa_String*                    pLocaleIds,
    const OpcUa_ExtensionObject*           pUserIdentityToken,
    const OpcUa_SignatureData*             pUserTokenSignature,
    OpcUa_Channel_PfnRequestComplete*      pCallback,
    OpcUa_Void*                            pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_CloseSession
/*============================================================================
 * Synchronously calls the CloseSession service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_CloseSession(
    OpcUa_Channel              hChannel,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Boolean              bDeleteSubscriptions,
    OpcUa_ResponseHeader*      pResponseHeader);

/*============================================================================
 * Asynchronously calls the CloseSession service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginCloseSession(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_Boolean                     bDeleteSubscriptions,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_Cancel
/*============================================================================
 * Synchronously calls the Cancel service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_Cancel(
    OpcUa_Channel              hChannel,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_UInt32               nRequestHandle,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_UInt32*              pCancelCount);

/*============================================================================
 * Asynchronously calls the Cancel service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginCancel(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_UInt32                      nRequestHandle,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_AddNodes
/*============================================================================
 * Synchronously calls the AddNodes service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_AddNodes(
    OpcUa_Channel              hChannel,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Int32                nNoOfNodesToAdd,
    const OpcUa_AddNodesItem*  pNodesToAdd,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_Int32*               pNoOfResults,
    OpcUa_AddNodesResult**     pResults,
    OpcUa_Int32*               pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     pDiagnosticInfos);

/*============================================================================
 * Asynchronously calls the AddNodes service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginAddNodes(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_Int32                       nNoOfNodesToAdd,
    const OpcUa_AddNodesItem*         pNodesToAdd,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_AddReferences
/*============================================================================
 * Synchronously calls the AddReferences service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_AddReferences(
    OpcUa_Channel                  hChannel,
    const OpcUa_RequestHeader*     pRequestHeader,
    OpcUa_Int32                    nNoOfReferencesToAdd,
    const OpcUa_AddReferencesItem* pReferencesToAdd,
    OpcUa_ResponseHeader*          pResponseHeader,
    OpcUa_Int32*                   pNoOfResults,
    OpcUa_StatusCode**             pResults,
    OpcUa_Int32*                   pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**         pDiagnosticInfos);

/*============================================================================
 * Asynchronously calls the AddReferences service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginAddReferences(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_Int32                       nNoOfReferencesToAdd,
    const OpcUa_AddReferencesItem*    pReferencesToAdd,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_DeleteNodes
/*============================================================================
 * Synchronously calls the DeleteNodes service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_DeleteNodes(
    OpcUa_Channel                hChannel,
    const OpcUa_RequestHeader*   pRequestHeader,
    OpcUa_Int32                  nNoOfNodesToDelete,
    const OpcUa_DeleteNodesItem* pNodesToDelete,
    OpcUa_ResponseHeader*        pResponseHeader,
    OpcUa_Int32*                 pNoOfResults,
    OpcUa_StatusCode**           pResults,
    OpcUa_Int32*                 pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**       pDiagnosticInfos);

/*============================================================================
 * Asynchronously calls the DeleteNodes service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginDeleteNodes(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_Int32                       nNoOfNodesToDelete,
    const OpcUa_DeleteNodesItem*      pNodesToDelete,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_DeleteReferences
/*============================================================================
 * Synchronously calls the DeleteReferences service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_DeleteReferences(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_Int32                       nNoOfReferencesToDelete,
    const OpcUa_DeleteReferencesItem* pReferencesToDelete,
    OpcUa_ResponseHeader*             pResponseHeader,
    OpcUa_Int32*                      pNoOfResults,
    OpcUa_StatusCode**                pResults,
    OpcUa_Int32*                      pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**            pDiagnosticInfos);

/*============================================================================
 * Asynchronously calls the DeleteReferences service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginDeleteReferences(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_Int32                       nNoOfReferencesToDelete,
    const OpcUa_DeleteReferencesItem* pReferencesToDelete,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_Browse
/*============================================================================
 * Synchronously calls the Browse service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_Browse(
    OpcUa_Channel                  hChannel,
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
 * Asynchronously calls the Browse service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginBrowse(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    const OpcUa_ViewDescription*      pView,
    OpcUa_UInt32                      nRequestedMaxReferencesPerNode,
    OpcUa_Int32                       nNoOfNodesToBrowse,
    const OpcUa_BrowseDescription*    pNodesToBrowse,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_BrowseNext
/*============================================================================
 * Synchronously calls the BrowseNext service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BrowseNext(
    OpcUa_Channel              hChannel,
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
 * Asynchronously calls the BrowseNext service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginBrowseNext(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_Boolean                     bReleaseContinuationPoints,
    OpcUa_Int32                       nNoOfContinuationPoints,
    const OpcUa_ByteString*           pContinuationPoints,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_TranslateBrowsePathsToNodeIds
/*============================================================================
 * Synchronously calls the TranslateBrowsePathsToNodeIds service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_TranslateBrowsePathsToNodeIds(
    OpcUa_Channel              hChannel,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Int32                nNoOfBrowsePaths,
    const OpcUa_BrowsePath*    pBrowsePaths,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_Int32*               pNoOfResults,
    OpcUa_BrowsePathResult**   pResults,
    OpcUa_Int32*               pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     pDiagnosticInfos);

/*============================================================================
 * Asynchronously calls the TranslateBrowsePathsToNodeIds service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginTranslateBrowsePathsToNodeIds(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_Int32                       nNoOfBrowsePaths,
    const OpcUa_BrowsePath*           pBrowsePaths,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_RegisterNodes
/*============================================================================
 * Synchronously calls the RegisterNodes service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_RegisterNodes(
    OpcUa_Channel              hChannel,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Int32                nNoOfNodesToRegister,
    const OpcUa_NodeId*        pNodesToRegister,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_Int32*               pNoOfRegisteredNodeIds,
    OpcUa_NodeId**             pRegisteredNodeIds);

/*============================================================================
 * Asynchronously calls the RegisterNodes service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginRegisterNodes(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_Int32                       nNoOfNodesToRegister,
    const OpcUa_NodeId*               pNodesToRegister,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_UnregisterNodes
/*============================================================================
 * Synchronously calls the UnregisterNodes service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_UnregisterNodes(
    OpcUa_Channel              hChannel,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Int32                nNoOfNodesToUnregister,
    const OpcUa_NodeId*        pNodesToUnregister,
    OpcUa_ResponseHeader*      pResponseHeader);

/*============================================================================
 * Asynchronously calls the UnregisterNodes service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginUnregisterNodes(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_Int32                       nNoOfNodesToUnregister,
    const OpcUa_NodeId*               pNodesToUnregister,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_QueryFirst
/*============================================================================
 * Synchronously calls the QueryFirst service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_QueryFirst(
    OpcUa_Channel                    hChannel,
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
 * Asynchronously calls the QueryFirst service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginQueryFirst(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    const OpcUa_ViewDescription*      pView,
    OpcUa_Int32                       nNoOfNodeTypes,
    const OpcUa_NodeTypeDescription*  pNodeTypes,
    const OpcUa_ContentFilter*        pFilter,
    OpcUa_UInt32                      nMaxDataSetsToReturn,
    OpcUa_UInt32                      nMaxReferencesToReturn,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_QueryNext
/*============================================================================
 * Synchronously calls the QueryNext service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_QueryNext(
    OpcUa_Channel              hChannel,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Boolean              bReleaseContinuationPoint,
    const OpcUa_ByteString*    pContinuationPoint,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_Int32*               pNoOfQueryDataSets,
    OpcUa_QueryDataSet**       pQueryDataSets,
    OpcUa_ByteString*          pRevisedContinuationPoint);

/*============================================================================
 * Asynchronously calls the QueryNext service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginQueryNext(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_Boolean                     bReleaseContinuationPoint,
    const OpcUa_ByteString*           pContinuationPoint,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_Read
/*============================================================================
 * Synchronously calls the Read service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_Read(
    OpcUa_Channel              hChannel,
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
 * Asynchronously calls the Read service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginRead(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_Double                      nMaxAge,
    OpcUa_TimestampsToReturn          eTimestampsToReturn,
    OpcUa_Int32                       nNoOfNodesToRead,
    const OpcUa_ReadValueId*          pNodesToRead,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_HistoryRead
/*============================================================================
 * Synchronously calls the HistoryRead service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_HistoryRead(
    OpcUa_Channel                   hChannel,
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
 * Asynchronously calls the HistoryRead service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginHistoryRead(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    const OpcUa_ExtensionObject*      pHistoryReadDetails,
    OpcUa_TimestampsToReturn          eTimestampsToReturn,
    OpcUa_Boolean                     bReleaseContinuationPoints,
    OpcUa_Int32                       nNoOfNodesToRead,
    const OpcUa_HistoryReadValueId*   pNodesToRead,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_Write
/*============================================================================
 * Synchronously calls the Write service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_Write(
    OpcUa_Channel              hChannel,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Int32                nNoOfNodesToWrite,
    const OpcUa_WriteValue*    pNodesToWrite,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_Int32*               pNoOfResults,
    OpcUa_StatusCode**         pResults,
    OpcUa_Int32*               pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     pDiagnosticInfos);

/*============================================================================
 * Asynchronously calls the Write service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginWrite(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_Int32                       nNoOfNodesToWrite,
    const OpcUa_WriteValue*           pNodesToWrite,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_HistoryUpdate
/*============================================================================
 * Synchronously calls the HistoryUpdate service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_HistoryUpdate(
    OpcUa_Channel                hChannel,
    const OpcUa_RequestHeader*   pRequestHeader,
    OpcUa_Int32                  nNoOfHistoryUpdateDetails,
    const OpcUa_ExtensionObject* pHistoryUpdateDetails,
    OpcUa_ResponseHeader*        pResponseHeader,
    OpcUa_Int32*                 pNoOfResults,
    OpcUa_HistoryUpdateResult**  pResults,
    OpcUa_Int32*                 pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**       pDiagnosticInfos);

/*============================================================================
 * Asynchronously calls the HistoryUpdate service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginHistoryUpdate(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_Int32                       nNoOfHistoryUpdateDetails,
    const OpcUa_ExtensionObject*      pHistoryUpdateDetails,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_Call
/*============================================================================
 * Synchronously calls the Call service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_Call(
    OpcUa_Channel                  hChannel,
    const OpcUa_RequestHeader*     pRequestHeader,
    OpcUa_Int32                    nNoOfMethodsToCall,
    const OpcUa_CallMethodRequest* pMethodsToCall,
    OpcUa_ResponseHeader*          pResponseHeader,
    OpcUa_Int32*                   pNoOfResults,
    OpcUa_CallMethodResult**       pResults,
    OpcUa_Int32*                   pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**         pDiagnosticInfos);

/*============================================================================
 * Asynchronously calls the Call service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginCall(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_Int32                       nNoOfMethodsToCall,
    const OpcUa_CallMethodRequest*    pMethodsToCall,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_CreateMonitoredItems
/*============================================================================
 * Synchronously calls the CreateMonitoredItems service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_CreateMonitoredItems(
    OpcUa_Channel                           hChannel,
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
 * Asynchronously calls the CreateMonitoredItems service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginCreateMonitoredItems(
    OpcUa_Channel                           hChannel,
    const OpcUa_RequestHeader*              pRequestHeader,
    OpcUa_UInt32                            nSubscriptionId,
    OpcUa_TimestampsToReturn                eTimestampsToReturn,
    OpcUa_Int32                             nNoOfItemsToCreate,
    const OpcUa_MonitoredItemCreateRequest* pItemsToCreate,
    OpcUa_Channel_PfnRequestComplete*       pCallback,
    OpcUa_Void*                             pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_ModifyMonitoredItems
/*============================================================================
 * Synchronously calls the ModifyMonitoredItems service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_ModifyMonitoredItems(
    OpcUa_Channel                           hChannel,
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
 * Asynchronously calls the ModifyMonitoredItems service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginModifyMonitoredItems(
    OpcUa_Channel                           hChannel,
    const OpcUa_RequestHeader*              pRequestHeader,
    OpcUa_UInt32                            nSubscriptionId,
    OpcUa_TimestampsToReturn                eTimestampsToReturn,
    OpcUa_Int32                             nNoOfItemsToModify,
    const OpcUa_MonitoredItemModifyRequest* pItemsToModify,
    OpcUa_Channel_PfnRequestComplete*       pCallback,
    OpcUa_Void*                             pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_SetMonitoringMode
/*============================================================================
 * Synchronously calls the SetMonitoringMode service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_SetMonitoringMode(
    OpcUa_Channel              hChannel,
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
 * Asynchronously calls the SetMonitoringMode service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginSetMonitoringMode(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_UInt32                      nSubscriptionId,
    OpcUa_MonitoringMode              eMonitoringMode,
    OpcUa_Int32                       nNoOfMonitoredItemIds,
    const OpcUa_UInt32*               pMonitoredItemIds,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_SetTriggering
/*============================================================================
 * Synchronously calls the SetTriggering service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_SetTriggering(
    OpcUa_Channel              hChannel,
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
 * Asynchronously calls the SetTriggering service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginSetTriggering(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_UInt32                      nSubscriptionId,
    OpcUa_UInt32                      nTriggeringItemId,
    OpcUa_Int32                       nNoOfLinksToAdd,
    const OpcUa_UInt32*               pLinksToAdd,
    OpcUa_Int32                       nNoOfLinksToRemove,
    const OpcUa_UInt32*               pLinksToRemove,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_DeleteMonitoredItems
/*============================================================================
 * Synchronously calls the DeleteMonitoredItems service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_DeleteMonitoredItems(
    OpcUa_Channel              hChannel,
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
 * Asynchronously calls the DeleteMonitoredItems service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginDeleteMonitoredItems(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_UInt32                      nSubscriptionId,
    OpcUa_Int32                       nNoOfMonitoredItemIds,
    const OpcUa_UInt32*               pMonitoredItemIds,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_CreateSubscription
/*============================================================================
 * Synchronously calls the CreateSubscription service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_CreateSubscription(
    OpcUa_Channel              hChannel,
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
 * Asynchronously calls the CreateSubscription service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginCreateSubscription(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_Double                      nRequestedPublishingInterval,
    OpcUa_UInt32                      nRequestedLifetimeCount,
    OpcUa_UInt32                      nRequestedMaxKeepAliveCount,
    OpcUa_UInt32                      nMaxNotificationsPerPublish,
    OpcUa_Boolean                     bPublishingEnabled,
    OpcUa_Byte                        nPriority,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_ModifySubscription
/*============================================================================
 * Synchronously calls the ModifySubscription service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_ModifySubscription(
    OpcUa_Channel              hChannel,
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
 * Asynchronously calls the ModifySubscription service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginModifySubscription(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_UInt32                      nSubscriptionId,
    OpcUa_Double                      nRequestedPublishingInterval,
    OpcUa_UInt32                      nRequestedLifetimeCount,
    OpcUa_UInt32                      nRequestedMaxKeepAliveCount,
    OpcUa_UInt32                      nMaxNotificationsPerPublish,
    OpcUa_Byte                        nPriority,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_SetPublishingMode
/*============================================================================
 * Synchronously calls the SetPublishingMode service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_SetPublishingMode(
    OpcUa_Channel              hChannel,
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
 * Asynchronously calls the SetPublishingMode service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginSetPublishingMode(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_Boolean                     bPublishingEnabled,
    OpcUa_Int32                       nNoOfSubscriptionIds,
    const OpcUa_UInt32*               pSubscriptionIds,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_Publish
/*============================================================================
 * Synchronously calls the Publish service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_Publish(
    OpcUa_Channel                            hChannel,
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
 * Asynchronously calls the Publish service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginPublish(
    OpcUa_Channel                            hChannel,
    const OpcUa_RequestHeader*               pRequestHeader,
    OpcUa_Int32                              nNoOfSubscriptionAcknowledgements,
    const OpcUa_SubscriptionAcknowledgement* pSubscriptionAcknowledgements,
    OpcUa_Channel_PfnRequestComplete*        pCallback,
    OpcUa_Void*                              pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_Republish
/*============================================================================
 * Synchronously calls the Republish service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_Republish(
    OpcUa_Channel              hChannel,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_UInt32               nSubscriptionId,
    OpcUa_UInt32               nRetransmitSequenceNumber,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_NotificationMessage* pNotificationMessage);

/*============================================================================
 * Asynchronously calls the Republish service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginRepublish(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_UInt32                      nSubscriptionId,
    OpcUa_UInt32                      nRetransmitSequenceNumber,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_TransferSubscriptions
/*============================================================================
 * Synchronously calls the TransferSubscriptions service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_TransferSubscriptions(
    OpcUa_Channel              hChannel,
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
 * Asynchronously calls the TransferSubscriptions service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginTransferSubscriptions(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_Int32                       nNoOfSubscriptionIds,
    const OpcUa_UInt32*               pSubscriptionIds,
    OpcUa_Boolean                     bSendInitialValues,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

#ifndef OPCUA_EXCLUDE_DeleteSubscriptions
/*============================================================================
 * Synchronously calls the DeleteSubscriptions service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_DeleteSubscriptions(
    OpcUa_Channel              hChannel,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Int32                nNoOfSubscriptionIds,
    const OpcUa_UInt32*        pSubscriptionIds,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_Int32*               pNoOfResults,
    OpcUa_StatusCode**         pResults,
    OpcUa_Int32*               pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     pDiagnosticInfos);

/*============================================================================
 * Asynchronously calls the DeleteSubscriptions service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginDeleteSubscriptions(
    OpcUa_Channel                     hChannel,
    const OpcUa_RequestHeader*        pRequestHeader,
    OpcUa_Int32                       nNoOfSubscriptionIds,
    const OpcUa_UInt32*               pSubscriptionIds,
    OpcUa_Channel_PfnRequestComplete* pCallback,
    OpcUa_Void*                       pCallbackData);
#endif

OPCUA_END_EXTERN_C

#endif /* OPCUA_HAVE_CLIENTAPI */
#endif /* _OpcUa_ClientApi_H_ */
/* This is the last line of an autogenerated file. */
