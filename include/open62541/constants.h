/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2016-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2016 (c) Sten Grüner
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Florian Palm
 */

#ifndef UA_CONSTANTS_H_
#define UA_CONSTANTS_H_

#include <open62541/config.h>

_UA_BEGIN_DECLS

/**
 * Standard-Defined Constants
 * ==========================
 * This section contains numerical and string constants that are defined in the
 * OPC UA standard.
 *
 * .. _attribute-id:
 *
 * Attribute Id
 * ------------
 * Every node in an OPC UA information model contains attributes depending on
 * the node type. Possible attributes are as follows: */

typedef enum {
    UA_ATTRIBUTEID_NODEID                  = 1,
    UA_ATTRIBUTEID_NODECLASS               = 2,
    UA_ATTRIBUTEID_BROWSENAME              = 3,
    UA_ATTRIBUTEID_DISPLAYNAME             = 4,
    UA_ATTRIBUTEID_DESCRIPTION             = 5,
    UA_ATTRIBUTEID_WRITEMASK               = 6,
    UA_ATTRIBUTEID_USERWRITEMASK           = 7,
    UA_ATTRIBUTEID_ISABSTRACT              = 8,
    UA_ATTRIBUTEID_SYMMETRIC               = 9,
    UA_ATTRIBUTEID_INVERSENAME             = 10,
    UA_ATTRIBUTEID_CONTAINSNOLOOPS         = 11,
    UA_ATTRIBUTEID_EVENTNOTIFIER           = 12,
    UA_ATTRIBUTEID_VALUE                   = 13,
    UA_ATTRIBUTEID_DATATYPE                = 14,
    UA_ATTRIBUTEID_VALUERANK               = 15,
    UA_ATTRIBUTEID_ARRAYDIMENSIONS         = 16,
    UA_ATTRIBUTEID_ACCESSLEVEL             = 17,
    UA_ATTRIBUTEID_USERACCESSLEVEL         = 18,
    UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL = 19,
    UA_ATTRIBUTEID_HISTORIZING             = 20,
    UA_ATTRIBUTEID_EXECUTABLE              = 21,
    UA_ATTRIBUTEID_USEREXECUTABLE          = 22
} UA_AttributeId;

/**
 * Access Level Masks
 * ------------------
 * The access level to a node is given by the following constants that are ANDed
 * with the overall access level. */

#define UA_ACCESSLEVELMASK_READ           (0x01u << 0u)
#define UA_ACCESSLEVELMASK_WRITE          (0x01u << 1u)
#define UA_ACCESSLEVELMASK_HISTORYREAD    (0x01u << 2u)
#define UA_ACCESSLEVELMASK_HISTORYWRITE   (0x01u << 3u)
#define UA_ACCESSLEVELMASK_SEMANTICCHANGE (0x01u << 4u)
#define UA_ACCESSLEVELMASK_STATUSWRITE    (0x01u << 5u)
#define UA_ACCESSLEVELMASK_TIMESTAMPWRITE (0x01u << 6u)

/**
 * Write Masks
 * -----------
 * The write mask and user write mask is given by the following constants that
 * are ANDed for the overall write mask. Part 3: 5.2.7 Table 2 */

#define UA_WRITEMASK_ACCESSLEVEL             (0x01u << 0u)
#define UA_WRITEMASK_ARRRAYDIMENSIONS        (0x01u << 1u)
#define UA_WRITEMASK_BROWSENAME              (0x01u << 2u)
#define UA_WRITEMASK_CONTAINSNOLOOPS         (0x01u << 3u)
#define UA_WRITEMASK_DATATYPE                (0x01u << 4u)
#define UA_WRITEMASK_DESCRIPTION             (0x01u << 5u)
#define UA_WRITEMASK_DISPLAYNAME             (0x01u << 6u)
#define UA_WRITEMASK_EVENTNOTIFIER           (0x01u << 7u)
#define UA_WRITEMASK_EXECUTABLE              (0x01u << 8u)
#define UA_WRITEMASK_HISTORIZING             (0x01u << 9u)
#define UA_WRITEMASK_INVERSENAME             (0x01u << 10u)
#define UA_WRITEMASK_ISABSTRACT              (0x01u << 11u)
#define UA_WRITEMASK_MINIMUMSAMPLINGINTERVAL (0x01u << 12u)
#define UA_WRITEMASK_NODECLASS               (0x01u << 13u)
#define UA_WRITEMASK_NODEID                  (0x01u << 14u)
#define UA_WRITEMASK_SYMMETRIC               (0x01u << 15u)
#define UA_WRITEMASK_USERACCESSLEVEL         (0x01u << 16u)
#define UA_WRITEMASK_USEREXECUTABLE          (0x01u << 17u)
#define UA_WRITEMASK_USERWRITEMASK           (0x01u << 18u)
#define UA_WRITEMASK_VALUERANK               (0x01u << 19u)
#define UA_WRITEMASK_WRITEMASK               (0x01u << 20u)
#define UA_WRITEMASK_VALUEFORVARIABLETYPE    (0x01u << 21u)

/**
 * ValueRanks
 * ----------
 * The following are the most common ValueRanks used for Variables,
 * VariableTypes and method arguments. ValueRanks higher than 3 are valid as
 * well (but less common). */

#define UA_VALUERANK_SCALAR_OR_ONE_DIMENSION  -3
#define UA_VALUERANK_ANY                      -2
#define UA_VALUERANK_SCALAR                   -1
#define UA_VALUERANK_ONE_OR_MORE_DIMENSIONS    0
#define UA_VALUERANK_ONE_DIMENSION             1
#define UA_VALUERANK_TWO_DIMENSIONS            2
#define UA_VALUERANK_THREE_DIMENSIONS          3

/**
 * General Configuration Constants
 * ===============================
 *
 * This section defines constants that are used for the configuration of both
 * clients and servers.
 *
 * Rule Handling
 * -------------
 *
 * The RuleHanding settings define how error cases that result from rules in the
 * OPC UA specification shall be handled. The rule handling can be softened,
 * e.g. to workaround misbehaving implementations or to mitigate the impact of
 * additional rules that are introduced in later versions of the OPC UA
 * specification. */
typedef enum {
    UA_RULEHANDLING_DEFAULT = 0,
    UA_RULEHANDLING_ABORT,  /* Abort the operation and return an error code */
    UA_RULEHANDLING_WARN,   /* Print a message in the logs and continue */
    UA_RULEHANDLING_ACCEPT, /* Continue and disregard the broken rule */
} UA_RuleHandling;

_UA_END_DECLS

#endif /* UA_CONSTANTS_H_ */
