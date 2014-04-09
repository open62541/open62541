/*
 ============================================================================
 Name        : check_calcsize.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description :
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

#include "opcua.h"
#include "ua_transportLayer.h"
#include "check.h"


/*START_TEST(diagnosticInfo_calcSize_test)
{

	UA_Int32 valreal = 0;
	UA_Int32 valcalc = 0;
	UA_DiagnosticInfo diagnosticInfo;
	diagnosticInfo.encodingMask = 0x01 | 0x02 | 0x04 | 0x08 | 0x10;
	diagnosticInfo.symbolicId = 30;
	diagnosticInfo.namespaceUri = 25;
	diagnosticInfo.localizedText = 22;
	diagnosticInfo.additionalInfo.data = "OPCUA";
	diagnosticInfo.additionalInfo.length = 5;

	ck_assert_int_eq(UA_DiagnosticInfo_calcSize(&diagnosticInfo),26);

}
END_TEST

START_TEST(extensionObject_calcSize_test)
{

	UA_Int32 valreal = 0;
	UA_Int32 valcalc = 0;
	UA_Byte data[3] = {1,2,3};
	UA_ExtensionObject extensionObject;

	// empty ExtensionObject, handcoded
	extensionObject.typeId.encodingByte = UA_NODEIDTYPE_TWOBYTE;
	extensionObject.typeId.identifier.numeric = 0;
	extensionObject.encoding = UA_EXTENSIONOBJECT_ENCODINGMASKTYPE_NOBODYISENCODED;
	ck_assert_int_eq(UA_ExtensionObject_calcSize(&extensionObject), 1 + 1 + 1);

	// ExtensionObject with ByteString-Body
	extensionObject.encoding = UA_EXTENSIONOBJECT_ENCODINGMASKTYPE_BODYISBYTESTRING;
	extensionObject.body.data = data;
	extensionObject.body.length = 3;
	ck_assert_int_eq(UA_ExtensionObject_calcSize(&extensionObject), 3 + 4 + 3);
}
END_TEST*/

START_TEST(responseHeader_calcSize_test)
{
	UA_ResponseHeader responseHeader;
	UA_DiagnosticInfo diagnosticInfo;
	UA_ExtensionObject extensionObject;
	UA_DiagnosticInfo  emptyDO = {0x00};
	UA_ExtensionObject emptyEO = {{UA_NODEIDTYPE_TWOBYTE,0},UA_EXTENSIONOBJECT_ENCODINGMASKTYPE_NOBODYISENCODED};
	//Should have the size of 26 Bytes
	diagnosticInfo.encodingMask = UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_SYMBOLICID | UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_NAMESPACE | UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_LOCALIZEDTEXT | UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_LOCALE | UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_ADDITIONALINFO;		// Byte:   1
	// Indices into to Stringtable of the responseHeader (62541-6 §5.5.12 )
	diagnosticInfo.symbolicId = -1;										// Int32:  4
	diagnosticInfo.namespaceUri = -1;									// Int32:  4
	diagnosticInfo.localizedText = -1;									// Int32:  4
	diagnosticInfo.locale = -1;											// Int32:  4
	// Additional Info
	diagnosticInfo.additionalInfo.length = 5;							// Int32:  4
	diagnosticInfo.additionalInfo.data = "OPCUA";						// Byte[]: 5
	responseHeader.serviceDiagnostics = &diagnosticInfo;
	ck_assert_int_eq(UA_DiagnosticInfo_calcSize(&diagnosticInfo),1+(4+4+4+4)+(4+5));

	responseHeader.stringTableSize = -1;								// Int32:	4
	responseHeader.stringTable = NULL;

	responseHeader.additionalHeader = &emptyEO;	//		    3
	ck_assert_int_eq(UA_ResponseHeader_calcSize(&responseHeader),16+26+4+3);

	responseHeader.serviceDiagnostics = &emptyDO;
	ck_assert_int_eq(UA_ResponseHeader_calcSize(&responseHeader),16+1+4+3);
}
END_TEST
/*//ToDo: Function needs to be filled
START_TEST(expandedNodeId_calcSize_test)
{
	UA_Int32 valreal = 300;
	UA_Int32 valcalc = 0;
	ck_assert_int_eq(valcalc,valreal);
}
END_TEST
START_TEST(DataValue_calcSize_test)
{
	UA_DataValue dataValue;
	dataValue.encodingMask = UA_DATAVALUE_STATUSCODE |  UA_DATAVALUE_SOURCETIMESTAMP |  UA_DATAVALUE_SOURCEPICOSECONDS;
	dataValue.status = 12;
	UA_DateTime dateTime;
	dateTime = 80;
	dataValue.sourceTimestamp = dateTime;
	UA_DateTime sourceTime;
	dateTime = 214;
	dataValue.sourcePicoseconds = sourceTime;

	int size = 0;
	size = UA_DataValue_calcSize(&dataValue);

	ck_assert_int_eq(size, 21);
}
END_TEST*/

/*Suite* testSuite_diagnosticInfo_calcSize()
{
	Suite *s = suite_create("diagnosticInfo_calcSize");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, diagnosticInfo_calcSize_test);
	suite_add_tcase(s,tc_core);
	return s;
}
Suite* testSuite_extensionObject_calcSize()
{
	Suite *s = suite_create("extensionObject_calcSize");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, extensionObject_calcSize_test);
	suite_add_tcase(s,tc_core);
	return s;
}*/
Suite* testSuite_responseHeader_calcSize()
{
	Suite *s = suite_create("responseHeader_calcSize");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, responseHeader_calcSize_test);
	suite_add_tcase(s,tc_core);
	return s;
}
/*Suite* testSuite_dataValue_calcSize(void)
{
	Suite *s = suite_create("dataValue_calcSize");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core,DataValue_calcSize_test);
	suite_add_tcase(s,tc_core);
	return s;
}
Suite* testSuite_expandedNodeId_calcSize(void)
{
	Suite *s = suite_create("expandedNodeId_calcSize");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core,expandedNodeId_calcSize_test);
	suite_add_tcase(s,tc_core);
	return s;
}*/

int main (void)
{
	int number_failed = 0;
	Suite *s;
	SRunner *sr;
/*
	s = testSuite_diagnosticInfo_calcSize();
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_extensionObject_calcSize();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

*/
	s = testSuite_responseHeader_calcSize();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);
/*

	s = testSuite_expandedNodeId_calcSize();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_dataValue_calcSize();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_expandedNodeId_calcSize();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_dataValue_calcSize();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);
*/

	/* <TESTSUITE_TEMPLATE>
	s =  <TESTSUITENAME>;
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);
	*/
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
