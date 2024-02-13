=======
Case 2
=======

.. code-block:: c

    /* Eventfilter */
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);

    /* query string */
    char *inp = "SELECT\n"
                "\n"
                "PATH \"/Message\", PATH \"/Severity\", PATH \"/EventType\"\n"
                "\n"
                "WHERE\n"
                "OR(OR(OR(OFTYPE ns=1;i=5002, $4), OR($5, OFTYPE i=3035)), OR($1,$2))\n"
                "\n"
                "FOR\n"
                "$1:= OFTYPE $7\n"
                "$2:= OFTYPE $8\n"
                "$4:= OFTYPE ns=1;i=5003\n"
                "$5:= OFTYPE ns=1;i=5004\n"
                "$7:= NODEID ns=1;i=5000\n"
                "$8:= ns=1;i=5001";

    /* UA_EventFilter_parse takes a Bytestring as input*/
    UA_ByteString case_2 = UA_String_fromChars(inp);
    /* create the eventfilter from the string */
    UA_EventFilter_parse(filter, &case_2);

    UA_ByteString_clear(&case_2);

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
                                FilterOperator: 11,
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
                                FilterOperator: 11,
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
                                FilterOperator: 11,
                                FilterOperands: [
                                        {
                                                TypeId: "i=592",
                                                Body: {
                                                        Index: 5
                                                }
                                        },
                                        {
                                                TypeId: "i=592",
                                                Body: {
                                                        Index: 6
                                                }
                                        }
                                ]
                        },
                        {
                                FilterOperator: 11,
                                FilterOperands: [
                                        {
                                                TypeId: "i=592",
                                                Body: {
                                                        Index: 7
                                                }
                                        },
                                        {
                                                TypeId: "i=592",
                                                Body: {
                                                        Index: 8
                                                }
                                        }
                                ]
                        },
                        {
                                FilterOperator: 11,
                                FilterOperands: [
                                        {
                                                TypeId: "i=592",
                                                Body: {
                                                        Index: 9
                                                }
                                        },
                                        {
                                                TypeId: "i=592",
                                                Body: {
                                                        Index: 10
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
                                                                Body: "ns=1;i=5000"
                                                        }
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
                                FilterOperator: 14,
                                FilterOperands: [
                                        {
                                                TypeId: "i=595",
                                                Body: {
                                                        Value: {
                                                                Type: NodeId,
                                                                Body: "ns=1;i=5002"
                                                        }
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
                                                                Body: "ns=1;i=5003"
                                                        }
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
                                                                Body: "ns=1;i=5004"
                                                        }
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
                                                                Body: "i=3035"
                                                        }
                                                }
                                        }
                                ]
                        }
                ]
        }
    }