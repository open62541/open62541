/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_INTERNAL_GENERATED_TYPES_H_
#define UA_INTERNAL_GENERATED_TYPES_H_

#include <open62541/server.h>

_UA_BEGIN_DECLS

/* Support interface for generated C implementation files, not an application
 * API. Generated code and this header must match the library version.
 * Regenerate datatype and namespace files together when updating the library.
 * Applications use the public generated arrays and
 * UA_Server_getGeneratedDataType instead of these records. */
/* Generated datatype arrays retain an immutable origin for namespace remapping.
 * originTypes uses namespaceUris indexes. staticTypes is the exported array;
 * its namespace indexes may be adjusted by an in-place loader. Only generated
 * implementation code defines these records; the runtime owns inUse. */
typedef struct {
    const UA_DataType *originTypes;
    const UA_DataType *staticTypes;
    size_t typesSize;
    const UA_String *namespaceUris;
    size_t namespaceUrisSize;
    UA_Boolean staticTypesAreConst;
    UA_atomic(uintptr_t) inUse;
} UA_DataTypeArraySource;

/* Register a generated array after its DataType nodes and dependencies exist.
 * copyTypes creates server-owned descriptors using the description converter.
 * Otherwise mutable static arrays are bound once and cannot be reused by a
 * second server, even after the first server is deleted. Const/pinned arrays
 * can be shared when their namespace indexes match. Dependencies must already
 * be registered; in-place member pointers must match those registrations.
 * Registration is atomic for the array. Adding namespace nodes is separate. */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_addGeneratedDataTypes(UA_Server *server, UA_DataTypeArraySource *source,
                               size_t dependenciesSize,
                               UA_DataTypeArraySource *const *dependencies,
                               UA_Boolean copyTypes);

_UA_END_DECLS

#endif /* UA_INTERNAL_GENERATED_TYPES_H_ */
