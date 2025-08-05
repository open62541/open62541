/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "ua_services.h"

#ifdef UA_ENABLE_QUERY /* conditional compilation */

typedef enum {
    UA_QUERYCOLUMNTYPE_OPERATOR = 0,
    UA_QUERYCOLUMNTYPE_LITERAL,
    UA_QUERYCOLUMNTYPE_ATTRIBUTE,
    UA_QUERYCOLUMNTYPE_SIMPLEATTRIBUTE
} UA_QueryColumnType;

typedef struct {
    UA_Boolean isDefined;
    UA_QueryColumnType columnType;
    union {
        struct {
            /* The operand index is rewritten and points to a column */
            UA_FilterOperator op;
            UA_UInt32 operandsSize;
            UA_UInt32 operands[UA_EVENTFILTER_MAXOPERANDS];
        } op;

        UA_Variant literal; /* (Singleton) literal, the row-cells are empty */
        UA_SimpleAttributeOperand sao;
        UA_AttributeOperand ao;
    } content;
} UA_QueryColumnDescription;

typedef struct {
    size_t columnCount;
    UA_QueryColumnDescription *columns;

    size_t rowSize;  /* Allocated */
    size_t rowCount; /* Actually used */
    UA_Variant **rows; /* Each row is an array of variants */
} UA_QueryTable;

/* Returns NULL if an error occurs */
UA_Variant *
UA_QueryTable_addRow(UA_QueryTable *qt);

void
UA_QueryTable_removeRow(UA_QueryTable *qt, size_t index);

UA_StatusCode
UA_QueryTable_init(UA_QueryTable *qt, const UA_ContentFilter *filter);

void
UA_QueryTable_clear(UA_QueryTable *qt);

#endif /* UA_ENABLE_QUERY */
