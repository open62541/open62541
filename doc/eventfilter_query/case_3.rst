=======
Case 3
=======

.. code-block:: c

    /* Eventfilter */
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);

    /* query string */
    char *inp = "SELECT\n"
                "\n"
                "PATH \"/Message\",\n"
                "PATH \"/Severity\",\n"
                "PATH \"/EventType\"\n"
                "\n"
                "WHERE\n"
                "AND((OFTYPE ns=1;i=5001), $1)\n"
                "\n"
                "FOR\n"
                "$1:=  AND($20, $30)\n"
                "$20:= INT64 99 == 99\n"
                "$30:= TYPEID i=5000 PATH \"/Severity\" > 99";

    /* UA_EventFilter_parse takes a Bytestring as input*/
    UA_ByteString case_3 = UA_String_fromChars(inp);
    /* create the eventfilter from the string */
    UA_EventFilter_parse(filter, &case_3);

    UA_ByteString_clear(&case_3);

The generated Eventfilter looks likes this:

.. code-block:: json

    {
        SelectClauses: [
                {
                        TypeDefinitionId: "i=2041",
                        BrowsePath: [
                                {
                                        Name: "Message"
                                }
                        ],
                        AttributeId: 13,
                        IndexRange: null
                },
                {
                        TypeDefinitionId: "i=2041",
                        BrowsePath: [
                                {
                                        Name: "Severity"
                                }
                        ],
                        AttributeId: 13,
                        IndexRange: null
                },
                {
                        TypeDefinitionId: "i=2041",
                        BrowsePath: [
                                {
                                        Name: "EventType"
                                }
                        ],
                        AttributeId: 13,
                        IndexRange: null
                }
        ],
        WhereClause: {
                Elements: [
                        {
                                FilterOperator: 10,
                                FilterOperands: [
                                        {
                                                TypeId: "i=592",
                                                Body: {
                                                        Index: 1
                                                }
                                        },
                                        {
                                                TypeId: "i=592",
                                                Body: {
                                                        Index: 2
                                                }
                                        }
                                ]
                        },
                        {
                                FilterOperator: 14,
                                FilterOperands: [
                                        {
                                                TypeId: "i=595",
                                                Body: {
                                                        Value: {
                                                                Type: NodeId,
                                                                Body: "ns=1;i=5001"
                                                        }
                                                }
                                        }
                                ]
                        },
                        {
                                FilterOperator: 10,
                                FilterOperands: [
                                        {
                                                TypeId: "i=592",
                                                Body: {
                                                        Index: 3
                                                }
                                        },
                                        {
                                                TypeId: "i=592",
                                                Body: {
                                                        Index: 4
                                                }
                                        }
                                ]
                        },
                        {
                                FilterOperator: 0,
                                FilterOperands: [
                                        {
                                                TypeId: "i=595",
                                                Body: {
                                                        Value: {
                                                                Type: Int64,
                                                                Body: "99"
                                                        }
                                                }
                                        },
                                        {
                                                TypeId: "i=595",
                                                Body: {
                                                        Value: {
                                                                Type: Int64,
                                                                Body: "99"
                                                        }
                                                }
                                        }
                                ]
                        },
                        {
                                FilterOperator: 2,
                                FilterOperands: [
                                        {
                                                TypeId: "i=601",
                                                Body: {
                                                        TypeDefinitionId: "i=5000",
                                                        BrowsePath: [
                                                                {
                                                                        Name: "Severity"
                                                                }
                                                        ],
                                                        AttributeId: 13,
                                                        IndexRange: null
                                                }
                                        },
                                        {
                                                TypeId: "i=595",
                                                Body: {
                                                        Value: {
                                                                Type: Int64,
                                                                Body: "99"
                                                        }
                                                }
                                        }
                                ]
                        }
                ]
        }
    }