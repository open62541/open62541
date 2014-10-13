#ifndef UA_TRANSPORT_H_
#define UA_TRANSPORT_H_

#include "ua_types.h"
#include "ua_types_encoding_binary.h"

typedef enum SecurityTokenRequestType {
    UA_SECURITYTOKEN_ISSUE = 0,
    UA_SECURITYTOKEN_RENEW = 1
} SecurityTokenRequestType;

typedef enum {
    UA_SECURITYMODE_INVALID        = 0,
    UA_SECURITYMODE_NONE           = 1,
    UA_SECURITYMODE_SIGNANDENCRYPT = 2
} SecurityMode;

/* MessageType */
typedef UA_Int32 UA_MessageType;
enum UA_MessageType {
    UA_MESSAGETYPE_HEL = 0x48454C, // H E L
    UA_MESSAGETYPE_ACK = 0x41434B, // A C k
    UA_MESSAGETYPE_ERR = 0x455151, // E R R
    UA_MESSAGETYPE_OPN = 0x4F504E, // O P N
    UA_MESSAGETYPE_MSG = 0x4D5347, // M S G
    UA_MESSAGETYPE_CLO = 0x434C4F  // C L O
};
UA_TYPE_PROTOTYPES(UA_MessageType)
UA_TYPE_BINARY_ENCODING(UA_MessageType)

/* All other transport types are auto-generated from a schema definition. */

#endif /* UA_TRANSPORT_H_ */
