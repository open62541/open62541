=======
Case 4
=======

.. code-block:: c

    /* Eventfilter */
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);

    /* query string */
    char *inp = "SELECT\n"
                "\n"
                "PATH \"/Message\",\n"
                "PATH \"/0:Severity\",\n"
                "PATH \"/EventType\"\n"
                "\n"
                "WHERE\n"
                "\n"
                "AND($4, TYPEID i=5000 PATH \"/Severity\" GREATERTHAN $\"ref\")\n"
                "\n"
                "FOR\n"
                "$\"ref\":= 99\n"
                "$4:= OFTYPE ns=1;i=5000";

    /* UA_EventFilter_parse takes a Bytestring as input*/
    UA_ByteString case_4 = UA_String_fromChars(inp);
    /* create the eventfilter from the string */
    UA_EventFilter_parse(filter, &case_4);

    UA_ByteString_clear(&case_4);

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
                                                                Body: "ns=1;i=5000"
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