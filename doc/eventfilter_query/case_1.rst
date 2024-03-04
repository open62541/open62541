=======
Case 1
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
                "OFTYPE ns=1;i=5001";

    /* UA_EventFilter_parse takes a Bytestring as input*/
    UA_ByteString case_1 = UA_String_fromChars(inp);
    /* create the eventfilter from the string */
    UA_EventFilter_parse(filter, &case_1);

    UA_ByteString_clear(&case_1);

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
                        }
                ]
        }
    }