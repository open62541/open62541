#ifndef UA_NAMESPACE_H_
#define UA_NAMESPACE_H_

#include "ua_nodestore_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UA_NAMESPACE_UNDEFINED UA_UINT16_MAX

typedef struct UA_Namespace{
    UA_UInt16 index;
    UA_String uri;
    UA_NodestoreInterface* nodestore;
    UA_DataType* dataTypes;
    size_t dataTypesSize;
}UA_Namespace;

void UA_EXPORT
UA_Namespace_init(UA_Namespace * namespace, const UA_String * namespaceUri);
UA_Namespace UA_EXPORT *
UA_Namespace_new(const UA_String * namespaceUri);
UA_Namespace UA_EXPORT *
UA_Namespace_newFromChar(const char * namespaceUri);
void UA_EXPORT
UA_Namespace_deleteMembers(UA_Namespace* namespaceUri);

void UA_Namespace_updateDataTypes(UA_Namespace * namespaceToUpdate, UA_Namespace * namespaceNewDataTypes);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_NAMESPACE_H_ */
