/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2019 (c) Julius Pfrommer, Fraunhofer IOSB
 *    Copyright 2020 (c) Kalycito Infotech Pvt Ltd
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

#ifdef UA_ENABLE_USE_ENCODED_NODES
/*
 * Binary encoded Nodestore contains nodes in a compressed format. The lookupTable
 * is used to locate the index location of compressed node. The MINIMAL nodes
 * should be present in the information model for adding datasource and method
 * nodes. It uses the copy of zipTree to hold the MINIMAL nodes
 */
UA_EXPORT UA_StatusCode
UA_Nodestore_BinaryEncoded(UA_Nodestore *ns, const char *const lookupTablePath,
                         const char *const enocdedBinPath);

/* Lookup table for the encoded nodes */
struct lookUpTable;
typedef struct lookUpTable lookUpTable;
struct lookUpTable {
    UA_NodeId nodeId;
    size_t nodePosition;
    size_t nodeSize;
};

UA_Node*
decodeNode(void *ctx, UA_ByteString encodedBin, size_t offset);

void
encodeEditedNode(const UA_Node *node, UA_ByteString *encodedBin,
                          lookUpTable *lt, UA_UInt32 ltSize);
#endif

#ifdef UA_ENABLE_ENCODE_AND_DUMP
void
encodeNodeCallback(void *visitorCtx, const UA_Node *node);
#endif

_UA_END_DECLS

#endif /* UA_NODESTORE_DEFAULT_H_ */
