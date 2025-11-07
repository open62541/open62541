#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
#    Copyright 2023-2024 (c) Fraunhofer IOSB (Author: Florian Düwel)

# Python script to generate the *.svg images for the eventfilter parser query language ebnf
# This script uses the railroad.py file from
# https://github.com/tabatkins/railroad-diagrams (branch gh-pages)

from railroad import  Diagram, Choice, Sequence, OneOrMore, ZeroOrMore, Optional, Group

name_list = ["eventFilter", "operator", "simpleAttributeOperand", "operand", "literal", "nodeId"]
diagram_list = [
    #eventfilter
    Diagram(Group(Sequence(
        Group(Sequence("SELECT", OneOrMore("simpleAttributeOperand", repeat=",")),label="SelectClauses"),
        Group(Sequence("WHERE", Choice(1, "operator", Sequence("\"$\"", Choice(1, "\"String\"", "Number")))), label="WhereClause"),
        Group(Optional(Sequence("FOR",ZeroOrMore(Sequence(Sequence("\"$\"", Choice(1, "\"String\"", "Number")), ":=", Choice(1, "operand", "operator"))))),label="ForClause")
                           ),label="EventFilter")),
    #operator
    Diagram(Group(Choice(1, Choice(1,
                                   Group(Sequence("OFTYPE", Choice(1, "operand", "nodeId")),label="OfType"),
                                   Group(Sequence(Choice(1, "ISNULL", "\"0=\""), "operand"),label="IsNull"),
                                   Group(Sequence(Choice(1, "NOT", "!"), "operand"),label="Not")
                                   ),
                         Sequence("operand", Choice(1,
                                                    Group(Choice(1, "GREATERTHAN", ">", "GT"),label="GreaterThan"),
                                                    Group(Choice(1, "EQUALS", "==", "EQ"),label="Equals"),
                                                    Group(Choice(1, "LESSTHAN", "<", "LT"),label="LessThan"),
                                                    Group(Choice(1, "GREATEROREQUAL", ">=", "GE"),label="GreaterThanOrEqual"),
                                                    Group(Choice(1, "LESSOREQUAL", "<=", "LE"),label="LessThanOrEqual"),
                                                    Group(Choice(1, "LIKE", "<=>"),label="Like"),
                                                    Group(Choice(1, "CAST", "->"),label="Cast"),
                                                    Group(Choice(1, "BITAND", "&"),label="BitwiseAnd"),
                                                    Group(Choice(1, "BITOR", "|"),label="BitwiseOr")),
                                  "operand"),
                         Group(Sequence("operand", "BETWEEN", "[",  "operand", ",", "operand", "]"),label="Between"),
                         Group(Sequence("operand", "INLIST", "[",OneOrMore("operand",repeat=","), "]"),label="InList"),
                         Sequence(Choice(1, Group("AND",label="And"), Group("OR",label="Or")), Sequence("(", Choice(1, "operand", "operator"), ",", Choice(1, "operand", "operator"), ")"))),label="Operator")),
    #SAO
    Diagram(Group(Sequence(
        Group(Optional(Sequence("TYPEID", "nodeId")),label="typeDefinitionId"),
        Group(Sequence("PATH", "\"relativePath""\""),label="browsePath"),
        Group(Optional(Sequence("ATTRIBUTE", "Number")),label="attributeId"),
        Group(Optional(Sequence("INDEX", "NumericRange")),label="indexRange")

    ),label="SimpleAttributeOperand")),
    #operand
    Diagram(Group(Choice(1,
                         Group("SimpleAttributeOperand",label="SimpleAttributeOperand"),
                         Group(Choice(1, "JSONString", "literal"),label="LiteralOperand"),
                         Group(Sequence("\"$\"", Choice(1, "\"String\"", "Number")),label="ElementOperand")
                         ), label="Operand")),
    #literal
    Diagram(Group(Choice(1,
                         Group(Sequence("INT32", Optional("-"), "Number"),label="Int32"),
                         Group(Sequence("INT16", Optional("-"), "Number"),label="Int16"),
                         Group(Sequence(Optional("STRING"), "\"String\""),label="String"),
                         Group(Choice(1, Sequence("BOOL", Choice(1, "true", "false", "True", "False", "1", "0")), Choice(1, "true", "false", "True", "False")),label="Boolean"),
                         Group(Sequence(Optional("INT64"), Optional("-"), "Number"),label="Int64"),
                         Group(Sequence("UINT16", "Number"),label="UInt16"),
                         Group(Sequence("UINT32", "Number"),label="UInt32"),
                         Group(Sequence("UINT64", "Number"),label="UInt64"),
                         Group(Sequence("FLOAT", Optional("-"), "Floating Point"),label="Float"),
                         Group(Sequence( Optional("DOUBLE"), Optional("-"),  "Floating Point"),label="Double"),
                         Group(Sequence( Optional("NODEID"),  "NodeId"),label="NodeId"),
                         Group(Sequence("SBYTE", Optional("-"), "Number"),label="SByte"),
                         Group(Sequence("BYTE", "Number"),label="Byte"),
                         Group(Sequence("TIME", "\"TimeValueString\""),label="DateTime"),
                         Group(Sequence("GUID", "GUID-String"),label="Guid"),
                         Group(Sequence("BSTRING", "\"ByteString\""),label="ByteString"),
                         Group(Sequence("STATUSCODE", Choice(1, "Number", "\"String\"")),label="StatusCode"),
                         Group(Sequence("EXPNODEID", "\"String\""),label="ExpandedNodeId"),
                         Group(Sequence(Optional("QNAME"), "\"relativePathElement\""),label="QualifiedName"),
                         Group(Sequence("LOCALIZED", "\"String\""),label="LocalizedText")
                         ), label= "Literal")),
    #nodeId
    Diagram(Group(Sequence(Optional(Sequence("ns=", "Number", ";")),
                           Choice(1,
                                  Group(Sequence("s=" "\"String\""),label="String NodeId"),
                                  Group(Sequence("b=" "\"String\""),label="Bytestring NodeId"),
                                  Group(Sequence("i=", "Number"),label="Numeric NodeId"),
                                  Group(Sequence("g=", "\"GUID-String\""),label="GUID NodeId")
                                  )), label = "NodeId"))
]

for i in range(len(diagram_list)):
    f = open(name_list[i]+'.svg', 'w')
    svg = diagram_list[i].writeStandalone(f.write)
    f.close()



