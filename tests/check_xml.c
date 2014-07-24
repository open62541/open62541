#include <stdio.h>
#include <stdlib.h>
#include "ua_xml.h"
#include "util/ua_util.h"
#include "ua_types_generated.h"
#include "ua_namespace.h"
#include "ua_namespace_xml.h"
#include "check.h"

START_TEST(parseNumericNodeIdWithoutNsIdShallYieldNs0NodeId)
{
	// given
	char txt[] = "i=2";
	UA_NodeId nodeId = { (UA_Byte) 0, (UA_UInt16) 0, { 0 } };

	// when
	UA_Int32 retval = UA_NodeId_copycstring(txt,&nodeId,UA_NULL);

	// then
	ck_assert_int_eq(retval,UA_SUCCESS);
	ck_assert_int_eq(nodeId.encodingByte,UA_NODEIDTYPE_FOURBYTE);
	ck_assert_int_eq(nodeId.namespace,0);
	ck_assert_int_eq(nodeId.identifier.numeric,2);

	// finally
}
END_TEST

START_TEST(parseNumericNodeIdWithNsShallYieldNodeIdWithNs)
{
	// given
	char txt[] = "ns=1;i=2";
	UA_NodeId nodeId = { (UA_Byte) 0, (UA_UInt16) 0, { 0 } };

	// when
	UA_Int32 retval = UA_NodeId_copycstring(txt,&nodeId,UA_NULL);

	// then
	ck_assert_int_eq(retval,UA_SUCCESS);
	ck_assert_int_eq(nodeId.encodingByte,UA_NODEIDTYPE_FOURBYTE);
	ck_assert_int_eq(nodeId.namespace,1);
	ck_assert_int_eq(nodeId.identifier.numeric,2);

	// finally
}
END_TEST

START_TEST(loadUserNamespaceWithSingleProcessVariableShallSucceed)
{
	// given
	char xml[]=
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<UANodeSet>"
	"<UAVariable NodeId=\"ns=1;i=2\" BrowseName=\"X1\" DataType=\"i=6\">"
		"<DisplayName>Integer Variable</DisplayName>"
		"<References>"
			"<Reference ReferenceType=\"i=40\">i=63</Reference>"
		"</References>"
	"</UAVariable>"
"</UANodeSet>";
	Namespace *ns;
	UA_NodeId nodeId;
	UA_Int32 retval;

	// when
	retval = Namespace_loadFromString(&ns,1,"ROOT",xml);

	// then
	ck_assert_int_eq(retval,UA_SUCCESS);
	ck_assert_ptr_ne(ns,UA_NULL);

	UA_NodeId_copycstring("ns=1;i=2",&nodeId,UA_NULL);
	ck_assert_int_eq(Namespace_contains(ns,&nodeId),UA_TRUE);

	const UA_Node* nr = UA_NULL;
	Namespace_Entry_Lock* nl = UA_NULL;
	retval = Namespace_get(ns,&nodeId,&nr,&nl);
	ck_assert_int_eq(retval,UA_SUCCESS);
	ck_assert_ptr_ne(nr,UA_NULL);
	ck_assert_int_eq(nr->references[0].referenceTypeId.identifier.numeric,40);
	ck_assert_int_eq(nr->references[0].targetId.nodeId.identifier.numeric,63);


	UA_NodeId_copycstring("i=2",&nodeId,UA_NULL);
	ck_assert_int_eq(Namespace_contains(ns,&nodeId),UA_FALSE);

	// finally
	Namespace_delete(ns);
}
END_TEST

START_TEST(loadUserNamespaceWithSingleProcessVariableAndAliasesShallSucceed)
{
	// given
	char xml[]=
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<UANodeSet>"
	"<Aliases>"
		"<Alias Alias=\"Int32\">i=6</Alias>"
		"<Alias Alias=\"HasTypeDefinition\">i=40</Alias>"
	"</Aliases>"
	"<UAVariable NodeId=\"ns=1;i=4\" BrowseName=\"X1\" DataType=\"Int32\">"
		"<DisplayName>Integer Variable</DisplayName>"
		"<References>"
			"<Reference ReferenceType=\"HasTypeDefinition\">i=63</Reference>"
		"</References>"
	"</UAVariable>"
"</UANodeSet>";
	Namespace *ns;
	UA_NodeId nodeId;
	UA_Int32 retval;

	// when
	retval = Namespace_loadFromString(&ns,1,"ROOT",xml);

	// then
	ck_assert_ptr_ne(ns,UA_NULL);
	ck_assert_int_eq(retval,UA_SUCCESS);

	UA_NodeId_copycstring("ns=1;i=4",&nodeId,UA_NULL);
	ck_assert_int_eq(Namespace_contains(ns,&nodeId),UA_TRUE);

	const UA_Node* nr = UA_NULL;
	Namespace_Entry_Lock* nl = UA_NULL;
	retval = Namespace_get(ns,&nodeId,&nr,&nl);
	ck_assert_int_eq(retval,UA_SUCCESS);
	ck_assert_ptr_ne(nr,UA_NULL);
	ck_assert_int_eq(nr->references[0].referenceTypeId.identifier.numeric,40);
	ck_assert_int_eq(nr->references[0].targetId.nodeId.identifier.numeric,63);

	UA_NodeId_copycstring("ns=1;i=2",&nodeId,UA_NULL);
	ck_assert_int_eq(Namespace_contains(ns,&nodeId),UA_FALSE);

	UA_NodeId_copycstring("ns=2;i=4",&nodeId,UA_NULL);
	ck_assert_int_eq(Namespace_contains(ns,&nodeId),UA_FALSE);

	UA_NodeId_copycstring("i=4",&nodeId,UA_NULL);
	ck_assert_int_eq(Namespace_contains(ns,&nodeId),UA_FALSE);

	// finally
	Namespace_delete(ns);
}
END_TEST

Suite* testSuite()
{
	Suite *s = suite_create("XML Test");
	TCase *tc_core = tcase_create("Core");

	tcase_add_test(tc_core, parseNumericNodeIdWithoutNsIdShallYieldNs0NodeId);
	tcase_add_test(tc_core, parseNumericNodeIdWithNsShallYieldNodeIdWithNs);
	tcase_add_test(tc_core, loadUserNamespaceWithSingleProcessVariableShallSucceed);
	tcase_add_test(tc_core, loadUserNamespaceWithSingleProcessVariableAndAliasesShallSucceed);
	suite_add_tcase(s,tc_core);
	return s;
}

int main (void)
{
	int number_failed = 0;

	Suite *s;
	SRunner *sr;

	s = testSuite();
	sr = srunner_create(s);
	srunner_set_fork_status(sr, CK_NOFORK);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
