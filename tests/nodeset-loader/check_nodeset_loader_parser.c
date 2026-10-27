/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "ua_nodeset_loader_internal.h"

#include <open62541/server_config_default.h>

#include <stdio.h>
#include <string.h>

typedef struct {
    UA_Server *server;
    NodeSet nodeset;
} ParserFixture;

static void
parserFixtureClear(ParserFixture *fixture) {
    UA_NodeSet_clear(&fixture->nodeset);
    UA_Server_delete(fixture->server);
}

static bool
parserFixtureInit(ParserFixture *fixture) {
    memset(fixture, 0, sizeof(*fixture));
    fixture->server = UA_Server_new();
    if(!fixture->server)
        return false;
    UA_ServerConfig *config = UA_Server_getConfig(fixture->server);
    if(UA_ServerConfig_setDefault(config) != UA_STATUSCODE_GOOD) {
        parserFixtureClear(fixture);
        return false;
    }
    UA_StatusCode status = UA_NodeSet_init(&fixture->nodeset, fixture->server);
    if(status == UA_STATUSCODE_GOOD)
        return true;
    parserFixtureClear(fixture);
    return false;
}

static UA_StatusCode
importPath(NodeSet *nodeset, const char *path) {
    FILE *file = fopen(path, "rb");
    if(!file)
        return UA_STATUSCODE_BADNOTFOUND;
    if(fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    long length = ftell(file);
    if(length < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_ByteString xml = UA_BYTESTRING_NULL;
    UA_StatusCode status = UA_ByteString_allocBuffer(&xml, (size_t)length);
    if(status == UA_STATUSCODE_GOOD && fread(xml.data, 1, xml.length, file) != xml.length)
        status = UA_STATUSCODE_BADINTERNALERROR;
    fclose(file);

    if(status == UA_STATUSCODE_GOOD)
        status = UA_NodeSet_import(nodeset, &xml);
    UA_ByteString_clear(&xml);
    return status;
}

static bool
stringEqual(const UA_String *value, const char *expected) {
    size_t length = strlen(expected);
    return value->length == length && !memcmp(value->data, expected, length);
}

typedef struct {
    bool objectSeen;
    bool variableSeen;
    bool memberTypeSeen;
    bool dependentTypeSeen;
    bool failed;
} TestContext;

static bool
checkNode(void *context, NL_Node *node) {
    TestContext *test = (TestContext *)context;
    if(node->id.identifierType != UA_NODEIDTYPE_NUMERIC)
        return true;

    if(node->id.identifier.numeric == 50002) {
        test->memberTypeSeen = true;
    } else if(node->id.identifier.numeric == 50003) {
        test->dependentTypeSeen = true;
    } else if(node->id.identifier.numeric == 50000) {
        test->objectSeen = true;
        if(!stringEqual(&node->browseName.name, "A&B"))
            fprintf(stderr, "BrowseName was not entity-decoded\n");
        if(!stringEqual(&node->displayName.locale, "en&US"))
            fprintf(stderr, "Locale was not entity-decoded\n");
        if(!stringEqual(&node->displayName.text, "A < B & C"))
            fprintf(stderr, "DisplayName was not entity-decoded\n");
        test->failed |= !stringEqual(&node->browseName.name, "A&B") ||
                        !stringEqual(&node->displayName.locale, "en&US") ||
                        !stringEqual(&node->displayName.text, "A < B & C");
    } else if(node->id.identifier.numeric == 50001) {
        test->variableSeen = true;
        const NL_VariableNode *variable = (const NL_VariableNode *)node;
        if(variable->valueRank != UA_VALUERANK_SCALAR || variable->accessLevel != 1 ||
           variable->userAccessLevel != 1 || variable->minimumSamplingInterval != -1.0 ||
           !variable->historizing) {
            fprintf(stderr, "Malformed attributes did not retain their defaults\n");
            test->failed = true;
        }
        if(!stringEqual(&variable->value, "<x:Value><uax:String>hello &amp; "
                                          "goodbye</uax:String></x:Value>")) {
            fprintf(stderr, "Raw Value was not preserved: %.*s\n", (int)variable->value.length,
                    variable->value.data);
            test->failed = true;
        }
        if(!stringEqual(&node->description.text, "After Value")) {
            fprintf(stderr, "Element after Value was not parsed\n");
            test->failed = true;
        }
    } else if(node->id.identifier.numeric == 59999) {
        fprintf(stderr, "Node below an unknown element was imported\n");
        test->failed = true;
    }
    return !test->failed;
}

static bool
parseValid(const char *directory) {
    char path[1024];
    int written = snprintf(path, sizeof(path), "%s/parser.xml", directory);
    if(written < 0 || (size_t)written >= sizeof(path))
        return false;

    ParserFixture fixture;
    if(!parserFixtureInit(&fixture))
        return false;

    bool success = importPath(&fixture.nodeset, path) == UA_STATUSCODE_GOOD;

    TestContext context;
    memset(&context, 0, sizeof(context));
    if(success) {
        UA_NodeSet_resolveReferences(&fixture.nodeset);
        for(NL_Node *node = fixture.nodeset.nodes; node; node = node->next) {
            if(!checkNode(&context, node)) {
                success = false;
                break;
            }
        }
    }
    success = success && context.objectSeen && context.variableSeen && context.memberTypeSeen &&
              context.dependentTypeSeen && !context.failed;
    parserFixtureClear(&fixture);
    return success;
}

static bool
rejectMalformed(const char *directory) {
    char path[1024];
    int written = snprintf(path, sizeof(path), "%s/parser_malformed.xml", directory);
    if(written < 0 || (size_t)written >= sizeof(path))
        return false;

    ParserFixture fixture;
    if(!parserFixtureInit(&fixture))
        return false;

    bool rejected = importPath(&fixture.nodeset, path) != UA_STATUSCODE_GOOD;
    parserFixtureClear(&fixture);
    return rejected;
}

static bool
rejectInvalidNodes(const char *directory) {
    char path[1024];
    int written = snprintf(path, sizeof(path), "%s/invalidNodeDefinitions.xml", directory);
    if(written < 0 || (size_t)written >= sizeof(path))
        return false;

    ParserFixture fixture;
    if(!parserFixtureInit(&fixture))
        return false;

    bool rejected = importPath(&fixture.nodeset, path) != UA_STATUSCODE_GOOD;
    parserFixtureClear(&fixture);
    return rejected;
}

static bool
rejectUnsortable(const char *directory) {
    char path[1024];
    int written = snprintf(path, sizeof(path), "%s/unsortable.xml", directory);
    if(written < 0 || (size_t)written >= sizeof(path))
        return false;

    ParserFixture fixture;
    if(!parserFixtureInit(&fixture))
        return false;

    bool success = importPath(&fixture.nodeset, path) == UA_STATUSCODE_GOOD;
    bool rejected = success && UA_NodeSet_apply(&fixture.nodeset) == UA_STATUSCODE_GOOD;
    for(NL_Node *node = fixture.nodeset.nodes; rejected && node; node = node->next)
        rejected = node->addState == NL_NODE_REJECTED;
    parserFixtureClear(&fixture);
    return rejected;
}

static bool
parseLegacyAttributeOverrides(void) {
    const UA_XmlElement xml = UA_STRING_STATIC(
        "<UANodeSet><NamespaceUris><Uri>urn:open62541:parser:attributes</Uri>"
        "</NamespaceUris>"
        "<UAObjectType NodeId=\"ns=1;i=1\" BrowseName=\"1:Object\" IsAbstract=\"false\">"
        "<IsAbstract>true</IsAbstract></UAObjectType>"
        "<UAVariableType NodeId=\"ns=1;i=2\" BrowseName=\"1:VariableType\" "
        "DataType=\"i=12\" ValueRank=\"-1\" ArrayDimensions=\"9\">"
        "<IsAbstract>1</IsAbstract><DataType>Int32</DataType><ValueRank>2</ValueRank>"
        "<ArrayDimensions><ListOfUInt32><UInt32>2</UInt32><UInt32>3</UInt32>"
        "</ListOfUInt32></ArrayDimensions></UAVariableType>"
        "<UADataType NodeId=\"ns=1;i=3\" BrowseName=\"1:DataType\" IsAbstract=\"true\">"
        "<IsAbstract>false</IsAbstract></UADataType>"
        "<UAReferenceType NodeId=\"ns=1;i=4\" BrowseName=\"1:ReferenceType\" IsAbstract=\"true\">"
        "<IsAbstract>0</IsAbstract></UAReferenceType>"
        "<UAVariable NodeId=\"ns=1;i=5\" BrowseName=\"1:Variable\" Historizing=\"true\">"
        "<WriteMask>4294967295</WriteMask><UserWriteMask>4294967295</UserWriteMask>"
        "<AccessLevel>255</AccessLevel><UserAccessLevel>255</UserAccessLevel>"
        "<Historizing>0</Historizing>"
        "<ArrayDimensions><ListOfUInt32><UInt32>2</UInt32><UInt32>3</UInt32>"
        "</ListOfUInt32></ArrayDimensions><ArrayDimensions>4,5</ArrayDimensions>"
        "</UAVariable>"
        /* Unresolved symbolic types remain null until the server applies the nodes. */
        "<UAVariable NodeId=\"ns=1;i=6\" BrowseName=\"1:Unresolved\" "
        "DataType=\"ns=1;s=PreviousType\" Historizing=\"true\">"
        "<DataType>UnknownAlias</DataType><Historizing>false</Historizing>"
        "</UAVariable></UANodeSet>");
    ParserFixture fixture;
    if(!parserFixtureInit(&fixture))
        return false;
    bool success = UA_NodeSet_import(&fixture.nodeset, &xml) == UA_STATUSCODE_GOOD &&
                   fixture.nodeset.namespaceMapping.remote2localSize == 2;
    NL_Node *nodes[6] = {NULL, NULL, NULL, NULL, NULL, NULL};
    if(success) {
        UA_UInt16 ns = fixture.nodeset.namespaceMapping.remote2local[1];
        for(size_t i = 0; i < 6; i++) {
            UA_NodeId id = UA_NODEID_NUMERIC(ns, (UA_UInt32)i + 1);
            nodes[i] = UA_NodeSet_findNode(&fixture.nodeset, &id);
            success = success && nodes[i] != NULL;
        }
    }
    const UA_NodeClass classes[6] = {UA_NODECLASS_OBJECTTYPE, UA_NODECLASS_VARIABLETYPE,
        UA_NODECLASS_DATATYPE, UA_NODECLASS_REFERENCETYPE,
        UA_NODECLASS_VARIABLE, UA_NODECLASS_VARIABLE};
    for(size_t i = 0; success && i < 6; i++)
        success = nodes[i]->nodeClass == classes[i];
    if(success) {
        const NL_VariableTypeNode *type = (const NL_VariableTypeNode*)nodes[1];
        const NL_VariableNode *variable = (const NL_VariableNode*)nodes[4];
        const NL_VariableNode *unresolved = (const NL_VariableNode*)nodes[5];
        UA_NodeId int32Id = UA_NODEID_NUMERIC(0, 6);
        success = ((NL_ObjectTypeNode*)nodes[0])->isAbstract && type->isAbstract &&
                  !((NL_DataTypeNode*)nodes[2])->isAbstract &&
                  !((NL_ReferenceTypeNode*)nodes[3])->isAbstract &&
                  UA_NodeId_equal(&type->datatype, &int32Id) &&
                  type->valueRankDefined && type->valueRank == 2 &&
                  type->arrayDimensionsOwned && type->arrayDimensions &&
                  strcmp(type->arrayDimensions, "2,3") == 0 &&
                  variable->writeMask == UA_UINT32_MAX &&
                  variable->userWriteMask == UA_UINT32_MAX &&
                  variable->accessLevel == UA_BYTE_MAX &&
                  variable->userAccessLevel == UA_BYTE_MAX && !variable->historizing &&
                  !variable->arrayDimensionsOwned && variable->arrayDimensions &&
                  strcmp(variable->arrayDimensions, "4,5") == 0 &&
                  !unresolved->historizing && UA_NodeId_isNull(&unresolved->datatype);
    }
    if(!success)
        fprintf(stderr, "Legacy child attributes did not override their XML attributes\n");
    parserFixtureClear(&fixture);
    return success;
}

static bool
rejectInvalidChildAttributes(void) {
    static const struct {
        const char *nodeType;
        const char *children;
    } cases[] = {
        {"UAVariable", "<WriteMask/>"},
        {"UAVariable", "<WriteMask>-1</WriteMask>"},
        {"UAVariable", "<WriteMask>4294967296</WriteMask>"},
        {"UAVariable", "<UserWriteMask>1x</UserWriteMask>"},
        {"UAVariable", "<ValueRank>2147483648</ValueRank>"},
        {"UAVariableType", "<ValueRank>-2147483649</ValueRank>"},
        {"UAVariable", "<AccessLevel>256</AccessLevel>"},
        {"UAVariable", "<UserAccessLevel>-1</UserAccessLevel>"},
        {"UAVariable", "<MinimumSamplingInterval>1.5x</MinimumSamplingInterval>"},
        {"UAVariable", "<Historizing>yes</Historizing>"},
        {"UAObjectType", "<IsAbstract>2</IsAbstract>"},
        {"UAVariableType", "<IsAbstract>yes</IsAbstract>"},
        {"UADataType", "<IsAbstract>-1</IsAbstract>"},
        {"UAReferenceType", "<IsAbstract>TRUE</IsAbstract>"},
        {"UAVariable", "<ArrayDimensions><ListOfUInt32><UInt32>-1</UInt32>"
                       "</ListOfUInt32></ArrayDimensions>"},
        {"UAVariableType", "<ArrayDimensions><ListOfUInt32><UInt32>2</UInt32>"
                           "<UInt32>4294967296</UInt32></ListOfUInt32></ArrayDimensions>"},
        {"UAVariable", "<ArrayDimensions><UInt32/></ArrayDimensions>"},
        {"UAObject", "<ArrayDimensions><UInt32>1</UInt32></ArrayDimensions>"}
    };
    ParserFixture fixture;
    if(!parserFixtureInit(&fixture))
        return false;
    bool success = true;
    for(size_t i = 0; success && i < sizeof(cases) / sizeof(cases[0]); i++) {
        char buffer[512];
        int length = snprintf(buffer, sizeof(buffer),
                              "<UANodeSet><%s NodeId=\"i=60001\" BrowseName=\"Test\">"
                              "%s</%s></UANodeSet>",
                              cases[i].nodeType, cases[i].children, cases[i].nodeType);
        if(length < 0 || (size_t)length >= sizeof(buffer)) {
            success = false;
            break;
        }
        UA_XmlElement xml = {(size_t)length, (UA_Byte*)buffer};
        success = UA_NodeSet_import(&fixture.nodeset, &xml) == UA_STATUSCODE_BADDECODINGERROR;
        if(!success)
            fprintf(stderr, "Invalid child attribute accepted in case %u: %s\n",
                    (unsigned)i, cases[i].children);
        UA_NodeSet_clear(&fixture.nodeset);
        if(UA_NodeSet_init(&fixture.nodeset, fixture.server) != UA_STATUSCODE_GOOD)
            success = false;
    }
    parserFixtureClear(&fixture);
    return success;
}

static bool
rejectInvalidImportArguments(void) {
    ParserFixture fixture;
    if(!parserFixtureInit(&fixture))
        return false;
    const UA_XmlElement xml = UA_STRING_STATIC(
        "<UANodeSet><UAObject NodeId=\"i=60001\" BrowseName=\"Kept\"/></UANodeSet>");
    UA_XmlElement missingData = {1, NULL};
    UA_XmlElement oversized = {SIZE_MAX, (UA_Byte*)(uintptr_t)""};
    bool success =
        UA_NodeSet_init(NULL, fixture.server) == UA_STATUSCODE_BADINVALIDARGUMENT &&
        UA_NodeSet_import(NULL, &xml) == UA_STATUSCODE_BADINVALIDARGUMENT &&
        UA_NodeSet_import(&fixture.nodeset, NULL) == UA_STATUSCODE_BADINVALIDARGUMENT &&
        UA_NodeSet_import(&fixture.nodeset, &missingData) == UA_STATUSCODE_BADINVALIDARGUMENT &&
        UA_NodeSet_import(&fixture.nodeset, &oversized) == UA_STATUSCODE_BADINVALIDARGUMENT;
    if(success)
        success = UA_NodeSet_import(&fixture.nodeset, &xml) == UA_STATUSCODE_GOOD;
    if(success) {
        /* A second import cannot replace storage referenced by existing nodes. */
        char *text = fixture.nodeset.text;
        NL_Node *node = fixture.nodeset.nodes;
        success = UA_NodeSet_import(&fixture.nodeset, &xml) == UA_STATUSCODE_BADINVALIDARGUMENT &&
                  fixture.nodeset.text == text && node && fixture.nodeset.nodes == node &&
                  stringEqual(&node->browseName.name, "Kept");
    }
    if(!success)
        fprintf(stderr, "Invalid or repeated NodeSet import was not rejected\n");
    parserFixtureClear(&fixture);
    return success;
}

int
main(void) {
    if(!parseValid(OPEN62541_NODESET_LOADER_PARSER_DIR))
        return 2;
    if(!rejectMalformed(OPEN62541_NODESET_LOADER_PARSER_DIR))
        return 3;
    if(!rejectInvalidNodes(OPEN62541_NODESET_LOADER_PARSER_DIR))
        return 4;
    if(!rejectUnsortable(OPEN62541_NODESET_LOADER_PARSER_DIR))
        return 5;
    if(!parseLegacyAttributeOverrides())
        return 6;
    if(!rejectInvalidChildAttributes())
        return 7;
    if(!rejectInvalidImportArguments())
        return 8;
    return EXIT_SUCCESS;
}
