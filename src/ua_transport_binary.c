#include <memory.h>
#include "ua_transport_binary.h"
#include "ua_transport.h"
#include "ua_transport_binary_secure.h"

static UA_Int32 TL_check(TL_Connection* connection, UA_ByteString* msg) {
	if(msg->length > (UA_Int32) connection->localConf.maxMessageSize || msg->length > (UA_Int32) connection->remoteConf.maxMessageSize) {
		DBG_ERR(printf("TL_check - length error \n"));
		return UA_ERR_INCONSISTENT;
	}
	return UA_SUCCESS;
}

static UA_Int32 TL_handleHello(TL_Connection* connection, UA_ByteString* msg, UA_Int32* pos) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Int32 tmpPos = 0;
	UA_OPCUATcpHelloMessage helloMessage;

	printf("\nstate: %i", connection->connectionState);
	printf("\nwanted state: %i", CONNECTIONSTATE_CLOSED);
	if (connection->connectionState == CONNECTIONSTATE_CLOSED) {
		DBG_VERBOSE(printf("TL_process - extracting header information \n"));
		UA_OPCUATcpHelloMessage_decodeBinary(msg,pos,&helloMessage);

		// memorize buffer info and change mode to established
		connection->remoteConf.protocolVersion = helloMessage.protocolVersion;
		connection->remoteConf.recvBufferSize = helloMessage.receiveBufferSize;
		connection->remoteConf.sendBufferSize = helloMessage.sendBufferSize;
		connection->remoteConf.maxMessageSize = helloMessage.maxMessageSize;
		connection->remoteConf.maxChunkCount = helloMessage.maxChunkCount;
		UA_String_copy(&(helloMessage.endpointUrl), &(connection->remoteEndpointUrl));
		UA_OPCUATcpHelloMessage_deleteMembers(&helloMessage);

		DBG_VERBOSE(printf("TL_process - protocolVersion = %d \n",connection->remoteConf.protocolVersion));
		DBG_VERBOSE(printf("TL_process - recvBufferSize = %d \n",connection->remoteConf.recvBufferSize));
		DBG_VERBOSE(printf("TL_process - sendBufferSize = %d \n",connection->remoteConf.sendBufferSize));
		DBG_VERBOSE(printf("TL_process - maxMessageSize = %d \n",connection->remoteConf.maxMessageSize));
		DBG_VERBOSE(printf("TL_process - maxChunkCount = %d \n",connection->remoteConf.maxChunkCount));
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

		DBG_VERBOSE(printf("TL_process - Size messageToSend = %d, pos=%d\n",ackHeader.messageSize, tmpPos));
		UA_ByteString_printx("ack: ", ack_msg);
		TL_Send(connection, &ack_msg, 1);
		printf("finished wiritng");
		UA_ByteString_delete(ack_msg);
	} else {
		DBG_ERR(printf("TL_process - wrong connection state \n"));
		retval = UA_ERROR_MULTIPLE_HEL;
	}
	return retval;
}

static UA_Int32 TL_handleOpen(TL_Connection* connection, UA_ByteString* msg, UA_Int32* pos) {
	if (connection->connectionState == CONNECTIONSTATE_ESTABLISHED) {
		return SL_Channel_new(connection,msg,pos); // create new secure channel and associate with this TL connection
	}
	return UA_ERR_INVALID_VALUE;
}

static UA_Int32 TL_handleMsg(TL_Connection* connection, UA_ByteString* msg, UA_Int32* pos) {
	SL_Channel* slc = connection->secureChannel;
	return SL_process(slc,msg,pos);
}

static UA_Int32 TL_handleClo(TL_Connection* connection, UA_ByteString* msg, UA_Int32* pos) {
	SL_Channel* slc = connection->secureChannel;
	connection->connectionState = CONNECTIONSTATE_CLOSE;
	return SL_process(slc,msg,pos);
}

UA_Int32 TL_Process(TL_Connection* connection, UA_ByteString* msg) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Int32 pos = 0;
	UA_OPCUATcpMessageHeader tcpMessageHeader;

	DBG_VERBOSE(printf("TL_process - entered \n"));

	if ((retval = UA_OPCUATcpMessageHeader_decodeBinary(msg, &pos, &tcpMessageHeader)) == UA_SUCCESS) {
		printf("TL_process - messageType=%.*s\n",3,msg->data);
		switch(tcpMessageHeader.messageType) {
		case UA_MESSAGETYPE_HEL:
			retval = TL_handleHello(connection, msg, &pos);
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
	if (retval != UA_SUCCESS) {
		// FIXME: compose real error message
		UA_ByteString errorMsg;
		UA_ByteString *errorMsg_ptr = &errorMsg;
		UA_ByteString_newMembers(&errorMsg,10);
		TL_Send(connection,&errorMsg_ptr, 1);
		UA_ByteString_deleteMembers(&errorMsg);
	}
	UA_OPCUATcpMessageHeader_deleteMembers(&tcpMessageHeader);
	return retval;
}

/** respond to client request */
UA_Int32 TL_Send(TL_Connection* connection, UA_ByteString** gather_buf, UA_UInt32 gather_len) {
	UA_Int32 retval = UA_SUCCESS;
	DBG_VERBOSE(printf("TL_send - entered \n"));
	//	if (TL_check(connection,msg,TL_CHECK_REMOTE) == UA_SUCCESS) {
	retval = connection->writerCallback(connection, gather_buf, gather_len);
	DBG_VERBOSE(printf("TL_send - exited \n"));
		//}
	/* else */
	/* { */
	/* 	DBG_ERR(printf("TL_send - ERROR: packet size greater than remote buffer size")); */
	/* 	retval = UA_ERROR; */
	/* } */
	return retval;
}
