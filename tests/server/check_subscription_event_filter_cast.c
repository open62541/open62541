/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include "server/ua_server_internal.h"
#include "server/ua_subscription.h"
#include "test_helpers.h"

#include <check.h>
#include <math.h>
#include <stdlib.h>

static UA_Server *server;

static void
setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);
}

static void
teardown(void) {
    UA_Server_delete(server);
}

/* Use ISNULL as well as NOT to distinguish unknown from false. */
enum { TRUTH_FALSE, TRUTH_TRUE, TRUTH_NULL };

static void
checkResult(UA_ContentFilterElement *element, unsigned expected) {
    UA_ElementOperand reference = {1};
    UA_ExtensionObject operand;
    UA_ExtensionObject_setValueNoDelete(&operand, &reference,
                                        &UA_TYPES[UA_TYPES_ELEMENTOPERAND]);
    UA_ContentFilterElement elements[2];
    UA_ContentFilterElement_init(&elements[0]);
    elements[0].filterOperandsSize = 1;
    elements[0].filterOperands = &operand;
    elements[1] = *element;

    UA_ContentFilter filter;
    UA_ContentFilter_init(&filter);
    UA_NodeId eventNode = UA_NODEID_NUMERIC(1, 1000);
    for(unsigned probe = 0; probe < 3; probe++) {
        filter.elementsSize = (probe == 0) ? 1 : 2;
        filter.elements = (probe == 0) ? &elements[1] : elements;
        elements[0].filterOperator = (probe == 1) ?
            UA_FILTEROPERATOR_NOT : UA_FILTEROPERATOR_ISNULL;
        UA_FilterEvalContext ctx;
        UA_FilterEvalContext_init(&ctx);
        ctx.server = server;
        ctx.session = &server->adminSession;
        ctx.ed.eventInstance = &eventNode;
        ctx.filter.whereClause = filter;
        lockServer(server);
        UA_StatusCode res = evaluateWhereClause(&ctx);
        UA_FilterEvalContext_reset(&ctx);
        unlockServer(server);
        UA_Boolean matches = (probe == 0 && expected == TRUTH_TRUE) ||
            (probe == 1 && expected == TRUTH_FALSE) ||
            (probe == 2 && expected == TRUTH_NULL);
        ck_assert_msg(res == (matches ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADNOMATCH),
                      "Operator %u, probe %u, truth %u: %s",
                      (unsigned)element->filterOperator, probe, expected,
                      UA_StatusCode_name(res));
    }
}

static void
checkLiteralResult(UA_FilterOperator op, const UA_Variant *left,
                   const UA_Variant *right, unsigned expected) {
    UA_LiteralOperand literals[2];
    literals[0].value = *left;
    UA_Variant_init(&literals[1].value);
    if(right)
        literals[1].value = *right;
    UA_ExtensionObject operands[2];
    for(size_t i = 0; i < 2; i++)
        UA_ExtensionObject_setValueNoDelete(&operands[i], &literals[i],
                                            &UA_TYPES[UA_TYPES_LITERALOPERAND]);
    UA_ContentFilterElement element;
    UA_ContentFilterElement_init(&element);
    element.filterOperator = op;
    element.filterOperandsSize = right ? 2 : 1;
    element.filterOperands = operands;
    checkResult(&element, expected);
}

static void
checkLogicalOperand(const UA_Variant *value, unsigned truth) {
    /* Rows and columns are false, true, unknown. */
    static const unsigned andTable[3][3] = {
        {TRUTH_FALSE, TRUTH_FALSE, TRUTH_FALSE},
        {TRUTH_FALSE, TRUTH_TRUE, TRUTH_NULL},
        {TRUTH_FALSE, TRUTH_NULL, TRUTH_NULL}
    };
    static const unsigned orTable[3][3] = {
        {TRUTH_FALSE, TRUTH_TRUE, TRUTH_NULL},
        {TRUTH_TRUE, TRUTH_TRUE, TRUTH_TRUE},
        {TRUTH_NULL, TRUTH_TRUE, TRUTH_NULL}
    };
    static const unsigned notTable[3] = {TRUTH_TRUE, TRUTH_FALSE, TRUTH_NULL};
    checkLiteralResult(UA_FILTEROPERATOR_NOT, value, NULL, notTable[truth]);
    for(unsigned otherTruth = 0; otherTruth < 3; otherTruth++) {
        UA_Boolean b = (otherTruth == TRUTH_TRUE);
        UA_Variant other;
        UA_Variant_init(&other);
        if(otherTruth != TRUTH_NULL)
            UA_Variant_setScalar(&other, &b, &UA_TYPES[UA_TYPES_BOOLEAN]);
        checkLiteralResult(UA_FILTEROPERATOR_AND, value, &other, andTable[truth][otherTruth]);
        checkLiteralResult(UA_FILTEROPERATOR_AND, &other, value, andTable[otherTruth][truth]);
        checkLiteralResult(UA_FILTEROPERATOR_OR, value, &other, orTable[truth][otherTruth]);
        checkLiteralResult(UA_FILTEROPERATOR_OR, &other, value, orTable[otherTruth][truth]);
    }
}

START_TEST(numericLogicalOperands) {
    /* Cover every numerical input kind, zero, and nonzero values. Fractional
     * values must not be rounded to an integer before conversion to Boolean. */
    for(int v = -1; v <= 1; v++) {
        UA_Boolean boolean = (v != 0);
        UA_SByte sbyte = (v < 0) ? UA_SBYTE_MIN : (UA_SByte)v;
        UA_Byte byte = (v < 0) ? UA_BYTE_MAX : (UA_Byte)v;
        UA_Int16 int16 = (v < 0) ? UA_INT16_MIN : (UA_Int16)v;
        UA_UInt16 uint16 = (v < 0) ? UA_UINT16_MAX : (UA_UInt16)v;
        UA_Int32 int32 = (v < 0) ? UA_INT32_MIN : (UA_Int32)v;
        UA_UInt32 uint32 = (v < 0) ? UA_UINT32_MAX : (UA_UInt32)v;
        UA_Int64 int64 = (v < 0) ? UA_INT64_MIN : (UA_Int64)v;
        UA_UInt64 uint64 = (v < 0) ? UA_UINT64_MAX : (UA_UInt64)v;
        UA_Float f = (UA_Float)v * 0.25f;
        UA_Double d = (UA_Double)v * 0.25;
        UA_StatusCode status = (v == 0) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BAD;
        void *values[] = {&boolean, &sbyte, &byte, &int16, &uint16, &int32,
                          &uint32, &int64, &uint64, &f, &d, &status};
        const size_t types[] = {
            UA_TYPES_BOOLEAN, UA_TYPES_SBYTE, UA_TYPES_BYTE, UA_TYPES_INT16,
            UA_TYPES_UINT16, UA_TYPES_INT32, UA_TYPES_UINT32, UA_TYPES_INT64,
            UA_TYPES_UINT64, UA_TYPES_FLOAT, UA_TYPES_DOUBLE, UA_TYPES_STATUSCODE
        };
        UA_Variant value;
        UA_Variant_setScalar(&value, values[_i], &UA_TYPES[types[_i]]);
        checkLogicalOperand(&value, (v == 0) ? TRUTH_FALSE : TRUTH_TRUE);
    }
} END_TEST

START_TEST(floatingPointLogicalOperands) {
    UA_Double values[] = {-0.0, INFINITY, -INFINITY, NAN};
    UA_Variant value;
    UA_Variant_setScalar(&value, &values[_i], &UA_TYPES[UA_TYPES_DOUBLE]);
    checkLogicalOperand(&value, (_i == 0) ? TRUTH_FALSE : TRUTH_TRUE);
    UA_Float f = (UA_Float)values[_i];
    UA_Variant_setScalar(&value, &f, &UA_TYPES[UA_TYPES_FLOAT]);
    checkLogicalOperand(&value, (_i == 0) ? TRUTH_FALSE : TRUTH_TRUE);
} END_TEST

static const struct {
    const char *text;
    unsigned truth;
} stringCases[] = {
    {"0", TRUTH_FALSE}, {"1", TRUTH_TRUE},
    {"true", TRUTH_TRUE}, {"TRUE", TRUTH_TRUE}, {"TrUe", TRUTH_TRUE},
    {"false", TRUTH_FALSE}, {"FALSE", TRUTH_FALSE}, {"FaLsE", TRUTH_FALSE},
    {"", TRUTH_NULL}, {"2", TRUTH_NULL}, {" true", TRUTH_NULL},
    {"false ", TRUTH_NULL}, {"trueX", TRUTH_NULL}, {"yes", TRUTH_NULL}
};

START_TEST(stringLogicalOperands) {
    UA_String str = UA_STRING((char*)(uintptr_t)stringCases[_i].text);
    UA_Variant value;
    UA_Variant_setScalar(&value, &str, &UA_TYPES[UA_TYPES_STRING]);
    checkLogicalOperand(&value, stringCases[_i].truth);
} END_TEST

START_TEST(unknownLogicalOperands) {
    UA_Variant value;
    UA_Variant_init(&value);
    checkLogicalOperand(&value, TRUTH_NULL);

    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, 1);
    UA_Variant_setScalar(&value, &nodeId, &UA_TYPES[UA_TYPES_NODEID]);
    checkLogicalOperand(&value, TRUTH_NULL);
    UA_DateTime date = 1;
    UA_Variant_setScalar(&value, &date, &UA_TYPES[UA_TYPES_DATETIME]);
    checkLogicalOperand(&value, TRUTH_NULL);

    UA_Int32 numbers[] = {0, 1};
    UA_Variant_setArray(&value, numbers, 2, &UA_TYPES[UA_TYPES_INT32]);
    checkLogicalOperand(&value, TRUTH_NULL);
    UA_Boolean booleans[] = {true};
    UA_Variant_setArray(&value, booleans, 1, &UA_TYPES[UA_TYPES_BOOLEAN]);
    checkLogicalOperand(&value, TRUTH_NULL);
    UA_Variant_setArray(&value, UA_EMPTY_ARRAY_SENTINEL, 0, &UA_TYPES[UA_TYPES_BOOLEAN]);
    checkLogicalOperand(&value, TRUTH_NULL);
} END_TEST

START_TEST(implicitStringToBoolean) {
    UA_String str = UA_STRING((char*)(uintptr_t)stringCases[_i].text);
    UA_Variant value, boolean;
    UA_Variant_setScalar(&value, &str, &UA_TYPES[UA_TYPES_STRING]);
    for(unsigned truth = 0; truth < 2; truth++) {
        UA_Boolean b = (truth == TRUTH_TRUE);
        UA_Variant_setScalar(&boolean, &b, &UA_TYPES[UA_TYPES_BOOLEAN]);
        unsigned expected = (stringCases[_i].truth == truth) ? TRUTH_TRUE : TRUTH_FALSE;
        checkLiteralResult(UA_FILTEROPERATOR_EQUALS, &value, &boolean, expected);
        checkLiteralResult(UA_FILTEROPERATOR_EQUALS, &boolean, &value, expected);
    }
} END_TEST

/* A data source returns owned storage, unlike a borrowed literal. Exercise
 * cleanup when casting succeeds and when an invalid string cannot be cast. */
static UA_StatusCode
readString(UA_Server *srv, const UA_NodeId *sessionId, void *sessionContext,
           const UA_NodeId *nodeId, void *nodeContext, UA_Boolean sourceTimeStamp,
           const UA_NumericRange *range, UA_DataValue *value) {
    UA_StatusCode res = UA_Variant_setScalarCopy(&value->value, nodeContext,
                                                &UA_TYPES[UA_TYPES_STRING]);
    value->hasValue = (res == UA_STATUSCODE_GOOD);
    return res;
}

START_TEST(attributeStringToBoolean) {
    UA_String str = UA_STRING((char*)(uintptr_t)stringCases[_i].text);
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    attr.valueRank = UA_VALUERANK_SCALAR;
    UA_DataSource source = {readString, NULL};
    UA_StatusCode res = UA_Server_addDataSourceVariableNode(
        server, UA_NODEID_NUMERIC(1, 1000),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(1, "Operand"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, source, &str, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_SimpleAttributeOperand attribute;
    UA_SimpleAttributeOperand_init(&attribute);
    attribute.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_Boolean b = true;
    UA_LiteralOperand literal;
    UA_Variant_setScalar(&literal.value, &b, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_ExtensionObject operands[2];
    UA_ExtensionObject_setValueNoDelete(&operands[0], &attribute,
                                        &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    UA_ExtensionObject_setValueNoDelete(&operands[1], &literal,
                                        &UA_TYPES[UA_TYPES_LITERALOPERAND]);
    UA_ContentFilterElement element;
    UA_ContentFilterElement_init(&element);
    element.filterOperator = UA_FILTEROPERATOR_EQUALS;
    element.filterOperandsSize = 2;
    element.filterOperands = operands;
    checkResult(&element, stringCases[_i].truth == TRUTH_TRUE ? TRUTH_TRUE : TRUTH_FALSE);

    element.filterOperator = UA_FILTEROPERATOR_NOT;
    element.filterOperandsSize = 1;
    const unsigned notTable[] = {TRUTH_TRUE, TRUTH_FALSE, TRUTH_NULL};
    checkResult(&element, notTable[stringCases[_i].truth]);
} END_TEST

START_TEST(numericComparisonRemainsNumeric) {
    /* Comparisons use implicit numeric conversion, not Boolean truthiness. */
    UA_Int32 number = 2;
    UA_Boolean b = true;
    UA_Variant numeric, boolean;
    UA_Variant_setScalar(&numeric, &number, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&boolean, &b, &UA_TYPES[UA_TYPES_BOOLEAN]);
    checkLiteralResult(UA_FILTEROPERATOR_EQUALS, &numeric, &boolean, TRUTH_FALSE);
    checkLiteralResult(UA_FILTEROPERATOR_EQUALS, &boolean, &numeric, TRUTH_FALSE);
} END_TEST

START_TEST(numericElementResult) {
    /* Bitwise operators return a numeric Variant. Check conversion both at
     * the filter root and through an ElementOperand used by NOT. */
    UA_UInt16 left = (_i == 0) ? 0 : 3;
    UA_UInt32 right = (_i == 0) ? 0 : 1;
    UA_Variant a, b;
    UA_Variant_setScalar(&a, &left, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Variant_setScalar(&b, &right, &UA_TYPES[UA_TYPES_UINT32]);
    unsigned expected = (_i == 0) ? TRUTH_FALSE : TRUTH_TRUE;
    checkLiteralResult(UA_FILTEROPERATOR_BITWISEAND, &a, &b, expected);
    checkLiteralResult(UA_FILTEROPERATOR_BITWISEOR, &a, &b, expected);
} END_TEST

int main(void) {
    Suite *s = suite_create("Event Filter Boolean Casting");
    TCase *tc = tcase_create("Logical operands and implicit casts");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_loop_test(tc, numericLogicalOperands, 0, 12);
    tcase_add_loop_test(tc, floatingPointLogicalOperands, 0, 4);
    tcase_add_loop_test(tc, stringLogicalOperands, 0,
                       sizeof(stringCases) / sizeof(stringCases[0]));
    tcase_add_test(tc, unknownLogicalOperands);
    tcase_add_loop_test(tc, implicitStringToBoolean, 0,
                       sizeof(stringCases) / sizeof(stringCases[0]));
    tcase_add_loop_test(tc, attributeStringToBoolean, 0,
                       sizeof(stringCases) / sizeof(stringCases[0]));
    tcase_add_test(tc, numericComparisonRemainsNumeric);
    tcase_add_loop_test(tc, numericElementResult, 0, 2);
    suite_add_tcase(s, tc);
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
