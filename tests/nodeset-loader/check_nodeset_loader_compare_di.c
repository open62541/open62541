/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/nodesetloader.h>
#include <open62541/server.h>
#include <open62541/types.h>

#include <stdio.h>
#include <string.h>

#include "check.h"
#include "test_helpers.h"
#include "tests/namespace_nodesetloader_autoid_generated.h"
#include "tests/namespace_nodesetloader_di_generated.h"
#include "tests/nodeids_nodesetloader_autoid.h"
#include "tests/nodeids_nodesetloader_di.h"

UA_Server *server = NULL;

static void setupDI(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    ck_assert_uint_eq(UA_STATUSCODE_GOOD, UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "DI/Opc.Ua.Di.NodeSet2.xml", NULL));
    UA_Server_run_startup(server);
}

static void setupAutoID(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    ck_assert_uint_eq(UA_STATUSCODE_GOOD, UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "DI/Opc.Ua.Di.NodeSet2.xml", NULL));
    ck_assert_uint_eq(UA_STATUSCODE_GOOD, UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "AutoID/Opc.Ua.AutoID.NodeSet2.xml", NULL));

    const UA_NodeId a = UA_NODEID_NUMERIC(3, 3019);
    const UA_DataType* x = UA_Server_findDataType(server, &a);
    UA_Server_run_startup(server);
}



static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

/*
  BSD and NodeSet2 define LocationIndicationType differently:
  
                 BSD    |   Nodeset
         ------------------------------
         None = 0       | Visual = 0
         Visual = 1     | Audible = 1
         Audible = 2    |

  Therefore, definition of LocationIndicationType differs depending on whether nodeset compiler or
  nodeset loader is used. Same for UpdateBehaviorType.

  For the purpose of this test, we compare the options by name rather than by strict numeric member lists.
  
  Details from BSD:
  <opc:EnumeratedType Name="LocationIndicationType" LengthInBits="16" IsOptionSet="true">
    <opc:EnumeratedValue Name="None" Value="0" />
    <opc:EnumeratedValue Name="Visual" Value="1" />
    <opc:EnumeratedValue Name="Audible" Value="2" />
  </opc:EnumeratedType>

  Details from Nodeset2:
  <UADataType NodeId="ns=1;i=410" BrowseName="1:LocationIndicationType">
    ...
    <Definition Name="1:LocationIndicationType" IsOptionSet="true">
      <Field Name="Visual" Value="0">
        ...
      </Field>
      <Field Name="Audible" Value="1">
        ...
      </Field>
    </Definition>
  </UADataType>
*/

static UA_Boolean
compare_options_with_none_prefix(const UA_DataType *compiledType,
                                 const UA_DataType *loadedType) {
    /*
     * Expect compiled to have an extra None=0 member at index 0 and
     *  otherwise the same members as loaded (offset by +1).
     */
    if(compiledType->membersSize != loadedType->membersSize + 1) {
        return false;
    }
    const UA_DataTypeMember *first = &compiledType->members[0];
    if(!first->memberName || strcmp(first->memberName, "None") != 0) {
        return false;
    }
    for(size_t k = 0; k < loadedType->membersSize; ++k) {
        const UA_DataTypeMember *comp = &compiledType->members[k + 1];
        const UA_DataTypeMember *load = &loadedType->members[k];
        if(!comp->memberName || !load->memberName) {
            return false;
        }
        if(strcmp(comp->memberName, load->memberName) != 0) {
            return false;
        }
    }
    return true;
}

static bool relaxedComparison(const UA_NodeId actualLoaded, const UA_NodeId
    actualCompiled, const UA_UInt16 autoIdIdx) {
    /*
     * Allowed simple type pairs:
     * - UtcTime              / DateTime
     * - CodeTypeDataType     / String
     * - NMEACoordinateString / String
     * - LocationName         / String
     * - Duration             / Double
     */
    const UA_NodeId utcTimeId = UA_NODEID_NUMERIC(0, UA_NS0ID_UTCTIME);
    const UA_NodeId dateTimeId = UA_NODEID_NUMERIC(0, UA_NS0ID_DATETIME);

    const UA_NodeId codeTypeId = UA_NODEID_NUMERIC(autoIdIdx, UA_NODESETLOADER_AUTOIDID_CODETYPEDATATYPE);
    const UA_NodeId nmeaId = UA_NODEID_NUMERIC(autoIdIdx, UA_NODESETLOADER_AUTOIDID_NMEACOORDINATESTRING);
    const UA_NodeId locationNameId = UA_NODEID_NUMERIC(autoIdIdx, UA_NODESETLOADER_AUTOIDID_LOCATIONNAME);
    const UA_NodeId stringId = UA_NODEID_NUMERIC(0, UA_NS0ID_STRING);

    const UA_NodeId durationId = UA_NODEID_NUMERIC(0, UA_NS0ID_DURATION);
    const UA_NodeId doubleId = UA_NODEID_NUMERIC(0, UA_NS0ID_DOUBLE);

    if (UA_NodeId_equal(&actualLoaded, &utcTimeId)
        && UA_NodeId_equal(&actualCompiled, &dateTimeId)) {
        return true;
    }
    if ((UA_NodeId_equal(&actualLoaded, &codeTypeId)
        || UA_NodeId_equal(&actualLoaded,&nmeaId)
        || UA_NodeId_equal(&actualLoaded,&locationNameId))
        && UA_NodeId_equal(&actualCompiled, &stringId)) {
        return true;
    }
    if (UA_NodeId_equal(&actualLoaded, &durationId)
    && UA_NodeId_equal(&actualCompiled, &doubleId)) {
        return true;
    }
    return UA_NodeId_equal(&actualLoaded, &actualCompiled);
}

static void compareNodeSet(const char * namespaceUri, UA_DataType* compiledTypes,
    const size_t compiledTypesSize) {
    const UA_UInt16 nsIndex = UA_Server_addNamespace(server, namespaceUri);

    for(size_t i = 0; i < compiledTypesSize; ++i) {
        compiledTypes[i].typeId.namespaceIndex = nsIndex;
        compiledTypes[i].binaryEncodingId.namespaceIndex = nsIndex;

        const UA_DataType *compiledType = &compiledTypes[i];
        const UA_DataType *loadedType = UA_Server_findDataType(server, &compiledType->typeId);

        ck_assert(loadedType != NULL);
        ck_assert_uint_eq(compiledType->typeKind, loadedType->typeKind);

        /*
         * If the compiled type uses a BSD-style optionset with an explicit
         * leading `None=0` member, allow semantic comparison: compiled
         * members == loaded members + 1 and names match with an offset.
         */
        if(compiledType->membersSize == loadedType->membersSize + 1
            && (compiledType->typeKind == UA_DATATYPEKIND_BYTE
            || compiledType->typeKind == UA_DATATYPEKIND_UINT16
            || compiledType->typeKind == UA_DATATYPEKIND_UINT32
            || compiledType->typeKind == UA_DATATYPEKIND_UINT64)) {
            ck_assert(compare_options_with_none_prefix(compiledType, loadedType));
            continue;
        }

        ck_assert_uint_eq(compiledType->membersSize, loadedType->membersSize);
        ck_assert_uint_eq(compiledType->memSize, loadedType->memSize);
        ck_assert(compiledType->overlayable == loadedType->overlayable);
        ck_assert(compiledType->pointerFree == loadedType->pointerFree);
        ck_assert(!strcmp(compiledType->typeName, loadedType->typeName));

        for(int j = 0; j < compiledType->membersSize; ++j) {
            const UA_DataTypeMember *compMember = &compiledType->members[j];
            const UA_DataTypeMember *loadMember = &loadedType->members[j];

            ck_assert(compMember->isArray == loadMember->isArray);
            ck_assert(compMember->padding == loadMember->padding);
            ck_assert(compMember->isOptional == loadMember->isOptional);
            if(compiledType->typeKind != UA_DATATYPEKIND_ENUM) {
                if (0 == strcmp(namespaceUri, "http://opcfoundation.org/UA/AutoID/")) {
                    /*
                     * This exception is needed because the AutoID BSD uses
                     * type  DateTime to member instances of simple type UtcTime
                     * --> nodeset compiler sets DateTime, but nodeset loader sets UtcTime.
                     */
                    ck_assert(relaxedComparison(loadMember->memberType->typeId,
                        compMember->memberType->typeId, nsIndex));
                }
                else {
                    ck_assert(UA_NodeId_equal(&compMember->memberType->typeId,
                        &loadMember->memberType->typeId));
                }
            }
        }
    }
}

static void compareNodeSetReverse(const char * namespaceUri, UA_DataType* compiledTypes,
    const size_t compiledTypesSize) {
    const UA_UInt16 nsIndex = UA_Server_addNamespace(server, namespaceUri);
    const bool isAutoID = (0 == strcmp(namespaceUri, "http://opcfoundation.org/UA/AutoID/"));
    const bool isDI = (0 == strcmp(namespaceUri, "http://opcfoundation.org/UA/DI/"));
    const UA_NodeId codeTypeDataType = UA_NODEID_NUMERIC(nsIndex,UA_NODESETLOADER_AUTOIDID_CODETYPEDATATYPE);
    const UA_NodeId locationName = UA_NODEID_NUMERIC(nsIndex, UA_NODESETLOADER_AUTOIDID_LOCATIONNAME);
    const UA_NodeId nmeaCoordinateString = UA_NODEID_NUMERIC(nsIndex, UA_NODESETLOADER_AUTOIDID_NMEACOORDINATESTRING);
    const UA_NodeId fetchResultDataType = UA_NODEID_NUMERIC(nsIndex,UA_NODESETLOADER_DIID_FETCHRESULTDATATYPE);

    /* Align compiled types to the server namespace index for comparison */
    for(size_t i = 0; i < compiledTypesSize; ++i) {
        compiledTypes[i].typeId.namespaceIndex = nsIndex;
        compiledTypes[i].binaryEncodingId.namespaceIndex = nsIndex;
    }

    const UA_DataTypeArray *dta = UA_Server_getDataTypes(server);
    ck_assert_ptr_ne(dta, NULL);

    size_t loadedCount = 0;
    size_t exceptionsCount = 0;
    for(const UA_DataTypeArray *cur = dta; NULL != cur; cur = cur->next) {
        for(size_t t = 0; t < cur->typesSize; ++t) {
            const UA_DataType *loadedType = &cur->types[t];
            if(loadedType->typeId.namespaceIndex != nsIndex) {
                continue;
            }

            /* These types are not in DI's or AutoID's BSD file --> nodeset compiler
             * does not generate them. */
            const bool exception1 = isAutoID && UA_NodeId_equal(&loadedType->typeId, &codeTypeDataType);
            const bool exception2 = isAutoID && UA_NodeId_equal(&loadedType->typeId, &locationName);
            const bool exception3 = isAutoID && UA_NodeId_equal(&loadedType->typeId, &nmeaCoordinateString);
            /* FetchResultDatatype is an abstract, empty structure. Nodeset
             * compiler does not generate it as type, nodeset loader does. */
            const bool exception4 = isDI && UA_NodeId_equal(&loadedType->typeId, &fetchResultDataType);
            if (exception1 || exception2 || exception3 || exception4) {
                ++exceptionsCount;
                printf("%lu. making an exception for  %s\n",
                    exceptionsCount + loadedCount, loadedType->typeName);
                continue;
            }
            bool found = false;
            for(size_t i = 0; i < compiledTypesSize; ++i) {
                const UA_DataType *ct = &compiledTypes[i];
                UA_NodeId compiledType = ct->typeId;
                compiledType.namespaceIndex = loadedType->typeId.namespaceIndex;
                if(UA_NodeId_equal(&compiledType, &loadedType->typeId)) {
                    found = true;
                    break;
                }
            }
            if (found) {
                ++loadedCount;
                printf("%lu. %s found in compiled types\n",
                     loadedCount + exceptionsCount + 1u, loadedType->typeName);
            }
            else {
                printf("loaded type %s not found in compiled types\n",
                    loadedType->typeName);
            }
            ck_assert(found);
        }
    }
    /* Number of compiled and number of loaded types should be equal */
    ck_assert_uint_eq(compiledTypesSize, loadedCount);
    ck_assert((isAutoID && exceptionsCount == 3) || (isDI && exceptionsCount == 1));
}

START_TEST(Server_compareDI) {
    compareNodeSet("http://opcfoundation.org/UA/DI/",
        UA_TYPES_NODESETLOADER_DI,UA_TYPES_NODESETLOADER_DI_COUNT);
}
END_TEST

START_TEST(Server_compareDIReverse) {
    compareNodeSetReverse("http://opcfoundation.org/UA/DI/",
        UA_TYPES_NODESETLOADER_DI, UA_TYPES_NODESETLOADER_DI_COUNT);
}
END_TEST
START_TEST(Server_compareAutoID) {
    compareNodeSet("http://opcfoundation.org/UA/AutoID/",
        UA_TYPES_NODESETLOADER_AUTOID,UA_TYPES_NODESETLOADER_AUTOID_COUNT);
}
END_TEST

START_TEST(Server_compareAutoIDReverse) {
    compareNodeSetReverse("http://opcfoundation.org/UA/AutoID/",
        UA_TYPES_NODESETLOADER_AUTOID, UA_TYPES_NODESETLOADER_AUTOID_COUNT);
}
END_TEST
static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Server Nodeset Loader");
    TCase *tc_di = tcase_create("Compare DI Nodeset");
    tcase_add_unchecked_fixture(tc_di, setupDI, teardown);
    tcase_add_test(tc_di, Server_compareDI);
    tcase_add_test(tc_di, Server_compareDIReverse);
    suite_add_tcase(s, tc_di);
    TCase *tc_autoid = tcase_create("Compare AutoID Nodeset");
    tcase_add_unchecked_fixture(tc_autoid, setupAutoID, teardown);
    tcase_add_test(tc_autoid, Server_compareAutoID);
    tcase_add_test(tc_autoid, Server_compareAutoIDReverse);
    suite_add_tcase(s, tc_autoid);
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
