=======
Case 0
=======

.. code-block:: c

    /* Eventfilter */
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    /* query string */
    char *inp = "SELECT\n"
                "PATH \"/Message\", PATH \"/0:Severity\", PATH \"/EventType\"\n"
                "WHERE\n"
                "OR($\"ref_1\", $\"ref_2\")\n"
                "FOR\n"
                "$\"ref_2\":= OFTYPE ns=1;i=5003\n"
                "$\"ref_1\":= OFTYPE i=3035";
    /* UA_EventFilter_parse takes a Bytestring as input*/
    UA_ByteString case_0 = UA_String_fromChars(inp);
    /* create the eventfilter from the string */
    UA_EventFilter_parse(filter, &case_0);
    UA_ByteString_clear(&case_0);

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
                        }
                ]
        }
    }