#include "value.h"
#include "conversion.h"

enum VALUE_STATE {
    VALUE_STATE_INIT,
    VALUE_STATE_DATA,
    VALUE_STATE_FINISHED,
    VALUE_STATE_BUILTIN,
    VALUE_STATE_NODEID,
    VALUE_STATE_LOCALIZEDTEXT,
    VALUE_STATE_QUALIFIEDNAME,
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
    const char *name;
};

typedef void (*ConversionFn)(uintptr_t adr, char *value);

static UA_StatusCode
getMem(Value *val) {
    if(!val->typestack->type) {
        return UA_STATUSCODE_BADDATATYPEIDUNKNOWN;
    }
    void *newVal = UA_calloc(val->arrayCnt + 1, val->typestack->type->memSize);
    if(!newVal) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    memcpy(newVal, val->value, val->typestack->type->memSize * val->arrayCnt);
    UA_free(val->value);
    val->value = newVal;
    val->offset = val->typestack->type->memSize * val->arrayCnt;
    val->arrayCnt++;
    return UA_STATUSCODE_GOOD;
}

static TypeList* TypeList_push(TypeList* stack, const UA_DataType* newType)
{
    TypeList *newStack = (TypeList *)(UA_calloc(1, sizeof(TypeList)));
    newStack->next = stack;
    newStack->type = newType;
    return newStack;
}

Value *
Value_new(const UA_Node *node) {
    Value *val = (Value *)UA_calloc(1, sizeof(Value));
    val->state = VALUE_STATE_INIT;
    val->typestack = (TypeList *)UA_calloc(1, sizeof(TypeList));

    // looks only in UA_TYPES, should also look in custom types
    val->typestack->type = UA_findDataType(&((const UA_VariableNode *)node)->dataType);
    if(!val->typestack->type) {
        printf("could not determine type, value processing stopped\n");
        val->state = VALUE_STATE_ERROR;
        return val;
    }
    val->name = val->typestack->type->typeName;
    val->typestack->memberIndex = 0;
    if(getMem(val) != UA_STATUSCODE_GOOD) {
        printf("getMem failed, value processing stopped\n");
        val->state = VALUE_STATE_ERROR;
    }

    return val;
}

static void
isBuiltinSpecialType(Value *val) {
    switch(val->typestack->type->typeKind) {
        case UA_DATATYPEKIND_NODEID:
            val->state = VALUE_STATE_NODEID;
            break;
        case UA_DATATYPEKIND_LOCALIZEDTEXT:
            val->state = VALUE_STATE_LOCALIZEDTEXT;
            break;
        case UA_DATATYPEKIND_QUALIFIEDNAME:
            val->state = VALUE_STATE_QUALIFIEDNAME;
        default:
            break;
    }
}

void
Value_start(Value *val, const UA_Node *node, const char *localname) {
    if(!(UA_NODECLASS_VARIABLE == node->nodeClass)) {
        UA_assert(false && "called on wrong node class");
    }
    switch(val->state) {
        case VALUE_STATE_ERROR:
            break;
        case VALUE_STATE_FINISHED:
            val->typestack->memberIndex = 0;
            if(getMem(val) != UA_STATUSCODE_GOOD) {
                printf("getMem failed, value processing stopped\n");
                val->state = VALUE_STATE_ERROR;
            }
            val->state = VALUE_STATE_DATA;
            if(val->typestack->type->members) {
                val->name =
                    val->typestack->type->members[val->typestack->memberIndex].memberName;
            } else {
                val->name = val->typestack->type->typeName;
            }
        // intentional fall trough
        case VALUE_STATE_INIT:
            val->state = VALUE_STATE_DATA;
            if(!strncmp(localname, "ListOf", strlen("ListOf"))) {
                val->isArray = true;
                break;
            }
        // intentional fall through
        case VALUE_STATE_DATA:
            if(!val->name)
                break;
            if(!strcmp(val->name, localname)) {

                size_t idx = val->typestack->memberIndex;
                if(val->typestack->type->members) {
                    val->name = val->typestack->type->members[idx].memberName;
                    val->offset =
                        val->offset + val->typestack->type->members[idx].padding;
                    
                    val->typestack = TypeList_push(
                        val->typestack,
                        &UA_TYPES[val->typestack->type->members[idx].memberTypeIndex]);
                    val->state = VALUE_STATE_DATA;
                    if(val->typestack->type->members) {
                        val->name = val->typestack->type->members[0].memberName;
                    }
                }
                if(!val->typestack->type->members) {
                    val->state = VALUE_STATE_BUILTIN;
                    isBuiltinSpecialType(val);
                }
            }
            break;
        case VALUE_STATE_BUILTIN:
            break;
        case VALUE_STATE_NODEID:
            break;
        case VALUE_STATE_LOCALIZEDTEXT:
            break;
        case VALUE_STATE_QUALIFIEDNAME:
            break;
        default:
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
    setString,  // handle xmlElement like string
    setNodeId,         setNotImplemented, setNotImplemented, setNotImplemented,
    setNotImplemented,  // special handling needed
    setNotImplemented, setNotImplemented, setNotImplemented, setNotImplemented,
    setNotImplemented,
    setInt32,  // handle enum the same way like int32
    setNotImplemented, setNotImplemented, setNotImplemented, setNotImplemented};

static void
setScalarValueWithAddress(uintptr_t adr, UA_UInt32 kind, char *value) {
    if(value) {
        conversionTable[kind](adr, value);
    }
}

static void
setScalarValue(Value *val, const UA_DataType *type, char *value) {

    uintptr_t adr = (uintptr_t)val->value + val->offset;
    setScalarValueWithAddress(adr, type->typeKind, value);
    val->offset = val->offset + type->memSize;
}

static void
nextType(Value *val) {
    if(val->typestack->next == NULL) {
        if(!val->typestack->type->members) {
            val->state = VALUE_STATE_FINISHED;
            return;
        }
    } else {
        TypeList *tmp = val->typestack->next;
        UA_free(val->typestack);
        val->typestack = tmp;
        val->state = VALUE_STATE_DATA;
    }
    if(val->typestack->memberIndex >= val->typestack->type->membersSize) {
        val->state = VALUE_STATE_FINISHED;
        return;
    }
    val->typestack->memberIndex++;
    if(val->typestack->type->members) {
        val->name = val->typestack->type->members[val->typestack->memberIndex].memberName;
    } else {
        val->name = val->typestack->type->typeName;
    }
}

void
Value_end(Value *val, UA_Node *node, const char *localname, char *value) {
    switch(val->state) {
        case VALUE_STATE_INIT:
            break;
        case VALUE_STATE_BUILTIN:
            setScalarValue(val, val->typestack->type, value);
            nextType(val);
            break;
        case VALUE_STATE_NODEID:
            if(!strcmp(localname, "Identifier")) {
                setScalarValue(val, val->typestack->type, value);
                nextType(val);
            }
            break;
        case VALUE_STATE_LOCALIZEDTEXT:
            if(!strcmp(localname, "Locale")) {
                setScalarValueWithAddress(val->offset + (uintptr_t) &
                                              ((UA_LocalizedText *)val->value)->locale,
                                          UA_DATATYPEKIND_STRING, value);
            } else if(!strcmp(localname, "Text")) {
                setScalarValueWithAddress(val->offset + (uintptr_t) &
                                              ((UA_LocalizedText *)val->value)->text,
                                          UA_DATATYPEKIND_STRING, value);
            } else {
                val->offset = val->offset + sizeof(UA_LocalizedText);
                nextType(val);
            }
            break;
        case VALUE_STATE_QUALIFIEDNAME:
            if(!strcmp(localname, "NamespaceIndex")) {
                setScalarValueWithAddress(val->offset + (uintptr_t) &
                                              ((UA_QualifiedName *)val->value)->namespaceIndex,
                                          UA_DATATYPEKIND_UINT16, value);
            } else if(!strcmp(localname, "Name")) {
                setScalarValueWithAddress(val->offset + (uintptr_t) &
                                              ((UA_QualifiedName *)val->value)->name,
                                          UA_DATATYPEKIND_STRING, value);
            } else {
                val->offset = val->offset + sizeof(UA_QualifiedName);
                nextType(val);
            }
            break;
        case VALUE_STATE_DATA:
            if(!strcmp(localname, val->typestack->type->typeName)) {
                nextType(val);
                break;
            }
            break;
        case VALUE_STATE_ERROR:
            break;
        case VALUE_STATE_FINISHED:
            break;
        default:
            break;
    }
}

void
Value_finish(Value *val, UA_Node *node) {
    if(VALUE_STATE_FINISHED != val->state) {
        printf("Warning: value finish called while value state != finished\n");
    }
    if(val->value) {
        UA_VariableNode *varnode = (UA_VariableNode *)node;
        if(!val->isArray) {
            UA_StatusCode retval = UA_Variant_setScalarCopy(
                &varnode->value.data.value.value, val->value, val->typestack->type);
            if(!(UA_STATUSCODE_GOOD == retval)) {
                printf("error on seting scalar\n");
            } else {
                varnode->value.data.value.hasValue = UA_TRUE;
            }
        } else {
            UA_StatusCode retval =
                UA_Variant_setArrayCopy(&varnode->value.data.value.value, val->value,
                                        val->arrayCnt, val->typestack->type);
            if(!(UA_STATUSCODE_GOOD == retval)) {
                printf("error on seting array\n");
            } else {
                varnode->value.data.value.hasValue = UA_TRUE;
            }
        }
    }
    UA_free(val->value);
    UA_free(val->typestack);
    UA_free(val);
}
