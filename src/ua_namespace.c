#include "ua_namespace.h"
#include "ua_types_generated_handling.h"

void UA_Namespace_init(UA_Namespace * namespace, const UA_String * namespaceUri){
    namespace->dataTypesSize = 0;
    namespace->dataTypes = NULL;
    namespace->nodestore = NULL;
    namespace->index = UA_NAMESPACE_UNDEFINED;
    UA_String_copy(namespaceUri, &namespace->uri);
}

UA_Namespace* UA_Namespace_new(const UA_String * namespaceUri){
    UA_Namespace* ns = UA_malloc(sizeof(UA_Namespace));
    UA_Namespace_init(ns,namespaceUri);
    return ns;
}

UA_Namespace* UA_Namespace_newFromChar(const char * namespaceUri){
    // Override const attribute to get string (dirty hack) /
    const UA_String nameString = {.length = strlen(namespaceUri),
                                  .data = (UA_Byte*)(uintptr_t)namespaceUri};
    return UA_Namespace_new(&nameString);
}


void UA_Namespace_deleteMembers(UA_Namespace* namespace){
    if(namespace->nodestore){
        namespace->nodestore->deleteNodestore(
                namespace->nodestore->handle);
    }
    UA_String_deleteMembers(&namespace->uri);
}

void UA_Namespace_updateDataTypes(UA_Namespace * namespaceToUpdate, UA_Namespace * namespaceNewDataTypes){
    if(namespaceNewDataTypes){
        //update data types
        namespaceToUpdate->dataTypesSize = 0;
        namespaceToUpdate->dataTypes = namespaceNewDataTypes->dataTypes;
        namespaceToUpdate->dataTypesSize = namespaceNewDataTypes->dataTypesSize;
    }
    // change indices in dataTypes
    for(size_t j = 0; j < namespaceToUpdate->dataTypesSize; j++){
        namespaceToUpdate->dataTypes[j].typeId.namespaceIndex = namespaceToUpdate->index;
        //TODO maybe better a pointer for data types namespaceIndex -> Only one pointer update needed.
    }
}
