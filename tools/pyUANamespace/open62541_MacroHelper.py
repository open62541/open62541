#!/usr/bin/env/python
# -*- coding: utf-8 -*-

# This Source Code Form is subject to the terms of the Mozilla Public 
# License, v. 2.0. If a copy of the MPL was not distributed with this 
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

###
### Author:  Chris Iatrou (ichrispa@core-vector.net)
### Version: rev 13
###
### This program was created for educational purposes and has been
### contributed to the open62541 project by the author. All licensing
### terms for this source is inherited by the terms and conditions
### specified for by the open62541 project (see the projects readme
### file for more information on the MPLv2 terms and restrictions).
###
### This program is not meant to be used in a production environment. The
### author is not liable for any complications arising due to the use of
### this program.
###

import logging
from ua_constants import *
import string


logger = logging.getLogger(__name__)

__unique_item_id = 0

defined_typealiases = []

class open62541_MacroHelper():
  def __init__(self, supressGenerationOfAttribute=[]):
    self.supressGenerationOfAttribute = supressGenerationOfAttribute

  def getCreateExpandedNodeIDMacro(self, node):
    if node.id().i != None:
      return "UA_EXPANDEDNODEID_NUMERIC(" + str(node.id().ns) + ", " + str(node.id().i) + ")"
    elif node.id().s != None:
      return "UA_EXPANDEDNODEID_STRING("  + str(node.id().ns) + ", " + node.id().s + ")"
    elif node.id().b != None:
      logger.debug("NodeID Generation macro for bytestrings has not been implemented.")
      return ""
    elif node.id().g != None:
      logger.debug("NodeID Generation macro for guids has not been implemented.")
      return ""
    else:
      return ""

  def substitutePunctuationCharacters(self, input):
    ''' substitutePunctuationCharacters

        Replace punctuation characters in input. Part of this class because it is used by
        ua_namespace on occasion.

        returns: C-printable string representation of input
    '''
    # No punctuation characters <>!$
    for illegal_char in list(string.punctuation):
        if illegal_char == '_': # underscore is allowed
            continue
        input = input.replace(illegal_char, "_") # map to underscore
    return input

  def getNodeIdDefineString(self, node):
    code = []
    extrNs = node.browseName().split(":")
    symbolic_name = ""
    # strip all characters that would be illegal in C-Code
    if len(extrNs) > 1:
        nodename = extrNs[1]
    else:
        nodename = extrNs[0]

    symbolic_name = self.substitutePunctuationCharacters(nodename)
    if symbolic_name != nodename :
        logger.warn("Subsituted characters in browsename for nodeid " + str(node.id().i) + " while generating C-Code ")

    if symbolic_name in defined_typealiases:
      logger.warn(self, "Typealias definition of " + str(node.id().i) + " is non unique!")
      extendedN = 1
      while (symbolic_name+"_"+str(extendedN) in defined_typealiases):
        logger.warn("Typealias definition of " + str(node.id().i) + " is non unique!")
        extendedN+=1
      symbolic_name = symbolic_name+"_"+str(extendedN)

    defined_typealiases.append(symbolic_name)

    code.append("#define UA_NS"  + str(node.id().ns) + "ID_" + symbolic_name.upper() + " " + str(node.id().i))
    return code

  def getCreateNodeIDMacro(self, node):
    if node.id().i != None:
      return "UA_NODEID_NUMERIC(" + str(node.id().ns) + ", " + str(node.id().i) + ")"
    elif node.id().s != None:
      return "UA_NODEID_STRING("  + str(node.id().ns) + ", " + node.id().s + ")"
    elif node.id().b != None:
      logger.debug("NodeID Generation macro for bytestrings has not been implemented.")
      return ""
    elif node.id().g != None:
      logger.debug("NodeID Generation macro for guids has not been implemented.")
      return ""
    else:
      return ""

  def getCreateStandaloneReference(self, sourcenode, reference):
    code = []

    if reference.isForward():
      code.append("UA_Server_addReference(server, " + self.getCreateNodeIDMacro(sourcenode) + ", " + self.getCreateNodeIDMacro(reference.referenceType()) + ", " + self.getCreateExpandedNodeIDMacro(reference.target()) + ", true);")
    else:
      code.append("UA_Server_addReference(server, " + self.getCreateNodeIDMacro(sourcenode) + ", " + self.getCreateNodeIDMacro(reference.referenceType()) + ", " + self.getCreateExpandedNodeIDMacro(reference.target()) + ", false);")
    return code

  def getCreateNodeNoBootstrap(self, node, unprintedNodes):
    code = []

    if node.nodeClass() == NODE_CLASS_OBJECT:
      nodetype = "Object"
    elif node.nodeClass() == NODE_CLASS_VARIABLE:
      nodetype = "Variable"
    elif node.nodeClass() == NODE_CLASS_METHOD:
      nodetype = "Method"
    elif node.nodeClass() == NODE_CLASS_OBJECTTYPE:
      nodetype = "ObjectType"
    elif node.nodeClass() == NODE_CLASS_REFERENCETYPE:
      nodetype = "ReferenceType"
    elif node.nodeClass() == NODE_CLASS_VARIABLETYPE:
      nodetype = "VariableType"
    elif node.nodeClass() == NODE_CLASS_DATATYPE:
      nodetype = "DataType"
    elif node.nodeClass() == NODE_CLASS_VIEW:
      nodetype = "View"
    else:
      code.append("/* undefined nodeclass */")
      return code;

    # If this is a method, construct in/outargs for addMethod
    #inputArguments.arrayDimensionsSize = 0;
    #inputArguments.arrayDimensions = NULL;
    #inputArguments.dataType = UA_TYPES[UA_TYPES_STRING].typeId;

    # Node ordering should have made sure that arguments, if they exist, have not been printed yet
    if node.nodeClass() == NODE_CLASS_METHOD:
        inArgVal = []
        outArgVal = []
        code.append("UA_Argument *inputArguments = NULL;")
        code.append("UA_Argument *outputArguments = NULL;")
        for r in node.getReferences():
            if r.isForward():
                if r.target() != None and r.target().nodeClass() == NODE_CLASS_VARIABLE and r.target().browseName() == 'InputArguments':
                    while r.target() in unprintedNodes:
                        unprintedNodes.remove(r.target())
                    if r.target().value() != None:
                        inArgVal = r.target().value().value
                elif r.target() != None and r.target().nodeClass() == NODE_CLASS_VARIABLE and r.target().browseName() == 'OutputArguments':
                    while r.target() in unprintedNodes:
                        unprintedNodes.remove(r.target())
                    if r.target().value() != None:
                        outArgVal = r.target().value().value
        if len(inArgVal)>0:
            code.append("")
            code.append("inputArguments = (UA_Argument *) malloc(sizeof(UA_Argument) * " + str(len(inArgVal)) + ");")
            code.append("int inputArgumentCnt;")
            code.append("for (inputArgumentCnt=0; inputArgumentCnt<" + str(len(inArgVal)) + "; ++inputArgumentCnt) UA_Argument_init(&inputArguments[inputArgumentCnt]); ")
            argumentCnt = 0
            for inArg in inArgVal:
                if inArg.getValueFieldByAlias("Description") != None:
                    code.append("inputArguments[" + str(argumentCnt) + "].description = UA_LOCALIZEDTEXT(\"" + str(inArg.getValueFieldByAlias("Description")[0]) + "\",\"" + str(inArg.getValueFieldByAlias("Description")[1]) + "\");")
                if inArg.getValueFieldByAlias("Name") != None:
                    code.append("inputArguments[" + str(argumentCnt) + "].name = UA_STRING(\"" + str(inArg.getValueFieldByAlias("Name")) + "\");")
                if inArg.getValueFieldByAlias("ValueRank") != None:
                    code.append("inputArguments[" + str(argumentCnt) + "].valueRank = " + str(inArg.getValueFieldByAlias("ValueRank")) + ";")
                if inArg.getValueFieldByAlias("DataType") != None:
                    code.append("inputArguments[" + str(argumentCnt) + "].dataType = " + str(self.getCreateNodeIDMacro(inArg.getValueFieldByAlias("DataType"))) + ";")
                #if inArg.getValueFieldByAlias("ArrayDimensions") != None:
                #  code.append("inputArguments[" + str(argumentCnt) + "].arrayDimensions = " + str(inArg.getValueFieldByAlias("ArrayDimensions")) + ";")
                argumentCnt += 1
        if len(outArgVal)>0:
            code.append("")
            code.append("outputArguments = (UA_Argument *) malloc(sizeof(UA_Argument) * " + str(len(outArgVal)) + ");")
            code.append("int outputArgumentCnt;")
            code.append("for (outputArgumentCnt=0; outputArgumentCnt<" + str(len(outArgVal)) + "; ++outputArgumentCnt) UA_Argument_init(&outputArguments[outputArgumentCnt]); ")
            argumentCnt = 0
            for outArg in outArgVal:
                if outArg.getValueFieldByAlias("Description") != None:
                    code.append("outputArguments[" + str(argumentCnt) + "].description = UA_LOCALIZEDTEXT(\"" + str(outArg.getValueFieldByAlias("Description")[0]) + "\",\"" + str(outArg.getValueFieldByAlias("Description")[1]) + "\");")
                if outArg.getValueFieldByAlias("Name") != None:
                    code.append("outputArguments[" + str(argumentCnt) + "].name = UA_STRING(\"" + str(outArg.getValueFieldByAlias("Name")) + "\");")
                if outArg.getValueFieldByAlias("ValueRank") != None:
                    code.append("outputArguments[" + str(argumentCnt) + "].valueRank = " + str(outArg.getValueFieldByAlias("ValueRank")) + ";")
                if outArg.getValueFieldByAlias("DataType") != None:
                    code.append("outputArguments[" + str(argumentCnt) + "].dataType = " + str(self.getCreateNodeIDMacro(outArg.getValueFieldByAlias("DataType"))) + ";")
                #if outArg.getValueFieldByAlias("ArrayDimensions") != None:
                #  code.append("outputArguments[" + str(argumentCnt) + "].arrayDimensions = " + str(outArg.getValueFieldByAlias("ArrayDimensions")) + ";")
                argumentCnt += 1

    # Print the nodeids
    code.append("UA_NodeId nodeId = " + str(self.getCreateNodeIDMacro(node)) + ";")
    #code.append("UA_NodeId parentNodeId = " + str(self.getCreateNodeIDMacro(parentNode)) + ";")
    #code.append("UA_NodeId parentReferenceNodeId = " + str(self.getCreateNodeIDMacro(parentReference.referenceType())) + ";")

    # Print the attributes struct
    code.append("UA_%sAttributes attr;" % nodetype)
    code.append("UA_%sAttributes_init(&attr);" %  nodetype);
    code.append("attr.displayName = UA_LOCALIZEDTEXT(\"\", \"" + str(node.displayName()) + "\");")
    code.append("attr.description = UA_LOCALIZEDTEXT(\"\", \"" + str(node.description()) + "\");")

    if nodetype == "Variable":
      code.append("attr.accessLevel = %s;"     % str(node.accessLevel()))
      code.append("attr.userAccessLevel = %s;" % str(node.userAccessLevel()))
    if nodetype in ["Variable", "VariableType"]:
      code.append("attr.valueRank = %s;"       % str(node.valueRank()))

    if nodetype in ["Variable", "VariableType"]:
      if node.dataType():
        code.append("attr.dataType = %s;" % str(self.getCreateNodeIDMacro(node.dataType().target())))
      if node.dataType() and node.dataType().target().isEncodable():
        if node.value() != None:
          code.append(node.value().printOpen62541CCode())

    if nodetype == "Method":
      if node.executable():
        code.append("attr.executable = true;")
      if node.userExecutable():
        code.append("attr.userExecutable = true;")

    if nodetype in ["ObjectType", "ReferenceType", "DataType", "VariableType"]:
      if node.isAbstract():
        code.append("attr.isAbstract = true;")
    if nodetype == "ReferenceType":
      if node.__reference_inverseName__ != "":
        code.append("attr.inverseName = UA_LOCALIZEDTEXT(\"en_US\", \"" + node.__reference_inverseName__ + "\");")
      if node.symmetric():
        code.append("attr.symmetric  = true;")

    # Print the browsename
    extrNs = node.browseName().split(":")
    if len(extrNs) > 1:
      code.append("UA_QualifiedName nodeName = UA_QUALIFIEDNAME(" +  str(extrNs[0]) + ", \"" + extrNs[1] + "\");")
    else:
      code.append("UA_QualifiedName nodeName = UA_QUALIFIEDNAME(0, \"" + str(node.browseName()) + "\");")

    # In case of a MethodNode: Add in|outArg struct generation here. Mandates
    # that namespace reordering was done using Djikstra
    if nodetype in ["Object", "Variable", "VariableType", "ObjectType", "View",
                    "ReferenceType", "DataType"]:
        code.append("UA_Server_add%sNode_begin(server, nodeId, nodeName, attr, NULL);" % nodetype)
    elif nodetype != "Method":
        code.append("UA_Server_addMethodNode_begin(server, nodeId, nodeName, attr, NULL, NULL, NULL);" % nodetype)
    else:
      raise Exception("invaid node type")
    return code
