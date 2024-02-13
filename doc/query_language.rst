
QueryLanguage EventFilter
===========================


The following introduces a proposition for a new query language to create OPC UA EventFilter with the open62541 OPC UA SDK.
The OPC Foundation specifies OPC UA EventFilter in the OPC UA Specification part 4:Services. Each EventFilter
consists of a selectClauses that defines a list of values to be returned from a filtered event and a whereClause
that defines a ContentFilter. this ContentFilter contains criteria that have to be met to receive a notifications
about the event and the values specified in the selectClauses. The selectClauses is defined as a set of SimpleAttributeOperands.
The ContentFilter is a collection of operators where each of them features
a corresponding set of operands.


In accordance to the EventFilter definition, the query language is split into a SELECT and a WHERE statement. Since the
whereClause can describe a complex structure of operators, the resulting statement can be complex.
To increase the usability of the language, it introduces the FOR statement as an additional element that only occurs
within the language. Consequently, the concept of OPC UA EventFilter is not altered. The novelty of the FOR statement
are references for operators and operands. As a result, the WHERE statement can be simplified by referencing further
operands or operators as elements, which are then resolved within the FOR statement.


Such references can be used to link elements declared in the FOR statement or the in the  WHERE statement, to one
concrete element listed in the FOR statement. While multiple elements can reference one particular element,
each reference can only be resolved once. Since ElementOperands point to one ContentFilterElement inside the
ContentFilterElementArray, they can be seen as a reference from one operator to another.
References are equal to ElementOperands and can be arbitrary named within the query,
since they are replaced by the ContentFilterElementArrayIndex when constructing the EventFilter.

Query Example
-------------

The example below illustrates the logic of such references. Lines 1-4 specify the selectClauses of the filter.
Each operand in the selectClauses is separated by a comma.
The ContentFilter is defined from line 5 on. The basic structure consist of an OR operator (line 6), which references
a GREATHERTHAN operator with $"ref_1". The corresponding operator is then resolved in line 8 and includes a reference to
an operand ($"ref_2"). The concrete operand is then declared in line 9 by de-referencing $"ref_2":=. As last element, the OR
operator has an OFTYPE operator which references a NodeId that is defines in line 10 with reference $42.

1: SELECT

2: PATH "/Message",

3: PATH "/0:Severity",

4: PATH "/EventType"



5: WHERE

6: OR($"ref_1", OFTYPE $42)



7: FOR

8: $"ref_1":= PATH "/Severity" GREATERTHAN $"ref_2"

9: $"ref_2":= UINT32 10

10: $42:= ns=1;i=5003

Since both, references and ElementOperands specify a reference to either an operand or an operator, they are have an equal notation.
Both are defined by setting a $ in front of either a number or a string.
To resolve them, the reference is extended :=, followed by either an operand or operator (see e.g. $"ref_1" declared in line 6 is the resolved in line 8). It
is important to consider that references to operands are only allowed for Operators and references to Operators are only
allowed by ElementOperands to create valid EventFilters. In addition, references
can be nested, so that an initially referenced operator can contain further references to operands
(The OR operator in line 6 references the GREATERTHAN operator in line 8, which in turn references the LiteralOperand in line 9).

Usage
------------------


Utility Function
................
An implementation of this Query Language is accomplished with the open62541 OPC UA SDK. Examples on how to use it can be found here https://github.com/open62541/open62541/blob/master/examples/events

The open62541 stack provides the function UA_EventFilter_parse to generate EventFilters from queries. Its usage is shown below:

.. code-block:: c

    /*
     * UA_ByteString *content: query string in form of a UA_BytesString
     * UA_EventFilter *filter: returned EventFilter from the query
     */
    UA_StatusCode UA_EventFilter_parse(UA_ByteString *content,
                                       UA_EventFilter *filter)


EventFilter Examples
....................
The following queries are part of the EventFilter parser tutorial. The corresponding tutorials can be found here: https://github.com/open62541/open62541/tree/master/examples/events.
The tutorial itself consists of 5 cases:

.. toctree::

    eventfilter_query_examples/case_0
    eventfilter_query_examples/case_1
    eventfilter_query_examples/case_2
    eventfilter_query_examples/case_3
    eventfilter_query_examples/case_4

Since different queries can create identical EventFilter, some alternative queries are provided in the
dictionary https://github.com/open62541/open62541/tree/master/examples/events/example_queries

Extended Backus-Naur Form
-------------------------

EventFilter
...........

The following figures give a short overview of the Extended Backus-Naur form (EBNF) of the query language. In this context, String represents any given string value.
To use quotation marks within a string, they have to be escaped with \\\. Number represents any give number.

.. raw:: html

    <iframe src="_static/eventFilter.svg" height="200px" width="100%"></iframe>


........
Operator
........

The operators cover all FilterOperators specified in OPC UA Specification part 4:Services. Here, operators are defined by a keyword to select the corresponding Filteroperator and
a set of operands. The number of operands is determined by its definition.

.. raw:: html

    <iframe src="_static/operator.svg" height="600px" width="100%"></iframe>


........
Operand
........

Operands are either SimpleAttributeOperands, LiteralOperands or ElementOperands. Each ElementOperand is defined by "$:"
and an identifier, which can either be a string or a number.
LiteralOperands can be declare either with a JSON-String that contains a OPC UA JSON-encoded variant, or as a single value literal.

.. raw:: html

    <iframe src="_static/operand.svg" height="250px" width="100%"></iframe>


........................
Simple Attribute Operand
........................

.. raw:: html

    <iframe src="_static/simpleAttributeOperand.svg" height="150px" width="100%"></iframe>


.......
Literal
.......

Single value literals can be used to define single values for most build-in OPC UA types.

.. raw:: html

    <iframe src="_static/literal.svg" height="600px" width="100%"></iframe>


......
NodeId
......

.. raw:: html

    <iframe src="_static/nodeId.svg" height="200px" width="100%"></iframe>