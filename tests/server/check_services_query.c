/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "test_helpers.h"

#include "check.h"

static UA_Server *server = NULL;
UA_NodeId hasChildId, hasAnimalId, hasPetId, hasFarmAnimalId, hasScheduleId,
    personTypeId, animalTypeId, dogTypeId, catTypeId, pigTypeId,
    scheduleTypeId, feedingScheduleTypeId, areaTypeId;

static void addObject(UA_NodeId objId, UA_NodeId parentId,
                      UA_NodeId typeId, char *name) {
    UA_ObjectAttributes oattr = UA_ObjectAttributes_default;
    oattr.displayName = UA_LOCALIZEDTEXT("", name);
    UA_StatusCode res =
        UA_Server_addObjectNode(server, objId, parentId, UA_NS0ID(HASCOMPONENT),
                                UA_QUALIFIEDNAME(1, name), typeId, oattr, NULL, NULL);
    ck_assert(res == UA_STATUSCODE_GOOD);
}

static void addProperty(UA_NodeId parentId, char *name, char *value) {
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("", name);
    UA_String strval = UA_STRING(value);
    UA_Variant_setScalar(&vattr.value, &strval, &UA_TYPES[UA_TYPES_STRING]);

    /* Add the schedule instances */
    UA_StatusCode res =
        UA_Server_addVariableNode(server, UA_NODEID_NULL,
                                  parentId, UA_NS0ID(HASCOMPONENT),
                                  UA_QUALIFIEDNAME(1, name),
                                  UA_NODEID_NULL,
                                  vattr, NULL, NULL);
    ck_assert(res == UA_STATUSCODE_GOOD);
}

static void addRef(UA_NodeId source, UA_NodeId refType, UA_NodeId target) {
    UA_ExpandedNodeId t2;
    UA_ExpandedNodeId_init(&t2);
    t2.nodeId = target;
    UA_Server_addReference(server, source, refType, t2, true);
}

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    /* Add the type model from Part 4, Appendix B2 */

    /* New Reference types:
     * "HasChild" derived from HierarchicalReference.
     * "HasAnimal" derived from HierarchicalReference.
     * "HasPet" derived from HasAnimal.
     * "HasFarmAnimal" derived from HasAnimal.
     * "HasSchedule" derived from HierarchicalReference. */
    UA_ReferenceTypeAttributes rattr = UA_ReferenceTypeAttributes_default;

    hasChildId = UA_NODEID_STRING(1, "HasChild");
    rattr.displayName = UA_LOCALIZEDTEXT("", "HasChild");
    rattr.inverseName = UA_LOCALIZEDTEXT("", "ChildOf");
    UA_Server_addReferenceTypeNode(server, hasChildId,
                                   UA_NS0ID(HIERARCHICALREFERENCES),
                                   UA_NS0ID(HASSUBTYPE),
                                   UA_QUALIFIEDNAME(1, "hasChild"),
                                   rattr, NULL, NULL);

    hasAnimalId = UA_NODEID_STRING(1, "HasAnimal");
    rattr.displayName = UA_LOCALIZEDTEXT("", "HasAnimal");
    rattr.inverseName = UA_LOCALIZEDTEXT("", "AnimalOf");
    UA_Server_addReferenceTypeNode(server, hasAnimalId,
                                   UA_NS0ID(HIERARCHICALREFERENCES),
                                   UA_NS0ID(HASSUBTYPE),
                                   UA_QUALIFIEDNAME(1, "hasAnimal"),
                                   rattr, NULL, NULL);

    hasPetId = UA_NODEID_STRING(1, "HasPet");
    rattr.displayName = UA_LOCALIZEDTEXT("", "HasPet");
    rattr.inverseName = UA_LOCALIZEDTEXT("", "PetOf");
    UA_Server_addReferenceTypeNode(server, hasPetId, hasAnimalId,
                                   UA_NS0ID(HASSUBTYPE),
                                   UA_QUALIFIEDNAME(1, "hasPet"),
                                   rattr, NULL, NULL);

    hasFarmAnimalId = UA_NODEID_STRING(1, "HasFarmAnimal");
    rattr.displayName = UA_LOCALIZEDTEXT("", "HasFarmAnimal");
    rattr.inverseName = UA_LOCALIZEDTEXT("", "FarmAnimalOf");
    UA_Server_addReferenceTypeNode(server, hasFarmAnimalId, hasAnimalId,
                                   UA_NS0ID(HASSUBTYPE),
                                   UA_QUALIFIEDNAME(1, "hasFarmAnimal"),
                                   rattr, NULL, NULL);

    hasScheduleId = UA_NODEID_STRING(1, "HasSchedule");
    rattr.displayName = UA_LOCALIZEDTEXT("", "HasSchedule");
    rattr.inverseName = UA_LOCALIZEDTEXT("", "ScheduleOf");
    UA_Server_addReferenceTypeNode(server, hasScheduleId,
                                   UA_NS0ID(HIERARCHICALREFERENCES),
                                   UA_NS0ID(HASSUBTYPE),
                                   UA_QUALIFIEDNAME(1, "hasSchedule"),
                                   rattr, NULL, NULL);

    /* PersonType derived from BaseObjectType adds:
     * HasProperty "LastName".
     * HasProperty "FirstName".
     * HasProperty "StreetAddress".
     * HasProperty "City".
     * HasProperty "ZipCode".
     * May have HasChild reference to a node of type PersonType.
     * May have HasAnimal reference to a node of type AnimalType (or a subtype
     *     of this Reference type). */
    UA_ObjectTypeAttributes otattr = UA_ObjectTypeAttributes_default;
    personTypeId = UA_NODEID_STRING(1, "PersonType");
    otattr.displayName = UA_LOCALIZEDTEXT("", "PersonType");
    UA_Server_addObjectTypeNode(server, personTypeId,
                                UA_NS0ID(BASEOBJECTTYPE),
                                UA_NS0ID(HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "PersonType"),
                                otattr, NULL, NULL);

    /* AnimalType derived from BaseObjectType adds:
     * May have HasSchedule reference to a node of type FeedingScheduleType.
     * HasProperty "Name". */
    animalTypeId = UA_NODEID_STRING(1, "AnimalType");
    otattr.displayName = UA_LOCALIZEDTEXT("", "AnimalType");
    UA_Server_addObjectTypeNode(server, animalTypeId,
                                UA_NS0ID(BASEOBJECTTYPE),
                                UA_NS0ID(HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "AnimalType"),
                                otattr, NULL, NULL);

    /* DogType derived from AnimalType adds:
     * HasProperty "NickName".
     * HasProperty "DogBreed".
     * HasProperty "License". */
    dogTypeId = UA_NODEID_STRING(1, "DogType");
    otattr.displayName = UA_LOCALIZEDTEXT("", "DogType");
    UA_Server_addObjectTypeNode(server, dogTypeId, animalTypeId,
                                UA_NS0ID(HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "DogType"),
                                otattr, NULL, NULL);
    
    /* CatType derived from AnimalType adds:
     * HasProperty "NickName".
     * HasProperty "CatBreed". */
    catTypeId = UA_NODEID_STRING(1, "CatType");
    otattr.displayName = UA_LOCALIZEDTEXT("", "CatType");
    UA_Server_addObjectTypeNode(server, catTypeId, animalTypeId,
                                UA_NS0ID(HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "CatType"),
                                otattr, NULL, NULL);

    /* PigType derived from AnimalType adds:
     * HasProperty "PigBreed". */
    pigTypeId = UA_NODEID_STRING(1, "PigType");
    otattr.displayName = UA_LOCALIZEDTEXT("", "PigType");
    UA_Server_addObjectTypeNode(server, pigTypeId, animalTypeId,
                                UA_NS0ID(HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "PigType"),
                                otattr, NULL, NULL);
    
    /* ScheduleType derived from BaseObjectType adds:
     * HasProperty "Period". */
    scheduleTypeId = UA_NODEID_STRING(1, "ScheduleType");
    otattr.displayName = UA_LOCALIZEDTEXT("", "ScheduleType");
    UA_Server_addObjectTypeNode(server, scheduleTypeId,
                                UA_NS0ID(BASEOBJECTTYPE),
                                UA_NS0ID(HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "ScheduleType"),
                                otattr, NULL, NULL);

    /* FeedingScheduleType derived from ScheduleType adds:
     * HasProperty "Food".
     * HasProperty "Amount" (Stored as an Int32). */
    feedingScheduleTypeId = UA_NODEID_STRING(1, "FeedingScheduleType");
    otattr.displayName = UA_LOCALIZEDTEXT("", "FeedingScheduleType");
    UA_Server_addObjectTypeNode(server, feedingScheduleTypeId,
                                scheduleTypeId, UA_NS0ID(HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "FeedingScheduleType"),
                                otattr, NULL, NULL);

    /* AreaType derived from BaseObjectType is just a simple Folder and contains
     * no Properties. */
    areaTypeId = UA_NODEID_STRING(1, "AreaType");
    otattr.displayName = UA_LOCALIZEDTEXT("", "AreaType");
    UA_Server_addObjectTypeNode(server, areaTypeId,
                                UA_NS0ID(BASEOBJECTTYPE),
                                UA_NS0ID(HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "AreaType"),
                                otattr, NULL, NULL);

    /* Add the schedule instances */
    addObject(UA_NODEID_NUMERIC(1, 94), UA_NS0ID(OBJECTSFOLDER), areaTypeId, "Area1");
    addObject(UA_NODEID_NUMERIC(1, 95), UA_NS0ID(OBJECTSFOLDER), areaTypeId, "Area2");

    addObject(UA_NODEID_NUMERIC(1,30), UA_NODEID_NUMERIC(1,94), personTypeId, "JFamily1");
    addProperty(UA_NODEID_NUMERIC(1,30), "Lastname", "Jones");
    addProperty(UA_NODEID_NUMERIC(1,30), "FirstName", "John");
    addProperty(UA_NODEID_NUMERIC(1,30), "Address", "319 2nd Ave");
    addProperty(UA_NODEID_NUMERIC(1,30), "City", "Jersey");
    addProperty(UA_NODEID_NUMERIC(1,30), "Zipcode", "02138");

    addObject(UA_NODEID_NUMERIC(1,36), UA_NODEID_NUMERIC(1,94), personTypeId, "JFamily2");
    addProperty(UA_NODEID_NUMERIC(1,36), "Lastname", "Jones");
    addProperty(UA_NODEID_NUMERIC(1,36), "FirstName", "Sophia");
    addProperty(UA_NODEID_NUMERIC(1,36), "Address", "319 2nd Ave");
    addProperty(UA_NODEID_NUMERIC(1,36), "City", "Jersey");
    addProperty(UA_NODEID_NUMERIC(1,36), "Zipcode", "02138");

    addRef(UA_NODEID_NUMERIC(1,30), hasChildId, UA_NODEID_NUMERIC(1,36));

    addObject(UA_NODEID_NUMERIC(1,42), UA_NODEID_NUMERIC(1,95), personTypeId, "HFamily1");
    addProperty(UA_NODEID_NUMERIC(1,42), "Lastname", "Hervey");
    addProperty(UA_NODEID_NUMERIC(1,42), "FirstName", "Paul");
    addProperty(UA_NODEID_NUMERIC(1,42), "Address", "49 Main St");
    addProperty(UA_NODEID_NUMERIC(1,42), "City", "Cleveland");
    addProperty(UA_NODEID_NUMERIC(1,42), "Zipcode", "03854");

    addObject(UA_NODEID_NUMERIC(1,48), UA_NODEID_NUMERIC(1,95), personTypeId, "HFamily2");
    addProperty(UA_NODEID_NUMERIC(1,48), "Lastname", "Hervey");
    addProperty(UA_NODEID_NUMERIC(1,48), "FirstName", "Paul Jr");
    addProperty(UA_NODEID_NUMERIC(1,48), "Address", "49 Main St");
    addProperty(UA_NODEID_NUMERIC(1,48), "City", "Cleveland");
    addProperty(UA_NODEID_NUMERIC(1,48), "Zipcode", "03854");

    addRef(UA_NODEID_NUMERIC(1,42), hasChildId, UA_NODEID_NUMERIC(1,48));

    addObject(UA_NODEID_NUMERIC(1,54), UA_NODEID_NUMERIC(1,95), personTypeId, "HFamily3");
    addProperty(UA_NODEID_NUMERIC(1,54), "Lastname", "Hervey");
    addProperty(UA_NODEID_NUMERIC(1,54), "FirstName", "Sara");
    addProperty(UA_NODEID_NUMERIC(1,54), "Address", "54 Main St");
    addProperty(UA_NODEID_NUMERIC(1,54), "City", "Cleveland");
    addProperty(UA_NODEID_NUMERIC(1,54), "Zipcode", "03854");

    addRef(UA_NODEID_NUMERIC(1,48), hasChildId, UA_NODEID_NUMERIC(1,54));

    addObject(UA_NODEID_NUMERIC(1,70), UA_NS0ID(OBJECTSFOLDER), catTypeId, "Cat1");
    addProperty(UA_NODEID_NUMERIC(1,70), "Name", "Rosemary");
    addProperty(UA_NODEID_NUMERIC(1,70), "Nickname", "Rosie");
    addProperty(UA_NODEID_NUMERIC(1,70), "CatBreed", "Tabby");

    addRef(UA_NODEID_NUMERIC(1,30), hasPetId, UA_NODEID_NUMERIC(1,70));

    addObject(UA_NODEID_NUMERIC(1,74), UA_NS0ID(OBJECTSFOLDER), catTypeId, "Cat2");
    addProperty(UA_NODEID_NUMERIC(1,74), "Name", "Basil");
    addProperty(UA_NODEID_NUMERIC(1,74), "Nickname", "Trouble");
    addProperty(UA_NODEID_NUMERIC(1,74), "CatBreed", "Tabby");

    addRef(UA_NODEID_NUMERIC(1,30), hasPetId, UA_NODEID_NUMERIC(1,74));

    addObject(UA_NODEID_NUMERIC(1,82), UA_NS0ID(OBJECTSFOLDER), dogTypeId, "Dog1");
    addProperty(UA_NODEID_NUMERIC(1,82), "Name", "Oliver");
    addProperty(UA_NODEID_NUMERIC(1,82), "Nickname", "Olie");
    addProperty(UA_NODEID_NUMERIC(1,82), "DogBreed", "American Bull Dog");
    addProperty(UA_NODEID_NUMERIC(1,82), "License", "355403");

    addRef(UA_NODEID_NUMERIC(1,42), hasPetId, UA_NODEID_NUMERIC(1,82));

    addObject(UA_NODEID_NUMERIC(1,91), UA_NS0ID(OBJECTSFOLDER), pigTypeId, "Pig1");
    addProperty(UA_NODEID_NUMERIC(1,91), "PigBreed", "Meat");
    addProperty(UA_NODEID_NUMERIC(1,91), "Name", "Porker");

    addRef(UA_NODEID_NUMERIC(1,48), hasFarmAnimalId, UA_NODEID_NUMERIC(1,91));
    addRef(UA_NODEID_NUMERIC(1,54), hasPetId, UA_NODEID_NUMERIC(1,91));

    addObject(UA_NODEID_NUMERIC(1,78), UA_NS0ID(OBJECTSFOLDER), feedingScheduleTypeId, "Schedule1");
    addProperty(UA_NODEID_NUMERIC(1,78), "Period", "Hourly");
    addProperty(UA_NODEID_NUMERIC(1,78), "Food", "Purino");
    addProperty(UA_NODEID_NUMERIC(1,78), "Amount", "25");

    addRef(UA_NODEID_NUMERIC(1,70), hasScheduleId, UA_NODEID_NUMERIC(1,78));

    addObject(UA_NODEID_NUMERIC(1,87), UA_NS0ID(OBJECTSFOLDER), feedingScheduleTypeId, "Schedule2");
    addProperty(UA_NODEID_NUMERIC(1,87), "Period", "Daily");
    addProperty(UA_NODEID_NUMERIC(1,87), "Food", "ALPY");
    addProperty(UA_NODEID_NUMERIC(1,87), "Amount", "100");

    addRef(UA_NODEID_NUMERIC(1,74), hasScheduleId, UA_NODEID_NUMERIC(1,87));
}

static void teardown(void) {
    UA_Server_delete(server);
}

START_TEST(SimpleQuery) {
    char *query = "/1:FirstName";

    UA_QueryDataDescription qdd;
    UA_QueryDataDescription_init(&qdd);
    qdd.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_StatusCode res = UA_RelativePath_parseWithServer(server, &qdd.relativePath, UA_STRING(query));
    ck_assert(res == UA_STATUSCODE_GOOD);

    UA_NodeTypeDescription ntd[1];
    UA_NodeTypeDescription_init(ntd);
    ntd->typeDefinitionNode.nodeId = personTypeId;
    ntd->dataToReturn = &qdd;
    ntd->dataToReturnSize = 1;

    UA_ContentFilter cf;
    UA_ContentFilter_init(&cf);

    UA_QueryDataSet *outQ = NULL;
    size_t outQSize = 0;
    res = UA_Server_query(server, 1, ntd, cf, &outQSize, &outQ);
    ck_assert(res == UA_STATUSCODE_GOOD);
    ck_assert(outQSize == 5);

    UA_Array_delete(outQ, outQSize, &UA_TYPES[UA_TYPES_QUERYDATASET]);
    UA_RelativePath_clear(&qdd.relativePath);
} END_TEST

int main(void) {
    Suite *s = suite_create("services_query");
    TCase *tc_query = tcase_create("query tests");
    tcase_add_checked_fixture(tc_query, setup, teardown);
    tcase_add_test(tc_query, SimpleQuery);
    suite_add_tcase(s, tc_query);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
