#include <memory.h> // memset, memcpy
#include "ua_connection.h"
#include "ua_transportLayer.h"

#include "ua_secureLayer.h" // SL_process

UA_Int32 TL_check(UA_TL_connection* connection, UA_ByteString* msg, int checkLocal)
{
	UA_Int32 retval = UA_SUCCESS;

	UA_Int32 position = 4;
	UA_Int32 messageLength;

	DBG_VERBOSE(printf("TL_check - entered \n"));

	UA_Int32_decodeBinary(msg,&position,&messageLength);
	DBG_VERBOSE(printf("TL_check - messageLength = %d \n",messageLength));

	if (messageLength == -1 || messageLength != msg->length ||
			( ( checkLocal == UA_TL_CHECK_LOCAL) && messageLength > (UA_Int32) connection->localConf.maxMessageSize) ||
			( ( checkLocal == UA_TL_CHECK_REMOTE) && messageLength > (UA_Int32) connection->remoteConf.maxMessageSize))
	{
		DBG_ERR(printf("TL_check - length error \n"));
		retval = UA_ERR_INCONSISTENT;
	}
	return retval;
}

#define Cmp3Byte(data,pos,a,b,c) (*((Int32*) ((data)+(pos))) & 0xFFFFFF) == (Int32)(((Byte)(a))|((Byte)(b))<<8|((Byte)(c))<<16)

UA_Int32 UA_TL_handleHello(UA_TL_connection* connection, UA_ByteString* msg, UA_Int32* pos) {
	UA_Int32 retval = UA_SUCCESS;

	UA_Int32 tmpPos = 0;
	UA_ByteString tmpMessage;
	UA_OPCUATcpHelloMessage helloMessage;
	UA_OPCUATcpAcknowledgeMessage ackMessage;
	UA_OPCUATcpMessageHeader ackHeader;

	if (connection->connectionState == connectionState_CLOSED) {
		DBG_VERBOSE(printf("TL_process - extracting header information \n"));
		UA_OPCUATcpHelloMessage_decodeBinary(msg,pos,&helloMessage);

		// memorize buffer info and change mode to established
		connection->remoteConf.protocolVersion = helloMessage.protocolVersion;
		connection->remoteConf.recvBufferSize = helloMessage.receiveBufferSize;
		connection->remoteConf.sendBufferSize = helloMessage.sendBufferSize;
		connection->remoteConf.maxMessageSize = helloMessage.maxMessageSize;
		connection->remoteConf.maxChunkCount = helloMessage.maxChunkCount;
		UA_String_copy(&(helloMessage.endpointUrl), &(connection->remoteEndpointUrl));
		connection->connectionState = connectionState_ESTABLISHED;
		// clean up
		UA_OPCUATcpHelloMessage_deleteMembers(&helloMessage);

		DBG_VERBOSE(printf("TL_process - protocolVersion = %d \n",connection->remoteConf.protocolVersion));
		DBG_VERBOSE(printf("TL_process - recvBufferSize = %d \n",connection->remoteConf.recvBufferSize));
		DBG_VERBOSE(printf("TL_process - sendBufferSize = %d \n",connection->remoteConf.sendBufferSize));
		DBG_VERBOSE(printf("TL_process - maxMessageSize = %d \n",connection->remoteConf.maxMessageSize));
		DBG_VERBOSE(printf("TL_process - maxChunkCount = %d \n",connection->remoteConf.maxChunkCount));

		// build acknowledge response
		ackMessage.protocolVersion = connection->localConf.protocolVersion;
		ackMessage.receiveBufferSize = connection->localConf.recvBufferSize;
		ackMessage.sendBufferSize = connection->localConf.sendBufferSize;
		ackMessage.maxMessageSize = connection->localConf.maxMessageSize;
		ackMessage.maxChunkCount = connection->localConf.maxChunkCount;

		ackHeader.messageType = UA_MESSAGETYPE_ACK;
		ackHeader.isFinal = 'F';

		// encode header and message to buffer
		ackHeader.messageSize = UA_OPCUATcpAcknowledgeMessage_calcSize(&ackMessage)
		+ UA_OPCUATcpMessageHeader_calcSize(&ackHeader);
		UA_ByteString_newMembers(&tmpMessage, ackHeader.messageSize);
		UA_OPCUATcpMessageHeader_encodeBinary(&ackHeader,&tmpPos,&tmpMessage);
		UA_OPCUATcpAcknowledgeMessage_encodeBinary(&ackMessage,&tmpPos,&tmpMessage);

		DBG_VERBOSE(printf("TL_process - Size messageToSend = %d, pos=%d\n",ackHeader.messageSize, tmpPos));
		TL_send(connection, &tmpMessage);
		UA_ByteString_deleteMembers(&tmpMessage);
	}
	else
	{
		DBG_ERR(printf("TL_process - wrong connection state \n"));
		retval = UA_ERROR_MULTIPLE_HEL;
	}
	return retval;
}

UA_Int32 UA_TL_handleOpen(UA_TL_connection* connection, UA_ByteString* msg, UA_Int32* pos) {
	UA_Int32 retval = UA_SUCCESS;

	if (connection->connectionState == connectionState_ESTABLISHED) {
		// create new secure channel and associate with this TL connection
		retval |= UA_SL_Channel_new(connection,msg,pos);
	} else {
		retval = UA_ERR_INVALID_VALUE;
	}
	return retval;
}

UA_Int32 UA_TL_handleMsg(UA_TL_connection* connection, UA_ByteString* msg, UA_Int32* pos) {
	UA_Int32 retval = UA_SUCCESS;
	UA_SL_Channel* slc = connection->secureChannel;
	retval |= UA_SL_process(slc,msg,pos);
	return retval;
}

UA_Int32 UA_TL_handleClo(UA_TL_connection* connection, UA_ByteString* msg, UA_Int32* pos) {
	UA_Int32 retval = UA_SUCCESS;
	UA_SL_Channel* slc = connection->secureChannel;
	retval |= UA_SL_process(slc,msg,pos);
	connection->connectionState = connectionState_CLOSE;
	return retval;
}

UA_Int32 TL_process(UA_TL_connection* connection, UA_ByteString* msg)
{
	UA_Int32 retval = UA_SUCCESS;
	UA_Int32 pos = 0;
	UA_OPCUATcpMessageHeader tcpMessageHeader;

	DBG_VERBOSE(printf("TL_process - entered \n"));

	if ((retval = UA_OPCUATcpMessageHeader_decodeBinary(msg, &pos, &tcpMessageHeader)) == UA_SUCCESS) {
		printf("TL_process - messageType=%.*s\n",3,msg->data);
		switch(tcpMessageHeader.messageType) {
		case UA_MESSAGETYPE_HEL:
			retval = UA_TL_handleHello(connection, msg, &pos);
			break;
		case UA_MESSAGETYPE_OPN:
			retval = UA_TL_handleOpen(connection, msg, &pos);
			break;
		case UA_MESSAGETYPE_MSG:
			retval = UA_TL_handleMsg(connection, msg, &pos);
			break;
		case UA_MESSAGETYPE_CLO:
			retval = UA_TL_handleClo(connection, msg, &pos);
			break;
		default: // dispatch processing to secureLayer
			retval = UA_ERR_INVALID_VALUE;
			break;
		}
	}
	if (retval != UA_SUCCESS) {
		// FIXME: compose real error message
		UA_ByteString errorMsg;
		UA_ByteString_newMembers(&errorMsg,10);
		TL_send(connection,&errorMsg);
		UA_ByteString_deleteMembers(&errorMsg);
	}
	UA_OPCUATcpMessageHeader_deleteMembers(&tcpMessageHeader);
	return retval;
}

/** respond to client request */
UA_Int32 TL_send(UA_TL_connection* connection, UA_ByteString* msg)
{
	UA_Int32 retval = UA_SUCCESS;
	DBG_VERBOSE(printf("TL_send - entered \n"));

	if (TL_check(connection,msg,UA_TL_CHECK_REMOTE) == UA_SUCCESS) {
		connection->writerCallback(connection,msg);
	}
	else
	{
		DBG_ERR(printf("TL_send - ERROR: packet size greater than remote buffer size"));
		retval = UA_ERROR;
	}
	return retval;
}
