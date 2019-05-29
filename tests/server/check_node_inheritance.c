/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server_config_default.h>

#include "server/ua_server_internal.h"

#include <check.h>

UA_Server *server = NULL;
UA_UInt32 valueToBeInherited = 42;

static void setup(void) {
    server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

#ifdef UA_GENERATED_NAMESPACE_ZERO
/* finds the NodeId of a StateNumber child of a given node id */
static void
findChildId(UA_NodeId parentNode, UA_NodeId referenceType,
            const UA_QualifiedName targetName, UA_NodeId *result) {
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = referenceType;
    rpe.isInverse = false;
    rpe.includeSubtypes = false;
    rpe.targetName = targetName;

    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = parentNode;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;    //clion complains but is ok

    UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);

    ck_assert(bpr.targetsSize > 0);

    UA_NodeId_copy(&bpr.targets[0].targetId.nodeId, result);
    UA_BrowsePathResult_deleteMembers(&bpr);
}
#endif



START_TEST(Nodes_createCustomBrowseNameObjectType)
{
    /* Create a custom object type "CustomBrowseNameType" which has a
     * "DefaultInstanceBrowseName" property. */

    /* create new object type node which has a subcomponent of the type StateType */
    UA_ObjectTypeAttributes otAttr = UA_ObjectTypeAttributes_default;
    otAttr.displayName = UA_LOCALIZEDTEXT("", "CustomBrowseNameType");
    otAttr.description = UA_LOCALIZEDTEXT("", "");
    UA_StatusCode retval = UA_Server_addObjectTypeNode(server, UA_NODEID_NUMERIC(1, 7010),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                         UA_QUALIFIEDNAME(1, "CustomBrowseNameType"),
                                         otAttr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    // Now add a property "DefaultInstanceBrowseName"
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.minimumSamplingInterval = 0.000000;
    attr.userAccessLevel = 1;
    attr.accessLevel = 1;
    attr.valueRank = UA_VALUERANK_ANY;
    attr.dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_QUALIFIEDNAME);
    UA_QualifiedName defaultInstanceBrowseName = UA_QUALIFIEDNAME(1, "MyCustomBrowseName");
    UA_Variant_setScalar(&attr.value, &defaultInstanceBrowseName, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    attr.displayName = UA_LOCALIZEDTEXT("", "DefaultInstanceBrowseName");
    attr.description = UA_LOCALIZEDTEXT("", "");
    attr.writeMask = 0;
    attr.userWriteMask = 0;
    retval = UA_Server_addVariableNode(server,
                                       UA_NODEID_NUMERIC(1, 7011),
                                       UA_NODEID_NUMERIC(1, 7010),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                       UA_QUALIFIEDNAME(0, "DefaultInstanceBrowseName"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),
                                       attr,
                                       NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

} END_TEST



START_TEST(Nodes_checkDefaultInstanceBrowseName) {
    /* create an object/instance of the CustomDemoType.
     * This should fail if we do not specifiy a browse name.
     * CustomDemoType does not have a DefaultInstanceBrowseName
     * */
    UA_ObjectAttributes oAttr2 = UA_ObjectAttributes_default;
    oAttr2.displayName = UA_LOCALIZEDTEXT("", "DemoCustomBrowseNameFail");
    oAttr2.description = UA_LOCALIZEDTEXT("", "");
    UA_QualifiedName nullName;
    UA_QualifiedName_init(&nullName);
    UA_StatusCode retval =
            UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 7020),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                    nullName, UA_NODEID_NUMERIC(1, 6010),
                                    oAttr2, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADBROWSENAMEINVALID);

    /* create an object/instance of the CustomBrowseNameType and set the default browse name */
    oAttr2 = UA_ObjectAttributes_default;
    oAttr2.displayName = UA_LOCALIZEDTEXT("", "DemoCustomBrowseName");
    oAttr2.description = UA_LOCALIZEDTEXT("", "");
    UA_QualifiedName_init(&nullName);
    retval =
            UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 7021),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                    nullName, UA_NODEID_NUMERIC(1, 7010),
                                    oAttr2, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_QualifiedName receivedBrowseName;
    UA_QualifiedName_init(&receivedBrowseName);

    UA_QualifiedName defaultInstanceBrowseName = UA_QUALIFIEDNAME(1, "MyCustomBrowseName");

    retval = UA_Server_readBrowseName(server, UA_NODEID_NUMERIC(1, 7021), &receivedBrowseName);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(UA_QualifiedName_equal(&receivedBrowseName, &defaultInstanceBrowseName) == true);
    UA_QualifiedName_clear(&receivedBrowseName);

    /* create an object/instance of the CustomBrowseNameType and set a custom browse name */
    oAttr2 = UA_ObjectAttributes_default;
    oAttr2.displayName = UA_LOCALIZEDTEXT("", "DemoCustomBrowseName");
    oAttr2.description = UA_LOCALIZEDTEXT("", "");
    UA_QualifiedName overriddenBrowseName = UA_QUALIFIEDNAME(1, "MyOverriddenBrowseName");
    retval =
            UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 7022),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                    overriddenBrowseName, UA_NODEID_NUMERIC(1, 7010),
                                    oAttr2, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_QualifiedName_init(&receivedBrowseName);

    retval = UA_Server_readBrowseName(server, UA_NODEID_NUMERIC(1, 7022), &receivedBrowseName);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(UA_QualifiedName_equal(&receivedBrowseName, &overriddenBrowseName) == true);
    UA_QualifiedName_clear(&receivedBrowseName);
}
END_TEST

START_TEST(Nodes_createCustomStateType) {
    // Create a type "CustomStateType" with a variable "CustomStateNumber" as property
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    UA_ObjectTypeAttributes attrObject = UA_ObjectTypeAttributes_default;
    attrObject.displayName = UA_LOCALIZEDTEXT("", "CustomStateType");
    attrObject.description = UA_LOCALIZEDTEXT("", "");
    attrObject.writeMask = 0;
    attrObject.userWriteMask = 0;
    retval = UA_Server_addObjectTypeNode(server,
                                         UA_NODEID_NUMERIC(1, 6000),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                         UA_QUALIFIEDNAME(1, "CustomStateType"),
                                         attrObject,
                                         NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    // Now add a property "StateNumber"
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.minimumSamplingInterval = 0.000000;
    attr.userAccessLevel = 1;
    attr.accessLevel = 1;
    attr.valueRank = UA_VALUERANK_ANY;
    attr.dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_UINT32);
    UA_UInt32 val = 0;
    UA_Variant_setScalar(&attr.value, &val, &UA_TYPES[UA_TYPES_UINT32]);
    attr.displayName = UA_LOCALIZEDTEXT("", "CustomStateNumber");
    attr.description = UA_LOCALIZEDTEXT("", "");
    attr.writeMask = 0;
    attr.userWriteMask = 0;
    retval = UA_Server_addVariableNode(server,
                                       UA_NODEID_NUMERIC(1, 6001),
                                       UA_NODEID_NUMERIC(1, 6000),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                       UA_QUALIFIEDNAME(1, "CustomStateNumber"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),
                                       attr,
                                       NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

/* Minimal nodeset does not contain the modelling rule mandatory */
#ifdef UA_GENERATED_NAMESPACE_ZERO
    retval = UA_Server_addReference(server, UA_NODEID_NUMERIC(1, 6001),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                                    UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY),
                                    true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
#endif
}
END_TEST


START_TEST(Nodes_createCustomObjectType) {
    /* Create a custom object type "CustomDemoType" which has a
     * "CustomStateType" component */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* create new object type node which has a subcomponent of the type StateType */
    UA_ObjectTypeAttributes otAttr = UA_ObjectTypeAttributes_default;
    otAttr.displayName = UA_LOCALIZEDTEXT("", "CustomDemoType");
    otAttr.description = UA_LOCALIZEDTEXT("", "");
    retval = UA_Server_addObjectTypeNode(server, UA_NODEID_NUMERIC(1, 6010),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                         UA_QUALIFIEDNAME(1, "CustomDemoType"),
                                         otAttr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("", "State");
    oAttr.description = UA_LOCALIZEDTEXT("", "");
    retval = UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 6011), UA_NODEID_NUMERIC(1, 6010),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                     UA_QUALIFIEDNAME(1, "State"),
                                     UA_NODEID_NUMERIC(1, 6000),
                                     oAttr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

/* Minimal nodeset does not contain the modelling rule mandatory */
#ifdef UA_GENERATED_NAMESPACE_ZERO
    /* modelling rule is mandatory so it will be inherited for the object
     * created from CustomDemoType */
    retval = UA_Server_addReference(server, UA_NODEID_NUMERIC(1, 6011),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                                    UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY),
                                    true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);


    /* assign a default value to the attribute "StateNumber" inside the state
     * attribute (part of the MyDemoType) */
    UA_Variant stateNum;
    UA_Variant_init(&stateNum);
    UA_Variant_setScalar(&stateNum, &valueToBeInherited, &UA_TYPES[UA_TYPES_UINT32]);
    UA_NodeId childID;
    findChildId(UA_NODEID_NUMERIC(1, 6011), UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                UA_QUALIFIEDNAME(1, "CustomStateNumber"), &childID);
    ck_assert(!UA_NodeId_isNull(&childID));

    retval = UA_Server_writeValue(server, childID, stateNum);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

#endif

}
END_TEST

START_TEST(Nodes_createInheritedObject) {
    /* create an object/instance of the demo type */
    UA_ObjectAttributes oAttr2 = UA_ObjectAttributes_default;
    oAttr2.displayName = UA_LOCALIZEDTEXT("", "Demo");
    oAttr2.description = UA_LOCALIZEDTEXT("", "");
    UA_StatusCode retval =
        UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 6020),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                UA_QUALIFIEDNAME(1, "Demo"), UA_NODEID_NUMERIC(1, 6010),
                                oAttr2, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(Nodes_checkInheritedValue) {
/* Minimal nodeset does not contain the modelling rule mandatory, therefore there is no CustomStateNumber */
#ifdef UA_GENERATED_NAMESPACE_ZERO
    UA_NodeId childState;
    findChildId(UA_NODEID_NUMERIC(1, 6020),
                UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                UA_QUALIFIEDNAME(1, "State"), &childState);
    ck_assert(!UA_NodeId_isNull(&childState));
    UA_NodeId childNumber;
    findChildId(childState, UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                UA_QUALIFIEDNAME(1, "CustomStateNumber"), &childNumber);
    ck_assert(!UA_NodeId_isNull(&childNumber));

    UA_Variant inheritedValue;
    UA_Variant_init(&inheritedValue);
    UA_Server_readValue(server, childNumber, &inheritedValue);
    ck_assert(inheritedValue.type == &UA_TYPES[UA_TYPES_UINT32]);

    UA_UInt32 *value = (UA_UInt32 *) inheritedValue.data;

    ck_assert_int_eq(*value, valueToBeInherited);
    UA_Variant_deleteMembers(&inheritedValue);
#endif
}
END_TEST



START_TEST(Nodes_createCustomInterfaceType) {
/* Minimal nodeset does not have the Interface definitions */
#ifdef UA_GENERATED_NAMESPACE_ZERO
    /* Create a custom interface type */

    UA_ObjectTypeAttributes otAttr = UA_ObjectTypeAttributes_default;
    otAttr.displayName = UA_LOCALIZEDTEXT("", "ILocationType");
    otAttr.description = UA_LOCALIZEDTEXT("", "");
    UA_StatusCode retval = UA_Server_addObjectTypeNode(server, UA_NODEID_NUMERIC(1, 8000),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_BASEINTERFACETYPE),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                         UA_QUALIFIEDNAME(1, "ILocationType"),
                                         otAttr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("", "InterfaceChild");
    oAttr.description = UA_LOCALIZEDTEXT("", "");
    retval = UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 8001), UA_NODEID_NUMERIC(1, 8000),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                     UA_QUALIFIEDNAME(1, "InterfaceChild"),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                     oAttr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Server_addReference(server, UA_NODEID_NUMERIC(1, 8001),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                                    UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY),
                                    true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);


    /* create an object type which has the interface */
    otAttr = UA_ObjectTypeAttributes_default;
    otAttr.displayName = UA_LOCALIZEDTEXT("", "ObjectWithLocation");
    otAttr.description = UA_LOCALIZEDTEXT("", "");
    retval = UA_Server_addObjectTypeNode(server, UA_NODEID_NUMERIC(1, 8002),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                         UA_QUALIFIEDNAME(1, "ObjectWithLocation"),
                                         otAttr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Server_addReference(server, UA_NODEID_NUMERIC(1, 8002), UA_NODEID_NUMERIC(0, UA_NS0ID_HASINTERFACE), UA_EXPANDEDNODEID_NUMERIC(1, 8000), true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
#endif
}
END_TEST



START_TEST(Nodes_createObjectWithInterface) {

/* Minimal nodeset does not have the Interface definitions */
#ifdef UA_GENERATED_NAMESPACE_ZERO
    /* create an object/instance of the demo type */
    UA_ObjectAttributes oAttr2 = UA_ObjectAttributes_default;
    oAttr2.displayName = UA_LOCALIZEDTEXT("", "ObjectInstanceOfInterface");
    oAttr2.description = UA_LOCALIZEDTEXT("", "");
    UA_StatusCode retval =
            UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 8020),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                    UA_QUALIFIEDNAME(1, "ObjectInstanceOfInterface"), UA_NODEID_NUMERIC(1, 8002),
                                    oAttr2, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);


    /* Check that object has inherited the interface children */

    UA_NodeId childId;
    findChildId(UA_NODEID_NUMERIC(1, 8020),
                UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                UA_QUALIFIEDNAME(1, "InterfaceChild"), &childId);
    ck_assert(!UA_NodeId_isNull(&childId));
#endif
}
END_TEST

static Suite *testSuite_Client(void) {
    Suite *s = suite_create("Node inheritance");
    TCase *tc_inherit_subtype = tcase_create("Inherit subtype value");
    tcase_add_unchecked_fixture(tc_inherit_subtype, setup, teardown);
    tcase_add_test(tc_inherit_subtype, Nodes_createCustomStateType);
    tcase_add_test(tc_inherit_subtype, Nodes_createCustomObjectType);
    tcase_add_test(tc_inherit_subtype, Nodes_createInheritedObject);
    tcase_add_test(tc_inherit_subtype, Nodes_checkInheritedValue);
    tcase_add_test(tc_inherit_subtype, Nodes_createCustomBrowseNameObjectType);
    tcase_add_test(tc_inherit_subtype, Nodes_checkDefaultInstanceBrowseName);
    suite_add_tcase(s, tc_inherit_subtype);
    TCase *tc_interface_addin = tcase_create("Interfaces and Addins");
    tcase_add_unchecked_fixture(tc_interface_addin, setup, teardown);
    tcase_add_test(tc_interface_addin, Nodes_createCustomInterfaceType);
    tcase_add_test(tc_interface_addin, Nodes_createObjectWithInterface);
    suite_add_tcase(s, tc_interface_addin);
    return s;
}

int main(void) {
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
