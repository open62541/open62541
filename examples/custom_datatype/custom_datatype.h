/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

typedef struct {
    UA_Float x;
    UA_Float y;
    UA_Float z;
} Point;

/* The datatype description for the Point datatype */
#define Point_padding_y offsetof(Point,y) - offsetof(Point,x) - sizeof(UA_Float)
#define Point_padding_z offsetof(Point,z) - offsetof(Point,y) - sizeof(UA_Float)

static UA_DataTypeMember Point_members[3] = {
        /* x */
        {
                UA_TYPENAME("x") /* .memberName */
                UA_TYPES_FLOAT,  /* .memberTypeIndex, points into UA_TYPES since namespaceZero is true */
                0,               /* .padding */
                true,            /* .namespaceZero, see .memberTypeIndex */
                false,            /* .isArray */
                false
        },

        /* y */
        {
                UA_TYPENAME("y")
                UA_TYPES_FLOAT, Point_padding_y, true, false, false
        },

        /* z */
        {
                UA_TYPENAME("z")
                UA_TYPES_FLOAT, Point_padding_z, true, false, false
        }
};

static const UA_DataType PointType = {
        UA_TYPENAME("Point")             /* .tyspeName */
        {1, UA_NODEIDTYPE_NUMERIC, {4242}}, /* .typeId */
        sizeof(Point),                   /* .memSize */
        0,                               /* .typeIndex, in the array of custom types */
        UA_DATATYPEKIND_STRUCTURE,       /* .typeKind */
        true,                            /* .pointerFree */
        false,                           /* .overlayable (depends on endianness and
                                         the absence of padding) */
        3,                               /* .membersSize */
        0,                               /* .binaryEncodingId, the numeric
                                         identifier used on the wire (the
                                         namespaceindex is from .typeId) */
        Point_members
};

/* Array example start */

static UA_DataTypeMember Measurements_members[2] = {
    {
        UA_TYPENAME("Measurement description") /* .memberName */
        UA_TYPES_STRING,  /* .memberTypeIndex, points into UA_TYPES since namespaceZero is true */
        0,               /* .padding */
        true,            /* .namespaceZero, see .memberTypeIndex */
        false,            /* .isArray */
        false
    },
    {
        UA_TYPENAME("Measurements") /* .memberName */
        UA_TYPES_FLOAT,  /* .memberTypeIndex, points into UA_TYPES since namespaceZero is true */
        0,               /* .padding */
         true,            /* .namespaceZero, see .memberTypeIndex */
        true,            /* .isArray */
        false
    }
};

static const UA_DataType MeasurementType = {
    UA_TYPENAME("Point")             /* .tyspeName */
    {1, UA_NODEIDTYPE_NUMERIC, {4242}},     /* .typeId */
    sizeof(Point),                   /* .memSize */
    0,                               /* .typeIndex, in the array of custom types */
    UA_DATATYPEKIND_STRUCTURE,       /* .typeKind */
    false,                            /* .pointerFree */
    false,                           /* .overlayable (depends on endianness and
                                         the absence of padding) */
    2,                               /* .membersSize */
    0,                               /* .binaryEncodingId, the numeric
                                         identifier used on the wire (the
                                         namespaceindex is from .typeId) */
    Measurements_members
};

/* Array example end */



/* Adding a structure with optional fields */
typedef struct {
    UA_Int16 a;
    UA_Float *b;
    UA_Float *c;
} Opt;

/* flag "hasB" does not count as a member */
static UA_DataTypeMember Opt_members[3] = {
    /* a */
    {
        UA_TYPENAME("a") /* .memberName */
        UA_TYPES_INT16,  /* .memberTypeIndex, points into UA_TYPES since namespaceZero is true */
        0, /* .padding */
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
    },
    /* c */
    {
        UA_TYPENAME("c")
        UA_TYPES_FLOAT,
        offsetof(Opt,c) - offsetof(Opt,b) - sizeof(void *),
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
    false,                            /* .pointerFree */
    false,                           /* .overlayable (depends on endianness and
                                         the absence of padding) */
    3,                               /* .membersSize */
    0,                               /* .binaryEncodingId, the numeric
                                         identifier used on the wire (the
                                         namespaceindex is from .typeId) */
    Opt_members
};

typedef enum {UA_UNISWITCH_OPTIONA = 0, UA_UNISWITCH_OPTIONB = 1} UniSwitch;

typedef struct {
    UniSwitch switchField;
    union {
        UA_Double optionA;
        UA_String optionB;
    } fields;
} Uni;

static UA_DataTypeMember Uni_members[2] = {
    {
        UA_TYPENAME("optionA")
        UA_TYPES_DOUBLE,
        sizeof(UA_UInt32),
        true,
        false,
        false
    },
    {
        UA_TYPENAME("optionB")
        UA_TYPES_STRING,
        sizeof(UA_UInt32),
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
    false,
    false,
    2,
    0,
    Uni_members
};
