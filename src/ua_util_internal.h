/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014, 2017 (c) Florian Palm
 *    Copyright 2015 (c) LEvertz
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015 (c) Chris Iatrou
 *    Copyright 2015-2016 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_UTIL_H_
#define UA_UTIL_H_

#define UA_INTERNAL
#include <open62541/types.h>
#include <open62541/util.h>

_UA_BEGIN_DECLS

/* Macro-Expand for MSVC workarounds */
#define UA_MACRO_EXPAND(x) x

/* Integer Shortnames
 * ------------------
 * These are not exposed on the public API, since many user-applications make
 * the same definitions in their headers. */

typedef UA_Byte u8;
typedef UA_SByte i8;
typedef UA_UInt16 u16;
typedef UA_Int16 i16;
typedef UA_UInt32 u32;
typedef UA_Int32 i32;
typedef UA_UInt64 u64;
typedef UA_Int64 i64;
typedef UA_StatusCode status;

/* Utility Functions
 * ----------------- */

#ifdef UA_DEBUG_DUMP_PKGS
void UA_EXPORT UA_dump_hex_pkg(UA_Byte* buffer, size_t bufferLen);
#endif

_UA_END_DECLS

#endif /* UA_UTIL_H_ */
