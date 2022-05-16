/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2019 (c) Julius Pfrommer, Fraunhofer IOSB
 */

#ifndef UA_NODESTORE_DEFAULT_H_
#define UA_NODESTORE_DEFAULT_H_

#include <open62541/plugin/nodestore.h>

_UA_BEGIN_DECLS

/* The HashMap Nodestore holds all nodes in RAM in single hash-map. Lookip is
 * done based on hashing/comparison of the NodeId with close to O(1) lookup
 * time. However, sometimes the underlying array has to be resized when nodes
 * are added/removed. This can take O(n) time. */
UA_EXPORT UA_StatusCode
UA_Nodestore_HashMap(UA_Nodestore *ns);

/* The ZipTree Nodestore holds all nodes in RAM in a tree structure. The lookup
 * time is about O(log n). Adding/removing nodes does not require resizing of
 * the underlying array with the linear overhead.
 *
 *  For most usage scenarios the hash-map Nodestore will be faster.
 */
UA_EXPORT UA_StatusCode
UA_Nodestore_ZipTree(UA_Nodestore *ns);

_UA_END_DECLS

#endif /* UA_NODESTORE_DEFAULT_H_ */
