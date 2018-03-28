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

#ifdef __cplusplus
extern "C" {
#endif

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

#define UA_ACCESSLEVELMASK_READ           (0x01<<0)
#define UA_ACCESSLEVELMASK_WRITE          (0x01<<1)
#define UA_ACCESSLEVELMASK_HISTORYREAD    (0x01<<2)
#define UA_ACCESSLEVELMASK_HISTORYWRITE   (0x01<<3)
#define UA_ACCESSLEVELMASK_SEMANTICCHANGE (0x01<<4)
#define UA_ACCESSLEVELMASK_STATUSWRITE    (0x01<<5)
#define UA_ACCESSLEVELMASK_TIMESTAMPWRITE (0x01<<6)

/**
 * Write Masks
 * -----------
 * The write mask and user write mask is given by the following constants that
 * are ANDed for the overall write mask. Part 3: 5.2.7 Table 2 */

#define UA_WRITEMASK_ACCESSLEVEL             (0x01<<0)
#define UA_WRITEMASK_ARRRAYDIMENSIONS        (0x01<<1)
#define UA_WRITEMASK_BROWSENAME              (0x01<<2)
#define UA_WRITEMASK_CONTAINSNOLOOPS         (0x01<<3)
#define UA_WRITEMASK_DATATYPE                (0x01<<4)
#define UA_WRITEMASK_DESCRIPTION             (0x01<<5)
#define UA_WRITEMASK_DISPLAYNAME             (0x01<<6)
#define UA_WRITEMASK_EVENTNOTIFIER           (0x01<<7)
#define UA_WRITEMASK_EXECUTABLE              (0x01<<8)
#define UA_WRITEMASK_HISTORIZING             (0x01<<9)
#define UA_WRITEMASK_INVERSENAME             (0x01<<10)
#define UA_WRITEMASK_ISABSTRACT              (0x01<<11)
#define UA_WRITEMASK_MINIMUMSAMPLINGINTERVAL (0x01<<12)
#define UA_WRITEMASK_NODECLASS               (0x01<<13)
#define UA_WRITEMASK_NODEID                  (0x01<<14)
#define UA_WRITEMASK_SYMMETRIC               (0x01<<15)
#define UA_WRITEMASK_USERACCESSLEVEL         (0x01<<16)
#define UA_WRITEMASK_USEREXECUTABLE          (0x01<<17)
#define UA_WRITEMASK_USERWRITEMASK           (0x01<<18)
#define UA_WRITEMASK_VALUERANK               (0x01<<19)
#define UA_WRITEMASK_WRITEMASK               (0x01<<20)
#define UA_WRITEMASK_VALUEFORVARIABLETYPE    (0x01<<21)

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_CONSTANTS_H_ */
