#include "value.h"
#include "conversion.h"

enum VALUE_STATE {
    VALUE_STATE_INIT,
    VALUE_STATE_BUILTIN,
    VALUE_STATE_EXTENSIONOBJECT,
    VALUE_STATE_EXTENSIONOBJECT_BODY,
    VALUE_STATE_EXTENSIONOBJECT_FIELD,
    VALUE_STATE_NODEID,
    VALUE_STATE_EXTENSIONOBJECT_DATA,
    VALUE_STATE_ERROR
};

typedef struct TypeList TypeList;
struct TypeList {
    const UA_DataType *type;
    size_t memberIndex;
    TypeList *next;
};

struct Value {
    bool isArray;
    enum VALUE_STATE state;
    void *value;
    size_t arrayCnt;
    TypeList *typestack;
    size_t offset;
    const UA_DataType *currentMemberType;
};

typedef void (*ConversionFn)(uintptr_t adr, char *value);

Value *
Value_new() {
    Value *val = (Value *)UA_calloc(1, sizeof(Value));
    val->state = VALUE_STATE_INIT;
    val->typestack = (TypeList *)UA_calloc(1, sizeof(TypeList));
    return val;
}

static UA_StatusCode
getMem(Value *val) {
    if(!val->typestack->type) {
        return UA_STATUSCODE_BADDATATYPEIDUNKNOWN;
    }
    void *newVal = UA_calloc(val->arrayCnt + 1, val->typestack->type->memSize);
    if(!newVal)
    {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    memcpy(newVal, val->value, val->typestack->type->memSize * val->arrayCnt);
    UA_free(val->value);
    val->value = newVal;
    val->offset = val->typestack->type->memSize * val->arrayCnt;
    val->arrayCnt++;
    return UA_STATUSCODE_GOOD;
}

void
Value_start(Value *val, const UA_Node *node, const char *localname)
{
    if(!(UA_NODECLASS_VARIABLE == node->nodeClass)) {
        UA_assert(false && "called on wrong node class");
    }
    switch(val->state) {
        case VALUE_STATE_ERROR:
            break;
        case VALUE_STATE_INIT:
            if(!strncmp(localname, "ListOf", strlen("ListOf"))) {
                val->isArray = true;
                break;
            } else if(!strcmp(localname, "ExtensionObject")) {
                val->state = VALUE_STATE_EXTENSIONOBJECT;
            } else {
                val->state = VALUE_STATE_BUILTIN;
            }
            //looks only in UA_TYPES, should also look in custom types
            val->typestack->type = UA_findDataType(&((const UA_VariableNode *)node)->dataType);
            if(!val->typestack->type) {
                printf("could not determine type, value processing stopped\n");
                val->state = VALUE_STATE_ERROR;
            }
            val->typestack->memberIndex = 0;
            if(getMem(val)!=UA_STATUSCODE_GOOD)
            {
                printf("getMem failed, value processing stopped\n");
                val->state = VALUE_STATE_ERROR;
            }
            break;
        case VALUE_STATE_BUILTIN:
            break;
        case VALUE_STATE_EXTENSIONOBJECT:
            if(!strcmp(localname, "Body")) {
                val->state = VALUE_STATE_EXTENSIONOBJECT_BODY;
            }
            break;
        case VALUE_STATE_EXTENSIONOBJECT_BODY:
            if(!strcmp(localname, val->typestack->type->typeName)) {
                val->state = VALUE_STATE_EXTENSIONOBJECT_DATA;
            }
            break;
        case VALUE_STATE_EXTENSIONOBJECT_DATA:
            //only quick fix
            if(val->typestack->memberIndex >= val->typestack->type->membersSize)
                break;
            if(!strcmp(
                   val->typestack->type->members[val->typestack->memberIndex].memberName,
                   localname)) {
                size_t idx = val->typestack->memberIndex;
                // should also take a look in custom types
                if(val->typestack->type->members[idx].namespaceZero) {
                    val->currentMemberType =
                        &UA_TYPES[val->typestack->type->members[idx].memberTypeIndex];
                    val->state = VALUE_STATE_EXTENSIONOBJECT_FIELD;

                    //is there a better option than doing it like this? must be done for qualified name, nodeId, localized Text ...
                    if(val->currentMemberType->typeKind == UA_DATATYPEKIND_NODEID) {
                        val->state = VALUE_STATE_NODEID;
                        break;
                    }

                    if(val->currentMemberType->membersSize > 0) {
                        val->offset =
                            val->offset + val->typestack->type->members[idx].padding;
                        TypeList *newType = (TypeList *)(UA_calloc(1, sizeof(TypeList)));
                        newType->next = val->typestack;
                        newType->type = val->currentMemberType;
                        newType->memberIndex = 0;
                        val->typestack = newType;
                        val->state = VALUE_STATE_EXTENSIONOBJECT_DATA;
                    }
                }
            }
            break;
        case VALUE_STATE_EXTENSIONOBJECT_FIELD:
            break;
        case VALUE_STATE_NODEID:
            break;
    }
}

static void
setBoolean(uintptr_t adr, char *value) {
    *(UA_Boolean *)adr = isTrue(value);
}

static void
setSByte(uintptr_t adr, char *value) {
    *(UA_SByte *)adr = (UA_SByte)atoi(value);
}

static void
setByte(uintptr_t adr, char *value) {
    *(UA_Byte *)adr = (UA_Byte)atoi(value);
}

static void
setInt16(uintptr_t adr, char *value) {
    *(UA_Int16 *)adr = (UA_Int16)atoi(value);
}

static void
setUInt16(uintptr_t adr, char *value) {
    *(UA_UInt16 *)adr = (UA_UInt16)atoi(value);
}

static void
setInt32(uintptr_t adr, char *value) {
    *(UA_Int32 *)adr = atoi(value);
}

static void
setUInt32(uintptr_t adr, char *value) {
    *(UA_UInt32 *)adr = (UA_UInt32)atoi(value);
}

static void
setInt64(uintptr_t adr, char *value) {
    *(UA_Int64 *)adr = atoi(value);
}

static void
setUInt64(uintptr_t adr, char *value) {
    *(UA_UInt64 *)adr = (UA_UInt64)atoi(value);
}

static void
setFloat(uintptr_t adr, char *value) {
    *(UA_Float *)adr = (UA_Float)atof(value);
}

static void
setDouble(uintptr_t adr, char *value) {
    *(UA_Double *)adr = atof(value);
}

static void
setString(uintptr_t adr, char *value) {
    UA_String *s = (UA_String *)adr;
    s->length = strlen(value);
    s->data = (UA_Byte *)value;
}

static void
setLocalizedText(uintptr_t adr, char *value) {
    //todo
    UA_LocalizedText *s = (UA_LocalizedText *)adr;
    s->locale = UA_STRING_NULL;
    s->text.data = (UA_Byte *)value;
    s->text.length = strlen(value);
}

static void
setDateTime(uintptr_t adr, char *value) {
    printf("DateTime: %s currently not handled\n", value);
}

static void
setNodeId(uintptr_t adr, char *value) {
    // todo: namespaceIndex should be converted?
    *(UA_NodeId *)adr = extractNodeId(value);
}

static void
setNotImplemented(uintptr_t adr, char *value) {
    UA_assert(false && "not implemented");
}

static const ConversionFn conversionTable[UA_DATATYPEKINDS] = {
    setBoolean,        setSByte,          setByte,           setInt16,
    setUInt16,         setInt32,          setUInt32,         setInt64,
    setUInt64,         setFloat,          setDouble,         setString,
    setDateTime,       setNotImplemented,
    setString,  // handle bytestring like string
    setNotImplemented, setNodeId,         setNotImplemented, setNotImplemented,
    setNotImplemented, setLocalizedText,  setNotImplemented, setNotImplemented,
    setNotImplemented, setNotImplemented, setNotImplemented,
    setInt32,  // handle enum the same way like int32
    setNotImplemented, setNotImplemented, setNotImplemented, setNotImplemented};

static void
setScalarValue(Value *val, const UA_DataType *type, size_t padding, char *value) {
    uintptr_t adr = (uintptr_t)val->value + val->offset + padding;
    if(value) {
        conversionTable[type->typeKind](adr, value);
    }
    val->offset = val->offset + type->memSize + padding;
}

void
Value_end(Value *val, UA_Node *node, const char *localname, char *value) {
    switch(val->state) {
        case VALUE_STATE_INIT:
            break;
        case VALUE_STATE_BUILTIN:
            setScalarValue(val, val->typestack->type, 0, value);
            val->state = VALUE_STATE_INIT;
            break;
        case VALUE_STATE_EXTENSIONOBJECT:
            if(!strcmp(localname, "ExtensionObject")) {
                val->state = VALUE_STATE_INIT;
                break;
            }
            break;
        case VALUE_STATE_EXTENSIONOBJECT_BODY:
            if(!strcmp(localname, "Body")) {
                val->state = VALUE_STATE_EXTENSIONOBJECT;
                break;
            }
            break;
        case VALUE_STATE_EXTENSIONOBJECT_FIELD:
            setScalarValue(val, val->currentMemberType,
                           val->typestack->type->members[val->typestack->memberIndex].padding,
                           value);
            val->typestack->memberIndex++;
            val->state = VALUE_STATE_EXTENSIONOBJECT_DATA;
            break;
        case VALUE_STATE_NODEID:
            if(!strcmp(localname, "Identifier")) {
                setScalarValue(val, val->currentMemberType,
                               val->typestack->type->members[val->typestack->memberIndex].padding,
                               value);
                val->typestack->memberIndex++;
                val->state = VALUE_STATE_EXTENSIONOBJECT_DATA;
            }
            break;
        case VALUE_STATE_EXTENSIONOBJECT_DATA:
            if(!strcmp(localname, val->typestack->type->typeName)) {
                if(val->typestack->next == NULL)
                {
                    val->state = VALUE_STATE_EXTENSIONOBJECT_BODY;
                }
                else
                {
                    TypeList* tmp = val->typestack->next;
                    UA_free(val->typestack);
                    val->typestack = tmp;
                    val->typestack->memberIndex++;
                }
                break;
            }
            break;
        case VALUE_STATE_ERROR:
            break;

        default:
            UA_assert(false && "should never end up here");
    }
}

void
Value_finish(Value *val, UA_Node *node) {
    if(val->value) {
        UA_VariableNode *varnode = (UA_VariableNode *)node;
        if(!val->isArray) {
            UA_Variant_setScalarCopy(&varnode->value.data.value.value, val->value,
                                     val->typestack->type);
            varnode->value.data.value.hasValue = UA_TRUE;
        } else {
            UA_Variant_setArrayCopy(&varnode->value.data.value.value, val->value,
                                    val->arrayCnt, val->typestack->type);
            varnode->value.data.value.hasValue = UA_TRUE;
        }
    }
    UA_free(val->value);
    UA_free(val->typestack);
    UA_free(val);
}
