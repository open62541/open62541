# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
"""Emit file-local loading code with direct references to generated arrays."""
COPY = r'''static UA_StatusCode
UA_GeneratedTypes_copy(UA_Server *server, const UA_DataType *type,
                       const UA_NamespaceMapping *nsMapping) {
    UA_ExtensionObject description;
    UA_StatusCode r = UA_DataType_toDescription(type, &description);
    if(r) return r;
    UA_AddDataTypeFromDescriptionOptions options = {0};
    options.namespaceMapping = nsMapping;
    options.binaryEncodingId = &type->binaryEncodingId;
    options.xmlEncodingId = &type->xmlEncodingId;
    r = UA_Server_addDataTypeFromDescriptionEx(server, &description, &options);
    UA_ExtensionObject_clear(&description);
    return r;
}'''


def type_loader_name(namespace, index, count):
    suffix = str(index) if count > 1 else ""
    return namespace + "_loadTypes" + suffix


def generate_type_support(sources, owned, namespace):
    """Map compiler-known namespaces before SDK member resolution."""
    if not owned:
        return ""
    lines = []
    emit = lines.append
    condition = " || ".join(arr + "_COUNT > 0" for arr in owned)
    emit("#if " + condition)
    copy_function = namespace + "_copyType"
    emit(COPY.replace("UA_GeneratedTypes_copy", copy_function))
    for index, arr in enumerate(owned):
        guard_array = condition != arr + "_COUNT > 0"
        if guard_array:
            emit("#if " + arr + "_COUNT > 0")
        emit("static UA_StatusCode " + type_loader_name(namespace, index, len(owned)) + "(UA_Server *server, UA_Boolean copy, const UA_NamespaceMapping *nsMapping) {")
        emit("if(copy) {")
        emit("for(size_t j = 0; j < " + arr + "_COUNT; j++) {")
        emit("size_t i = " + arr + "_ORDER[j];")
        emit("UA_StatusCode r = " + copy_function + "(server, &" + arr + "[i], nsMapping);")
        emit("if(r) return r;")
        emit("} return UA_STATUSCODE_GOOD;")
        emit("}")
        emit("for(size_t i = 0; i < " + arr + "_COUNT; i++) {")
        emit("if(UA_Server_findDataType(server, &" + arr + "[i].typeId))")
        emit("return UA_STATUSCODE_BADNODEIDEXISTS;")
        emit("}")
        for dep in sources:
            if dep == arr:
                continue
            emit("#if " + dep + "_COUNT > 0")
            emit("for(size_t i = 0; i < " + dep + "_COUNT; i++) {")
            emit("const UA_DataType *actual = UA_Server_findDataType(server, &" + dep + "[i].typeId);")
            emit("if(actual && actual != &" + dep + "[i]) return UA_STATUSCODE_BADCONFIGURATIONERROR; }")
            emit("#endif")
        emit("UA_ServerConfig *config = UA_Server_getConfig(server);")
        emit("UA_DataTypeArray *entry = (UA_DataTypeArray*)UA_malloc(sizeof(UA_DataTypeArray));")
        emit("if(!entry) return UA_STATUSCODE_BADOUTOFMEMORY;")
        emit("*entry = (UA_DataTypeArray){config->customDataTypes, " + arr + "_COUNT, " + arr + ", true, true};")
        emit("config->customDataTypes = entry;")
        emit("return UA_STATUSCODE_GOOD;")
        emit("}")
        if guard_array:
            emit("#endif")
    emit("#endif")
    return "\n".join(lines)
