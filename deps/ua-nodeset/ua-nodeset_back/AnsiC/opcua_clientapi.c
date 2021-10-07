/* ========================================================================
 * Copyright (c) 2005-2020 The OPC Foundation, Inc. All rights reserved.
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

/* core */
#include <opcua.h>

#ifdef OPCUA_HAVE_CLIENTAPI

/* stack */
#include <opcua_stream.h>

/* types */
#include <opcua_builtintypes.h>
#include <opcua_encodeableobject.h>

#include <opcua_channel.h>
#include <opcua_identifiers.h>
#include <opcua_clientapi.h>

#ifndef OPCUA_EXCLUDE_FindServers
/*============================================================================
 * Synchronously calls the FindServers service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_FindServers(
    OpcUa_Channel                  a_hChannel,
    const OpcUa_RequestHeader*     a_pRequestHeader,
    const OpcUa_String*            a_pEndpointUrl,
    OpcUa_Int32                    a_nNoOfLocaleIds,
    const OpcUa_String*            a_pLocaleIds,
    OpcUa_Int32                    a_nNoOfServerUris,
    const OpcUa_String*            a_pServerUris,
    OpcUa_ResponseHeader*          a_pResponseHeader,
    OpcUa_Int32*                   a_pNoOfServers,
    OpcUa_ApplicationDescription** a_pServers)
{
    OpcUa_FindServersRequest cRequest;
    OpcUa_FindServersResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_FindServers");

    OpcUa_FindServersRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pEndpointUrl);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfLocaleIds, a_pLocaleIds);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfServerUris, a_pServerUris);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfServers);
    OpcUa_ReturnErrorIfArgumentNull(a_pServers);

    /* copy parameters into request object. */
    cRequest.RequestHeader  = *a_pRequestHeader;
    cRequest.EndpointUrl    = *a_pEndpointUrl;
    cRequest.NoOfLocaleIds  = a_nNoOfLocaleIds;
    cRequest.LocaleIds      = (OpcUa_String*)a_pLocaleIds;
    cRequest.NoOfServerUris = a_nNoOfServerUris;
    cRequest.ServerUris     = (OpcUa_String*)a_pServerUris;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "FindServers",
        (OpcUa_Void*)&cRequest,
        &OpcUa_FindServersRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_FindServersResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader = pResponse->ResponseHeader;
        *a_pNoOfServers    = pResponse->NoOfServers;
        *a_pServers        = pResponse->Servers;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the FindServers service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginFindServers(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    const OpcUa_String*               a_pEndpointUrl,
    OpcUa_Int32                       a_nNoOfLocaleIds,
    const OpcUa_String*               a_pLocaleIds,
    OpcUa_Int32                       a_nNoOfServerUris,
    const OpcUa_String*               a_pServerUris,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_FindServersRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginFindServers");

    OpcUa_FindServersRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pEndpointUrl);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfLocaleIds, a_pLocaleIds);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfServerUris, a_pServerUris);

    /* copy parameters into request object. */
    cRequest.RequestHeader  = *a_pRequestHeader;
    cRequest.EndpointUrl    = *a_pEndpointUrl;
    cRequest.NoOfLocaleIds  = a_nNoOfLocaleIds;
    cRequest.LocaleIds      = (OpcUa_String*)a_pLocaleIds;
    cRequest.NoOfServerUris = a_nNoOfServerUris;
    cRequest.ServerUris     = (OpcUa_String*)a_pServerUris;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "FindServers",
        (OpcUa_Void*)&cRequest,
        &OpcUa_FindServersRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_FindServersOnNetwork
/*============================================================================
 * Synchronously calls the FindServersOnNetwork service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_FindServersOnNetwork(
    OpcUa_Channel              a_hChannel,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_UInt32               a_nStartingRecordId,
    OpcUa_UInt32               a_nMaxRecordsToReturn,
    OpcUa_Int32                a_nNoOfServerCapabilityFilter,
    const OpcUa_String*        a_pServerCapabilityFilter,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_DateTime*            a_pLastCounterResetTime,
    OpcUa_Int32*               a_pNoOfServers,
    OpcUa_ServerOnNetwork**    a_pServers)
{
    OpcUa_FindServersOnNetworkRequest cRequest;
    OpcUa_FindServersOnNetworkResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_FindServersOnNetwork");

    OpcUa_FindServersOnNetworkRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nStartingRecordId);
    OpcUa_ReferenceParameter(a_nMaxRecordsToReturn);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfServerCapabilityFilter, a_pServerCapabilityFilter);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pLastCounterResetTime);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfServers);
    OpcUa_ReturnErrorIfArgumentNull(a_pServers);

    /* copy parameters into request object. */
    cRequest.RequestHeader              = *a_pRequestHeader;
    cRequest.StartingRecordId           = a_nStartingRecordId;
    cRequest.MaxRecordsToReturn         = a_nMaxRecordsToReturn;
    cRequest.NoOfServerCapabilityFilter = a_nNoOfServerCapabilityFilter;
    cRequest.ServerCapabilityFilter     = (OpcUa_String*)a_pServerCapabilityFilter;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "FindServersOnNetwork",
        (OpcUa_Void*)&cRequest,
        &OpcUa_FindServersOnNetworkRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_FindServersOnNetworkResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader       = pResponse->ResponseHeader;
        *a_pLastCounterResetTime = pResponse->LastCounterResetTime;
        *a_pNoOfServers          = pResponse->NoOfServers;
        *a_pServers              = pResponse->Servers;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the FindServersOnNetwork service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginFindServersOnNetwork(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_UInt32                      a_nStartingRecordId,
    OpcUa_UInt32                      a_nMaxRecordsToReturn,
    OpcUa_Int32                       a_nNoOfServerCapabilityFilter,
    const OpcUa_String*               a_pServerCapabilityFilter,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_FindServersOnNetworkRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginFindServersOnNetwork");

    OpcUa_FindServersOnNetworkRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nStartingRecordId);
    OpcUa_ReferenceParameter(a_nMaxRecordsToReturn);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfServerCapabilityFilter, a_pServerCapabilityFilter);

    /* copy parameters into request object. */
    cRequest.RequestHeader              = *a_pRequestHeader;
    cRequest.StartingRecordId           = a_nStartingRecordId;
    cRequest.MaxRecordsToReturn         = a_nMaxRecordsToReturn;
    cRequest.NoOfServerCapabilityFilter = a_nNoOfServerCapabilityFilter;
    cRequest.ServerCapabilityFilter     = (OpcUa_String*)a_pServerCapabilityFilter;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "FindServersOnNetwork",
        (OpcUa_Void*)&cRequest,
        &OpcUa_FindServersOnNetworkRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_GetEndpoints
/*============================================================================
 * Synchronously calls the GetEndpoints service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_GetEndpoints(
    OpcUa_Channel               a_hChannel,
    const OpcUa_RequestHeader*  a_pRequestHeader,
    const OpcUa_String*         a_pEndpointUrl,
    OpcUa_Int32                 a_nNoOfLocaleIds,
    const OpcUa_String*         a_pLocaleIds,
    OpcUa_Int32                 a_nNoOfProfileUris,
    const OpcUa_String*         a_pProfileUris,
    OpcUa_ResponseHeader*       a_pResponseHeader,
    OpcUa_Int32*                a_pNoOfEndpoints,
    OpcUa_EndpointDescription** a_pEndpoints)
{
    OpcUa_GetEndpointsRequest cRequest;
    OpcUa_GetEndpointsResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_GetEndpoints");

    OpcUa_GetEndpointsRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pEndpointUrl);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfLocaleIds, a_pLocaleIds);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfProfileUris, a_pProfileUris);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfEndpoints);
    OpcUa_ReturnErrorIfArgumentNull(a_pEndpoints);

    /* copy parameters into request object. */
    cRequest.RequestHeader   = *a_pRequestHeader;
    cRequest.EndpointUrl     = *a_pEndpointUrl;
    cRequest.NoOfLocaleIds   = a_nNoOfLocaleIds;
    cRequest.LocaleIds       = (OpcUa_String*)a_pLocaleIds;
    cRequest.NoOfProfileUris = a_nNoOfProfileUris;
    cRequest.ProfileUris     = (OpcUa_String*)a_pProfileUris;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "GetEndpoints",
        (OpcUa_Void*)&cRequest,
        &OpcUa_GetEndpointsRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_GetEndpointsResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader = pResponse->ResponseHeader;
        *a_pNoOfEndpoints  = pResponse->NoOfEndpoints;
        *a_pEndpoints      = pResponse->Endpoints;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the GetEndpoints service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginGetEndpoints(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    const OpcUa_String*               a_pEndpointUrl,
    OpcUa_Int32                       a_nNoOfLocaleIds,
    const OpcUa_String*               a_pLocaleIds,
    OpcUa_Int32                       a_nNoOfProfileUris,
    const OpcUa_String*               a_pProfileUris,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_GetEndpointsRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginGetEndpoints");

    OpcUa_GetEndpointsRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pEndpointUrl);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfLocaleIds, a_pLocaleIds);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfProfileUris, a_pProfileUris);

    /* copy parameters into request object. */
    cRequest.RequestHeader   = *a_pRequestHeader;
    cRequest.EndpointUrl     = *a_pEndpointUrl;
    cRequest.NoOfLocaleIds   = a_nNoOfLocaleIds;
    cRequest.LocaleIds       = (OpcUa_String*)a_pLocaleIds;
    cRequest.NoOfProfileUris = a_nNoOfProfileUris;
    cRequest.ProfileUris     = (OpcUa_String*)a_pProfileUris;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "GetEndpoints",
        (OpcUa_Void*)&cRequest,
        &OpcUa_GetEndpointsRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_RegisterServer
/*============================================================================
 * Synchronously calls the RegisterServer service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_RegisterServer(
    OpcUa_Channel                 a_hChannel,
    const OpcUa_RequestHeader*    a_pRequestHeader,
    const OpcUa_RegisteredServer* a_pServer,
    OpcUa_ResponseHeader*         a_pResponseHeader)
{
    OpcUa_RegisterServerRequest cRequest;
    OpcUa_RegisterServerResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_RegisterServer");

    OpcUa_RegisterServerRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pServer);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);

    /* copy parameters into request object. */
    cRequest.RequestHeader = *a_pRequestHeader;
    cRequest.Server        = *a_pServer;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "RegisterServer",
        (OpcUa_Void*)&cRequest,
        &OpcUa_RegisterServerRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_RegisterServerResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader = pResponse->ResponseHeader;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the RegisterServer service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginRegisterServer(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    const OpcUa_RegisteredServer*     a_pServer,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_RegisterServerRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginRegisterServer");

    OpcUa_RegisterServerRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pServer);

    /* copy parameters into request object. */
    cRequest.RequestHeader = *a_pRequestHeader;
    cRequest.Server        = *a_pServer;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "RegisterServer",
        (OpcUa_Void*)&cRequest,
        &OpcUa_RegisterServerRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_RegisterServer2
/*============================================================================
 * Synchronously calls the RegisterServer2 service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_RegisterServer2(
    OpcUa_Channel                 a_hChannel,
    const OpcUa_RequestHeader*    a_pRequestHeader,
    const OpcUa_RegisteredServer* a_pServer,
    OpcUa_Int32                   a_nNoOfDiscoveryConfiguration,
    const OpcUa_ExtensionObject*  a_pDiscoveryConfiguration,
    OpcUa_ResponseHeader*         a_pResponseHeader,
    OpcUa_Int32*                  a_pNoOfConfigurationResults,
    OpcUa_StatusCode**            a_pConfigurationResults,
    OpcUa_Int32*                  a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**        a_pDiagnosticInfos)
{
    OpcUa_RegisterServer2Request cRequest;
    OpcUa_RegisterServer2Response* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_RegisterServer2");

    OpcUa_RegisterServer2Request_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pServer);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfDiscoveryConfiguration, a_pDiscoveryConfiguration);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfConfigurationResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pConfigurationResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    /* copy parameters into request object. */
    cRequest.RequestHeader              = *a_pRequestHeader;
    cRequest.Server                     = *a_pServer;
    cRequest.NoOfDiscoveryConfiguration = a_nNoOfDiscoveryConfiguration;
    cRequest.DiscoveryConfiguration     = (OpcUa_ExtensionObject*)a_pDiscoveryConfiguration;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "RegisterServer2",
        (OpcUa_Void*)&cRequest,
        &OpcUa_RegisterServer2Request_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_RegisterServer2Response)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader           = pResponse->ResponseHeader;
        *a_pNoOfConfigurationResults = pResponse->NoOfConfigurationResults;
        *a_pConfigurationResults     = pResponse->ConfigurationResults;
        *a_pNoOfDiagnosticInfos      = pResponse->NoOfDiagnosticInfos;
        *a_pDiagnosticInfos          = pResponse->DiagnosticInfos;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the RegisterServer2 service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginRegisterServer2(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    const OpcUa_RegisteredServer*     a_pServer,
    OpcUa_Int32                       a_nNoOfDiscoveryConfiguration,
    const OpcUa_ExtensionObject*      a_pDiscoveryConfiguration,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_RegisterServer2Request cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginRegisterServer2");

    OpcUa_RegisterServer2Request_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pServer);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfDiscoveryConfiguration, a_pDiscoveryConfiguration);

    /* copy parameters into request object. */
    cRequest.RequestHeader              = *a_pRequestHeader;
    cRequest.Server                     = *a_pServer;
    cRequest.NoOfDiscoveryConfiguration = a_nNoOfDiscoveryConfiguration;
    cRequest.DiscoveryConfiguration     = (OpcUa_ExtensionObject*)a_pDiscoveryConfiguration;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "RegisterServer2",
        (OpcUa_Void*)&cRequest,
        &OpcUa_RegisterServer2Request_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_CreateSession
/*============================================================================
 * Synchronously calls the CreateSession service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_CreateSession(
    OpcUa_Channel                       a_hChannel,
    const OpcUa_RequestHeader*          a_pRequestHeader,
    const OpcUa_ApplicationDescription* a_pClientDescription,
    const OpcUa_String*                 a_pServerUri,
    const OpcUa_String*                 a_pEndpointUrl,
    const OpcUa_String*                 a_pSessionName,
    const OpcUa_ByteString*             a_pClientNonce,
    const OpcUa_ByteString*             a_pClientCertificate,
    OpcUa_Double                        a_nRequestedSessionTimeout,
    OpcUa_UInt32                        a_nMaxResponseMessageSize,
    OpcUa_ResponseHeader*               a_pResponseHeader,
    OpcUa_NodeId*                       a_pSessionId,
    OpcUa_NodeId*                       a_pAuthenticationToken,
    OpcUa_Double*                       a_pRevisedSessionTimeout,
    OpcUa_ByteString*                   a_pServerNonce,
    OpcUa_ByteString*                   a_pServerCertificate,
    OpcUa_Int32*                        a_pNoOfServerEndpoints,
    OpcUa_EndpointDescription**         a_pServerEndpoints,
    OpcUa_Int32*                        a_pNoOfServerSoftwareCertificates,
    OpcUa_SignedSoftwareCertificate**   a_pServerSoftwareCertificates,
    OpcUa_SignatureData*                a_pServerSignature,
    OpcUa_UInt32*                       a_pMaxRequestMessageSize)
{
    OpcUa_CreateSessionRequest cRequest;
    OpcUa_CreateSessionResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_CreateSession");

    OpcUa_CreateSessionRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pClientDescription);
    OpcUa_ReturnErrorIfArgumentNull(a_pServerUri);
    OpcUa_ReturnErrorIfArgumentNull(a_pEndpointUrl);
    OpcUa_ReturnErrorIfArgumentNull(a_pSessionName);
    OpcUa_ReturnErrorIfArgumentNull(a_pClientNonce);
    OpcUa_ReturnErrorIfArgumentNull(a_pClientCertificate);
    OpcUa_ReferenceParameter(a_nRequestedSessionTimeout);
    OpcUa_ReferenceParameter(a_nMaxResponseMessageSize);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pSessionId);
    OpcUa_ReturnErrorIfArgumentNull(a_pAuthenticationToken);
    OpcUa_ReturnErrorIfArgumentNull(a_pRevisedSessionTimeout);
    OpcUa_ReturnErrorIfArgumentNull(a_pServerNonce);
    OpcUa_ReturnErrorIfArgumentNull(a_pServerCertificate);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfServerEndpoints);
    OpcUa_ReturnErrorIfArgumentNull(a_pServerEndpoints);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfServerSoftwareCertificates);
    OpcUa_ReturnErrorIfArgumentNull(a_pServerSoftwareCertificates);
    OpcUa_ReturnErrorIfArgumentNull(a_pServerSignature);
    OpcUa_ReturnErrorIfArgumentNull(a_pMaxRequestMessageSize);

    /* copy parameters into request object. */
    cRequest.RequestHeader           = *a_pRequestHeader;
    cRequest.ClientDescription       = *a_pClientDescription;
    cRequest.ServerUri               = *a_pServerUri;
    cRequest.EndpointUrl             = *a_pEndpointUrl;
    cRequest.SessionName             = *a_pSessionName;
    cRequest.ClientNonce             = *a_pClientNonce;
    cRequest.ClientCertificate       = *a_pClientCertificate;
    cRequest.RequestedSessionTimeout = a_nRequestedSessionTimeout;
    cRequest.MaxResponseMessageSize  = a_nMaxResponseMessageSize;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "CreateSession",
        (OpcUa_Void*)&cRequest,
        &OpcUa_CreateSessionRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_CreateSessionResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader                 = pResponse->ResponseHeader;
        *a_pSessionId                      = pResponse->SessionId;
        *a_pAuthenticationToken            = pResponse->AuthenticationToken;
        *a_pRevisedSessionTimeout          = pResponse->RevisedSessionTimeout;
        *a_pServerNonce                    = pResponse->ServerNonce;
        *a_pServerCertificate              = pResponse->ServerCertificate;
        *a_pNoOfServerEndpoints            = pResponse->NoOfServerEndpoints;
        *a_pServerEndpoints                = pResponse->ServerEndpoints;
        *a_pNoOfServerSoftwareCertificates = pResponse->NoOfServerSoftwareCertificates;
        *a_pServerSoftwareCertificates     = pResponse->ServerSoftwareCertificates;
        *a_pServerSignature                = pResponse->ServerSignature;
        *a_pMaxRequestMessageSize          = pResponse->MaxRequestMessageSize;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the CreateSession service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginCreateSession(
    OpcUa_Channel                       a_hChannel,
    const OpcUa_RequestHeader*          a_pRequestHeader,
    const OpcUa_ApplicationDescription* a_pClientDescription,
    const OpcUa_String*                 a_pServerUri,
    const OpcUa_String*                 a_pEndpointUrl,
    const OpcUa_String*                 a_pSessionName,
    const OpcUa_ByteString*             a_pClientNonce,
    const OpcUa_ByteString*             a_pClientCertificate,
    OpcUa_Double                        a_nRequestedSessionTimeout,
    OpcUa_UInt32                        a_nMaxResponseMessageSize,
    OpcUa_Channel_PfnRequestComplete*   a_pCallback,
    OpcUa_Void*                         a_pCallbackData)
{
    OpcUa_CreateSessionRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginCreateSession");

    OpcUa_CreateSessionRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pClientDescription);
    OpcUa_ReturnErrorIfArgumentNull(a_pServerUri);
    OpcUa_ReturnErrorIfArgumentNull(a_pEndpointUrl);
    OpcUa_ReturnErrorIfArgumentNull(a_pSessionName);
    OpcUa_ReturnErrorIfArgumentNull(a_pClientNonce);
    OpcUa_ReturnErrorIfArgumentNull(a_pClientCertificate);
    OpcUa_ReferenceParameter(a_nRequestedSessionTimeout);
    OpcUa_ReferenceParameter(a_nMaxResponseMessageSize);

    /* copy parameters into request object. */
    cRequest.RequestHeader           = *a_pRequestHeader;
    cRequest.ClientDescription       = *a_pClientDescription;
    cRequest.ServerUri               = *a_pServerUri;
    cRequest.EndpointUrl             = *a_pEndpointUrl;
    cRequest.SessionName             = *a_pSessionName;
    cRequest.ClientNonce             = *a_pClientNonce;
    cRequest.ClientCertificate       = *a_pClientCertificate;
    cRequest.RequestedSessionTimeout = a_nRequestedSessionTimeout;
    cRequest.MaxResponseMessageSize  = a_nMaxResponseMessageSize;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "CreateSession",
        (OpcUa_Void*)&cRequest,
        &OpcUa_CreateSessionRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_ActivateSession
/*============================================================================
 * Synchronously calls the ActivateSession service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_ActivateSession(
    OpcUa_Channel                          a_hChannel,
    const OpcUa_RequestHeader*             a_pRequestHeader,
    const OpcUa_SignatureData*             a_pClientSignature,
    OpcUa_Int32                            a_nNoOfClientSoftwareCertificates,
    const OpcUa_SignedSoftwareCertificate* a_pClientSoftwareCertificates,
    OpcUa_Int32                            a_nNoOfLocaleIds,
    const OpcUa_String*                    a_pLocaleIds,
    const OpcUa_ExtensionObject*           a_pUserIdentityToken,
    const OpcUa_SignatureData*             a_pUserTokenSignature,
    OpcUa_ResponseHeader*                  a_pResponseHeader,
    OpcUa_ByteString*                      a_pServerNonce,
    OpcUa_Int32*                           a_pNoOfResults,
    OpcUa_StatusCode**                     a_pResults,
    OpcUa_Int32*                           a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**                 a_pDiagnosticInfos)
{
    OpcUa_ActivateSessionRequest cRequest;
    OpcUa_ActivateSessionResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_ActivateSession");

    OpcUa_ActivateSessionRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pClientSignature);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfClientSoftwareCertificates, a_pClientSoftwareCertificates);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfLocaleIds, a_pLocaleIds);
    OpcUa_ReturnErrorIfArgumentNull(a_pUserIdentityToken);
    OpcUa_ReturnErrorIfArgumentNull(a_pUserTokenSignature);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pServerNonce);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    /* copy parameters into request object. */
    cRequest.RequestHeader                  = *a_pRequestHeader;
    cRequest.ClientSignature                = *a_pClientSignature;
    cRequest.NoOfClientSoftwareCertificates = a_nNoOfClientSoftwareCertificates;
    cRequest.ClientSoftwareCertificates     = (OpcUa_SignedSoftwareCertificate*)a_pClientSoftwareCertificates;
    cRequest.NoOfLocaleIds                  = a_nNoOfLocaleIds;
    cRequest.LocaleIds                      = (OpcUa_String*)a_pLocaleIds;
    cRequest.UserIdentityToken              = *a_pUserIdentityToken;
    cRequest.UserTokenSignature             = *a_pUserTokenSignature;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "ActivateSession",
        (OpcUa_Void*)&cRequest,
        &OpcUa_ActivateSessionRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_ActivateSessionResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader      = pResponse->ResponseHeader;
        *a_pServerNonce         = pResponse->ServerNonce;
        *a_pNoOfResults         = pResponse->NoOfResults;
        *a_pResults             = pResponse->Results;
        *a_pNoOfDiagnosticInfos = pResponse->NoOfDiagnosticInfos;
        *a_pDiagnosticInfos     = pResponse->DiagnosticInfos;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the ActivateSession service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginActivateSession(
    OpcUa_Channel                          a_hChannel,
    const OpcUa_RequestHeader*             a_pRequestHeader,
    const OpcUa_SignatureData*             a_pClientSignature,
    OpcUa_Int32                            a_nNoOfClientSoftwareCertificates,
    const OpcUa_SignedSoftwareCertificate* a_pClientSoftwareCertificates,
    OpcUa_Int32                            a_nNoOfLocaleIds,
    const OpcUa_String*                    a_pLocaleIds,
    const OpcUa_ExtensionObject*           a_pUserIdentityToken,
    const OpcUa_SignatureData*             a_pUserTokenSignature,
    OpcUa_Channel_PfnRequestComplete*      a_pCallback,
    OpcUa_Void*                            a_pCallbackData)
{
    OpcUa_ActivateSessionRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginActivateSession");

    OpcUa_ActivateSessionRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pClientSignature);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfClientSoftwareCertificates, a_pClientSoftwareCertificates);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfLocaleIds, a_pLocaleIds);
    OpcUa_ReturnErrorIfArgumentNull(a_pUserIdentityToken);
    OpcUa_ReturnErrorIfArgumentNull(a_pUserTokenSignature);

    /* copy parameters into request object. */
    cRequest.RequestHeader                  = *a_pRequestHeader;
    cRequest.ClientSignature                = *a_pClientSignature;
    cRequest.NoOfClientSoftwareCertificates = a_nNoOfClientSoftwareCertificates;
    cRequest.ClientSoftwareCertificates     = (OpcUa_SignedSoftwareCertificate*)a_pClientSoftwareCertificates;
    cRequest.NoOfLocaleIds                  = a_nNoOfLocaleIds;
    cRequest.LocaleIds                      = (OpcUa_String*)a_pLocaleIds;
    cRequest.UserIdentityToken              = *a_pUserIdentityToken;
    cRequest.UserTokenSignature             = *a_pUserTokenSignature;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "ActivateSession",
        (OpcUa_Void*)&cRequest,
        &OpcUa_ActivateSessionRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_CloseSession
/*============================================================================
 * Synchronously calls the CloseSession service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_CloseSession(
    OpcUa_Channel              a_hChannel,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_Boolean              a_bDeleteSubscriptions,
    OpcUa_ResponseHeader*      a_pResponseHeader)
{
    OpcUa_CloseSessionRequest cRequest;
    OpcUa_CloseSessionResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_CloseSession");

    OpcUa_CloseSessionRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_bDeleteSubscriptions);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);

    /* copy parameters into request object. */
    cRequest.RequestHeader       = *a_pRequestHeader;
    cRequest.DeleteSubscriptions = a_bDeleteSubscriptions;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "CloseSession",
        (OpcUa_Void*)&cRequest,
        &OpcUa_CloseSessionRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_CloseSessionResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader = pResponse->ResponseHeader;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the CloseSession service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginCloseSession(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_Boolean                     a_bDeleteSubscriptions,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_CloseSessionRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginCloseSession");

    OpcUa_CloseSessionRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_bDeleteSubscriptions);

    /* copy parameters into request object. */
    cRequest.RequestHeader       = *a_pRequestHeader;
    cRequest.DeleteSubscriptions = a_bDeleteSubscriptions;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "CloseSession",
        (OpcUa_Void*)&cRequest,
        &OpcUa_CloseSessionRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_Cancel
/*============================================================================
 * Synchronously calls the Cancel service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_Cancel(
    OpcUa_Channel              a_hChannel,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_UInt32               a_nRequestHandle,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_UInt32*              a_pCancelCount)
{
    OpcUa_CancelRequest cRequest;
    OpcUa_CancelResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_Cancel");

    OpcUa_CancelRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nRequestHandle);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pCancelCount);

    /* copy parameters into request object. */
    cRequest.RequestHeader = *a_pRequestHeader;
    cRequest.RequestHandle = a_nRequestHandle;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "Cancel",
        (OpcUa_Void*)&cRequest,
        &OpcUa_CancelRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_CancelResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader = pResponse->ResponseHeader;
        *a_pCancelCount    = pResponse->CancelCount;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the Cancel service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginCancel(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_UInt32                      a_nRequestHandle,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_CancelRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginCancel");

    OpcUa_CancelRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nRequestHandle);

    /* copy parameters into request object. */
    cRequest.RequestHeader = *a_pRequestHeader;
    cRequest.RequestHandle = a_nRequestHandle;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "Cancel",
        (OpcUa_Void*)&cRequest,
        &OpcUa_CancelRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_AddNodes
/*============================================================================
 * Synchronously calls the AddNodes service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_AddNodes(
    OpcUa_Channel              a_hChannel,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_Int32                a_nNoOfNodesToAdd,
    const OpcUa_AddNodesItem*  a_pNodesToAdd,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_Int32*               a_pNoOfResults,
    OpcUa_AddNodesResult**     a_pResults,
    OpcUa_Int32*               a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     a_pDiagnosticInfos)
{
    OpcUa_AddNodesRequest cRequest;
    OpcUa_AddNodesResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_AddNodes");

    OpcUa_AddNodesRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodesToAdd, a_pNodesToAdd);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    /* copy parameters into request object. */
    cRequest.RequestHeader  = *a_pRequestHeader;
    cRequest.NoOfNodesToAdd = a_nNoOfNodesToAdd;
    cRequest.NodesToAdd     = (OpcUa_AddNodesItem*)a_pNodesToAdd;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "AddNodes",
        (OpcUa_Void*)&cRequest,
        &OpcUa_AddNodesRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_AddNodesResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader      = pResponse->ResponseHeader;
        *a_pNoOfResults         = pResponse->NoOfResults;
        *a_pResults             = pResponse->Results;
        *a_pNoOfDiagnosticInfos = pResponse->NoOfDiagnosticInfos;
        *a_pDiagnosticInfos     = pResponse->DiagnosticInfos;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the AddNodes service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginAddNodes(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_Int32                       a_nNoOfNodesToAdd,
    const OpcUa_AddNodesItem*         a_pNodesToAdd,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_AddNodesRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginAddNodes");

    OpcUa_AddNodesRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodesToAdd, a_pNodesToAdd);

    /* copy parameters into request object. */
    cRequest.RequestHeader  = *a_pRequestHeader;
    cRequest.NoOfNodesToAdd = a_nNoOfNodesToAdd;
    cRequest.NodesToAdd     = (OpcUa_AddNodesItem*)a_pNodesToAdd;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "AddNodes",
        (OpcUa_Void*)&cRequest,
        &OpcUa_AddNodesRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_AddReferences
/*============================================================================
 * Synchronously calls the AddReferences service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_AddReferences(
    OpcUa_Channel                  a_hChannel,
    const OpcUa_RequestHeader*     a_pRequestHeader,
    OpcUa_Int32                    a_nNoOfReferencesToAdd,
    const OpcUa_AddReferencesItem* a_pReferencesToAdd,
    OpcUa_ResponseHeader*          a_pResponseHeader,
    OpcUa_Int32*                   a_pNoOfResults,
    OpcUa_StatusCode**             a_pResults,
    OpcUa_Int32*                   a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**         a_pDiagnosticInfos)
{
    OpcUa_AddReferencesRequest cRequest;
    OpcUa_AddReferencesResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_AddReferences");

    OpcUa_AddReferencesRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfReferencesToAdd, a_pReferencesToAdd);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    /* copy parameters into request object. */
    cRequest.RequestHeader       = *a_pRequestHeader;
    cRequest.NoOfReferencesToAdd = a_nNoOfReferencesToAdd;
    cRequest.ReferencesToAdd     = (OpcUa_AddReferencesItem*)a_pReferencesToAdd;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "AddReferences",
        (OpcUa_Void*)&cRequest,
        &OpcUa_AddReferencesRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_AddReferencesResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader      = pResponse->ResponseHeader;
        *a_pNoOfResults         = pResponse->NoOfResults;
        *a_pResults             = pResponse->Results;
        *a_pNoOfDiagnosticInfos = pResponse->NoOfDiagnosticInfos;
        *a_pDiagnosticInfos     = pResponse->DiagnosticInfos;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the AddReferences service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginAddReferences(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_Int32                       a_nNoOfReferencesToAdd,
    const OpcUa_AddReferencesItem*    a_pReferencesToAdd,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_AddReferencesRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginAddReferences");

    OpcUa_AddReferencesRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfReferencesToAdd, a_pReferencesToAdd);

    /* copy parameters into request object. */
    cRequest.RequestHeader       = *a_pRequestHeader;
    cRequest.NoOfReferencesToAdd = a_nNoOfReferencesToAdd;
    cRequest.ReferencesToAdd     = (OpcUa_AddReferencesItem*)a_pReferencesToAdd;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "AddReferences",
        (OpcUa_Void*)&cRequest,
        &OpcUa_AddReferencesRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_DeleteNodes
/*============================================================================
 * Synchronously calls the DeleteNodes service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_DeleteNodes(
    OpcUa_Channel                a_hChannel,
    const OpcUa_RequestHeader*   a_pRequestHeader,
    OpcUa_Int32                  a_nNoOfNodesToDelete,
    const OpcUa_DeleteNodesItem* a_pNodesToDelete,
    OpcUa_ResponseHeader*        a_pResponseHeader,
    OpcUa_Int32*                 a_pNoOfResults,
    OpcUa_StatusCode**           a_pResults,
    OpcUa_Int32*                 a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**       a_pDiagnosticInfos)
{
    OpcUa_DeleteNodesRequest cRequest;
    OpcUa_DeleteNodesResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_DeleteNodes");

    OpcUa_DeleteNodesRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodesToDelete, a_pNodesToDelete);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    /* copy parameters into request object. */
    cRequest.RequestHeader     = *a_pRequestHeader;
    cRequest.NoOfNodesToDelete = a_nNoOfNodesToDelete;
    cRequest.NodesToDelete     = (OpcUa_DeleteNodesItem*)a_pNodesToDelete;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "DeleteNodes",
        (OpcUa_Void*)&cRequest,
        &OpcUa_DeleteNodesRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_DeleteNodesResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader      = pResponse->ResponseHeader;
        *a_pNoOfResults         = pResponse->NoOfResults;
        *a_pResults             = pResponse->Results;
        *a_pNoOfDiagnosticInfos = pResponse->NoOfDiagnosticInfos;
        *a_pDiagnosticInfos     = pResponse->DiagnosticInfos;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the DeleteNodes service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginDeleteNodes(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_Int32                       a_nNoOfNodesToDelete,
    const OpcUa_DeleteNodesItem*      a_pNodesToDelete,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_DeleteNodesRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginDeleteNodes");

    OpcUa_DeleteNodesRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodesToDelete, a_pNodesToDelete);

    /* copy parameters into request object. */
    cRequest.RequestHeader     = *a_pRequestHeader;
    cRequest.NoOfNodesToDelete = a_nNoOfNodesToDelete;
    cRequest.NodesToDelete     = (OpcUa_DeleteNodesItem*)a_pNodesToDelete;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "DeleteNodes",
        (OpcUa_Void*)&cRequest,
        &OpcUa_DeleteNodesRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_DeleteReferences
/*============================================================================
 * Synchronously calls the DeleteReferences service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_DeleteReferences(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_Int32                       a_nNoOfReferencesToDelete,
    const OpcUa_DeleteReferencesItem* a_pReferencesToDelete,
    OpcUa_ResponseHeader*             a_pResponseHeader,
    OpcUa_Int32*                      a_pNoOfResults,
    OpcUa_StatusCode**                a_pResults,
    OpcUa_Int32*                      a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**            a_pDiagnosticInfos)
{
    OpcUa_DeleteReferencesRequest cRequest;
    OpcUa_DeleteReferencesResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_DeleteReferences");

    OpcUa_DeleteReferencesRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfReferencesToDelete, a_pReferencesToDelete);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    /* copy parameters into request object. */
    cRequest.RequestHeader          = *a_pRequestHeader;
    cRequest.NoOfReferencesToDelete = a_nNoOfReferencesToDelete;
    cRequest.ReferencesToDelete     = (OpcUa_DeleteReferencesItem*)a_pReferencesToDelete;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "DeleteReferences",
        (OpcUa_Void*)&cRequest,
        &OpcUa_DeleteReferencesRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_DeleteReferencesResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader      = pResponse->ResponseHeader;
        *a_pNoOfResults         = pResponse->NoOfResults;
        *a_pResults             = pResponse->Results;
        *a_pNoOfDiagnosticInfos = pResponse->NoOfDiagnosticInfos;
        *a_pDiagnosticInfos     = pResponse->DiagnosticInfos;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the DeleteReferences service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginDeleteReferences(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_Int32                       a_nNoOfReferencesToDelete,
    const OpcUa_DeleteReferencesItem* a_pReferencesToDelete,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_DeleteReferencesRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginDeleteReferences");

    OpcUa_DeleteReferencesRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfReferencesToDelete, a_pReferencesToDelete);

    /* copy parameters into request object. */
    cRequest.RequestHeader          = *a_pRequestHeader;
    cRequest.NoOfReferencesToDelete = a_nNoOfReferencesToDelete;
    cRequest.ReferencesToDelete     = (OpcUa_DeleteReferencesItem*)a_pReferencesToDelete;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "DeleteReferences",
        (OpcUa_Void*)&cRequest,
        &OpcUa_DeleteReferencesRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_Browse
/*============================================================================
 * Synchronously calls the Browse service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_Browse(
    OpcUa_Channel                  a_hChannel,
    const OpcUa_RequestHeader*     a_pRequestHeader,
    const OpcUa_ViewDescription*   a_pView,
    OpcUa_UInt32                   a_nRequestedMaxReferencesPerNode,
    OpcUa_Int32                    a_nNoOfNodesToBrowse,
    const OpcUa_BrowseDescription* a_pNodesToBrowse,
    OpcUa_ResponseHeader*          a_pResponseHeader,
    OpcUa_Int32*                   a_pNoOfResults,
    OpcUa_BrowseResult**           a_pResults,
    OpcUa_Int32*                   a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**         a_pDiagnosticInfos)
{
    OpcUa_BrowseRequest cRequest;
    OpcUa_BrowseResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_Browse");

    OpcUa_BrowseRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pView);
    OpcUa_ReferenceParameter(a_nRequestedMaxReferencesPerNode);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodesToBrowse, a_pNodesToBrowse);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    /* copy parameters into request object. */
    cRequest.RequestHeader                 = *a_pRequestHeader;
    cRequest.View                          = *a_pView;
    cRequest.RequestedMaxReferencesPerNode = a_nRequestedMaxReferencesPerNode;
    cRequest.NoOfNodesToBrowse             = a_nNoOfNodesToBrowse;
    cRequest.NodesToBrowse                 = (OpcUa_BrowseDescription*)a_pNodesToBrowse;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "Browse",
        (OpcUa_Void*)&cRequest,
        &OpcUa_BrowseRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_BrowseResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader      = pResponse->ResponseHeader;
        *a_pNoOfResults         = pResponse->NoOfResults;
        *a_pResults             = pResponse->Results;
        *a_pNoOfDiagnosticInfos = pResponse->NoOfDiagnosticInfos;
        *a_pDiagnosticInfos     = pResponse->DiagnosticInfos;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the Browse service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginBrowse(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    const OpcUa_ViewDescription*      a_pView,
    OpcUa_UInt32                      a_nRequestedMaxReferencesPerNode,
    OpcUa_Int32                       a_nNoOfNodesToBrowse,
    const OpcUa_BrowseDescription*    a_pNodesToBrowse,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_BrowseRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginBrowse");

    OpcUa_BrowseRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pView);
    OpcUa_ReferenceParameter(a_nRequestedMaxReferencesPerNode);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodesToBrowse, a_pNodesToBrowse);

    /* copy parameters into request object. */
    cRequest.RequestHeader                 = *a_pRequestHeader;
    cRequest.View                          = *a_pView;
    cRequest.RequestedMaxReferencesPerNode = a_nRequestedMaxReferencesPerNode;
    cRequest.NoOfNodesToBrowse             = a_nNoOfNodesToBrowse;
    cRequest.NodesToBrowse                 = (OpcUa_BrowseDescription*)a_pNodesToBrowse;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "Browse",
        (OpcUa_Void*)&cRequest,
        &OpcUa_BrowseRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_BrowseNext
/*============================================================================
 * Synchronously calls the BrowseNext service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BrowseNext(
    OpcUa_Channel              a_hChannel,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_Boolean              a_bReleaseContinuationPoints,
    OpcUa_Int32                a_nNoOfContinuationPoints,
    const OpcUa_ByteString*    a_pContinuationPoints,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_Int32*               a_pNoOfResults,
    OpcUa_BrowseResult**       a_pResults,
    OpcUa_Int32*               a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     a_pDiagnosticInfos)
{
    OpcUa_BrowseNextRequest cRequest;
    OpcUa_BrowseNextResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BrowseNext");

    OpcUa_BrowseNextRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_bReleaseContinuationPoints);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfContinuationPoints, a_pContinuationPoints);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    /* copy parameters into request object. */
    cRequest.RequestHeader             = *a_pRequestHeader;
    cRequest.ReleaseContinuationPoints = a_bReleaseContinuationPoints;
    cRequest.NoOfContinuationPoints    = a_nNoOfContinuationPoints;
    cRequest.ContinuationPoints        = (OpcUa_ByteString*)a_pContinuationPoints;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "BrowseNext",
        (OpcUa_Void*)&cRequest,
        &OpcUa_BrowseNextRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_BrowseNextResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader      = pResponse->ResponseHeader;
        *a_pNoOfResults         = pResponse->NoOfResults;
        *a_pResults             = pResponse->Results;
        *a_pNoOfDiagnosticInfos = pResponse->NoOfDiagnosticInfos;
        *a_pDiagnosticInfos     = pResponse->DiagnosticInfos;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the BrowseNext service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginBrowseNext(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_Boolean                     a_bReleaseContinuationPoints,
    OpcUa_Int32                       a_nNoOfContinuationPoints,
    const OpcUa_ByteString*           a_pContinuationPoints,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_BrowseNextRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginBrowseNext");

    OpcUa_BrowseNextRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_bReleaseContinuationPoints);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfContinuationPoints, a_pContinuationPoints);

    /* copy parameters into request object. */
    cRequest.RequestHeader             = *a_pRequestHeader;
    cRequest.ReleaseContinuationPoints = a_bReleaseContinuationPoints;
    cRequest.NoOfContinuationPoints    = a_nNoOfContinuationPoints;
    cRequest.ContinuationPoints        = (OpcUa_ByteString*)a_pContinuationPoints;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "BrowseNext",
        (OpcUa_Void*)&cRequest,
        &OpcUa_BrowseNextRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_TranslateBrowsePathsToNodeIds
/*============================================================================
 * Synchronously calls the TranslateBrowsePathsToNodeIds service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_TranslateBrowsePathsToNodeIds(
    OpcUa_Channel              a_hChannel,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_Int32                a_nNoOfBrowsePaths,
    const OpcUa_BrowsePath*    a_pBrowsePaths,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_Int32*               a_pNoOfResults,
    OpcUa_BrowsePathResult**   a_pResults,
    OpcUa_Int32*               a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     a_pDiagnosticInfos)
{
    OpcUa_TranslateBrowsePathsToNodeIdsRequest cRequest;
    OpcUa_TranslateBrowsePathsToNodeIdsResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_TranslateBrowsePathsToNodeIds");

    OpcUa_TranslateBrowsePathsToNodeIdsRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfBrowsePaths, a_pBrowsePaths);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    /* copy parameters into request object. */
    cRequest.RequestHeader   = *a_pRequestHeader;
    cRequest.NoOfBrowsePaths = a_nNoOfBrowsePaths;
    cRequest.BrowsePaths     = (OpcUa_BrowsePath*)a_pBrowsePaths;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "TranslateBrowsePathsToNodeIds",
        (OpcUa_Void*)&cRequest,
        &OpcUa_TranslateBrowsePathsToNodeIdsRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_TranslateBrowsePathsToNodeIdsResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader      = pResponse->ResponseHeader;
        *a_pNoOfResults         = pResponse->NoOfResults;
        *a_pResults             = pResponse->Results;
        *a_pNoOfDiagnosticInfos = pResponse->NoOfDiagnosticInfos;
        *a_pDiagnosticInfos     = pResponse->DiagnosticInfos;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the TranslateBrowsePathsToNodeIds service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginTranslateBrowsePathsToNodeIds(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_Int32                       a_nNoOfBrowsePaths,
    const OpcUa_BrowsePath*           a_pBrowsePaths,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_TranslateBrowsePathsToNodeIdsRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginTranslateBrowsePathsToNodeIds");

    OpcUa_TranslateBrowsePathsToNodeIdsRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfBrowsePaths, a_pBrowsePaths);

    /* copy parameters into request object. */
    cRequest.RequestHeader   = *a_pRequestHeader;
    cRequest.NoOfBrowsePaths = a_nNoOfBrowsePaths;
    cRequest.BrowsePaths     = (OpcUa_BrowsePath*)a_pBrowsePaths;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "TranslateBrowsePathsToNodeIds",
        (OpcUa_Void*)&cRequest,
        &OpcUa_TranslateBrowsePathsToNodeIdsRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_RegisterNodes
/*============================================================================
 * Synchronously calls the RegisterNodes service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_RegisterNodes(
    OpcUa_Channel              a_hChannel,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_Int32                a_nNoOfNodesToRegister,
    const OpcUa_NodeId*        a_pNodesToRegister,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_Int32*               a_pNoOfRegisteredNodeIds,
    OpcUa_NodeId**             a_pRegisteredNodeIds)
{
    OpcUa_RegisterNodesRequest cRequest;
    OpcUa_RegisterNodesResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_RegisterNodes");

    OpcUa_RegisterNodesRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodesToRegister, a_pNodesToRegister);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfRegisteredNodeIds);
    OpcUa_ReturnErrorIfArgumentNull(a_pRegisteredNodeIds);

    /* copy parameters into request object. */
    cRequest.RequestHeader       = *a_pRequestHeader;
    cRequest.NoOfNodesToRegister = a_nNoOfNodesToRegister;
    cRequest.NodesToRegister     = (OpcUa_NodeId*)a_pNodesToRegister;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "RegisterNodes",
        (OpcUa_Void*)&cRequest,
        &OpcUa_RegisterNodesRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_RegisterNodesResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader        = pResponse->ResponseHeader;
        *a_pNoOfRegisteredNodeIds = pResponse->NoOfRegisteredNodeIds;
        *a_pRegisteredNodeIds     = pResponse->RegisteredNodeIds;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the RegisterNodes service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginRegisterNodes(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_Int32                       a_nNoOfNodesToRegister,
    const OpcUa_NodeId*               a_pNodesToRegister,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_RegisterNodesRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginRegisterNodes");

    OpcUa_RegisterNodesRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodesToRegister, a_pNodesToRegister);

    /* copy parameters into request object. */
    cRequest.RequestHeader       = *a_pRequestHeader;
    cRequest.NoOfNodesToRegister = a_nNoOfNodesToRegister;
    cRequest.NodesToRegister     = (OpcUa_NodeId*)a_pNodesToRegister;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "RegisterNodes",
        (OpcUa_Void*)&cRequest,
        &OpcUa_RegisterNodesRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_UnregisterNodes
/*============================================================================
 * Synchronously calls the UnregisterNodes service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_UnregisterNodes(
    OpcUa_Channel              a_hChannel,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_Int32                a_nNoOfNodesToUnregister,
    const OpcUa_NodeId*        a_pNodesToUnregister,
    OpcUa_ResponseHeader*      a_pResponseHeader)
{
    OpcUa_UnregisterNodesRequest cRequest;
    OpcUa_UnregisterNodesResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_UnregisterNodes");

    OpcUa_UnregisterNodesRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodesToUnregister, a_pNodesToUnregister);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);

    /* copy parameters into request object. */
    cRequest.RequestHeader         = *a_pRequestHeader;
    cRequest.NoOfNodesToUnregister = a_nNoOfNodesToUnregister;
    cRequest.NodesToUnregister     = (OpcUa_NodeId*)a_pNodesToUnregister;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "UnregisterNodes",
        (OpcUa_Void*)&cRequest,
        &OpcUa_UnregisterNodesRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_UnregisterNodesResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader = pResponse->ResponseHeader;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the UnregisterNodes service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginUnregisterNodes(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_Int32                       a_nNoOfNodesToUnregister,
    const OpcUa_NodeId*               a_pNodesToUnregister,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_UnregisterNodesRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginUnregisterNodes");

    OpcUa_UnregisterNodesRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodesToUnregister, a_pNodesToUnregister);

    /* copy parameters into request object. */
    cRequest.RequestHeader         = *a_pRequestHeader;
    cRequest.NoOfNodesToUnregister = a_nNoOfNodesToUnregister;
    cRequest.NodesToUnregister     = (OpcUa_NodeId*)a_pNodesToUnregister;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "UnregisterNodes",
        (OpcUa_Void*)&cRequest,
        &OpcUa_UnregisterNodesRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_QueryFirst
/*============================================================================
 * Synchronously calls the QueryFirst service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_QueryFirst(
    OpcUa_Channel                    a_hChannel,
    const OpcUa_RequestHeader*       a_pRequestHeader,
    const OpcUa_ViewDescription*     a_pView,
    OpcUa_Int32                      a_nNoOfNodeTypes,
    const OpcUa_NodeTypeDescription* a_pNodeTypes,
    const OpcUa_ContentFilter*       a_pFilter,
    OpcUa_UInt32                     a_nMaxDataSetsToReturn,
    OpcUa_UInt32                     a_nMaxReferencesToReturn,
    OpcUa_ResponseHeader*            a_pResponseHeader,
    OpcUa_Int32*                     a_pNoOfQueryDataSets,
    OpcUa_QueryDataSet**             a_pQueryDataSets,
    OpcUa_ByteString*                a_pContinuationPoint,
    OpcUa_Int32*                     a_pNoOfParsingResults,
    OpcUa_ParsingResult**            a_pParsingResults,
    OpcUa_Int32*                     a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**           a_pDiagnosticInfos,
    OpcUa_ContentFilterResult*       a_pFilterResult)
{
    OpcUa_QueryFirstRequest cRequest;
    OpcUa_QueryFirstResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_QueryFirst");

    OpcUa_QueryFirstRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pView);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodeTypes, a_pNodeTypes);
    OpcUa_ReturnErrorIfArgumentNull(a_pFilter);
    OpcUa_ReferenceParameter(a_nMaxDataSetsToReturn);
    OpcUa_ReferenceParameter(a_nMaxReferencesToReturn);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfQueryDataSets);
    OpcUa_ReturnErrorIfArgumentNull(a_pQueryDataSets);
    OpcUa_ReturnErrorIfArgumentNull(a_pContinuationPoint);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfParsingResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pParsingResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pFilterResult);

    /* copy parameters into request object. */
    cRequest.RequestHeader         = *a_pRequestHeader;
    cRequest.View                  = *a_pView;
    cRequest.NoOfNodeTypes         = a_nNoOfNodeTypes;
    cRequest.NodeTypes             = (OpcUa_NodeTypeDescription*)a_pNodeTypes;
    cRequest.Filter                = *a_pFilter;
    cRequest.MaxDataSetsToReturn   = a_nMaxDataSetsToReturn;
    cRequest.MaxReferencesToReturn = a_nMaxReferencesToReturn;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "QueryFirst",
        (OpcUa_Void*)&cRequest,
        &OpcUa_QueryFirstRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_QueryFirstResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader      = pResponse->ResponseHeader;
        *a_pNoOfQueryDataSets   = pResponse->NoOfQueryDataSets;
        *a_pQueryDataSets       = pResponse->QueryDataSets;
        *a_pContinuationPoint   = pResponse->ContinuationPoint;
        *a_pNoOfParsingResults  = pResponse->NoOfParsingResults;
        *a_pParsingResults      = pResponse->ParsingResults;
        *a_pNoOfDiagnosticInfos = pResponse->NoOfDiagnosticInfos;
        *a_pDiagnosticInfos     = pResponse->DiagnosticInfos;
        *a_pFilterResult        = pResponse->FilterResult;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the QueryFirst service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginQueryFirst(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    const OpcUa_ViewDescription*      a_pView,
    OpcUa_Int32                       a_nNoOfNodeTypes,
    const OpcUa_NodeTypeDescription*  a_pNodeTypes,
    const OpcUa_ContentFilter*        a_pFilter,
    OpcUa_UInt32                      a_nMaxDataSetsToReturn,
    OpcUa_UInt32                      a_nMaxReferencesToReturn,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_QueryFirstRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginQueryFirst");

    OpcUa_QueryFirstRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pView);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodeTypes, a_pNodeTypes);
    OpcUa_ReturnErrorIfArgumentNull(a_pFilter);
    OpcUa_ReferenceParameter(a_nMaxDataSetsToReturn);
    OpcUa_ReferenceParameter(a_nMaxReferencesToReturn);

    /* copy parameters into request object. */
    cRequest.RequestHeader         = *a_pRequestHeader;
    cRequest.View                  = *a_pView;
    cRequest.NoOfNodeTypes         = a_nNoOfNodeTypes;
    cRequest.NodeTypes             = (OpcUa_NodeTypeDescription*)a_pNodeTypes;
    cRequest.Filter                = *a_pFilter;
    cRequest.MaxDataSetsToReturn   = a_nMaxDataSetsToReturn;
    cRequest.MaxReferencesToReturn = a_nMaxReferencesToReturn;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "QueryFirst",
        (OpcUa_Void*)&cRequest,
        &OpcUa_QueryFirstRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_QueryNext
/*============================================================================
 * Synchronously calls the QueryNext service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_QueryNext(
    OpcUa_Channel              a_hChannel,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_Boolean              a_bReleaseContinuationPoint,
    const OpcUa_ByteString*    a_pContinuationPoint,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_Int32*               a_pNoOfQueryDataSets,
    OpcUa_QueryDataSet**       a_pQueryDataSets,
    OpcUa_ByteString*          a_pRevisedContinuationPoint)
{
    OpcUa_QueryNextRequest cRequest;
    OpcUa_QueryNextResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_QueryNext");

    OpcUa_QueryNextRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_bReleaseContinuationPoint);
    OpcUa_ReturnErrorIfArgumentNull(a_pContinuationPoint);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfQueryDataSets);
    OpcUa_ReturnErrorIfArgumentNull(a_pQueryDataSets);
    OpcUa_ReturnErrorIfArgumentNull(a_pRevisedContinuationPoint);

    /* copy parameters into request object. */
    cRequest.RequestHeader            = *a_pRequestHeader;
    cRequest.ReleaseContinuationPoint = a_bReleaseContinuationPoint;
    cRequest.ContinuationPoint        = *a_pContinuationPoint;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "QueryNext",
        (OpcUa_Void*)&cRequest,
        &OpcUa_QueryNextRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_QueryNextResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader           = pResponse->ResponseHeader;
        *a_pNoOfQueryDataSets        = pResponse->NoOfQueryDataSets;
        *a_pQueryDataSets            = pResponse->QueryDataSets;
        *a_pRevisedContinuationPoint = pResponse->RevisedContinuationPoint;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the QueryNext service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginQueryNext(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_Boolean                     a_bReleaseContinuationPoint,
    const OpcUa_ByteString*           a_pContinuationPoint,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_QueryNextRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginQueryNext");

    OpcUa_QueryNextRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_bReleaseContinuationPoint);
    OpcUa_ReturnErrorIfArgumentNull(a_pContinuationPoint);

    /* copy parameters into request object. */
    cRequest.RequestHeader            = *a_pRequestHeader;
    cRequest.ReleaseContinuationPoint = a_bReleaseContinuationPoint;
    cRequest.ContinuationPoint        = *a_pContinuationPoint;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "QueryNext",
        (OpcUa_Void*)&cRequest,
        &OpcUa_QueryNextRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_Read
/*============================================================================
 * Synchronously calls the Read service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_Read(
    OpcUa_Channel              a_hChannel,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_Double               a_nMaxAge,
    OpcUa_TimestampsToReturn   a_eTimestampsToReturn,
    OpcUa_Int32                a_nNoOfNodesToRead,
    const OpcUa_ReadValueId*   a_pNodesToRead,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_Int32*               a_pNoOfResults,
    OpcUa_DataValue**          a_pResults,
    OpcUa_Int32*               a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     a_pDiagnosticInfos)
{
    OpcUa_ReadRequest cRequest;
    OpcUa_ReadResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_Read");

    OpcUa_ReadRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nMaxAge);
    OpcUa_ReferenceParameter(a_eTimestampsToReturn);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodesToRead, a_pNodesToRead);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    /* copy parameters into request object. */
    cRequest.RequestHeader      = *a_pRequestHeader;
    cRequest.MaxAge             = a_nMaxAge;
    cRequest.TimestampsToReturn = a_eTimestampsToReturn;
    cRequest.NoOfNodesToRead    = a_nNoOfNodesToRead;
    cRequest.NodesToRead        = (OpcUa_ReadValueId*)a_pNodesToRead;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "Read",
        (OpcUa_Void*)&cRequest,
        &OpcUa_ReadRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_ReadResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader      = pResponse->ResponseHeader;
        *a_pNoOfResults         = pResponse->NoOfResults;
        *a_pResults             = pResponse->Results;
        *a_pNoOfDiagnosticInfos = pResponse->NoOfDiagnosticInfos;
        *a_pDiagnosticInfos     = pResponse->DiagnosticInfos;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the Read service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginRead(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_Double                      a_nMaxAge,
    OpcUa_TimestampsToReturn          a_eTimestampsToReturn,
    OpcUa_Int32                       a_nNoOfNodesToRead,
    const OpcUa_ReadValueId*          a_pNodesToRead,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_ReadRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginRead");

    OpcUa_ReadRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nMaxAge);
    OpcUa_ReferenceParameter(a_eTimestampsToReturn);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodesToRead, a_pNodesToRead);

    /* copy parameters into request object. */
    cRequest.RequestHeader      = *a_pRequestHeader;
    cRequest.MaxAge             = a_nMaxAge;
    cRequest.TimestampsToReturn = a_eTimestampsToReturn;
    cRequest.NoOfNodesToRead    = a_nNoOfNodesToRead;
    cRequest.NodesToRead        = (OpcUa_ReadValueId*)a_pNodesToRead;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "Read",
        (OpcUa_Void*)&cRequest,
        &OpcUa_ReadRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_HistoryRead
/*============================================================================
 * Synchronously calls the HistoryRead service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_HistoryRead(
    OpcUa_Channel                   a_hChannel,
    const OpcUa_RequestHeader*      a_pRequestHeader,
    const OpcUa_ExtensionObject*    a_pHistoryReadDetails,
    OpcUa_TimestampsToReturn        a_eTimestampsToReturn,
    OpcUa_Boolean                   a_bReleaseContinuationPoints,
    OpcUa_Int32                     a_nNoOfNodesToRead,
    const OpcUa_HistoryReadValueId* a_pNodesToRead,
    OpcUa_ResponseHeader*           a_pResponseHeader,
    OpcUa_Int32*                    a_pNoOfResults,
    OpcUa_HistoryReadResult**       a_pResults,
    OpcUa_Int32*                    a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**          a_pDiagnosticInfos)
{
    OpcUa_HistoryReadRequest cRequest;
    OpcUa_HistoryReadResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_HistoryRead");

    OpcUa_HistoryReadRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pHistoryReadDetails);
    OpcUa_ReferenceParameter(a_eTimestampsToReturn);
    OpcUa_ReferenceParameter(a_bReleaseContinuationPoints);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodesToRead, a_pNodesToRead);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    /* copy parameters into request object. */
    cRequest.RequestHeader             = *a_pRequestHeader;
    cRequest.HistoryReadDetails        = *a_pHistoryReadDetails;
    cRequest.TimestampsToReturn        = a_eTimestampsToReturn;
    cRequest.ReleaseContinuationPoints = a_bReleaseContinuationPoints;
    cRequest.NoOfNodesToRead           = a_nNoOfNodesToRead;
    cRequest.NodesToRead               = (OpcUa_HistoryReadValueId*)a_pNodesToRead;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "HistoryRead",
        (OpcUa_Void*)&cRequest,
        &OpcUa_HistoryReadRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_HistoryReadResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader      = pResponse->ResponseHeader;
        *a_pNoOfResults         = pResponse->NoOfResults;
        *a_pResults             = pResponse->Results;
        *a_pNoOfDiagnosticInfos = pResponse->NoOfDiagnosticInfos;
        *a_pDiagnosticInfos     = pResponse->DiagnosticInfos;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the HistoryRead service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginHistoryRead(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    const OpcUa_ExtensionObject*      a_pHistoryReadDetails,
    OpcUa_TimestampsToReturn          a_eTimestampsToReturn,
    OpcUa_Boolean                     a_bReleaseContinuationPoints,
    OpcUa_Int32                       a_nNoOfNodesToRead,
    const OpcUa_HistoryReadValueId*   a_pNodesToRead,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_HistoryReadRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginHistoryRead");

    OpcUa_HistoryReadRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pHistoryReadDetails);
    OpcUa_ReferenceParameter(a_eTimestampsToReturn);
    OpcUa_ReferenceParameter(a_bReleaseContinuationPoints);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodesToRead, a_pNodesToRead);

    /* copy parameters into request object. */
    cRequest.RequestHeader             = *a_pRequestHeader;
    cRequest.HistoryReadDetails        = *a_pHistoryReadDetails;
    cRequest.TimestampsToReturn        = a_eTimestampsToReturn;
    cRequest.ReleaseContinuationPoints = a_bReleaseContinuationPoints;
    cRequest.NoOfNodesToRead           = a_nNoOfNodesToRead;
    cRequest.NodesToRead               = (OpcUa_HistoryReadValueId*)a_pNodesToRead;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "HistoryRead",
        (OpcUa_Void*)&cRequest,
        &OpcUa_HistoryReadRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_Write
/*============================================================================
 * Synchronously calls the Write service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_Write(
    OpcUa_Channel              a_hChannel,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_Int32                a_nNoOfNodesToWrite,
    const OpcUa_WriteValue*    a_pNodesToWrite,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_Int32*               a_pNoOfResults,
    OpcUa_StatusCode**         a_pResults,
    OpcUa_Int32*               a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     a_pDiagnosticInfos)
{
    OpcUa_WriteRequest cRequest;
    OpcUa_WriteResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_Write");

    OpcUa_WriteRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodesToWrite, a_pNodesToWrite);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    /* copy parameters into request object. */
    cRequest.RequestHeader    = *a_pRequestHeader;
    cRequest.NoOfNodesToWrite = a_nNoOfNodesToWrite;
    cRequest.NodesToWrite     = (OpcUa_WriteValue*)a_pNodesToWrite;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "Write",
        (OpcUa_Void*)&cRequest,
        &OpcUa_WriteRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_WriteResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader      = pResponse->ResponseHeader;
        *a_pNoOfResults         = pResponse->NoOfResults;
        *a_pResults             = pResponse->Results;
        *a_pNoOfDiagnosticInfos = pResponse->NoOfDiagnosticInfos;
        *a_pDiagnosticInfos     = pResponse->DiagnosticInfos;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the Write service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginWrite(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_Int32                       a_nNoOfNodesToWrite,
    const OpcUa_WriteValue*           a_pNodesToWrite,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_WriteRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginWrite");

    OpcUa_WriteRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodesToWrite, a_pNodesToWrite);

    /* copy parameters into request object. */
    cRequest.RequestHeader    = *a_pRequestHeader;
    cRequest.NoOfNodesToWrite = a_nNoOfNodesToWrite;
    cRequest.NodesToWrite     = (OpcUa_WriteValue*)a_pNodesToWrite;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "Write",
        (OpcUa_Void*)&cRequest,
        &OpcUa_WriteRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_HistoryUpdate
/*============================================================================
 * Synchronously calls the HistoryUpdate service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_HistoryUpdate(
    OpcUa_Channel                a_hChannel,
    const OpcUa_RequestHeader*   a_pRequestHeader,
    OpcUa_Int32                  a_nNoOfHistoryUpdateDetails,
    const OpcUa_ExtensionObject* a_pHistoryUpdateDetails,
    OpcUa_ResponseHeader*        a_pResponseHeader,
    OpcUa_Int32*                 a_pNoOfResults,
    OpcUa_HistoryUpdateResult**  a_pResults,
    OpcUa_Int32*                 a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**       a_pDiagnosticInfos)
{
    OpcUa_HistoryUpdateRequest cRequest;
    OpcUa_HistoryUpdateResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_HistoryUpdate");

    OpcUa_HistoryUpdateRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfHistoryUpdateDetails, a_pHistoryUpdateDetails);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    /* copy parameters into request object. */
    cRequest.RequestHeader            = *a_pRequestHeader;
    cRequest.NoOfHistoryUpdateDetails = a_nNoOfHistoryUpdateDetails;
    cRequest.HistoryUpdateDetails     = (OpcUa_ExtensionObject*)a_pHistoryUpdateDetails;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "HistoryUpdate",
        (OpcUa_Void*)&cRequest,
        &OpcUa_HistoryUpdateRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_HistoryUpdateResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader      = pResponse->ResponseHeader;
        *a_pNoOfResults         = pResponse->NoOfResults;
        *a_pResults             = pResponse->Results;
        *a_pNoOfDiagnosticInfos = pResponse->NoOfDiagnosticInfos;
        *a_pDiagnosticInfos     = pResponse->DiagnosticInfos;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the HistoryUpdate service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginHistoryUpdate(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_Int32                       a_nNoOfHistoryUpdateDetails,
    const OpcUa_ExtensionObject*      a_pHistoryUpdateDetails,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_HistoryUpdateRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginHistoryUpdate");

    OpcUa_HistoryUpdateRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfHistoryUpdateDetails, a_pHistoryUpdateDetails);

    /* copy parameters into request object. */
    cRequest.RequestHeader            = *a_pRequestHeader;
    cRequest.NoOfHistoryUpdateDetails = a_nNoOfHistoryUpdateDetails;
    cRequest.HistoryUpdateDetails     = (OpcUa_ExtensionObject*)a_pHistoryUpdateDetails;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "HistoryUpdate",
        (OpcUa_Void*)&cRequest,
        &OpcUa_HistoryUpdateRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_Call
/*============================================================================
 * Synchronously calls the Call service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_Call(
    OpcUa_Channel                  a_hChannel,
    const OpcUa_RequestHeader*     a_pRequestHeader,
    OpcUa_Int32                    a_nNoOfMethodsToCall,
    const OpcUa_CallMethodRequest* a_pMethodsToCall,
    OpcUa_ResponseHeader*          a_pResponseHeader,
    OpcUa_Int32*                   a_pNoOfResults,
    OpcUa_CallMethodResult**       a_pResults,
    OpcUa_Int32*                   a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**         a_pDiagnosticInfos)
{
    OpcUa_CallRequest cRequest;
    OpcUa_CallResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_Call");

    OpcUa_CallRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfMethodsToCall, a_pMethodsToCall);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    /* copy parameters into request object. */
    cRequest.RequestHeader     = *a_pRequestHeader;
    cRequest.NoOfMethodsToCall = a_nNoOfMethodsToCall;
    cRequest.MethodsToCall     = (OpcUa_CallMethodRequest*)a_pMethodsToCall;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "Call",
        (OpcUa_Void*)&cRequest,
        &OpcUa_CallRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_CallResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader      = pResponse->ResponseHeader;
        *a_pNoOfResults         = pResponse->NoOfResults;
        *a_pResults             = pResponse->Results;
        *a_pNoOfDiagnosticInfos = pResponse->NoOfDiagnosticInfos;
        *a_pDiagnosticInfos     = pResponse->DiagnosticInfos;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the Call service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginCall(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_Int32                       a_nNoOfMethodsToCall,
    const OpcUa_CallMethodRequest*    a_pMethodsToCall,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_CallRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginCall");

    OpcUa_CallRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfMethodsToCall, a_pMethodsToCall);

    /* copy parameters into request object. */
    cRequest.RequestHeader     = *a_pRequestHeader;
    cRequest.NoOfMethodsToCall = a_nNoOfMethodsToCall;
    cRequest.MethodsToCall     = (OpcUa_CallMethodRequest*)a_pMethodsToCall;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "Call",
        (OpcUa_Void*)&cRequest,
        &OpcUa_CallRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_CreateMonitoredItems
/*============================================================================
 * Synchronously calls the CreateMonitoredItems service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_CreateMonitoredItems(
    OpcUa_Channel                           a_hChannel,
    const OpcUa_RequestHeader*              a_pRequestHeader,
    OpcUa_UInt32                            a_nSubscriptionId,
    OpcUa_TimestampsToReturn                a_eTimestampsToReturn,
    OpcUa_Int32                             a_nNoOfItemsToCreate,
    const OpcUa_MonitoredItemCreateRequest* a_pItemsToCreate,
    OpcUa_ResponseHeader*                   a_pResponseHeader,
    OpcUa_Int32*                            a_pNoOfResults,
    OpcUa_MonitoredItemCreateResult**       a_pResults,
    OpcUa_Int32*                            a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**                  a_pDiagnosticInfos)
{
    OpcUa_CreateMonitoredItemsRequest cRequest;
    OpcUa_CreateMonitoredItemsResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_CreateMonitoredItems");

    OpcUa_CreateMonitoredItemsRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nSubscriptionId);
    OpcUa_ReferenceParameter(a_eTimestampsToReturn);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfItemsToCreate, a_pItemsToCreate);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    /* copy parameters into request object. */
    cRequest.RequestHeader      = *a_pRequestHeader;
    cRequest.SubscriptionId     = a_nSubscriptionId;
    cRequest.TimestampsToReturn = a_eTimestampsToReturn;
    cRequest.NoOfItemsToCreate  = a_nNoOfItemsToCreate;
    cRequest.ItemsToCreate      = (OpcUa_MonitoredItemCreateRequest*)a_pItemsToCreate;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "CreateMonitoredItems",
        (OpcUa_Void*)&cRequest,
        &OpcUa_CreateMonitoredItemsRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_CreateMonitoredItemsResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader      = pResponse->ResponseHeader;
        *a_pNoOfResults         = pResponse->NoOfResults;
        *a_pResults             = pResponse->Results;
        *a_pNoOfDiagnosticInfos = pResponse->NoOfDiagnosticInfos;
        *a_pDiagnosticInfos     = pResponse->DiagnosticInfos;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the CreateMonitoredItems service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginCreateMonitoredItems(
    OpcUa_Channel                           a_hChannel,
    const OpcUa_RequestHeader*              a_pRequestHeader,
    OpcUa_UInt32                            a_nSubscriptionId,
    OpcUa_TimestampsToReturn                a_eTimestampsToReturn,
    OpcUa_Int32                             a_nNoOfItemsToCreate,
    const OpcUa_MonitoredItemCreateRequest* a_pItemsToCreate,
    OpcUa_Channel_PfnRequestComplete*       a_pCallback,
    OpcUa_Void*                             a_pCallbackData)
{
    OpcUa_CreateMonitoredItemsRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginCreateMonitoredItems");

    OpcUa_CreateMonitoredItemsRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nSubscriptionId);
    OpcUa_ReferenceParameter(a_eTimestampsToReturn);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfItemsToCreate, a_pItemsToCreate);

    /* copy parameters into request object. */
    cRequest.RequestHeader      = *a_pRequestHeader;
    cRequest.SubscriptionId     = a_nSubscriptionId;
    cRequest.TimestampsToReturn = a_eTimestampsToReturn;
    cRequest.NoOfItemsToCreate  = a_nNoOfItemsToCreate;
    cRequest.ItemsToCreate      = (OpcUa_MonitoredItemCreateRequest*)a_pItemsToCreate;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "CreateMonitoredItems",
        (OpcUa_Void*)&cRequest,
        &OpcUa_CreateMonitoredItemsRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_ModifyMonitoredItems
/*============================================================================
 * Synchronously calls the ModifyMonitoredItems service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_ModifyMonitoredItems(
    OpcUa_Channel                           a_hChannel,
    const OpcUa_RequestHeader*              a_pRequestHeader,
    OpcUa_UInt32                            a_nSubscriptionId,
    OpcUa_TimestampsToReturn                a_eTimestampsToReturn,
    OpcUa_Int32                             a_nNoOfItemsToModify,
    const OpcUa_MonitoredItemModifyRequest* a_pItemsToModify,
    OpcUa_ResponseHeader*                   a_pResponseHeader,
    OpcUa_Int32*                            a_pNoOfResults,
    OpcUa_MonitoredItemModifyResult**       a_pResults,
    OpcUa_Int32*                            a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**                  a_pDiagnosticInfos)
{
    OpcUa_ModifyMonitoredItemsRequest cRequest;
    OpcUa_ModifyMonitoredItemsResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_ModifyMonitoredItems");

    OpcUa_ModifyMonitoredItemsRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nSubscriptionId);
    OpcUa_ReferenceParameter(a_eTimestampsToReturn);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfItemsToModify, a_pItemsToModify);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    /* copy parameters into request object. */
    cRequest.RequestHeader      = *a_pRequestHeader;
    cRequest.SubscriptionId     = a_nSubscriptionId;
    cRequest.TimestampsToReturn = a_eTimestampsToReturn;
    cRequest.NoOfItemsToModify  = a_nNoOfItemsToModify;
    cRequest.ItemsToModify      = (OpcUa_MonitoredItemModifyRequest*)a_pItemsToModify;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "ModifyMonitoredItems",
        (OpcUa_Void*)&cRequest,
        &OpcUa_ModifyMonitoredItemsRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_ModifyMonitoredItemsResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader      = pResponse->ResponseHeader;
        *a_pNoOfResults         = pResponse->NoOfResults;
        *a_pResults             = pResponse->Results;
        *a_pNoOfDiagnosticInfos = pResponse->NoOfDiagnosticInfos;
        *a_pDiagnosticInfos     = pResponse->DiagnosticInfos;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the ModifyMonitoredItems service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginModifyMonitoredItems(
    OpcUa_Channel                           a_hChannel,
    const OpcUa_RequestHeader*              a_pRequestHeader,
    OpcUa_UInt32                            a_nSubscriptionId,
    OpcUa_TimestampsToReturn                a_eTimestampsToReturn,
    OpcUa_Int32                             a_nNoOfItemsToModify,
    const OpcUa_MonitoredItemModifyRequest* a_pItemsToModify,
    OpcUa_Channel_PfnRequestComplete*       a_pCallback,
    OpcUa_Void*                             a_pCallbackData)
{
    OpcUa_ModifyMonitoredItemsRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginModifyMonitoredItems");

    OpcUa_ModifyMonitoredItemsRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nSubscriptionId);
    OpcUa_ReferenceParameter(a_eTimestampsToReturn);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfItemsToModify, a_pItemsToModify);

    /* copy parameters into request object. */
    cRequest.RequestHeader      = *a_pRequestHeader;
    cRequest.SubscriptionId     = a_nSubscriptionId;
    cRequest.TimestampsToReturn = a_eTimestampsToReturn;
    cRequest.NoOfItemsToModify  = a_nNoOfItemsToModify;
    cRequest.ItemsToModify      = (OpcUa_MonitoredItemModifyRequest*)a_pItemsToModify;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "ModifyMonitoredItems",
        (OpcUa_Void*)&cRequest,
        &OpcUa_ModifyMonitoredItemsRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_SetMonitoringMode
/*============================================================================
 * Synchronously calls the SetMonitoringMode service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_SetMonitoringMode(
    OpcUa_Channel              a_hChannel,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_UInt32               a_nSubscriptionId,
    OpcUa_MonitoringMode       a_eMonitoringMode,
    OpcUa_Int32                a_nNoOfMonitoredItemIds,
    const OpcUa_UInt32*        a_pMonitoredItemIds,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_Int32*               a_pNoOfResults,
    OpcUa_StatusCode**         a_pResults,
    OpcUa_Int32*               a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     a_pDiagnosticInfos)
{
    OpcUa_SetMonitoringModeRequest cRequest;
    OpcUa_SetMonitoringModeResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_SetMonitoringMode");

    OpcUa_SetMonitoringModeRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nSubscriptionId);
    OpcUa_ReferenceParameter(a_eMonitoringMode);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfMonitoredItemIds, a_pMonitoredItemIds);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    /* copy parameters into request object. */
    cRequest.RequestHeader        = *a_pRequestHeader;
    cRequest.SubscriptionId       = a_nSubscriptionId;
    cRequest.MonitoringMode       = a_eMonitoringMode;
    cRequest.NoOfMonitoredItemIds = a_nNoOfMonitoredItemIds;
    cRequest.MonitoredItemIds     = (OpcUa_UInt32*)a_pMonitoredItemIds;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "SetMonitoringMode",
        (OpcUa_Void*)&cRequest,
        &OpcUa_SetMonitoringModeRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_SetMonitoringModeResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader      = pResponse->ResponseHeader;
        *a_pNoOfResults         = pResponse->NoOfResults;
        *a_pResults             = pResponse->Results;
        *a_pNoOfDiagnosticInfos = pResponse->NoOfDiagnosticInfos;
        *a_pDiagnosticInfos     = pResponse->DiagnosticInfos;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the SetMonitoringMode service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginSetMonitoringMode(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_UInt32                      a_nSubscriptionId,
    OpcUa_MonitoringMode              a_eMonitoringMode,
    OpcUa_Int32                       a_nNoOfMonitoredItemIds,
    const OpcUa_UInt32*               a_pMonitoredItemIds,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_SetMonitoringModeRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginSetMonitoringMode");

    OpcUa_SetMonitoringModeRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nSubscriptionId);
    OpcUa_ReferenceParameter(a_eMonitoringMode);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfMonitoredItemIds, a_pMonitoredItemIds);

    /* copy parameters into request object. */
    cRequest.RequestHeader        = *a_pRequestHeader;
    cRequest.SubscriptionId       = a_nSubscriptionId;
    cRequest.MonitoringMode       = a_eMonitoringMode;
    cRequest.NoOfMonitoredItemIds = a_nNoOfMonitoredItemIds;
    cRequest.MonitoredItemIds     = (OpcUa_UInt32*)a_pMonitoredItemIds;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "SetMonitoringMode",
        (OpcUa_Void*)&cRequest,
        &OpcUa_SetMonitoringModeRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_SetTriggering
/*============================================================================
 * Synchronously calls the SetTriggering service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_SetTriggering(
    OpcUa_Channel              a_hChannel,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_UInt32               a_nSubscriptionId,
    OpcUa_UInt32               a_nTriggeringItemId,
    OpcUa_Int32                a_nNoOfLinksToAdd,
    const OpcUa_UInt32*        a_pLinksToAdd,
    OpcUa_Int32                a_nNoOfLinksToRemove,
    const OpcUa_UInt32*        a_pLinksToRemove,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_Int32*               a_pNoOfAddResults,
    OpcUa_StatusCode**         a_pAddResults,
    OpcUa_Int32*               a_pNoOfAddDiagnosticInfos,
    OpcUa_DiagnosticInfo**     a_pAddDiagnosticInfos,
    OpcUa_Int32*               a_pNoOfRemoveResults,
    OpcUa_StatusCode**         a_pRemoveResults,
    OpcUa_Int32*               a_pNoOfRemoveDiagnosticInfos,
    OpcUa_DiagnosticInfo**     a_pRemoveDiagnosticInfos)
{
    OpcUa_SetTriggeringRequest cRequest;
    OpcUa_SetTriggeringResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_SetTriggering");

    OpcUa_SetTriggeringRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nSubscriptionId);
    OpcUa_ReferenceParameter(a_nTriggeringItemId);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfLinksToAdd, a_pLinksToAdd);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfLinksToRemove, a_pLinksToRemove);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfAddResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pAddResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfAddDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pAddDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfRemoveResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pRemoveResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfRemoveDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pRemoveDiagnosticInfos);

    /* copy parameters into request object. */
    cRequest.RequestHeader     = *a_pRequestHeader;
    cRequest.SubscriptionId    = a_nSubscriptionId;
    cRequest.TriggeringItemId  = a_nTriggeringItemId;
    cRequest.NoOfLinksToAdd    = a_nNoOfLinksToAdd;
    cRequest.LinksToAdd        = (OpcUa_UInt32*)a_pLinksToAdd;
    cRequest.NoOfLinksToRemove = a_nNoOfLinksToRemove;
    cRequest.LinksToRemove     = (OpcUa_UInt32*)a_pLinksToRemove;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "SetTriggering",
        (OpcUa_Void*)&cRequest,
        &OpcUa_SetTriggeringRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_SetTriggeringResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader            = pResponse->ResponseHeader;
        *a_pNoOfAddResults            = pResponse->NoOfAddResults;
        *a_pAddResults                = pResponse->AddResults;
        *a_pNoOfAddDiagnosticInfos    = pResponse->NoOfAddDiagnosticInfos;
        *a_pAddDiagnosticInfos        = pResponse->AddDiagnosticInfos;
        *a_pNoOfRemoveResults         = pResponse->NoOfRemoveResults;
        *a_pRemoveResults             = pResponse->RemoveResults;
        *a_pNoOfRemoveDiagnosticInfos = pResponse->NoOfRemoveDiagnosticInfos;
        *a_pRemoveDiagnosticInfos     = pResponse->RemoveDiagnosticInfos;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the SetTriggering service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginSetTriggering(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_UInt32                      a_nSubscriptionId,
    OpcUa_UInt32                      a_nTriggeringItemId,
    OpcUa_Int32                       a_nNoOfLinksToAdd,
    const OpcUa_UInt32*               a_pLinksToAdd,
    OpcUa_Int32                       a_nNoOfLinksToRemove,
    const OpcUa_UInt32*               a_pLinksToRemove,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_SetTriggeringRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginSetTriggering");

    OpcUa_SetTriggeringRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nSubscriptionId);
    OpcUa_ReferenceParameter(a_nTriggeringItemId);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfLinksToAdd, a_pLinksToAdd);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfLinksToRemove, a_pLinksToRemove);

    /* copy parameters into request object. */
    cRequest.RequestHeader     = *a_pRequestHeader;
    cRequest.SubscriptionId    = a_nSubscriptionId;
    cRequest.TriggeringItemId  = a_nTriggeringItemId;
    cRequest.NoOfLinksToAdd    = a_nNoOfLinksToAdd;
    cRequest.LinksToAdd        = (OpcUa_UInt32*)a_pLinksToAdd;
    cRequest.NoOfLinksToRemove = a_nNoOfLinksToRemove;
    cRequest.LinksToRemove     = (OpcUa_UInt32*)a_pLinksToRemove;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "SetTriggering",
        (OpcUa_Void*)&cRequest,
        &OpcUa_SetTriggeringRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_DeleteMonitoredItems
/*============================================================================
 * Synchronously calls the DeleteMonitoredItems service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_DeleteMonitoredItems(
    OpcUa_Channel              a_hChannel,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_UInt32               a_nSubscriptionId,
    OpcUa_Int32                a_nNoOfMonitoredItemIds,
    const OpcUa_UInt32*        a_pMonitoredItemIds,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_Int32*               a_pNoOfResults,
    OpcUa_StatusCode**         a_pResults,
    OpcUa_Int32*               a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     a_pDiagnosticInfos)
{
    OpcUa_DeleteMonitoredItemsRequest cRequest;
    OpcUa_DeleteMonitoredItemsResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_DeleteMonitoredItems");

    OpcUa_DeleteMonitoredItemsRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nSubscriptionId);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfMonitoredItemIds, a_pMonitoredItemIds);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    /* copy parameters into request object. */
    cRequest.RequestHeader        = *a_pRequestHeader;
    cRequest.SubscriptionId       = a_nSubscriptionId;
    cRequest.NoOfMonitoredItemIds = a_nNoOfMonitoredItemIds;
    cRequest.MonitoredItemIds     = (OpcUa_UInt32*)a_pMonitoredItemIds;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "DeleteMonitoredItems",
        (OpcUa_Void*)&cRequest,
        &OpcUa_DeleteMonitoredItemsRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_DeleteMonitoredItemsResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader      = pResponse->ResponseHeader;
        *a_pNoOfResults         = pResponse->NoOfResults;
        *a_pResults             = pResponse->Results;
        *a_pNoOfDiagnosticInfos = pResponse->NoOfDiagnosticInfos;
        *a_pDiagnosticInfos     = pResponse->DiagnosticInfos;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the DeleteMonitoredItems service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginDeleteMonitoredItems(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_UInt32                      a_nSubscriptionId,
    OpcUa_Int32                       a_nNoOfMonitoredItemIds,
    const OpcUa_UInt32*               a_pMonitoredItemIds,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_DeleteMonitoredItemsRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginDeleteMonitoredItems");

    OpcUa_DeleteMonitoredItemsRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nSubscriptionId);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfMonitoredItemIds, a_pMonitoredItemIds);

    /* copy parameters into request object. */
    cRequest.RequestHeader        = *a_pRequestHeader;
    cRequest.SubscriptionId       = a_nSubscriptionId;
    cRequest.NoOfMonitoredItemIds = a_nNoOfMonitoredItemIds;
    cRequest.MonitoredItemIds     = (OpcUa_UInt32*)a_pMonitoredItemIds;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "DeleteMonitoredItems",
        (OpcUa_Void*)&cRequest,
        &OpcUa_DeleteMonitoredItemsRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_CreateSubscription
/*============================================================================
 * Synchronously calls the CreateSubscription service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_CreateSubscription(
    OpcUa_Channel              a_hChannel,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_Double               a_nRequestedPublishingInterval,
    OpcUa_UInt32               a_nRequestedLifetimeCount,
    OpcUa_UInt32               a_nRequestedMaxKeepAliveCount,
    OpcUa_UInt32               a_nMaxNotificationsPerPublish,
    OpcUa_Boolean              a_bPublishingEnabled,
    OpcUa_Byte                 a_nPriority,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_UInt32*              a_pSubscriptionId,
    OpcUa_Double*              a_pRevisedPublishingInterval,
    OpcUa_UInt32*              a_pRevisedLifetimeCount,
    OpcUa_UInt32*              a_pRevisedMaxKeepAliveCount)
{
    OpcUa_CreateSubscriptionRequest cRequest;
    OpcUa_CreateSubscriptionResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_CreateSubscription");

    OpcUa_CreateSubscriptionRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nRequestedPublishingInterval);
    OpcUa_ReferenceParameter(a_nRequestedLifetimeCount);
    OpcUa_ReferenceParameter(a_nRequestedMaxKeepAliveCount);
    OpcUa_ReferenceParameter(a_nMaxNotificationsPerPublish);
    OpcUa_ReferenceParameter(a_bPublishingEnabled);
    OpcUa_ReferenceParameter(a_nPriority);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pSubscriptionId);
    OpcUa_ReturnErrorIfArgumentNull(a_pRevisedPublishingInterval);
    OpcUa_ReturnErrorIfArgumentNull(a_pRevisedLifetimeCount);
    OpcUa_ReturnErrorIfArgumentNull(a_pRevisedMaxKeepAliveCount);

    /* copy parameters into request object. */
    cRequest.RequestHeader               = *a_pRequestHeader;
    cRequest.RequestedPublishingInterval = a_nRequestedPublishingInterval;
    cRequest.RequestedLifetimeCount      = a_nRequestedLifetimeCount;
    cRequest.RequestedMaxKeepAliveCount  = a_nRequestedMaxKeepAliveCount;
    cRequest.MaxNotificationsPerPublish  = a_nMaxNotificationsPerPublish;
    cRequest.PublishingEnabled           = a_bPublishingEnabled;
    cRequest.Priority                    = a_nPriority;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "CreateSubscription",
        (OpcUa_Void*)&cRequest,
        &OpcUa_CreateSubscriptionRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_CreateSubscriptionResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader            = pResponse->ResponseHeader;
        *a_pSubscriptionId            = pResponse->SubscriptionId;
        *a_pRevisedPublishingInterval = pResponse->RevisedPublishingInterval;
        *a_pRevisedLifetimeCount      = pResponse->RevisedLifetimeCount;
        *a_pRevisedMaxKeepAliveCount  = pResponse->RevisedMaxKeepAliveCount;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the CreateSubscription service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginCreateSubscription(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_Double                      a_nRequestedPublishingInterval,
    OpcUa_UInt32                      a_nRequestedLifetimeCount,
    OpcUa_UInt32                      a_nRequestedMaxKeepAliveCount,
    OpcUa_UInt32                      a_nMaxNotificationsPerPublish,
    OpcUa_Boolean                     a_bPublishingEnabled,
    OpcUa_Byte                        a_nPriority,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_CreateSubscriptionRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginCreateSubscription");

    OpcUa_CreateSubscriptionRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nRequestedPublishingInterval);
    OpcUa_ReferenceParameter(a_nRequestedLifetimeCount);
    OpcUa_ReferenceParameter(a_nRequestedMaxKeepAliveCount);
    OpcUa_ReferenceParameter(a_nMaxNotificationsPerPublish);
    OpcUa_ReferenceParameter(a_bPublishingEnabled);
    OpcUa_ReferenceParameter(a_nPriority);

    /* copy parameters into request object. */
    cRequest.RequestHeader               = *a_pRequestHeader;
    cRequest.RequestedPublishingInterval = a_nRequestedPublishingInterval;
    cRequest.RequestedLifetimeCount      = a_nRequestedLifetimeCount;
    cRequest.RequestedMaxKeepAliveCount  = a_nRequestedMaxKeepAliveCount;
    cRequest.MaxNotificationsPerPublish  = a_nMaxNotificationsPerPublish;
    cRequest.PublishingEnabled           = a_bPublishingEnabled;
    cRequest.Priority                    = a_nPriority;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "CreateSubscription",
        (OpcUa_Void*)&cRequest,
        &OpcUa_CreateSubscriptionRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_ModifySubscription
/*============================================================================
 * Synchronously calls the ModifySubscription service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_ModifySubscription(
    OpcUa_Channel              a_hChannel,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_UInt32               a_nSubscriptionId,
    OpcUa_Double               a_nRequestedPublishingInterval,
    OpcUa_UInt32               a_nRequestedLifetimeCount,
    OpcUa_UInt32               a_nRequestedMaxKeepAliveCount,
    OpcUa_UInt32               a_nMaxNotificationsPerPublish,
    OpcUa_Byte                 a_nPriority,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_Double*              a_pRevisedPublishingInterval,
    OpcUa_UInt32*              a_pRevisedLifetimeCount,
    OpcUa_UInt32*              a_pRevisedMaxKeepAliveCount)
{
    OpcUa_ModifySubscriptionRequest cRequest;
    OpcUa_ModifySubscriptionResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_ModifySubscription");

    OpcUa_ModifySubscriptionRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nSubscriptionId);
    OpcUa_ReferenceParameter(a_nRequestedPublishingInterval);
    OpcUa_ReferenceParameter(a_nRequestedLifetimeCount);
    OpcUa_ReferenceParameter(a_nRequestedMaxKeepAliveCount);
    OpcUa_ReferenceParameter(a_nMaxNotificationsPerPublish);
    OpcUa_ReferenceParameter(a_nPriority);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pRevisedPublishingInterval);
    OpcUa_ReturnErrorIfArgumentNull(a_pRevisedLifetimeCount);
    OpcUa_ReturnErrorIfArgumentNull(a_pRevisedMaxKeepAliveCount);

    /* copy parameters into request object. */
    cRequest.RequestHeader               = *a_pRequestHeader;
    cRequest.SubscriptionId              = a_nSubscriptionId;
    cRequest.RequestedPublishingInterval = a_nRequestedPublishingInterval;
    cRequest.RequestedLifetimeCount      = a_nRequestedLifetimeCount;
    cRequest.RequestedMaxKeepAliveCount  = a_nRequestedMaxKeepAliveCount;
    cRequest.MaxNotificationsPerPublish  = a_nMaxNotificationsPerPublish;
    cRequest.Priority                    = a_nPriority;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "ModifySubscription",
        (OpcUa_Void*)&cRequest,
        &OpcUa_ModifySubscriptionRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_ModifySubscriptionResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader            = pResponse->ResponseHeader;
        *a_pRevisedPublishingInterval = pResponse->RevisedPublishingInterval;
        *a_pRevisedLifetimeCount      = pResponse->RevisedLifetimeCount;
        *a_pRevisedMaxKeepAliveCount  = pResponse->RevisedMaxKeepAliveCount;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the ModifySubscription service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginModifySubscription(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_UInt32                      a_nSubscriptionId,
    OpcUa_Double                      a_nRequestedPublishingInterval,
    OpcUa_UInt32                      a_nRequestedLifetimeCount,
    OpcUa_UInt32                      a_nRequestedMaxKeepAliveCount,
    OpcUa_UInt32                      a_nMaxNotificationsPerPublish,
    OpcUa_Byte                        a_nPriority,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_ModifySubscriptionRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginModifySubscription");

    OpcUa_ModifySubscriptionRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nSubscriptionId);
    OpcUa_ReferenceParameter(a_nRequestedPublishingInterval);
    OpcUa_ReferenceParameter(a_nRequestedLifetimeCount);
    OpcUa_ReferenceParameter(a_nRequestedMaxKeepAliveCount);
    OpcUa_ReferenceParameter(a_nMaxNotificationsPerPublish);
    OpcUa_ReferenceParameter(a_nPriority);

    /* copy parameters into request object. */
    cRequest.RequestHeader               = *a_pRequestHeader;
    cRequest.SubscriptionId              = a_nSubscriptionId;
    cRequest.RequestedPublishingInterval = a_nRequestedPublishingInterval;
    cRequest.RequestedLifetimeCount      = a_nRequestedLifetimeCount;
    cRequest.RequestedMaxKeepAliveCount  = a_nRequestedMaxKeepAliveCount;
    cRequest.MaxNotificationsPerPublish  = a_nMaxNotificationsPerPublish;
    cRequest.Priority                    = a_nPriority;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "ModifySubscription",
        (OpcUa_Void*)&cRequest,
        &OpcUa_ModifySubscriptionRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_SetPublishingMode
/*============================================================================
 * Synchronously calls the SetPublishingMode service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_SetPublishingMode(
    OpcUa_Channel              a_hChannel,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_Boolean              a_bPublishingEnabled,
    OpcUa_Int32                a_nNoOfSubscriptionIds,
    const OpcUa_UInt32*        a_pSubscriptionIds,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_Int32*               a_pNoOfResults,
    OpcUa_StatusCode**         a_pResults,
    OpcUa_Int32*               a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     a_pDiagnosticInfos)
{
    OpcUa_SetPublishingModeRequest cRequest;
    OpcUa_SetPublishingModeResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_SetPublishingMode");

    OpcUa_SetPublishingModeRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_bPublishingEnabled);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfSubscriptionIds, a_pSubscriptionIds);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    /* copy parameters into request object. */
    cRequest.RequestHeader       = *a_pRequestHeader;
    cRequest.PublishingEnabled   = a_bPublishingEnabled;
    cRequest.NoOfSubscriptionIds = a_nNoOfSubscriptionIds;
    cRequest.SubscriptionIds     = (OpcUa_UInt32*)a_pSubscriptionIds;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "SetPublishingMode",
        (OpcUa_Void*)&cRequest,
        &OpcUa_SetPublishingModeRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_SetPublishingModeResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader      = pResponse->ResponseHeader;
        *a_pNoOfResults         = pResponse->NoOfResults;
        *a_pResults             = pResponse->Results;
        *a_pNoOfDiagnosticInfos = pResponse->NoOfDiagnosticInfos;
        *a_pDiagnosticInfos     = pResponse->DiagnosticInfos;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the SetPublishingMode service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginSetPublishingMode(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_Boolean                     a_bPublishingEnabled,
    OpcUa_Int32                       a_nNoOfSubscriptionIds,
    const OpcUa_UInt32*               a_pSubscriptionIds,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_SetPublishingModeRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginSetPublishingMode");

    OpcUa_SetPublishingModeRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_bPublishingEnabled);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfSubscriptionIds, a_pSubscriptionIds);

    /* copy parameters into request object. */
    cRequest.RequestHeader       = *a_pRequestHeader;
    cRequest.PublishingEnabled   = a_bPublishingEnabled;
    cRequest.NoOfSubscriptionIds = a_nNoOfSubscriptionIds;
    cRequest.SubscriptionIds     = (OpcUa_UInt32*)a_pSubscriptionIds;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "SetPublishingMode",
        (OpcUa_Void*)&cRequest,
        &OpcUa_SetPublishingModeRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_Publish
/*============================================================================
 * Synchronously calls the Publish service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_Publish(
    OpcUa_Channel                            a_hChannel,
    const OpcUa_RequestHeader*               a_pRequestHeader,
    OpcUa_Int32                              a_nNoOfSubscriptionAcknowledgements,
    const OpcUa_SubscriptionAcknowledgement* a_pSubscriptionAcknowledgements,
    OpcUa_ResponseHeader*                    a_pResponseHeader,
    OpcUa_UInt32*                            a_pSubscriptionId,
    OpcUa_Int32*                             a_pNoOfAvailableSequenceNumbers,
    OpcUa_UInt32**                           a_pAvailableSequenceNumbers,
    OpcUa_Boolean*                           a_pMoreNotifications,
    OpcUa_NotificationMessage*               a_pNotificationMessage,
    OpcUa_Int32*                             a_pNoOfResults,
    OpcUa_StatusCode**                       a_pResults,
    OpcUa_Int32*                             a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**                   a_pDiagnosticInfos)
{
    OpcUa_PublishRequest cRequest;
    OpcUa_PublishResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_Publish");

    OpcUa_PublishRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfSubscriptionAcknowledgements, a_pSubscriptionAcknowledgements);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pSubscriptionId);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfAvailableSequenceNumbers);
    OpcUa_ReturnErrorIfArgumentNull(a_pAvailableSequenceNumbers);
    OpcUa_ReturnErrorIfArgumentNull(a_pMoreNotifications);
    OpcUa_ReturnErrorIfArgumentNull(a_pNotificationMessage);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    /* copy parameters into request object. */
    cRequest.RequestHeader                    = *a_pRequestHeader;
    cRequest.NoOfSubscriptionAcknowledgements = a_nNoOfSubscriptionAcknowledgements;
    cRequest.SubscriptionAcknowledgements     = (OpcUa_SubscriptionAcknowledgement*)a_pSubscriptionAcknowledgements;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "Publish",
        (OpcUa_Void*)&cRequest,
        &OpcUa_PublishRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_PublishResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader               = pResponse->ResponseHeader;
        *a_pSubscriptionId               = pResponse->SubscriptionId;
        *a_pNoOfAvailableSequenceNumbers = pResponse->NoOfAvailableSequenceNumbers;
        *a_pAvailableSequenceNumbers     = pResponse->AvailableSequenceNumbers;
        *a_pMoreNotifications            = pResponse->MoreNotifications;
        *a_pNotificationMessage          = pResponse->NotificationMessage;
        *a_pNoOfResults                  = pResponse->NoOfResults;
        *a_pResults                      = pResponse->Results;
        *a_pNoOfDiagnosticInfos          = pResponse->NoOfDiagnosticInfos;
        *a_pDiagnosticInfos              = pResponse->DiagnosticInfos;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the Publish service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginPublish(
    OpcUa_Channel                            a_hChannel,
    const OpcUa_RequestHeader*               a_pRequestHeader,
    OpcUa_Int32                              a_nNoOfSubscriptionAcknowledgements,
    const OpcUa_SubscriptionAcknowledgement* a_pSubscriptionAcknowledgements,
    OpcUa_Channel_PfnRequestComplete*        a_pCallback,
    OpcUa_Void*                              a_pCallbackData)
{
    OpcUa_PublishRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginPublish");

    OpcUa_PublishRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfSubscriptionAcknowledgements, a_pSubscriptionAcknowledgements);

    /* copy parameters into request object. */
    cRequest.RequestHeader                    = *a_pRequestHeader;
    cRequest.NoOfSubscriptionAcknowledgements = a_nNoOfSubscriptionAcknowledgements;
    cRequest.SubscriptionAcknowledgements     = (OpcUa_SubscriptionAcknowledgement*)a_pSubscriptionAcknowledgements;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "Publish",
        (OpcUa_Void*)&cRequest,
        &OpcUa_PublishRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_Republish
/*============================================================================
 * Synchronously calls the Republish service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_Republish(
    OpcUa_Channel              a_hChannel,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_UInt32               a_nSubscriptionId,
    OpcUa_UInt32               a_nRetransmitSequenceNumber,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_NotificationMessage* a_pNotificationMessage)
{
    OpcUa_RepublishRequest cRequest;
    OpcUa_RepublishResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_Republish");

    OpcUa_RepublishRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nSubscriptionId);
    OpcUa_ReferenceParameter(a_nRetransmitSequenceNumber);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNotificationMessage);

    /* copy parameters into request object. */
    cRequest.RequestHeader            = *a_pRequestHeader;
    cRequest.SubscriptionId           = a_nSubscriptionId;
    cRequest.RetransmitSequenceNumber = a_nRetransmitSequenceNumber;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "Republish",
        (OpcUa_Void*)&cRequest,
        &OpcUa_RepublishRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_RepublishResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader      = pResponse->ResponseHeader;
        *a_pNotificationMessage = pResponse->NotificationMessage;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the Republish service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginRepublish(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_UInt32                      a_nSubscriptionId,
    OpcUa_UInt32                      a_nRetransmitSequenceNumber,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_RepublishRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginRepublish");

    OpcUa_RepublishRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nSubscriptionId);
    OpcUa_ReferenceParameter(a_nRetransmitSequenceNumber);

    /* copy parameters into request object. */
    cRequest.RequestHeader            = *a_pRequestHeader;
    cRequest.SubscriptionId           = a_nSubscriptionId;
    cRequest.RetransmitSequenceNumber = a_nRetransmitSequenceNumber;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "Republish",
        (OpcUa_Void*)&cRequest,
        &OpcUa_RepublishRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_TransferSubscriptions
/*============================================================================
 * Synchronously calls the TransferSubscriptions service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_TransferSubscriptions(
    OpcUa_Channel              a_hChannel,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_Int32                a_nNoOfSubscriptionIds,
    const OpcUa_UInt32*        a_pSubscriptionIds,
    OpcUa_Boolean              a_bSendInitialValues,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_Int32*               a_pNoOfResults,
    OpcUa_TransferResult**     a_pResults,
    OpcUa_Int32*               a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     a_pDiagnosticInfos)
{
    OpcUa_TransferSubscriptionsRequest cRequest;
    OpcUa_TransferSubscriptionsResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_TransferSubscriptions");

    OpcUa_TransferSubscriptionsRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfSubscriptionIds, a_pSubscriptionIds);
    OpcUa_ReferenceParameter(a_bSendInitialValues);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    /* copy parameters into request object. */
    cRequest.RequestHeader       = *a_pRequestHeader;
    cRequest.NoOfSubscriptionIds = a_nNoOfSubscriptionIds;
    cRequest.SubscriptionIds     = (OpcUa_UInt32*)a_pSubscriptionIds;
    cRequest.SendInitialValues   = a_bSendInitialValues;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "TransferSubscriptions",
        (OpcUa_Void*)&cRequest,
        &OpcUa_TransferSubscriptionsRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_TransferSubscriptionsResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader      = pResponse->ResponseHeader;
        *a_pNoOfResults         = pResponse->NoOfResults;
        *a_pResults             = pResponse->Results;
        *a_pNoOfDiagnosticInfos = pResponse->NoOfDiagnosticInfos;
        *a_pDiagnosticInfos     = pResponse->DiagnosticInfos;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the TransferSubscriptions service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginTransferSubscriptions(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_Int32                       a_nNoOfSubscriptionIds,
    const OpcUa_UInt32*               a_pSubscriptionIds,
    OpcUa_Boolean                     a_bSendInitialValues,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_TransferSubscriptionsRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginTransferSubscriptions");

    OpcUa_TransferSubscriptionsRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfSubscriptionIds, a_pSubscriptionIds);
    OpcUa_ReferenceParameter(a_bSendInitialValues);

    /* copy parameters into request object. */
    cRequest.RequestHeader       = *a_pRequestHeader;
    cRequest.NoOfSubscriptionIds = a_nNoOfSubscriptionIds;
    cRequest.SubscriptionIds     = (OpcUa_UInt32*)a_pSubscriptionIds;
    cRequest.SendInitialValues   = a_bSendInitialValues;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "TransferSubscriptions",
        (OpcUa_Void*)&cRequest,
        &OpcUa_TransferSubscriptionsRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#ifndef OPCUA_EXCLUDE_DeleteSubscriptions
/*============================================================================
 * Synchronously calls the DeleteSubscriptions service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_DeleteSubscriptions(
    OpcUa_Channel              a_hChannel,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_Int32                a_nNoOfSubscriptionIds,
    const OpcUa_UInt32*        a_pSubscriptionIds,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_Int32*               a_pNoOfResults,
    OpcUa_StatusCode**         a_pResults,
    OpcUa_Int32*               a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     a_pDiagnosticInfos)
{
    OpcUa_DeleteSubscriptionsRequest cRequest;
    OpcUa_DeleteSubscriptionsResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_DeleteSubscriptions");

    OpcUa_DeleteSubscriptionsRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfSubscriptionIds, a_pSubscriptionIds);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    /* copy parameters into request object. */
    cRequest.RequestHeader       = *a_pRequestHeader;
    cRequest.NoOfSubscriptionIds = a_nNoOfSubscriptionIds;
    cRequest.SubscriptionIds     = (OpcUa_UInt32*)a_pSubscriptionIds;

    /* invoke service */
    uStatus = OpcUa_Channel_InvokeService(
        a_hChannel,
        "DeleteSubscriptions",
        (OpcUa_Void*)&cRequest,
        &OpcUa_DeleteSubscriptionsRequest_EncodeableType,
        (OpcUa_Void**)&pResponse,
        &pResponseType);

    OpcUa_GotoErrorIfBad(uStatus);

    /* check for fault */
    if (pResponseType->TypeId == OpcUaId_ServiceFault)
    {
        *a_pResponseHeader = ((OpcUa_ServiceFault*)pResponse)->ResponseHeader;
        OpcUa_Free(pResponse);
        OpcUa_ReturnStatusCode;
    }

    /* check response type */
    else if (pResponseType->TypeId != OpcUaId_DeleteSubscriptionsResponse)
    {
        pResponseType->Clear(pResponse);
        OpcUa_GotoErrorWithStatus(OpcUa_BadUnknownResponse);
    }

    /* copy parameters from response object into return parameters. */
    else
    {
        *a_pResponseHeader      = pResponse->ResponseHeader;
        *a_pNoOfResults         = pResponse->NoOfResults;
        *a_pResults             = pResponse->Results;
        *a_pNoOfDiagnosticInfos = pResponse->NoOfDiagnosticInfos;
        *a_pDiagnosticInfos     = pResponse->DiagnosticInfos;
    }

    /* memory contained in the reponse objects is owned by the caller */
    OpcUa_Free(pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_Free(pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Asynchronously calls the DeleteSubscriptions service.
 *===========================================================================*/
OPCUA_EXPORT OpcUa_StatusCode OpcUa_ClientApi_BeginDeleteSubscriptions(
    OpcUa_Channel                     a_hChannel,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_Int32                       a_nNoOfSubscriptionIds,
    const OpcUa_UInt32*               a_pSubscriptionIds,
    OpcUa_Channel_PfnRequestComplete* a_pCallback,
    OpcUa_Void*                       a_pCallbackData)
{
    OpcUa_DeleteSubscriptionsRequest cRequest;

    OpcUa_InitializeStatus(OpcUa_Module_Client, "OpcUa_ClientApi_BeginDeleteSubscriptions");

    OpcUa_DeleteSubscriptionsRequest_Initialize(&cRequest);

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfSubscriptionIds, a_pSubscriptionIds);

    /* copy parameters into request object. */
    cRequest.RequestHeader       = *a_pRequestHeader;
    cRequest.NoOfSubscriptionIds = a_nNoOfSubscriptionIds;
    cRequest.SubscriptionIds     = (OpcUa_UInt32*)a_pSubscriptionIds;

    /* begin invoke service */
    uStatus = OpcUa_Channel_BeginInvokeService(
        a_hChannel,
        "DeleteSubscriptions",
        (OpcUa_Void*)&cRequest,
        &OpcUa_DeleteSubscriptionsRequest_EncodeableType,
        (OpcUa_Channel_PfnRequestComplete*)a_pCallback,
        a_pCallbackData);

    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}
#endif

#endif /* OPCUA_HAVE_CLIENTAPI */
/* This is the last line of an autogenerated file. */
