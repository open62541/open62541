/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

typedef struct {
    UA_Boolean hasB;    /* defining if optional field "b" is defined or not */
    UA_Int16 a;
    UA_Float b;
} Opt;

/* flag "hasB" does not count as a member */
static UA_DataTypeMember Opt_members[2] = {
        /* a */
        {
            UA_TYPENAME("a") /* .memberName */
            UA_TYPES_INT16,  /* .memberTypeIndex, points into UA_TYPES since namespaceZero is true */
            offsetof(Opt,a) - offsetof(Opt,hasB) - sizeof(UA_Boolean),  /* .padding */
            true,       /* .namespaceZero, see .memberTypeIndex */
            false,      /* .isArray */
            false       /* .isOptional */
        },

        /* b */
        {
            UA_TYPENAME("b")
            UA_TYPES_FLOAT,
            offsetof(Opt,b) - offsetof(Opt,a) - sizeof(UA_Int16),
            true,
            false,
            true        /* b is an optional field */
        }
};

static const UA_DataType OptType = {
        UA_TYPENAME("Opt")             /* .typeName */
        {1, UA_NODEIDTYPE_NUMERIC, {4242}}, /* .typeId */
        sizeof(Opt),                   /* .memSize */
        0,                               /* .typeIndex, in the array of custom types */
        UA_DATATYPEKIND_OPTSTRUCT,       /* .typeKind */
        true,                            /* .pointerFree */
        false,                           /* .overlayable (depends on endianness and
                                         the absence of padding) */
        2,                               /* .membersSize */
        0,                               /* .binaryEncodingId, the numeric
                                         identifier used on the wire (the
                                         namespaceindex is from .typeId) */
        Opt_members
};


typedef struct {
    UA_UInt32 switchField;      /* defining which field is defined for the union */
    UA_Double x;
    UA_String y;
} Uni;

static UA_DataTypeMember Uni_members[3] = {
        /* switchField */
        {
            UA_TYPENAME("switchField") /* .memberName */
            UA_TYPES_UINT32,  /* .memberTypeIndex, points into UA_TYPES since namespaceZero is true */
            0,  /* .padding */
            true,       /* .namespaceZero, see .memberTypeIndex */
            false,      /* .isArray */
            false       /* .isOptional */
        },

        /* x */
        {
            UA_TYPENAME("x")
            UA_TYPES_DOUBLE,
            offsetof(Uni,x) - offsetof(Uni,switchField) - sizeof(UA_UInt32),
            true,
            false,
            false
        },

        /* y */
        {
            UA_TYPENAME("y")
            UA_TYPES_STRING,
            offsetof(Uni,y) - offsetof(Uni,x) - sizeof(UA_Double),
            true,
            false,
            false
        }
};

static const UA_DataType UniType = {
        UA_TYPENAME("Uni")
        {1, UA_NODEIDTYPE_NUMERIC, {4243}},
        sizeof(Uni),
        1,
        UA_DATATYPEKIND_UNION,
        true,
        false,
        3,
        0,
        Uni_members
};
