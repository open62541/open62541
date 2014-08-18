#include "ua_transport_connection.h"
#include <memory.h>
#include "ua_transport_binary.h"
#include "ua_transport.h"
#include "ua_transport_binary_secure.h"


static UA_Int32 TL_handleHello(UA_TL_Connection *connection, const UA_ByteString* msg, UA_UInt32* pos){
	UA_Int32 retval = UA_SUCCESS;
	UA_UInt32 tmpPos = 0;
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
		UA_TL_Connection_getLocalConfig(connection, &localConfig);
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

		ackHeader.messageSize = UA_OPCUATcpAcknowledgeMessage_calcSizeBinary(&ackMessage) + UA_OPCUATcpMessageHeader_calcSizeBinary(&ackHeader);
		UA_ByteString *ack_msg;
		UA_alloc((void **)&ack_msg, sizeof(UA_ByteString));
		UA_ByteString_newMembers(ack_msg, ackHeader.messageSize);
		UA_OPCUATcpMessageHeader_encodeBinary(&ackHeader,ack_msg,&tmpPos);
		UA_OPCUATcpAcknowledgeMessage_encodeBinary(&ackMessage, ack_msg,&tmpPos);

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

static UA_Int32 TL_handleOpen(UA_TL_Connection *connection, const UA_ByteString* msg, UA_UInt32* pos) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Int32 state;
	SL_Channel *channel;
	UA_UInt32 secureChannelId;
	retval |= UA_TL_Connection_getState(connection, &state);
	if (state == CONNECTIONSTATE_ESTABLISHED) {
		UA_UInt32_decodeBinary(msg, pos, &secureChannelId);
		SL_ChannelManager_getChannel(secureChannelId, &channel);
		if(channel == UA_NULL)
		{
			SL_Channel *newChannel;
			//create new channel
			retval |= SL_Channel_new(&newChannel);//just create channel
			retval |= SL_Channel_init(newChannel, connection,SL_ChannelManager_generateChannelId, SL_ChannelManager_generateToken);
			retval |= SL_Channel_bind(newChannel,connection);
			retval |= SL_ProcessOpenChannel(newChannel, msg, pos);
			retval |= SL_ChannelManager_addChannel(newChannel);
			return retval;
		}
		// channel already exists, renew token?
		retval |= SL_ProcessOpenChannel(channel, msg, pos);
		return retval;
	}else{
		printf("TL_handleOpen - ERROR: could not create new secureChannel");
	}

	return UA_ERR_INVALID_VALUE;
}

static UA_Int32 TL_handleMsg(UA_TL_Connection *connection, const UA_ByteString* msg, UA_UInt32* pos) {
	UA_Int32 state;
	UA_TL_Connection_getState(connection,&state);
	if (state == CONNECTIONSTATE_ESTABLISHED) {
		return SL_Process(msg, pos);
	}
	return UA_ERR_INVALID_VALUE;
}

static UA_Int32 TL_handleClo(UA_TL_Connection *connection, const UA_ByteString* msg, UA_UInt32* pos) {
	UA_Int32 retval = UA_SUCCESS;
	SL_Process(msg,pos);
	// just prepare closing, closing and freeing structures is done elsewhere
	// UA_TL_Connection_close(connection);
	UA_TL_Connection_setState(connection, CONNECTIONSTATE_CLOSE);
	return retval;
}

UA_Int32 TL_Process(UA_TL_Connection *connection, const UA_ByteString* msg) {
	UA_Int32 retval = UA_SUCCESS;
	UA_UInt32 pos = 0;
	UA_OPCUATcpMessageHeader tcpMessageHeader;

	DBG_VERBOSE(printf("TL_Process - entered \n"));
	UA_Int32 messageCounter = 0;

	do{

		if ((retval = UA_OPCUATcpMessageHeader_decodeBinary(msg,&pos,&tcpMessageHeader)) == UA_SUCCESS) {
			printf("TL_Process - messageType=%.*s\n",3,msg->data);
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
				//Invalid packet was received which could have led to a wrong offset (pos).
				//It was not possible to extract the following packet from the buffer
				retval = UA_ERR_INVALID_VALUE;
				break;
			}
		}
		else
		{
			printf("TL_Process - ERROR:decoding of header failed \n");
		}

		messageCounter++;
		printf("TL_Process - multipleMessage in Buffer: %i \n",messageCounter);


	}while(msg->length > (UA_Int32)pos);
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
UA_Int32 TL_Send(UA_TL_Connection *connection, const UA_ByteString** gather_buf, UA_UInt32 gather_len) {
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
