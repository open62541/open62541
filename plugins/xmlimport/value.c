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
    VALUE_STATE_UNKNOWN_TYPE
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
    TypeList *types;
    size_t offset;
    const UA_DataType *currentMemberType;
};

typedef void (*ConversionFn)(uintptr_t adr, char *value);

Value *
Value_new() {
    Value *val = (Value *)UA_calloc(1, sizeof(Value));
    val->state = VALUE_STATE_INIT;
    val->types = (TypeList *)UA_calloc(1, sizeof(TypeList));
    return val;
}

static void
getMem(Value *val) {
    if(!val->types->type) {
        return;
    }
    void *newVal = UA_calloc(val->arrayCnt + 1, val->types->type->memSize);
    memcpy(newVal, val->value, val->types->type->memSize * val->arrayCnt);
    UA_free(val->value);
    val->value = newVal;
    val->offset = val->types->type->memSize * val->arrayCnt;
    val->arrayCnt++;
}

void
Value_start(Value *val, UA_Node *node, const char *localname, int attributeSize,
            const char **attribute) {
    if(!(UA_NODECLASS_VARIABLE == node->nodeClass)) {
        UA_assert(false && "called on wrong node class");
    }
    switch(val->state) {
        case VALUE_STATE_UNKNOWN_TYPE:
            break;
        case VALUE_STATE_INIT:
            if(!strcmp(localname, "ListOfExtensionObject")) {
                val->isArray = true;
                val->state = VALUE_STATE_EXTENSIONOBJECT;
            } else if(!strncmp(localname, "ListOf", strlen("ListOf"))) {
                val->isArray = true;
                val->state = VALUE_STATE_BUILTIN;
            } else if(!strcmp(localname, "ExtensionObject")) {
                val->state = VALUE_STATE_EXTENSIONOBJECT;
            } else {
                val->state = VALUE_STATE_BUILTIN;
            }
            val->types->type = UA_findDataType(&((UA_VariableNode *)node)->dataType);
            if(!val->types->type) {
                printf("could not determine type, value processing stopped\n");
                val->state = VALUE_STATE_UNKNOWN_TYPE;
            }
            val->types->memberIndex = 0;
            getMem(val);
            break;
        case VALUE_STATE_BUILTIN:
            break;
        case VALUE_STATE_EXTENSIONOBJECT:
            if(!strcmp(localname, "Body")) {
                val->state = VALUE_STATE_EXTENSIONOBJECT_BODY;
            }
            break;
        case VALUE_STATE_EXTENSIONOBJECT_BODY:
            if(!strcmp(localname, val->types->type->typeName)) {
                val->state = VALUE_STATE_EXTENSIONOBJECT_DATA;
            }
            break;
        case VALUE_STATE_EXTENSIONOBJECT_DATA:
            if(!strcmp(val->types->type->members[val->types->memberIndex].memberName,
                       localname)) {
                size_t idx = val->types->memberIndex;
                if(val->types->type->members[idx].namespaceZero) {
                    val->currentMemberType =
                        &UA_TYPES[val->types->type->members[idx].memberTypeIndex];
                    val->state = VALUE_STATE_EXTENSIONOBJECT_FIELD;

                    if(val->currentMemberType->typeKind == UA_DATATYPEKIND_NODEID) {
                        val->state = VALUE_STATE_NODEID;
                        break;
                    }

                    if(val->currentMemberType->membersSize > 0) {
                        //have to increment offset of struct in struct
                        val->offset =
                            val->offset + val->types->type->members[idx].padding;
                        TypeList *newType = (TypeList *)(UA_calloc(1, sizeof(TypeList)));
                        newType->next = val->types;
                        newType->type = val->currentMemberType;
                        newType->memberIndex = 0;
                        val->types = newType;
                        val->state = VALUE_STATE_EXTENSIONOBJECT_DATA; //stay here
                       
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
    UA_assert(false && "not implemented");
}

static void
setUInt64(uintptr_t adr, char *value) {
    UA_assert(false && "not implemented");
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
    setString,  // handle like bytestring like string
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
    if(!strncmp(localname, "ListOf", strlen("ListOf")))
        return;
    switch(val->state) {
        case VALUE_STATE_INIT:
            break;
        case VALUE_STATE_BUILTIN:
            setScalarValue(val, val->types->type, 0, value);
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
                           val->types->type->members[val->types->memberIndex].padding,
                           value);
            val->types->memberIndex++;
            val->state = VALUE_STATE_EXTENSIONOBJECT_DATA;
            break;
        case VALUE_STATE_NODEID:
            if(!strcmp(localname, "Identifier")) {
                setScalarValue(val, val->currentMemberType,
                               val->types->type->members[val->types->memberIndex].padding,
                               value);
                val->types->memberIndex++;
                val->state = VALUE_STATE_EXTENSIONOBJECT_DATA;
            }
            break;
        case VALUE_STATE_EXTENSIONOBJECT_DATA:
            if(!strcmp(localname, val->types->type->typeName)) {
                if(val->types->next == NULL)
                {
                    val->state = VALUE_STATE_EXTENSIONOBJECT_BODY;
                }
                else
                {
                    TypeList* tmp = val->types->next;
                    UA_free(val->types);
                    val->types = tmp;
                    val->types->memberIndex++;
                }
                break;
            }
            break;
        case VALUE_STATE_UNKNOWN_TYPE:
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
                                     val->types->type);
            varnode->value.data.value.hasValue = UA_TRUE;
        } else {
            UA_Variant_setArrayCopy(&varnode->value.data.value.value, val->value,
                                    val->arrayCnt, val->types->type);
            varnode->value.data.value.hasValue = UA_TRUE;
        }
    }
    UA_free(val->value);
    UA_free(val->types);
    UA_free(val);
}
