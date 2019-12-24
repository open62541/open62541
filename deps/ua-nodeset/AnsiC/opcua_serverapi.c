/* ========================================================================
 * Copyright (c) 2005-2019 The OPC Foundation, Inc. All rights reserved.
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

/* base */
#include <opcua.h>

#ifdef OPCUA_HAVE_SERVERAPI

/* types */
#include <opcua_types.h>
#include <opcua_builtintypes.h>
#include <opcua_extensionobject.h>
#include <opcua_encodeableobject.h>
#include <opcua_identifiers.h>

/* server related */
#include <opcua_endpoint.h>
#include <opcua_servicetable.h>
#include <opcua_serverapi.h>

#ifndef OPCUA_EXCLUDE_FindServers
/*============================================================================
 * A pointer to a function that implements the FindServers service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnFindServers)(
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
 * A stub method which implements the FindServers service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_FindServers(
    OpcUa_Endpoint                 a_hEndpoint,
    OpcUa_Handle                   a_hContext,
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
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_FindServers");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pEndpointUrl);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfLocaleIds, a_pLocaleIds);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfServerUris, a_pServerUris);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfServers);
    OpcUa_ReturnErrorIfArgumentNull(a_pServers);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a FindServers service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginFindServers(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_FindServersRequest* pRequest = OpcUa_Null;
    OpcUa_FindServersResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnFindServers* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginFindServers");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_FindServersRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_FindServersRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        &pRequest->EndpointUrl,
        pRequest->NoOfLocaleIds,
        pRequest->LocaleIds,
        pRequest->NoOfServerUris,
        pRequest->ServerUris,
        &pResponse->ResponseHeader,
        &pResponse->NoOfServers,
        &pResponse->Servers);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_FindServersResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information FindServers service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_FindServers_ServiceType =
{
    OpcUaId_FindServersRequest,
    &OpcUa_FindServersResponse_EncodeableType,
    OpcUa_Server_BeginFindServers,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_FindServers
};
#endif

#ifndef OPCUA_EXCLUDE_FindServersOnNetwork
/*============================================================================
 * A pointer to a function that implements the FindServersOnNetwork service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnFindServersOnNetwork)(
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
 * A stub method which implements the FindServersOnNetwork service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_FindServersOnNetwork(
    OpcUa_Endpoint             a_hEndpoint,
    OpcUa_Handle               a_hContext,
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
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_FindServersOnNetwork");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nStartingRecordId);
    OpcUa_ReferenceParameter(a_nMaxRecordsToReturn);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfServerCapabilityFilter, a_pServerCapabilityFilter);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pLastCounterResetTime);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfServers);
    OpcUa_ReturnErrorIfArgumentNull(a_pServers);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a FindServersOnNetwork service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginFindServersOnNetwork(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_FindServersOnNetworkRequest* pRequest = OpcUa_Null;
    OpcUa_FindServersOnNetworkResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnFindServersOnNetwork* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginFindServersOnNetwork");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_FindServersOnNetworkRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_FindServersOnNetworkRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->StartingRecordId,
        pRequest->MaxRecordsToReturn,
        pRequest->NoOfServerCapabilityFilter,
        pRequest->ServerCapabilityFilter,
        &pResponse->ResponseHeader,
        &pResponse->LastCounterResetTime,
        &pResponse->NoOfServers,
        &pResponse->Servers);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_FindServersOnNetworkResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information FindServersOnNetwork service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_FindServersOnNetwork_ServiceType =
{
    OpcUaId_FindServersOnNetworkRequest,
    &OpcUa_FindServersOnNetworkResponse_EncodeableType,
    OpcUa_Server_BeginFindServersOnNetwork,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_FindServersOnNetwork
};
#endif

#ifndef OPCUA_EXCLUDE_GetEndpoints
/*============================================================================
 * A pointer to a function that implements the GetEndpoints service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnGetEndpoints)(
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
 * A stub method which implements the GetEndpoints service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_GetEndpoints(
    OpcUa_Endpoint              a_hEndpoint,
    OpcUa_Handle                a_hContext,
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
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_GetEndpoints");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pEndpointUrl);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfLocaleIds, a_pLocaleIds);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfProfileUris, a_pProfileUris);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfEndpoints);
    OpcUa_ReturnErrorIfArgumentNull(a_pEndpoints);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a GetEndpoints service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginGetEndpoints(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_GetEndpointsRequest* pRequest = OpcUa_Null;
    OpcUa_GetEndpointsResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnGetEndpoints* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginGetEndpoints");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_GetEndpointsRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_GetEndpointsRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        &pRequest->EndpointUrl,
        pRequest->NoOfLocaleIds,
        pRequest->LocaleIds,
        pRequest->NoOfProfileUris,
        pRequest->ProfileUris,
        &pResponse->ResponseHeader,
        &pResponse->NoOfEndpoints,
        &pResponse->Endpoints);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_GetEndpointsResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information GetEndpoints service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_GetEndpoints_ServiceType =
{
    OpcUaId_GetEndpointsRequest,
    &OpcUa_GetEndpointsResponse_EncodeableType,
    OpcUa_Server_BeginGetEndpoints,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_GetEndpoints
};
#endif

#ifndef OPCUA_EXCLUDE_RegisterServer
/*============================================================================
 * A pointer to a function that implements the RegisterServer service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnRegisterServer)(
    OpcUa_Endpoint                hEndpoint,
    OpcUa_Handle                  hContext,
    const OpcUa_RequestHeader*    pRequestHeader,
    const OpcUa_RegisteredServer* pServer,
    OpcUa_ResponseHeader*         pResponseHeader);

/*============================================================================
 * A stub method which implements the RegisterServer service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_RegisterServer(
    OpcUa_Endpoint                a_hEndpoint,
    OpcUa_Handle                  a_hContext,
    const OpcUa_RequestHeader*    a_pRequestHeader,
    const OpcUa_RegisteredServer* a_pServer,
    OpcUa_ResponseHeader*         a_pResponseHeader)
{
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_RegisterServer");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pServer);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a RegisterServer service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginRegisterServer(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_RegisterServerRequest* pRequest = OpcUa_Null;
    OpcUa_RegisterServerResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnRegisterServer* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginRegisterServer");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_RegisterServerRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_RegisterServerRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        &pRequest->Server,
        &pResponse->ResponseHeader);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_RegisterServerResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information RegisterServer service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_RegisterServer_ServiceType =
{
    OpcUaId_RegisterServerRequest,
    &OpcUa_RegisterServerResponse_EncodeableType,
    OpcUa_Server_BeginRegisterServer,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_RegisterServer
};
#endif

#ifndef OPCUA_EXCLUDE_RegisterServer2
/*============================================================================
 * A pointer to a function that implements the RegisterServer2 service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnRegisterServer2)(
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
 * A stub method which implements the RegisterServer2 service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_RegisterServer2(
    OpcUa_Endpoint                a_hEndpoint,
    OpcUa_Handle                  a_hContext,
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
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_RegisterServer2");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pServer);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfDiscoveryConfiguration, a_pDiscoveryConfiguration);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfConfigurationResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pConfigurationResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a RegisterServer2 service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginRegisterServer2(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_RegisterServer2Request* pRequest = OpcUa_Null;
    OpcUa_RegisterServer2Response* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnRegisterServer2* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginRegisterServer2");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_RegisterServer2Request, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_RegisterServer2Request*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        &pRequest->Server,
        pRequest->NoOfDiscoveryConfiguration,
        pRequest->DiscoveryConfiguration,
        &pResponse->ResponseHeader,
        &pResponse->NoOfConfigurationResults,
        &pResponse->ConfigurationResults,
        &pResponse->NoOfDiagnosticInfos,
        &pResponse->DiagnosticInfos);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_RegisterServer2Response*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information RegisterServer2 service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_RegisterServer2_ServiceType =
{
    OpcUaId_RegisterServer2Request,
    &OpcUa_RegisterServer2Response_EncodeableType,
    OpcUa_Server_BeginRegisterServer2,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_RegisterServer2
};
#endif

#ifndef OPCUA_EXCLUDE_CreateSession
/*============================================================================
 * A pointer to a function that implements the CreateSession service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnCreateSession)(
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
 * A stub method which implements the CreateSession service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_CreateSession(
    OpcUa_Endpoint                      a_hEndpoint,
    OpcUa_Handle                        a_hContext,
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
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_CreateSession");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
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

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a CreateSession service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginCreateSession(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_CreateSessionRequest* pRequest = OpcUa_Null;
    OpcUa_CreateSessionResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnCreateSession* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginCreateSession");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_CreateSessionRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_CreateSessionRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        &pRequest->ClientDescription,
        &pRequest->ServerUri,
        &pRequest->EndpointUrl,
        &pRequest->SessionName,
        &pRequest->ClientNonce,
        &pRequest->ClientCertificate,
        pRequest->RequestedSessionTimeout,
        pRequest->MaxResponseMessageSize,
        &pResponse->ResponseHeader,
        &pResponse->SessionId,
        &pResponse->AuthenticationToken,
        &pResponse->RevisedSessionTimeout,
        &pResponse->ServerNonce,
        &pResponse->ServerCertificate,
        &pResponse->NoOfServerEndpoints,
        &pResponse->ServerEndpoints,
        &pResponse->NoOfServerSoftwareCertificates,
        &pResponse->ServerSoftwareCertificates,
        &pResponse->ServerSignature,
        &pResponse->MaxRequestMessageSize);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_CreateSessionResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information CreateSession service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_CreateSession_ServiceType =
{
    OpcUaId_CreateSessionRequest,
    &OpcUa_CreateSessionResponse_EncodeableType,
    OpcUa_Server_BeginCreateSession,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_CreateSession
};
#endif

#ifndef OPCUA_EXCLUDE_ActivateSession
/*============================================================================
 * A pointer to a function that implements the ActivateSession service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnActivateSession)(
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
 * A stub method which implements the ActivateSession service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_ActivateSession(
    OpcUa_Endpoint                         a_hEndpoint,
    OpcUa_Handle                           a_hContext,
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
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_ActivateSession");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
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

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a ActivateSession service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginActivateSession(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_ActivateSessionRequest* pRequest = OpcUa_Null;
    OpcUa_ActivateSessionResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnActivateSession* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginActivateSession");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_ActivateSessionRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_ActivateSessionRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        &pRequest->ClientSignature,
        pRequest->NoOfClientSoftwareCertificates,
        pRequest->ClientSoftwareCertificates,
        pRequest->NoOfLocaleIds,
        pRequest->LocaleIds,
        &pRequest->UserIdentityToken,
        &pRequest->UserTokenSignature,
        &pResponse->ResponseHeader,
        &pResponse->ServerNonce,
        &pResponse->NoOfResults,
        &pResponse->Results,
        &pResponse->NoOfDiagnosticInfos,
        &pResponse->DiagnosticInfos);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_ActivateSessionResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information ActivateSession service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_ActivateSession_ServiceType =
{
    OpcUaId_ActivateSessionRequest,
    &OpcUa_ActivateSessionResponse_EncodeableType,
    OpcUa_Server_BeginActivateSession,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_ActivateSession
};
#endif

#ifndef OPCUA_EXCLUDE_CloseSession
/*============================================================================
 * A pointer to a function that implements the CloseSession service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnCloseSession)(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Boolean              bDeleteSubscriptions,
    OpcUa_ResponseHeader*      pResponseHeader);

/*============================================================================
 * A stub method which implements the CloseSession service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_CloseSession(
    OpcUa_Endpoint             a_hEndpoint,
    OpcUa_Handle               a_hContext,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_Boolean              a_bDeleteSubscriptions,
    OpcUa_ResponseHeader*      a_pResponseHeader)
{
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_CloseSession");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_bDeleteSubscriptions);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a CloseSession service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginCloseSession(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_CloseSessionRequest* pRequest = OpcUa_Null;
    OpcUa_CloseSessionResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnCloseSession* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginCloseSession");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_CloseSessionRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_CloseSessionRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->DeleteSubscriptions,
        &pResponse->ResponseHeader);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_CloseSessionResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information CloseSession service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_CloseSession_ServiceType =
{
    OpcUaId_CloseSessionRequest,
    &OpcUa_CloseSessionResponse_EncodeableType,
    OpcUa_Server_BeginCloseSession,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_CloseSession
};
#endif

#ifndef OPCUA_EXCLUDE_Cancel
/*============================================================================
 * A pointer to a function that implements the Cancel service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnCancel)(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_UInt32               nRequestHandle,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_UInt32*              pCancelCount);

/*============================================================================
 * A stub method which implements the Cancel service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_Cancel(
    OpcUa_Endpoint             a_hEndpoint,
    OpcUa_Handle               a_hContext,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_UInt32               a_nRequestHandle,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_UInt32*              a_pCancelCount)
{
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_Cancel");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nRequestHandle);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pCancelCount);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a Cancel service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginCancel(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_CancelRequest* pRequest = OpcUa_Null;
    OpcUa_CancelResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnCancel* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginCancel");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_CancelRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_CancelRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->RequestHandle,
        &pResponse->ResponseHeader,
        &pResponse->CancelCount);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_CancelResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information Cancel service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_Cancel_ServiceType =
{
    OpcUaId_CancelRequest,
    &OpcUa_CancelResponse_EncodeableType,
    OpcUa_Server_BeginCancel,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_Cancel
};
#endif

#ifndef OPCUA_EXCLUDE_AddNodes
/*============================================================================
 * A pointer to a function that implements the AddNodes service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnAddNodes)(
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
 * A stub method which implements the AddNodes service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_AddNodes(
    OpcUa_Endpoint             a_hEndpoint,
    OpcUa_Handle               a_hContext,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_Int32                a_nNoOfNodesToAdd,
    const OpcUa_AddNodesItem*  a_pNodesToAdd,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_Int32*               a_pNoOfResults,
    OpcUa_AddNodesResult**     a_pResults,
    OpcUa_Int32*               a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     a_pDiagnosticInfos)
{
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_AddNodes");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodesToAdd, a_pNodesToAdd);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a AddNodes service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginAddNodes(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_AddNodesRequest* pRequest = OpcUa_Null;
    OpcUa_AddNodesResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnAddNodes* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginAddNodes");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_AddNodesRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_AddNodesRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->NoOfNodesToAdd,
        pRequest->NodesToAdd,
        &pResponse->ResponseHeader,
        &pResponse->NoOfResults,
        &pResponse->Results,
        &pResponse->NoOfDiagnosticInfos,
        &pResponse->DiagnosticInfos);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_AddNodesResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information AddNodes service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_AddNodes_ServiceType =
{
    OpcUaId_AddNodesRequest,
    &OpcUa_AddNodesResponse_EncodeableType,
    OpcUa_Server_BeginAddNodes,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_AddNodes
};
#endif

#ifndef OPCUA_EXCLUDE_AddReferences
/*============================================================================
 * A pointer to a function that implements the AddReferences service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnAddReferences)(
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
 * A stub method which implements the AddReferences service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_AddReferences(
    OpcUa_Endpoint                 a_hEndpoint,
    OpcUa_Handle                   a_hContext,
    const OpcUa_RequestHeader*     a_pRequestHeader,
    OpcUa_Int32                    a_nNoOfReferencesToAdd,
    const OpcUa_AddReferencesItem* a_pReferencesToAdd,
    OpcUa_ResponseHeader*          a_pResponseHeader,
    OpcUa_Int32*                   a_pNoOfResults,
    OpcUa_StatusCode**             a_pResults,
    OpcUa_Int32*                   a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**         a_pDiagnosticInfos)
{
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_AddReferences");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfReferencesToAdd, a_pReferencesToAdd);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a AddReferences service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginAddReferences(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_AddReferencesRequest* pRequest = OpcUa_Null;
    OpcUa_AddReferencesResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnAddReferences* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginAddReferences");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_AddReferencesRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_AddReferencesRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->NoOfReferencesToAdd,
        pRequest->ReferencesToAdd,
        &pResponse->ResponseHeader,
        &pResponse->NoOfResults,
        &pResponse->Results,
        &pResponse->NoOfDiagnosticInfos,
        &pResponse->DiagnosticInfos);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_AddReferencesResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information AddReferences service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_AddReferences_ServiceType =
{
    OpcUaId_AddReferencesRequest,
    &OpcUa_AddReferencesResponse_EncodeableType,
    OpcUa_Server_BeginAddReferences,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_AddReferences
};
#endif

#ifndef OPCUA_EXCLUDE_DeleteNodes
/*============================================================================
 * A pointer to a function that implements the DeleteNodes service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnDeleteNodes)(
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
 * A stub method which implements the DeleteNodes service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_DeleteNodes(
    OpcUa_Endpoint               a_hEndpoint,
    OpcUa_Handle                 a_hContext,
    const OpcUa_RequestHeader*   a_pRequestHeader,
    OpcUa_Int32                  a_nNoOfNodesToDelete,
    const OpcUa_DeleteNodesItem* a_pNodesToDelete,
    OpcUa_ResponseHeader*        a_pResponseHeader,
    OpcUa_Int32*                 a_pNoOfResults,
    OpcUa_StatusCode**           a_pResults,
    OpcUa_Int32*                 a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**       a_pDiagnosticInfos)
{
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_DeleteNodes");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodesToDelete, a_pNodesToDelete);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a DeleteNodes service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginDeleteNodes(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_DeleteNodesRequest* pRequest = OpcUa_Null;
    OpcUa_DeleteNodesResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnDeleteNodes* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginDeleteNodes");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_DeleteNodesRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_DeleteNodesRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->NoOfNodesToDelete,
        pRequest->NodesToDelete,
        &pResponse->ResponseHeader,
        &pResponse->NoOfResults,
        &pResponse->Results,
        &pResponse->NoOfDiagnosticInfos,
        &pResponse->DiagnosticInfos);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_DeleteNodesResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information DeleteNodes service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_DeleteNodes_ServiceType =
{
    OpcUaId_DeleteNodesRequest,
    &OpcUa_DeleteNodesResponse_EncodeableType,
    OpcUa_Server_BeginDeleteNodes,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_DeleteNodes
};
#endif

#ifndef OPCUA_EXCLUDE_DeleteReferences
/*============================================================================
 * A pointer to a function that implements the DeleteReferences service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnDeleteReferences)(
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
 * A stub method which implements the DeleteReferences service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_DeleteReferences(
    OpcUa_Endpoint                    a_hEndpoint,
    OpcUa_Handle                      a_hContext,
    const OpcUa_RequestHeader*        a_pRequestHeader,
    OpcUa_Int32                       a_nNoOfReferencesToDelete,
    const OpcUa_DeleteReferencesItem* a_pReferencesToDelete,
    OpcUa_ResponseHeader*             a_pResponseHeader,
    OpcUa_Int32*                      a_pNoOfResults,
    OpcUa_StatusCode**                a_pResults,
    OpcUa_Int32*                      a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**            a_pDiagnosticInfos)
{
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_DeleteReferences");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfReferencesToDelete, a_pReferencesToDelete);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a DeleteReferences service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginDeleteReferences(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_DeleteReferencesRequest* pRequest = OpcUa_Null;
    OpcUa_DeleteReferencesResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnDeleteReferences* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginDeleteReferences");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_DeleteReferencesRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_DeleteReferencesRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->NoOfReferencesToDelete,
        pRequest->ReferencesToDelete,
        &pResponse->ResponseHeader,
        &pResponse->NoOfResults,
        &pResponse->Results,
        &pResponse->NoOfDiagnosticInfos,
        &pResponse->DiagnosticInfos);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_DeleteReferencesResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information DeleteReferences service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_DeleteReferences_ServiceType =
{
    OpcUaId_DeleteReferencesRequest,
    &OpcUa_DeleteReferencesResponse_EncodeableType,
    OpcUa_Server_BeginDeleteReferences,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_DeleteReferences
};
#endif

#ifndef OPCUA_EXCLUDE_Browse
/*============================================================================
 * A pointer to a function that implements the Browse service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnBrowse)(
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
 * A stub method which implements the Browse service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_Browse(
    OpcUa_Endpoint                 a_hEndpoint,
    OpcUa_Handle                   a_hContext,
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
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_Browse");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pView);
    OpcUa_ReferenceParameter(a_nRequestedMaxReferencesPerNode);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodesToBrowse, a_pNodesToBrowse);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a Browse service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginBrowse(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_BrowseRequest* pRequest = OpcUa_Null;
    OpcUa_BrowseResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnBrowse* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginBrowse");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_BrowseRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_BrowseRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        &pRequest->View,
        pRequest->RequestedMaxReferencesPerNode,
        pRequest->NoOfNodesToBrowse,
        pRequest->NodesToBrowse,
        &pResponse->ResponseHeader,
        &pResponse->NoOfResults,
        &pResponse->Results,
        &pResponse->NoOfDiagnosticInfos,
        &pResponse->DiagnosticInfos);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_BrowseResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information Browse service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_Browse_ServiceType =
{
    OpcUaId_BrowseRequest,
    &OpcUa_BrowseResponse_EncodeableType,
    OpcUa_Server_BeginBrowse,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_Browse
};
#endif

#ifndef OPCUA_EXCLUDE_BrowseNext
/*============================================================================
 * A pointer to a function that implements the BrowseNext service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnBrowseNext)(
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
 * A stub method which implements the BrowseNext service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_BrowseNext(
    OpcUa_Endpoint             a_hEndpoint,
    OpcUa_Handle               a_hContext,
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
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_BrowseNext");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_bReleaseContinuationPoints);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfContinuationPoints, a_pContinuationPoints);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a BrowseNext service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginBrowseNext(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_BrowseNextRequest* pRequest = OpcUa_Null;
    OpcUa_BrowseNextResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnBrowseNext* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginBrowseNext");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_BrowseNextRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_BrowseNextRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->ReleaseContinuationPoints,
        pRequest->NoOfContinuationPoints,
        pRequest->ContinuationPoints,
        &pResponse->ResponseHeader,
        &pResponse->NoOfResults,
        &pResponse->Results,
        &pResponse->NoOfDiagnosticInfos,
        &pResponse->DiagnosticInfos);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_BrowseNextResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information BrowseNext service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_BrowseNext_ServiceType =
{
    OpcUaId_BrowseNextRequest,
    &OpcUa_BrowseNextResponse_EncodeableType,
    OpcUa_Server_BeginBrowseNext,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_BrowseNext
};
#endif

#ifndef OPCUA_EXCLUDE_TranslateBrowsePathsToNodeIds
/*============================================================================
 * A pointer to a function that implements the TranslateBrowsePathsToNodeIds service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnTranslateBrowsePathsToNodeIds)(
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
 * A stub method which implements the TranslateBrowsePathsToNodeIds service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_TranslateBrowsePathsToNodeIds(
    OpcUa_Endpoint             a_hEndpoint,
    OpcUa_Handle               a_hContext,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_Int32                a_nNoOfBrowsePaths,
    const OpcUa_BrowsePath*    a_pBrowsePaths,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_Int32*               a_pNoOfResults,
    OpcUa_BrowsePathResult**   a_pResults,
    OpcUa_Int32*               a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     a_pDiagnosticInfos)
{
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_TranslateBrowsePathsToNodeIds");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfBrowsePaths, a_pBrowsePaths);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a TranslateBrowsePathsToNodeIds service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginTranslateBrowsePathsToNodeIds(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_TranslateBrowsePathsToNodeIdsRequest* pRequest = OpcUa_Null;
    OpcUa_TranslateBrowsePathsToNodeIdsResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnTranslateBrowsePathsToNodeIds* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginTranslateBrowsePathsToNodeIds");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_TranslateBrowsePathsToNodeIdsRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_TranslateBrowsePathsToNodeIdsRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->NoOfBrowsePaths,
        pRequest->BrowsePaths,
        &pResponse->ResponseHeader,
        &pResponse->NoOfResults,
        &pResponse->Results,
        &pResponse->NoOfDiagnosticInfos,
        &pResponse->DiagnosticInfos);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_TranslateBrowsePathsToNodeIdsResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information TranslateBrowsePathsToNodeIds service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_TranslateBrowsePathsToNodeIds_ServiceType =
{
    OpcUaId_TranslateBrowsePathsToNodeIdsRequest,
    &OpcUa_TranslateBrowsePathsToNodeIdsResponse_EncodeableType,
    OpcUa_Server_BeginTranslateBrowsePathsToNodeIds,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_TranslateBrowsePathsToNodeIds
};
#endif

#ifndef OPCUA_EXCLUDE_RegisterNodes
/*============================================================================
 * A pointer to a function that implements the RegisterNodes service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnRegisterNodes)(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Int32                nNoOfNodesToRegister,
    const OpcUa_NodeId*        pNodesToRegister,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_Int32*               pNoOfRegisteredNodeIds,
    OpcUa_NodeId**             pRegisteredNodeIds);

/*============================================================================
 * A stub method which implements the RegisterNodes service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_RegisterNodes(
    OpcUa_Endpoint             a_hEndpoint,
    OpcUa_Handle               a_hContext,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_Int32                a_nNoOfNodesToRegister,
    const OpcUa_NodeId*        a_pNodesToRegister,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_Int32*               a_pNoOfRegisteredNodeIds,
    OpcUa_NodeId**             a_pRegisteredNodeIds)
{
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_RegisterNodes");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodesToRegister, a_pNodesToRegister);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfRegisteredNodeIds);
    OpcUa_ReturnErrorIfArgumentNull(a_pRegisteredNodeIds);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a RegisterNodes service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginRegisterNodes(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_RegisterNodesRequest* pRequest = OpcUa_Null;
    OpcUa_RegisterNodesResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnRegisterNodes* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginRegisterNodes");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_RegisterNodesRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_RegisterNodesRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->NoOfNodesToRegister,
        pRequest->NodesToRegister,
        &pResponse->ResponseHeader,
        &pResponse->NoOfRegisteredNodeIds,
        &pResponse->RegisteredNodeIds);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_RegisterNodesResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information RegisterNodes service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_RegisterNodes_ServiceType =
{
    OpcUaId_RegisterNodesRequest,
    &OpcUa_RegisterNodesResponse_EncodeableType,
    OpcUa_Server_BeginRegisterNodes,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_RegisterNodes
};
#endif

#ifndef OPCUA_EXCLUDE_UnregisterNodes
/*============================================================================
 * A pointer to a function that implements the UnregisterNodes service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnUnregisterNodes)(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_Int32                nNoOfNodesToUnregister,
    const OpcUa_NodeId*        pNodesToUnregister,
    OpcUa_ResponseHeader*      pResponseHeader);

/*============================================================================
 * A stub method which implements the UnregisterNodes service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_UnregisterNodes(
    OpcUa_Endpoint             a_hEndpoint,
    OpcUa_Handle               a_hContext,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_Int32                a_nNoOfNodesToUnregister,
    const OpcUa_NodeId*        a_pNodesToUnregister,
    OpcUa_ResponseHeader*      a_pResponseHeader)
{
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_UnregisterNodes");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodesToUnregister, a_pNodesToUnregister);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a UnregisterNodes service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginUnregisterNodes(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_UnregisterNodesRequest* pRequest = OpcUa_Null;
    OpcUa_UnregisterNodesResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnUnregisterNodes* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginUnregisterNodes");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_UnregisterNodesRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_UnregisterNodesRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->NoOfNodesToUnregister,
        pRequest->NodesToUnregister,
        &pResponse->ResponseHeader);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_UnregisterNodesResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information UnregisterNodes service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_UnregisterNodes_ServiceType =
{
    OpcUaId_UnregisterNodesRequest,
    &OpcUa_UnregisterNodesResponse_EncodeableType,
    OpcUa_Server_BeginUnregisterNodes,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_UnregisterNodes
};
#endif

#ifndef OPCUA_EXCLUDE_QueryFirst
/*============================================================================
 * A pointer to a function that implements the QueryFirst service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnQueryFirst)(
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
 * A stub method which implements the QueryFirst service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_QueryFirst(
    OpcUa_Endpoint                   a_hEndpoint,
    OpcUa_Handle                     a_hContext,
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
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_QueryFirst");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
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

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a QueryFirst service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginQueryFirst(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_QueryFirstRequest* pRequest = OpcUa_Null;
    OpcUa_QueryFirstResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnQueryFirst* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginQueryFirst");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_QueryFirstRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_QueryFirstRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        &pRequest->View,
        pRequest->NoOfNodeTypes,
        pRequest->NodeTypes,
        &pRequest->Filter,
        pRequest->MaxDataSetsToReturn,
        pRequest->MaxReferencesToReturn,
        &pResponse->ResponseHeader,
        &pResponse->NoOfQueryDataSets,
        &pResponse->QueryDataSets,
        &pResponse->ContinuationPoint,
        &pResponse->NoOfParsingResults,
        &pResponse->ParsingResults,
        &pResponse->NoOfDiagnosticInfos,
        &pResponse->DiagnosticInfos,
        &pResponse->FilterResult);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_QueryFirstResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information QueryFirst service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_QueryFirst_ServiceType =
{
    OpcUaId_QueryFirstRequest,
    &OpcUa_QueryFirstResponse_EncodeableType,
    OpcUa_Server_BeginQueryFirst,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_QueryFirst
};
#endif

#ifndef OPCUA_EXCLUDE_QueryNext
/*============================================================================
 * A pointer to a function that implements the QueryNext service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnQueryNext)(
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
 * A stub method which implements the QueryNext service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_QueryNext(
    OpcUa_Endpoint             a_hEndpoint,
    OpcUa_Handle               a_hContext,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_Boolean              a_bReleaseContinuationPoint,
    const OpcUa_ByteString*    a_pContinuationPoint,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_Int32*               a_pNoOfQueryDataSets,
    OpcUa_QueryDataSet**       a_pQueryDataSets,
    OpcUa_ByteString*          a_pRevisedContinuationPoint)
{
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_QueryNext");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_bReleaseContinuationPoint);
    OpcUa_ReturnErrorIfArgumentNull(a_pContinuationPoint);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfQueryDataSets);
    OpcUa_ReturnErrorIfArgumentNull(a_pQueryDataSets);
    OpcUa_ReturnErrorIfArgumentNull(a_pRevisedContinuationPoint);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a QueryNext service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginQueryNext(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_QueryNextRequest* pRequest = OpcUa_Null;
    OpcUa_QueryNextResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnQueryNext* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginQueryNext");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_QueryNextRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_QueryNextRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->ReleaseContinuationPoint,
        &pRequest->ContinuationPoint,
        &pResponse->ResponseHeader,
        &pResponse->NoOfQueryDataSets,
        &pResponse->QueryDataSets,
        &pResponse->RevisedContinuationPoint);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_QueryNextResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information QueryNext service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_QueryNext_ServiceType =
{
    OpcUaId_QueryNextRequest,
    &OpcUa_QueryNextResponse_EncodeableType,
    OpcUa_Server_BeginQueryNext,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_QueryNext
};
#endif

#ifndef OPCUA_EXCLUDE_Read
/*============================================================================
 * A pointer to a function that implements the Read service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnRead)(
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
 * A stub method which implements the Read service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_Read(
    OpcUa_Endpoint             a_hEndpoint,
    OpcUa_Handle               a_hContext,
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
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_Read");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nMaxAge);
    OpcUa_ReferenceParameter(a_eTimestampsToReturn);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodesToRead, a_pNodesToRead);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a Read service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginRead(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_ReadRequest* pRequest = OpcUa_Null;
    OpcUa_ReadResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnRead* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginRead");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_ReadRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_ReadRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->MaxAge,
        pRequest->TimestampsToReturn,
        pRequest->NoOfNodesToRead,
        pRequest->NodesToRead,
        &pResponse->ResponseHeader,
        &pResponse->NoOfResults,
        &pResponse->Results,
        &pResponse->NoOfDiagnosticInfos,
        &pResponse->DiagnosticInfos);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_ReadResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information Read service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_Read_ServiceType =
{
    OpcUaId_ReadRequest,
    &OpcUa_ReadResponse_EncodeableType,
    OpcUa_Server_BeginRead,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_Read
};
#endif

#ifndef OPCUA_EXCLUDE_HistoryRead
/*============================================================================
 * A pointer to a function that implements the HistoryRead service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnHistoryRead)(
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
 * A stub method which implements the HistoryRead service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_HistoryRead(
    OpcUa_Endpoint                  a_hEndpoint,
    OpcUa_Handle                    a_hContext,
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
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_HistoryRead");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
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

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a HistoryRead service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginHistoryRead(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_HistoryReadRequest* pRequest = OpcUa_Null;
    OpcUa_HistoryReadResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnHistoryRead* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginHistoryRead");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_HistoryReadRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_HistoryReadRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        &pRequest->HistoryReadDetails,
        pRequest->TimestampsToReturn,
        pRequest->ReleaseContinuationPoints,
        pRequest->NoOfNodesToRead,
        pRequest->NodesToRead,
        &pResponse->ResponseHeader,
        &pResponse->NoOfResults,
        &pResponse->Results,
        &pResponse->NoOfDiagnosticInfos,
        &pResponse->DiagnosticInfos);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_HistoryReadResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information HistoryRead service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_HistoryRead_ServiceType =
{
    OpcUaId_HistoryReadRequest,
    &OpcUa_HistoryReadResponse_EncodeableType,
    OpcUa_Server_BeginHistoryRead,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_HistoryRead
};
#endif

#ifndef OPCUA_EXCLUDE_Write
/*============================================================================
 * A pointer to a function that implements the Write service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnWrite)(
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
 * A stub method which implements the Write service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_Write(
    OpcUa_Endpoint             a_hEndpoint,
    OpcUa_Handle               a_hContext,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_Int32                a_nNoOfNodesToWrite,
    const OpcUa_WriteValue*    a_pNodesToWrite,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_Int32*               a_pNoOfResults,
    OpcUa_StatusCode**         a_pResults,
    OpcUa_Int32*               a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     a_pDiagnosticInfos)
{
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_Write");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfNodesToWrite, a_pNodesToWrite);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a Write service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginWrite(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_WriteRequest* pRequest = OpcUa_Null;
    OpcUa_WriteResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnWrite* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginWrite");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_WriteRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_WriteRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->NoOfNodesToWrite,
        pRequest->NodesToWrite,
        &pResponse->ResponseHeader,
        &pResponse->NoOfResults,
        &pResponse->Results,
        &pResponse->NoOfDiagnosticInfos,
        &pResponse->DiagnosticInfos);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_WriteResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information Write service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_Write_ServiceType =
{
    OpcUaId_WriteRequest,
    &OpcUa_WriteResponse_EncodeableType,
    OpcUa_Server_BeginWrite,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_Write
};
#endif

#ifndef OPCUA_EXCLUDE_HistoryUpdate
/*============================================================================
 * A pointer to a function that implements the HistoryUpdate service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnHistoryUpdate)(
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
 * A stub method which implements the HistoryUpdate service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_HistoryUpdate(
    OpcUa_Endpoint               a_hEndpoint,
    OpcUa_Handle                 a_hContext,
    const OpcUa_RequestHeader*   a_pRequestHeader,
    OpcUa_Int32                  a_nNoOfHistoryUpdateDetails,
    const OpcUa_ExtensionObject* a_pHistoryUpdateDetails,
    OpcUa_ResponseHeader*        a_pResponseHeader,
    OpcUa_Int32*                 a_pNoOfResults,
    OpcUa_HistoryUpdateResult**  a_pResults,
    OpcUa_Int32*                 a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**       a_pDiagnosticInfos)
{
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_HistoryUpdate");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfHistoryUpdateDetails, a_pHistoryUpdateDetails);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a HistoryUpdate service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginHistoryUpdate(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_HistoryUpdateRequest* pRequest = OpcUa_Null;
    OpcUa_HistoryUpdateResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnHistoryUpdate* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginHistoryUpdate");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_HistoryUpdateRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_HistoryUpdateRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->NoOfHistoryUpdateDetails,
        pRequest->HistoryUpdateDetails,
        &pResponse->ResponseHeader,
        &pResponse->NoOfResults,
        &pResponse->Results,
        &pResponse->NoOfDiagnosticInfos,
        &pResponse->DiagnosticInfos);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_HistoryUpdateResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information HistoryUpdate service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_HistoryUpdate_ServiceType =
{
    OpcUaId_HistoryUpdateRequest,
    &OpcUa_HistoryUpdateResponse_EncodeableType,
    OpcUa_Server_BeginHistoryUpdate,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_HistoryUpdate
};
#endif

#ifndef OPCUA_EXCLUDE_Call
/*============================================================================
 * A pointer to a function that implements the Call service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnCall)(
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
 * A stub method which implements the Call service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_Call(
    OpcUa_Endpoint                 a_hEndpoint,
    OpcUa_Handle                   a_hContext,
    const OpcUa_RequestHeader*     a_pRequestHeader,
    OpcUa_Int32                    a_nNoOfMethodsToCall,
    const OpcUa_CallMethodRequest* a_pMethodsToCall,
    OpcUa_ResponseHeader*          a_pResponseHeader,
    OpcUa_Int32*                   a_pNoOfResults,
    OpcUa_CallMethodResult**       a_pResults,
    OpcUa_Int32*                   a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**         a_pDiagnosticInfos)
{
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_Call");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfMethodsToCall, a_pMethodsToCall);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a Call service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginCall(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_CallRequest* pRequest = OpcUa_Null;
    OpcUa_CallResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnCall* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginCall");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_CallRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_CallRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->NoOfMethodsToCall,
        pRequest->MethodsToCall,
        &pResponse->ResponseHeader,
        &pResponse->NoOfResults,
        &pResponse->Results,
        &pResponse->NoOfDiagnosticInfos,
        &pResponse->DiagnosticInfos);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_CallResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information Call service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_Call_ServiceType =
{
    OpcUaId_CallRequest,
    &OpcUa_CallResponse_EncodeableType,
    OpcUa_Server_BeginCall,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_Call
};
#endif

#ifndef OPCUA_EXCLUDE_CreateMonitoredItems
/*============================================================================
 * A pointer to a function that implements the CreateMonitoredItems service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnCreateMonitoredItems)(
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
 * A stub method which implements the CreateMonitoredItems service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_CreateMonitoredItems(
    OpcUa_Endpoint                          a_hEndpoint,
    OpcUa_Handle                            a_hContext,
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
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_CreateMonitoredItems");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nSubscriptionId);
    OpcUa_ReferenceParameter(a_eTimestampsToReturn);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfItemsToCreate, a_pItemsToCreate);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a CreateMonitoredItems service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginCreateMonitoredItems(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_CreateMonitoredItemsRequest* pRequest = OpcUa_Null;
    OpcUa_CreateMonitoredItemsResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnCreateMonitoredItems* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginCreateMonitoredItems");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_CreateMonitoredItemsRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_CreateMonitoredItemsRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->SubscriptionId,
        pRequest->TimestampsToReturn,
        pRequest->NoOfItemsToCreate,
        pRequest->ItemsToCreate,
        &pResponse->ResponseHeader,
        &pResponse->NoOfResults,
        &pResponse->Results,
        &pResponse->NoOfDiagnosticInfos,
        &pResponse->DiagnosticInfos);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_CreateMonitoredItemsResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information CreateMonitoredItems service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_CreateMonitoredItems_ServiceType =
{
    OpcUaId_CreateMonitoredItemsRequest,
    &OpcUa_CreateMonitoredItemsResponse_EncodeableType,
    OpcUa_Server_BeginCreateMonitoredItems,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_CreateMonitoredItems
};
#endif

#ifndef OPCUA_EXCLUDE_ModifyMonitoredItems
/*============================================================================
 * A pointer to a function that implements the ModifyMonitoredItems service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnModifyMonitoredItems)(
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
 * A stub method which implements the ModifyMonitoredItems service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_ModifyMonitoredItems(
    OpcUa_Endpoint                          a_hEndpoint,
    OpcUa_Handle                            a_hContext,
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
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_ModifyMonitoredItems");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nSubscriptionId);
    OpcUa_ReferenceParameter(a_eTimestampsToReturn);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfItemsToModify, a_pItemsToModify);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a ModifyMonitoredItems service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginModifyMonitoredItems(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_ModifyMonitoredItemsRequest* pRequest = OpcUa_Null;
    OpcUa_ModifyMonitoredItemsResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnModifyMonitoredItems* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginModifyMonitoredItems");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_ModifyMonitoredItemsRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_ModifyMonitoredItemsRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->SubscriptionId,
        pRequest->TimestampsToReturn,
        pRequest->NoOfItemsToModify,
        pRequest->ItemsToModify,
        &pResponse->ResponseHeader,
        &pResponse->NoOfResults,
        &pResponse->Results,
        &pResponse->NoOfDiagnosticInfos,
        &pResponse->DiagnosticInfos);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_ModifyMonitoredItemsResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information ModifyMonitoredItems service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_ModifyMonitoredItems_ServiceType =
{
    OpcUaId_ModifyMonitoredItemsRequest,
    &OpcUa_ModifyMonitoredItemsResponse_EncodeableType,
    OpcUa_Server_BeginModifyMonitoredItems,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_ModifyMonitoredItems
};
#endif

#ifndef OPCUA_EXCLUDE_SetMonitoringMode
/*============================================================================
 * A pointer to a function that implements the SetMonitoringMode service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnSetMonitoringMode)(
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
 * A stub method which implements the SetMonitoringMode service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_SetMonitoringMode(
    OpcUa_Endpoint             a_hEndpoint,
    OpcUa_Handle               a_hContext,
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
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_SetMonitoringMode");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nSubscriptionId);
    OpcUa_ReferenceParameter(a_eMonitoringMode);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfMonitoredItemIds, a_pMonitoredItemIds);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a SetMonitoringMode service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginSetMonitoringMode(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_SetMonitoringModeRequest* pRequest = OpcUa_Null;
    OpcUa_SetMonitoringModeResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnSetMonitoringMode* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginSetMonitoringMode");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_SetMonitoringModeRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_SetMonitoringModeRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->SubscriptionId,
        pRequest->MonitoringMode,
        pRequest->NoOfMonitoredItemIds,
        pRequest->MonitoredItemIds,
        &pResponse->ResponseHeader,
        &pResponse->NoOfResults,
        &pResponse->Results,
        &pResponse->NoOfDiagnosticInfos,
        &pResponse->DiagnosticInfos);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_SetMonitoringModeResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information SetMonitoringMode service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_SetMonitoringMode_ServiceType =
{
    OpcUaId_SetMonitoringModeRequest,
    &OpcUa_SetMonitoringModeResponse_EncodeableType,
    OpcUa_Server_BeginSetMonitoringMode,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_SetMonitoringMode
};
#endif

#ifndef OPCUA_EXCLUDE_SetTriggering
/*============================================================================
 * A pointer to a function that implements the SetTriggering service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnSetTriggering)(
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
 * A stub method which implements the SetTriggering service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_SetTriggering(
    OpcUa_Endpoint             a_hEndpoint,
    OpcUa_Handle               a_hContext,
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
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_SetTriggering");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
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

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a SetTriggering service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginSetTriggering(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_SetTriggeringRequest* pRequest = OpcUa_Null;
    OpcUa_SetTriggeringResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnSetTriggering* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginSetTriggering");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_SetTriggeringRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_SetTriggeringRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->SubscriptionId,
        pRequest->TriggeringItemId,
        pRequest->NoOfLinksToAdd,
        pRequest->LinksToAdd,
        pRequest->NoOfLinksToRemove,
        pRequest->LinksToRemove,
        &pResponse->ResponseHeader,
        &pResponse->NoOfAddResults,
        &pResponse->AddResults,
        &pResponse->NoOfAddDiagnosticInfos,
        &pResponse->AddDiagnosticInfos,
        &pResponse->NoOfRemoveResults,
        &pResponse->RemoveResults,
        &pResponse->NoOfRemoveDiagnosticInfos,
        &pResponse->RemoveDiagnosticInfos);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_SetTriggeringResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information SetTriggering service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_SetTriggering_ServiceType =
{
    OpcUaId_SetTriggeringRequest,
    &OpcUa_SetTriggeringResponse_EncodeableType,
    OpcUa_Server_BeginSetTriggering,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_SetTriggering
};
#endif

#ifndef OPCUA_EXCLUDE_DeleteMonitoredItems
/*============================================================================
 * A pointer to a function that implements the DeleteMonitoredItems service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnDeleteMonitoredItems)(
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
 * A stub method which implements the DeleteMonitoredItems service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_DeleteMonitoredItems(
    OpcUa_Endpoint             a_hEndpoint,
    OpcUa_Handle               a_hContext,
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
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_DeleteMonitoredItems");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nSubscriptionId);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfMonitoredItemIds, a_pMonitoredItemIds);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a DeleteMonitoredItems service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginDeleteMonitoredItems(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_DeleteMonitoredItemsRequest* pRequest = OpcUa_Null;
    OpcUa_DeleteMonitoredItemsResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnDeleteMonitoredItems* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginDeleteMonitoredItems");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_DeleteMonitoredItemsRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_DeleteMonitoredItemsRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->SubscriptionId,
        pRequest->NoOfMonitoredItemIds,
        pRequest->MonitoredItemIds,
        &pResponse->ResponseHeader,
        &pResponse->NoOfResults,
        &pResponse->Results,
        &pResponse->NoOfDiagnosticInfos,
        &pResponse->DiagnosticInfos);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_DeleteMonitoredItemsResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information DeleteMonitoredItems service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_DeleteMonitoredItems_ServiceType =
{
    OpcUaId_DeleteMonitoredItemsRequest,
    &OpcUa_DeleteMonitoredItemsResponse_EncodeableType,
    OpcUa_Server_BeginDeleteMonitoredItems,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_DeleteMonitoredItems
};
#endif

#ifndef OPCUA_EXCLUDE_CreateSubscription
/*============================================================================
 * A pointer to a function that implements the CreateSubscription service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnCreateSubscription)(
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
 * A stub method which implements the CreateSubscription service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_CreateSubscription(
    OpcUa_Endpoint             a_hEndpoint,
    OpcUa_Handle               a_hContext,
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
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_CreateSubscription");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
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

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a CreateSubscription service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginCreateSubscription(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_CreateSubscriptionRequest* pRequest = OpcUa_Null;
    OpcUa_CreateSubscriptionResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnCreateSubscription* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginCreateSubscription");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_CreateSubscriptionRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_CreateSubscriptionRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->RequestedPublishingInterval,
        pRequest->RequestedLifetimeCount,
        pRequest->RequestedMaxKeepAliveCount,
        pRequest->MaxNotificationsPerPublish,
        pRequest->PublishingEnabled,
        pRequest->Priority,
        &pResponse->ResponseHeader,
        &pResponse->SubscriptionId,
        &pResponse->RevisedPublishingInterval,
        &pResponse->RevisedLifetimeCount,
        &pResponse->RevisedMaxKeepAliveCount);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_CreateSubscriptionResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information CreateSubscription service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_CreateSubscription_ServiceType =
{
    OpcUaId_CreateSubscriptionRequest,
    &OpcUa_CreateSubscriptionResponse_EncodeableType,
    OpcUa_Server_BeginCreateSubscription,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_CreateSubscription
};
#endif

#ifndef OPCUA_EXCLUDE_ModifySubscription
/*============================================================================
 * A pointer to a function that implements the ModifySubscription service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnModifySubscription)(
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
 * A stub method which implements the ModifySubscription service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_ModifySubscription(
    OpcUa_Endpoint             a_hEndpoint,
    OpcUa_Handle               a_hContext,
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
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_ModifySubscription");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
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

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a ModifySubscription service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginModifySubscription(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_ModifySubscriptionRequest* pRequest = OpcUa_Null;
    OpcUa_ModifySubscriptionResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnModifySubscription* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginModifySubscription");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_ModifySubscriptionRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_ModifySubscriptionRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->SubscriptionId,
        pRequest->RequestedPublishingInterval,
        pRequest->RequestedLifetimeCount,
        pRequest->RequestedMaxKeepAliveCount,
        pRequest->MaxNotificationsPerPublish,
        pRequest->Priority,
        &pResponse->ResponseHeader,
        &pResponse->RevisedPublishingInterval,
        &pResponse->RevisedLifetimeCount,
        &pResponse->RevisedMaxKeepAliveCount);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_ModifySubscriptionResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information ModifySubscription service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_ModifySubscription_ServiceType =
{
    OpcUaId_ModifySubscriptionRequest,
    &OpcUa_ModifySubscriptionResponse_EncodeableType,
    OpcUa_Server_BeginModifySubscription,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_ModifySubscription
};
#endif

#ifndef OPCUA_EXCLUDE_SetPublishingMode
/*============================================================================
 * A pointer to a function that implements the SetPublishingMode service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnSetPublishingMode)(
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
 * A stub method which implements the SetPublishingMode service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_SetPublishingMode(
    OpcUa_Endpoint             a_hEndpoint,
    OpcUa_Handle               a_hContext,
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
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_SetPublishingMode");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_bPublishingEnabled);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfSubscriptionIds, a_pSubscriptionIds);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a SetPublishingMode service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginSetPublishingMode(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_SetPublishingModeRequest* pRequest = OpcUa_Null;
    OpcUa_SetPublishingModeResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnSetPublishingMode* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginSetPublishingMode");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_SetPublishingModeRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_SetPublishingModeRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->PublishingEnabled,
        pRequest->NoOfSubscriptionIds,
        pRequest->SubscriptionIds,
        &pResponse->ResponseHeader,
        &pResponse->NoOfResults,
        &pResponse->Results,
        &pResponse->NoOfDiagnosticInfos,
        &pResponse->DiagnosticInfos);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_SetPublishingModeResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information SetPublishingMode service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_SetPublishingMode_ServiceType =
{
    OpcUaId_SetPublishingModeRequest,
    &OpcUa_SetPublishingModeResponse_EncodeableType,
    OpcUa_Server_BeginSetPublishingMode,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_SetPublishingMode
};
#endif

#ifndef OPCUA_EXCLUDE_Publish
/*============================================================================
 * A pointer to a function that implements the Publish service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnPublish)(
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
 * A stub method which implements the Publish service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_Publish(
    OpcUa_Endpoint                           a_hEndpoint,
    OpcUa_Handle                             a_hContext,
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
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_Publish");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
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

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a Publish service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginPublish(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_PublishRequest* pRequest = OpcUa_Null;
    OpcUa_PublishResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnPublish* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginPublish");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_PublishRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_PublishRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->NoOfSubscriptionAcknowledgements,
        pRequest->SubscriptionAcknowledgements,
        &pResponse->ResponseHeader,
        &pResponse->SubscriptionId,
        &pResponse->NoOfAvailableSequenceNumbers,
        &pResponse->AvailableSequenceNumbers,
        &pResponse->MoreNotifications,
        &pResponse->NotificationMessage,
        &pResponse->NoOfResults,
        &pResponse->Results,
        &pResponse->NoOfDiagnosticInfos,
        &pResponse->DiagnosticInfos);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_PublishResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information Publish service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_Publish_ServiceType =
{
    OpcUaId_PublishRequest,
    &OpcUa_PublishResponse_EncodeableType,
    OpcUa_Server_BeginPublish,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_Publish
};
#endif

#ifndef OPCUA_EXCLUDE_Republish
/*============================================================================
 * A pointer to a function that implements the Republish service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnRepublish)(
    OpcUa_Endpoint             hEndpoint,
    OpcUa_Handle               hContext,
    const OpcUa_RequestHeader* pRequestHeader,
    OpcUa_UInt32               nSubscriptionId,
    OpcUa_UInt32               nRetransmitSequenceNumber,
    OpcUa_ResponseHeader*      pResponseHeader,
    OpcUa_NotificationMessage* pNotificationMessage);

/*============================================================================
 * A stub method which implements the Republish service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_Republish(
    OpcUa_Endpoint             a_hEndpoint,
    OpcUa_Handle               a_hContext,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_UInt32               a_nSubscriptionId,
    OpcUa_UInt32               a_nRetransmitSequenceNumber,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_NotificationMessage* a_pNotificationMessage)
{
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_Republish");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReferenceParameter(a_nSubscriptionId);
    OpcUa_ReferenceParameter(a_nRetransmitSequenceNumber);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNotificationMessage);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a Republish service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginRepublish(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_RepublishRequest* pRequest = OpcUa_Null;
    OpcUa_RepublishResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnRepublish* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginRepublish");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_RepublishRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_RepublishRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->SubscriptionId,
        pRequest->RetransmitSequenceNumber,
        &pResponse->ResponseHeader,
        &pResponse->NotificationMessage);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_RepublishResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information Republish service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_Republish_ServiceType =
{
    OpcUaId_RepublishRequest,
    &OpcUa_RepublishResponse_EncodeableType,
    OpcUa_Server_BeginRepublish,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_Republish
};
#endif

#ifndef OPCUA_EXCLUDE_TransferSubscriptions
/*============================================================================
 * A pointer to a function that implements the TransferSubscriptions service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnTransferSubscriptions)(
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
 * A stub method which implements the TransferSubscriptions service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_TransferSubscriptions(
    OpcUa_Endpoint             a_hEndpoint,
    OpcUa_Handle               a_hContext,
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
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_TransferSubscriptions");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfSubscriptionIds, a_pSubscriptionIds);
    OpcUa_ReferenceParameter(a_bSendInitialValues);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a TransferSubscriptions service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginTransferSubscriptions(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_TransferSubscriptionsRequest* pRequest = OpcUa_Null;
    OpcUa_TransferSubscriptionsResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnTransferSubscriptions* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginTransferSubscriptions");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_TransferSubscriptionsRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_TransferSubscriptionsRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->NoOfSubscriptionIds,
        pRequest->SubscriptionIds,
        pRequest->SendInitialValues,
        &pResponse->ResponseHeader,
        &pResponse->NoOfResults,
        &pResponse->Results,
        &pResponse->NoOfDiagnosticInfos,
        &pResponse->DiagnosticInfos);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_TransferSubscriptionsResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information TransferSubscriptions service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_TransferSubscriptions_ServiceType =
{
    OpcUaId_TransferSubscriptionsRequest,
    &OpcUa_TransferSubscriptionsResponse_EncodeableType,
    OpcUa_Server_BeginTransferSubscriptions,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_TransferSubscriptions
};
#endif

#ifndef OPCUA_EXCLUDE_DeleteSubscriptions
/*============================================================================
 * A pointer to a function that implements the DeleteSubscriptions service.
 *===========================================================================*/
typedef OpcUa_StatusCode (OpcUa_ServerApi_PfnDeleteSubscriptions)(
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
 * A stub method which implements the DeleteSubscriptions service.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_DeleteSubscriptions(
    OpcUa_Endpoint             a_hEndpoint,
    OpcUa_Handle               a_hContext,
    const OpcUa_RequestHeader* a_pRequestHeader,
    OpcUa_Int32                a_nNoOfSubscriptionIds,
    const OpcUa_UInt32*        a_pSubscriptionIds,
    OpcUa_ResponseHeader*      a_pResponseHeader,
    OpcUa_Int32*               a_pNoOfResults,
    OpcUa_StatusCode**         a_pResults,
    OpcUa_Int32*               a_pNoOfDiagnosticInfos,
    OpcUa_DiagnosticInfo**     a_pDiagnosticInfos)
{
    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_ServerApi_DeleteSubscriptions");

    /* validate arguments. */
    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArrayArgumentNull(a_nNoOfSubscriptionIds, a_pSubscriptionIds);
    OpcUa_ReturnErrorIfArgumentNull(a_pResponseHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pResults);
    OpcUa_ReturnErrorIfArgumentNull(a_pNoOfDiagnosticInfos);
    OpcUa_ReturnErrorIfArgumentNull(a_pDiagnosticInfos);

    uStatus = OpcUa_BadNotImplemented;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* nothing to do */

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Begins processing of a DeleteSubscriptions service request.
 *===========================================================================*/
OpcUa_StatusCode OpcUa_Server_BeginDeleteSubscriptions(
    OpcUa_Endpoint        a_hEndpoint,
    OpcUa_Handle          a_hContext,
    OpcUa_Void**          a_ppRequest,
    OpcUa_EncodeableType* a_pRequestType)
{
    OpcUa_DeleteSubscriptionsRequest* pRequest = OpcUa_Null;
    OpcUa_DeleteSubscriptionsResponse* pResponse = OpcUa_Null;
    OpcUa_EncodeableType* pResponseType = OpcUa_Null;
    OpcUa_ServerApi_PfnDeleteSubscriptions* pfnInvoke = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_Server, "OpcUa_Server_BeginDeleteSubscriptions");

    OpcUa_ReturnErrorIfArgumentNull(a_hEndpoint);
    OpcUa_ReturnErrorIfArgumentNull(a_hContext);
    OpcUa_ReturnErrorIfArgumentNull(a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(*a_ppRequest);
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestType);

    OpcUa_ReturnErrorIfTrue(a_pRequestType->TypeId != OpcUaId_DeleteSubscriptionsRequest, OpcUa_BadInvalidArgument);

    pRequest = (OpcUa_DeleteSubscriptionsRequest*)*a_ppRequest;

    /* create a context to use for sending a response */
    uStatus = OpcUa_Endpoint_BeginSendResponse(a_hEndpoint, a_hContext, (OpcUa_Void**)&pResponse, &pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    /* get the function that implements the service call. */
    uStatus = OpcUa_Endpoint_GetServiceFunction(a_hEndpoint, a_hContext, (OpcUa_PfnInvokeService**)&pfnInvoke);
    OpcUa_GotoErrorIfBad(uStatus);

    /* invoke the service */
    uStatus = pfnInvoke(
        a_hEndpoint,        a_hContext,
        &pRequest->RequestHeader,
        pRequest->NoOfSubscriptionIds,
        pRequest->SubscriptionIds,
        &pResponse->ResponseHeader,
        &pResponse->NoOfResults,
        &pResponse->Results,
        &pResponse->NoOfDiagnosticInfos,
        &pResponse->DiagnosticInfos);

    if (OpcUa_IsBad(uStatus))
    {
        OpcUa_Void* pFault = OpcUa_Null;
        OpcUa_EncodeableType* pFaultType = OpcUa_Null;

        /* create a fault */
        uStatus = OpcUa_ServerApi_CreateFault(
            &pRequest->RequestHeader,
            uStatus,
            &pResponse->ResponseHeader.ServiceDiagnostics,
            &pResponse->ResponseHeader.NoOfStringTable,
            &pResponse->ResponseHeader.StringTable,
            &pFault,
            &pFaultType);

        OpcUa_GotoErrorIfBad(uStatus);

        /* free the response */
        OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

        /* make the response the fault */
        pResponse = (OpcUa_DeleteSubscriptionsResponse*)pFault;
        pResponseType = pFaultType;
    }

    /* send the response */
    uStatus = OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, OpcUa_Good, pResponse, pResponseType);
    OpcUa_GotoErrorIfBad(uStatus);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    /* send an error response */
    OpcUa_Endpoint_EndSendResponse(a_hEndpoint, &a_hContext, uStatus, OpcUa_Null, OpcUa_Null);

    OpcUa_EncodeableObject_Delete(pResponseType, (OpcUa_Void**)&pResponse);

    OpcUa_FinishErrorHandling;
}

/*============================================================================
 * The service dispatch information DeleteSubscriptions service.
 *===========================================================================*/
struct _OpcUa_ServiceType OpcUa_DeleteSubscriptions_ServiceType =
{
    OpcUaId_DeleteSubscriptionsRequest,
    &OpcUa_DeleteSubscriptionsResponse_EncodeableType,
    OpcUa_Server_BeginDeleteSubscriptions,
    (OpcUa_PfnInvokeService*)OpcUa_ServerApi_DeleteSubscriptions
};
#endif

/*============================================================================
 * Table of standard services.
 *===========================================================================*/
OpcUa_ServiceType* OpcUa_SupportedServiceTypes[] =
{
    #ifndef OPCUA_EXCLUDE_FindServers
    &OpcUa_FindServers_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_FindServersOnNetwork
    &OpcUa_FindServersOnNetwork_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_GetEndpoints
    &OpcUa_GetEndpoints_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_RegisterServer
    &OpcUa_RegisterServer_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_RegisterServer2
    &OpcUa_RegisterServer2_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_CreateSession
    &OpcUa_CreateSession_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_ActivateSession
    &OpcUa_ActivateSession_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_CloseSession
    &OpcUa_CloseSession_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_Cancel
    &OpcUa_Cancel_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_AddNodes
    &OpcUa_AddNodes_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_AddReferences
    &OpcUa_AddReferences_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_DeleteNodes
    &OpcUa_DeleteNodes_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_DeleteReferences
    &OpcUa_DeleteReferences_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_Browse
    &OpcUa_Browse_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_BrowseNext
    &OpcUa_BrowseNext_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_TranslateBrowsePathsToNodeIds
    &OpcUa_TranslateBrowsePathsToNodeIds_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_RegisterNodes
    &OpcUa_RegisterNodes_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_UnregisterNodes
    &OpcUa_UnregisterNodes_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_QueryFirst
    &OpcUa_QueryFirst_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_QueryNext
    &OpcUa_QueryNext_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_Read
    &OpcUa_Read_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_HistoryRead
    &OpcUa_HistoryRead_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_Write
    &OpcUa_Write_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_HistoryUpdate
    &OpcUa_HistoryUpdate_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_Call
    &OpcUa_Call_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_CreateMonitoredItems
    &OpcUa_CreateMonitoredItems_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_ModifyMonitoredItems
    &OpcUa_ModifyMonitoredItems_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_SetMonitoringMode
    &OpcUa_SetMonitoringMode_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_SetTriggering
    &OpcUa_SetTriggering_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_DeleteMonitoredItems
    &OpcUa_DeleteMonitoredItems_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_CreateSubscription
    &OpcUa_CreateSubscription_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_ModifySubscription
    &OpcUa_ModifySubscription_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_SetPublishingMode
    &OpcUa_SetPublishingMode_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_Publish
    &OpcUa_Publish_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_Republish
    &OpcUa_Republish_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_TransferSubscriptions
    &OpcUa_TransferSubscriptions_ServiceType,
    #endif
    #ifndef OPCUA_EXCLUDE_DeleteSubscriptions
    &OpcUa_DeleteSubscriptions_ServiceType,
    #endif
    OpcUa_Null
};

#endif /* OPCUA_HAVE_SERVERAPI */
/* This is the last line of an autogenerated file. */
