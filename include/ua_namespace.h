/* This Source Code Form is subject to the terms of the Mozilla Public
*  License, v. 2.0. If a copy of the MPL was not distributed with this
*  file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_NAMESPACE_H_
#define UA_NAMESPACE_H_

#include "ua_nodestore_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UA_NAMESPACE_UNDEFINED UA_UINT16_MAX

struct UA_Namespace{
    UA_UInt16 index;
    UA_String uri;
    UA_NodestoreInterface* nodestore;
    UA_DataType* dataTypes;
    size_t dataTypesSize;
};

void UA_EXPORT
UA_Namespace_init(UA_Namespace * namespacePtr, const UA_String * namespaceUri);
UA_Namespace UA_EXPORT *
UA_Namespace_new(const UA_String * namespaceUri);
UA_Namespace UA_EXPORT *
UA_Namespace_newFromChar(const char * namespaceUri);
void UA_EXPORT
UA_Namespace_deleteMembers(UA_Namespace* namespacePtr);

void
UA_Namespace_updateDataTypes(UA_Namespace * namespaceToUpdate,
                             UA_Namespace * namespaceNewDataTypes, UA_UInt16 newNamespaceIndex);
void
UA_Namespace_changeNodestore(UA_Namespace * namespacesToUpdate,
                             UA_Namespace * namespaceNewNodestore,
                             UA_NodestoreInterface * defaultNodestore,
                             UA_UInt16 newIdx);
void
UA_Namespace_updateNodestores(UA_Namespace * namespacesToUpdate, size_t namespacesToUpdateSize,
                              size_t* oldNsIdxToNewNsIdx, size_t oldNsIdxToNewNsIdxSize);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_NAMESPACE_H_ */
