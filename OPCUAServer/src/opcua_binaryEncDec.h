/*
 * opcua_BinaryEncDec.h
 *
 *  Created on: Dec 18, 2013
 *      Author: opcua
 */

#ifndef OPCUA_BINARYENCDEC_H_
#define OPCUA_BINARYENCDEC_H_



enum BED_ApplicationType {SERVER_0, CLIENT_1, CLIENTANDSERVER_2, DISCOVERYSERVER_3};


struct BED_ApplicationDescription
{
	char *applicationUri;
	char *productUri;
	enum BED_ApplicationType applicationType;
	char *gatewayServerUri;
};


struct BED_EndpointDescription
{
	char *endpointUrl;
	char *securityPolicyUri;
	struct BED_ApplicationDescription *server;
};

#endif /* OPCUA_BINARYENCDEC_H_ */
