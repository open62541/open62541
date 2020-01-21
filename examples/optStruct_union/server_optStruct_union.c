/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

#include "optStruct_union.h"

#include "ua_types_encoding_binary.h"

#include <signal.h>
#include <stdlib.h>
#include <open62541/types.h>

UA_Boolean running = true;

static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}
static void
addOptDataType(UA_Server *server){
    UA_DataTypeAttributes dattr = UA_DataTypeAttributes_default;
    dattr.description = UA_LOCALIZEDTEXT("en-US", "Example structure with optional field");
    dattr.displayName = UA_LOCALIZEDTEXT("en-US", "OptType");
    dattr.isAbstract = false;

    UA_StatusCode s = UA_Server_addDataTypeNode(server, OptType.typeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_STRUCTURE),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                              UA_QUALIFIEDNAME(1, "OptType"),
                              dattr, NULL, NULL);
    printf("optDataType: %s", UA_StatusCode_name(s));
}

static void
addOptVariable(UA_Server *server){
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    vattr.description = UA_LOCALIZEDTEXT("en-US", "OptVar");
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "OptVar");
    vattr.dataType = OptType.typeId;
    vattr.valueRank = UA_VALUERANK_SCALAR;

    Opt o;
    o.hasB = true;      /* means optional field "b" is defined and will be encoded */
    o.a = 3;
    o.b = 2.5;

    UA_ByteString *optBuf = UA_ByteString_new();
    size_t optMsgSize = UA_calcSizeBinary(&o, &OptType);
    UA_ByteString_allocBuffer(optBuf, optMsgSize);
    memset(optBuf->data, 0, optMsgSize);
    UA_Byte *optBufPos = optBuf->data;
    const UA_Byte *optBufEnd = &optBuf->data[optBuf->length];
    UA_StatusCode st = UA_encodeBinary(&o, &OptType, &optBufPos, &optBufEnd, NULL, NULL);
    if(st == UA_STATUSCODE_GOOD) {
        UA_ExtensionObject optEo;
        optEo.encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
        optEo.content.encoded.typeId = UA_NODEID_NUMERIC(1, OptType.binaryEncodingId);
        optEo.content.encoded.body = *optBuf;
        UA_Variant_setScalar(&vattr.value, &optEo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);

        UA_StatusCode s = UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "OptVar"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                  UA_QUALIFIEDNAME(1, "OptVar"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);
        printf("optVar: %s", UA_StatusCode_name(s));
    }
}

static void
addUniDataType(UA_Server *server){
    UA_DataTypeAttributes dattr = UA_DataTypeAttributes_default;
    dattr.description = UA_LOCALIZEDTEXT("en-US", "Example union");
    dattr.displayName = UA_LOCALIZEDTEXT("en-US", "UniType");
    dattr.isAbstract = false;

    UA_StatusCode s = UA_Server_addDataTypeNode(server, UniType.typeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_UNION),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                              UA_QUALIFIEDNAME(1, "OptType"),
                              dattr, NULL, NULL);
    printf("uniDataType: %s", UA_StatusCode_name(s));
}

static void
addUniVariable(UA_Server *server){
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    vattr.description = UA_LOCALIZEDTEXT("en-US", "UniVar");
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "UniVar");
    vattr.dataType = UniType.typeId;
    vattr.valueRank = UA_VALUERANK_SCALAR;

    Uni u;
    u.switchField = 2;      /* means only second field "y" is defined for the union and will be encoded */
    u.x = 4.3;
    u.y = UA_STRING("Test");

    UA_ByteString *uniBuf = UA_ByteString_new();
    size_t uniMsgSize = UA_calcSizeBinary(&u, &UniType);
    UA_ByteString_allocBuffer(uniBuf, uniMsgSize);
    memset(uniBuf->data, 0, uniMsgSize);
    UA_Byte *uniBufPos = uniBuf->data;
    const UA_Byte *uniBufEnd = &uniBuf->data[uniBuf->length];
    UA_StatusCode st = UA_encodeBinary(&u, &UniType, &uniBufPos, &uniBufEnd, NULL, NULL);
    if(st == UA_STATUSCODE_GOOD) {
        UA_ExtensionObject uniEo;
        uniEo.encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
        uniEo.content.encoded.typeId = UA_NODEID_NULL;
        uniEo.content.encoded.body = *uniBuf;
        UA_Variant_setScalar(&vattr.value, &uniEo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);

        UA_StatusCode s = UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "UniVar"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                  UA_QUALIFIEDNAME(1, "UniVar"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);

        printf("uniVar: %s", UA_StatusCode_name(s));
    }
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    /* Make the custom optStruct and union known to the stack */
    UA_DataType *types = (UA_DataType*)UA_malloc(sizeof(UA_DataType) * 2);

    UA_DataTypeMember *optMembers = (UA_DataTypeMember*)UA_malloc(sizeof(UA_DataTypeMember) * 2);
    optMembers[0] = Opt_members[0];
    optMembers[1] = Opt_members[1];

    types[0] = OptType;
    types[0].members = optMembers;

    UA_DataTypeMember *uniMembers = (UA_DataTypeMember*)UA_malloc(sizeof(UA_DataTypeMember) * 3);
    uniMembers[0] = Uni_members[0];
    uniMembers[1] = Uni_members[1];
    uniMembers[2] = Uni_members[2];

    types[1] = UniType;
    types[1].members = uniMembers;

    /* Attention! Here the custom datatypes are allocated on the stack. So they
     * cannot be accessed from parallel (worker) threads. */
    UA_DataTypeArray customDataTypes = {config->customDataTypes, 2, types};
    config->customDataTypes = &customDataTypes;

    addOptDataType(server);
    addOptVariable(server);

    addUniDataType(server);
    addUniVariable(server);

    UA_Server_run(server, &running);

    UA_Server_delete(server);
    UA_free(optMembers);
    UA_free(uniMembers);
    UA_free(types);
    return EXIT_SUCCESS;
}
