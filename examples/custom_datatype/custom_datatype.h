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
        1,                               /* .binaryEncodingId, the numeric
                                         identifier used on the wire (the
                                         namespaceindex is from .typeId) */
        Point_members
};

/* The datatype description for the Measurement-Series datatype (Array Example)*/
typedef struct {
    UA_String description;
    size_t measurementSize;
    UA_Float *measurement;
} Measurements;

static UA_DataTypeMember Measurements_members[2] = {
    {
        UA_TYPENAME("Measurement description") /* .memberName */
        UA_TYPES_STRING,  /* .memberTypeIndex, points into UA_TYPES since namespaceZero is true */
        0,               /* .padding */
        true,            /* .namespaceZero, see .memberTypeIndex */
        false,            /* .isArray */
        false /* .isOptional */
    },
    {
        UA_TYPENAME("Measurements") /* .memberName */
        UA_TYPES_FLOAT,  /* .memberTypeIndex, points into UA_TYPES since namespaceZero is true */
        0,               /* .padding */
         true,            /* .namespaceZero, see .memberTypeIndex */
        true,            /* .isArray */
        false /* .isOptional */
    }
};

static const UA_DataType MeasurementType = {
    UA_TYPENAME("Measurement")             /* .tyspeName */
    {1, UA_NODEIDTYPE_NUMERIC, {4443}},     /* .typeId */
    sizeof(Measurements),                   /* .memSize */
    1,                               /* .typeIndex, in the array of custom types */
    UA_DATATYPEKIND_STRUCTURE,       /* .typeKind */
    false,                            /* .pointerFree */
    false,                           /* .overlayable (depends on endianness and
                                         the absence of padding) */
    2,                               /* .membersSize */
    2,                               /* .binaryEncodingId, the numeric
                                         identifier used on the wire (the
                                         namespaceindex is from .typeId) */
    Measurements_members
};


/* The datatype description for the Opt datatype (Structure with optional fields example)*/
typedef struct {
    UA_Int16 a;
    UA_Float *b;
    UA_Float *c;
} Opt;

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
    {1, UA_NODEIDTYPE_NUMERIC, {4644}}, /* .typeId */
    sizeof(Opt),                   /* .memSize */
    2,                               /* .typeIndex, in the array of custom types */
    UA_DATATYPEKIND_OPTSTRUCT,       /* .typeKind */
    false,                            /* .pointerFree */
    false,                           /* .overlayable (depends on endianness and
                                         the absence of padding) */
    3,                               /* .membersSize */
    3,                               /* .binaryEncodingId, the numeric
                                         identifier used on the wire (the
                                         namespaceindex is from .typeId) */
    Opt_members
};

/* The datatype description for the Uni datatype (Union example) */
typedef enum {UA_UNISWITCH_NONE = 0, UA_UNISWITCH_OPTIONA = 1, UA_UNISWITCH_OPTIONB = 2} UniSwitch;

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
    {1, UA_NODEIDTYPE_NUMERIC, {4845}},
    sizeof(Uni),
    3,
    UA_DATATYPEKIND_UNION,
    false,
    false,
    2,
    4,
    Uni_members
};
