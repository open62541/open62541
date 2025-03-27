/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
* See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>
#include "open62541/namespace_testnodeset_generated.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../src/util/ua_util_internal.h"
#include "common.h"

/*
 * To deploy this example, the server_testnodeset example must be running
 */

UA_DataTypeArray customTypesArray = { NULL, UA_TYPES_TESTNODESET_COUNT, UA_TYPES_TESTNODESET, UA_FALSE};

int main(void) {
    /*define the client*/
    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(cc);
    /*hand over the custom types array to the client config*/
    cc->customDataTypes = &customTypesArray;
    /*connect to a server*/
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Not connected. Retrying to connect in 1 second");
        sleep_ms(1000);
    }
    /*get the nodeId of the PointWithArray_scalar_noInit from its browsepath*/
    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_String path = UA_String_fromChars("/2:NotBuiltinTypes/2:Point_scalar_init");
    UA_RelativePath_parse(&bp.relativePath, path);
    UA_String_clear(&path);
    UA_NodeId varId;
    UA_NodeId_init(&varId);
    UA_BrowsePathResult res = UA_Client_translateBrowsePathToNodeIds(client, &bp);
    UA_BrowsePath_clear(&bp);
    if(res.statusCode != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Failed to resolve the BrowsePath");
    varId = res.targets[res.targetsSize-1].targetId.nodeId;

    /*read the current value with the client*/
    UA_Variant value;
    UA_Variant_init(&value);
    retval = UA_Client_readValueAttribute(client, varId, &value);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Failed to read the variable node");

    /*convert the read value to custom UA_Point data type*/
    UA_ExtensionObject ext = *(UA_ExtensionObject*) value.data;
    /*decode the extensionobject*/
    const UA_DataType *typ = UA_Client_findDataType(client, &UA_TYPES_TESTNODESET[UA_TYPES_TESTNODESET_POINT].typeId);
    UA_DecodeBinaryOptions options;
    options.customTypes = cc->customDataTypes;
    UA_ExtensionObject dec;
    UA_ExtensionObject_init(&dec);
    dec.encoding = UA_EXTENSIONOBJECT_DECODED;
    dec.content.decoded.type = typ;
    dec.content.decoded.data = UA_malloc(typ->memSize);
    retval = UA_decodeBinary(&ext.content.encoded.body, dec.content.decoded.data, typ, &options);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Failed to decode the ExtensionObject");
    UA_Variant_clear(&value);
    /*convert the read value to a UA_Point structure*/
    UA_Point pointValue = *(UA_Point*) dec.content.decoded.data;
    /*Print out results*/
    UA_String out = UA_STRING_NULL;
    UA_print(&pointValue, &UA_TYPES_TESTNODESET[UA_TYPES_TESTNODESET_POINT], &out);
    printf("The Variable has the custom Data Type Value: %.*s\n", (int)out.length, out.data);
    UA_String_clear(&out);

    /*write a value to the server*/
    UA_Point newPointValue;
    newPointValue.x = 10;
    newPointValue.y = 42;

    /*store the value inside an ExtensionObject and encode it*/
    UA_ExtensionObject new_val;
    UA_ExtensionObject_init(&new_val);
    UA_EncodeBinaryOptions new_options;
    UA_encodeBinary(&newPointValue, typ, &new_val.content.encoded.body, &new_options);
    /*here, we use the typeid, which was initially read from the server to provide to correct server side typeId*/
    new_val.content.encoded.typeId = ext.content.encoded.typeId;
    new_val.encoding = ext.encoding;

    /*write the value into the server*/
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &new_val, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    retval = UA_Client_writeValueAttribute(client, varId, &value);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Failed to write the server Node");
    /* Clean up */
    UA_BrowsePathResult_clear(&res);
    UA_ByteString_clear(&new_val.content.encoded.body);
    UA_free(dec.content.decoded.data);
    UA_Client_delete(client); /* Disconnects the client internally */
    return EXIT_SUCCESS;
}

