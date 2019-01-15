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
                0,               /* .bitFieldSize */
                true,            /* .namespaceZero, see .memberTypeIndex */
                false,           /* .isArray */
                false            /* .isOptional */
        },

        /* y */
        {
                UA_TYPENAME("y")
                UA_TYPES_FLOAT, Point_padding_y, 0, true, false, false
        },

        /* z */
        {
                UA_TYPENAME("z")
                UA_TYPES_FLOAT, Point_padding_z, 0, true, false, false
        }
};

static const UA_DataType PointType = {
        UA_TYPENAME("Point")             /* .typeName */
        {1, UA_NODEIDTYPE_NUMERIC, {4242}}, /* .typeId */
        sizeof(Point),                   /* .memSize */
        0,                               /* .typeIndex, in the array of custom types */
        UA_DATATYPEKIND_STRUCTURE | UA_DATATYPEKIND_POINTERFREE,     /* .kind */
        3,                               /* .membersSize */
        0,                               /* .binaryEncodingId, the numeric
                                         identifier used on the wire (the
                                         namespaceindex is from .typeId) */
        Point_members,
        0 /* .encodingMaskFunc */
};
