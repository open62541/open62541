#include <memory.h>
#include "ua_transport_binary.h"
#include "ua_transport.h"
#include "ua_transport_binary_secure.h"
#include "ua_transport_connection.h"


static UA_Int32 TL_handleHello1(UA_TL_Connection1 connection, const UA_ByteString* msg, UA_Int32* pos){
	UA_Int32 retval = UA_SUCCESS;
	UA_Int32 tmpPos = 0;
	UA_Int32 connectionState;
	UA_OPCUATcpHelloMessage helloMessage;
	UA_TL_Connection_getState(connection, &connectionState);
	if (connectionState == CONNECTIONSTATE_CLOSED){
		DBG_VERBOSE(printf("TL_handleHello - extracting header information \n"));
		UA_OPCUATcpHelloMessage_decodeBinary(msg,pos,&helloMessage);

		UA_TL_Connection_configByHello(connection, &helloMessage);
		DBG_VERBOSE(printf("TL_handleHello - protocolVersion = %d \n",connection->remoteConf.protocolVersion));
		DBG_VERBOSE(printf("TL_handleHello - recvBufferSize = %d \n",connection->remoteConf.recvBufferSize));
		DBG_VERBOSE(printf("TL_handleHello - sendBufferSize = %d \n",connection->remoteConf.sendBufferSize));
		DBG_VERBOSE(printf("TL_handleHello - maxMessageSize = %d \n",connection->remoteConf.maxMessageSize));
		DBG_VERBOSE(printf("TL_handleHello - maxChunkCount = %d \n",connection->remoteConf.maxChunkCount));

		// build acknowledge response
		UA_OPCUATcpAcknowledgeMessage ackMessage;
		TL_Buffer localConfig;
		UA_TL_Connection_getLocalConfiguration(connection, &localConfig);
		ackMessage.protocolVersion = localConfig.protocolVersion;
		ackMessage.receiveBufferSize = localConfig.recvBufferSize;
		ackMessage.sendBufferSize = localConfig.sendBufferSize;
		ackMessage.maxMessageSize = localConfig.maxMessageSize;
		ackMessage.maxChunkCount = localConfig.maxChunkCount;

		UA_OPCUATcpMessageHeader ackHeader;
		ackHeader.messageType = UA_MESSAGETYPE_ACK;
		ackHeader.isFinal = 'F';

		// encode header and message to buffer
		tmpPos = 0;
		ackHeader.messageSize = UA_OPCUATcpAcknowledgeMessage_calcSize(&ackMessage) + UA_OPCUATcpMessageHeader_calcSize(&ackHeader);
		UA_ByteString *ack_msg;
		UA_alloc((void **)&ack_msg, sizeof(UA_ByteString));
		UA_ByteString_newMembers(ack_msg, ackHeader.messageSize);
		UA_OPCUATcpMessageHeader_encodeBinary(&ackHeader,&tmpPos,ack_msg);
		UA_OPCUATcpAcknowledgeMessage_encodeBinary(&ackMessage,&tmpPos,ack_msg);

		DBG_VERBOSE(printf("TL_handleHello - Size messageToSend = %d, pos=%d\n",ackHeader.messageSize, tmpPos));
		DBG_VERBOSE(UA_ByteString_printx("_handleHello - ack=", ack_msg));
		TL_Send(connection, (const UA_ByteString **) &ack_msg, 1);
		DBG_VERBOSE(printf("TL_handleHello - finished writing\n"));
		UA_ByteString_delete(ack_msg);
	} else {
		DBG_ERR(printf("TL_handleHello - wrong connection state \n"));
		retval = UA_ERROR_MULTIPLE_HEL;
	}
	return retval;
}
/*
static UA_Int32 TL_handleHello(TL_Connection* connection, const UA_ByteString* msg, UA_Int32* pos) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Int32 tmpPos = 0;
	UA_OPCUATcpHelloMessage helloMessage;


	if (connection->connectionState == CONNECTIONSTATE_CLOSED) {
		DBG_VERBOSE(printf("TL_handleHello - extracting header information \n"));
		UA_OPCUATcpHelloMessage_decodeBinary(msg,pos,&helloMessage);

		// memorize buffer info and change mode to established
		connection->remoteConf.protocolVersion = helloMessage.protocolVersion;
		connection->remoteConf.recvBufferSize = helloMessage.receiveBufferSize;
		connection->remoteConf.sendBufferSize = helloMessage.sendBufferSize;
		connection->remoteConf.maxMessageSize = helloMessage.maxMessageSize;
		connection->remoteConf.maxChunkCount = helloMessage.maxChunkCount;
		UA_String_copy(&(helloMessage.endpointUrl), &(connection->remoteEndpointUrl));
		UA_OPCUATcpHelloMessage_deleteMembers(&helloMessage);

		DBG_VERBOSE(printf("TL_handleHello - protocolVersion = %d \n",connection->remoteConf.protocolVersion));
		DBG_VERBOSE(printf("TL_handleHello - recvBufferSize = %d \n",connection->remoteConf.recvBufferSize));
		DBG_VERBOSE(printf("TL_handleHello - sendBufferSize = %d \n",connection->remoteConf.sendBufferSize));
		DBG_VERBOSE(printf("TL_handleHello - maxMessageSize = %d \n",connection->remoteConf.maxMessageSize));
		DBG_VERBOSE(printf("TL_handleHello - maxChunkCount = %d \n",connection->remoteConf.maxChunkCount));
		connection->connectionState = CONNECTIONSTATE_ESTABLISHED;

		// build acknowledge response
		UA_OPCUATcpAcknowledgeMessage ackMessage;
		ackMessage.protocolVersion = connection->localConf.protocolVersion;
		ackMessage.receiveBufferSize = connection->localConf.recvBufferSize;
		ackMessage.sendBufferSize = connection->localConf.sendBufferSize;
		ackMessage.maxMessageSize = connection->localConf.maxMessageSize;
		ackMessage.maxChunkCount = connection->localConf.maxChunkCount;

		UA_OPCUATcpMessageHeader ackHeader;
		ackHeader.messageType = UA_MESSAGETYPE_ACK;
		ackHeader.isFinal = 'F';

		// encode header and message to buffer
		tmpPos = 0;
		ackHeader.messageSize = UA_OPCUATcpAcknowledgeMessage_calcSize(&ackMessage) + UA_OPCUATcpMessageHeader_calcSize(&ackHeader);
		UA_ByteString *ack_msg;
		UA_alloc((void **)&ack_msg, sizeof(UA_ByteString));
		UA_ByteString_newMembers(ack_msg, ackHeader.messageSize);
		UA_OPCUATcpMessageHeader_encodeBinary(&ackHeader,&tmpPos,ack_msg);
		UA_OPCUATcpAcknowledgeMessage_encodeBinary(&ackMessage,&tmpPos,ack_msg);

		DBG_VERBOSE(printf("TL_handleHello - Size messageToSend = %d, pos=%d\n",ackHeader.messageSize, tmpPos));
		DBG_VERBOSE(UA_ByteString_printx("_handleHello - ack=", ack_msg));
		TL_Send(connection, (const UA_ByteString **) &ack_msg, 1);
		DBG_VERBOSE(printf("TL_handleHello - finished writing\n"));
		UA_ByteString_delete(ack_msg);
	} else {
		DBG_ERR(printf("TL_handleHello - wrong connection state \n"));
		retval = UA_ERROR_MULTIPLE_HEL;
	}
	return retval;
}
*/
static UA_Int32 TL_handleOpen(UA_TL_Connection1 connection, const UA_ByteString* msg, UA_Int32* pos) {
	UA_Int32 state;
	UA_TL_Connection_getState(connection,&state);
	SL_secureChannel channel = UA_NULL;
	if (state == CONNECTIONSTATE_ESTABLISHED) {
	//	return SL_Channel_new(connection, msg, pos);
		//UA_TL_Connection_getId(connection,connectionId);

		if(SL_Channel_newByRequest(connection, msg, pos, &channel) == UA_SUCCESS)
		{

			SL_Channel_registerTokenProvider(channel, SL_ChannelManager_generateToken);
			SL_ProcessOpenChannel(channel, msg, pos);
			SL_ChannelManager_addChannel(channel);
		}else
		{
			printf("TL_handleOpen - ERROR: could not create new secureChannel");
		}
	}

	return UA_ERR_INVALID_VALUE;
}

static UA_Int32 TL_handleMsg(UA_TL_Connection1 connection, const UA_ByteString* msg, UA_Int32* pos) {
	UA_Int32 state;
	UA_TL_Connection_getState(connection,&state);
	if (state == CONNECTIONSTATE_ESTABLISHED) {
		return SL_Process(msg,pos);
	}
	return UA_ERR_INVALID_VALUE;
}

static UA_Int32 TL_handleClo(UA_TL_Connection1 connection, const UA_ByteString* msg, UA_Int32* pos) {
	UA_Int32 retval = UA_SUCCESS;
	UA_SecureConversationMessageHeader *header;
	retval |= UA_SecureConversationMessageHeader_new(&header);
	retval |= UA_SecureConversationMessageHeader_decodeBinary(msg,pos,header);

	retval |= SL_ChannelManager_removeChannel(header->secureChannelId);

	retval |= UA_SecureConversationMessageHeader_delete(header);
	return retval;
}

UA_Int32 TL_Process(UA_TL_Connection1 connection, const UA_ByteString* msg) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Int32 pos = 0;
	UA_OPCUATcpMessageHeader tcpMessageHeader;

	DBG_VERBOSE(printf("TL_Process - entered \n"));

	if ((retval = UA_OPCUATcpMessageHeader_decodeBinary(msg, &pos, &tcpMessageHeader)) == UA_SUCCESS) {
		printf("TL_Process - messageType=%.*s\n",3,msg->data);
		switch(tcpMessageHeader.messageType) {
		case UA_MESSAGETYPE_HEL:
			retval = TL_handleHello1(connection, msg, &pos);
			//retval = TL_handleHello(connection, msg, &pos);

			break;
		case UA_MESSAGETYPE_OPN:
			retval = TL_handleOpen(connection, msg, &pos);
			break;
		case UA_MESSAGETYPE_MSG:
			retval = TL_handleMsg(connection, msg, &pos);
			break;
		case UA_MESSAGETYPE_CLO:
			retval = TL_handleClo(connection, msg, &pos);
			break;
		default: // dispatch processing to secureLayer
			retval = UA_ERR_INVALID_VALUE;
			break;
		}
	}
	/* if (retval != UA_SUCCESS) { */
	/* 	// FIXME: compose real error message */
	/* 	UA_ByteString errorMsg; */
	/* 	UA_ByteString *errorMsg_ptr = &errorMsg; */
	/* 	UA_ByteString_newMembers(&errorMsg,10); */
	/* 	TL_Send(connection,(const UA_ByteString **)&errorMsg_ptr, 1); */
	/* 	UA_ByteString_deleteMembers(&errorMsg); */
	/* } */
	UA_OPCUATcpMessageHeader_deleteMembers(&tcpMessageHeader);
	return retval;
}

/** respond to client request */
UA_Int32 TL_Send(UA_TL_Connection1 connection, const UA_ByteString** gather_buf, UA_UInt32 gather_len) {
	UA_Int32 retval = UA_SUCCESS;


	DBG_VERBOSE(printf("TL_send - entered \n"));
	//	if (TL_check(connection,msg,TL_CHECK_REMOTE) == UA_SUCCESS) {

	retval = UA_TL_Connection_callWriter(connection, gather_buf, gather_len);
	DBG_VERBOSE(printf("TL_send - exited \n"));
		//}
	/* else */
	/* { */
	/* 	DBG_ERR(printf("TL_send - ERROR: packet size greater than remote buffer size")); */
	/* 	retval = UA_ERROR; */
	/* } */
	return retval;
}
