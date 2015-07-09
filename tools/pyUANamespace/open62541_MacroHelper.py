#!/usr/bin/env/python
# -*- coding: utf-8 -*-

###
### Author:  Chris Iatrou (ichrispa@core-vector.net)
### Version: rev 13
###
### This program was created for educational purposes and has been
### contributed to the open62541 project by the author. All licensing
### terms for this source is inherited by the terms and conditions
### specified for by the open62541 project (see the projects readme
### file for more information on the LGPL terms and restrictions).
###
### This program is not meant to be used in a production environment. The
### author is not liable for any complications arising due to the use of
### this program.
###

from logger import *
from ua_constants import *

__unique_item_id = 0

class open62541_MacroHelper():
  def __init__(self, supressGenerationOfAttribute=[]):
    self.supressGenerationOfAttribute = supressGenerationOfAttribute

  def getCreateExpandedNodeIDMacro(self, node):
    if node.id().i != None:
      return "UA_EXPANDEDNODEID_NUMERIC(" + str(node.id().ns) + ", " + str(node.id().i) + ")"
    elif node.id().s != None:
      return "UA_EXPANDEDNODEID_STRING("  + str(node.id().ns) + ", " + node.id().s + ")"
    elif node.id().b != None:
      log(self, "NodeID Generation macro for bytestrings has not been implemented.")
      return ""
    elif node.id().g != None:
      log(self, "NodeID Generation macro for guids has not been implemented.")
      return ""
    else:
      return ""

  def getCreateNodeIDMacro(self, node):
    if node.id().i != None:
      return "UA_NODEID_NUMERIC(" + str(node.id().ns) + ", " + str(node.id().i) + ")"
    elif node.id().s != None:
      return "UA_NODEID_STRING("  + str(node.id().ns) + ", " + node.id().s + ")"
    elif node.id().b != None:
      log(self, "NodeID Generation macro for bytestrings has not been implemented.")
      return ""
    elif node.id().g != None:
      log(self, "NodeID Generation macro for guids has not been implemented.")
      return ""
    else:
      return ""

  def getCreateStandaloneReference(self, sourcenode, reference):
  # As reference from open62541 (we need to alter the attributes)
  #    UA_Server_addReference(UA_Server *server, const UA_NodeId sourceId, const UA_NodeId refTypeId,
  #                                   const UA_ExpandedNodeId targetId)
    code = []
    #refid = "ref_" + reference.getCodePrintableID()
    #code.append("UA_AddReferencesItem " + refid + ";")
    #code.append("UA_AddReferencesItem_init(&" + refid + ");")
    #code.append(refid + ".sourceNodeId = " + self.getCreateNodeIDMacro(sourcenode) + ";")
    #code.append(refid + ".referenceTypeId = " + self.getCreateNodeIDMacro(reference.referenceType()) + ";")
    #if reference.isForward():
      #code.append(refid + ".isForward = UA_TRUE;")
    #else:
      #code.append(refid + ".isForward = UA_FALSE;")
    #code.append(refid + ".targetNodeId = " + self.getCreateExpandedNodeIDMacro(reference.target()) + ";")
    #code.append("addOneWayReferenceWithSession(server, (UA_Session *) UA_NULL, &" + refid + ");")

    if reference.isForward():
      code.append("UA_Server_AddMonodirectionalReference(server, " + self.getCreateNodeIDMacro(sourcenode) + ", " + self.getCreateExpandedNodeIDMacro(reference.target()) + ", " + self.getCreateNodeIDMacro(reference.referenceType()) + ", UA_TRUE);")
    else:
      code.append("UA_Server_AddMonodirectionalReference(server, " + self.getCreateNodeIDMacro(sourcenode) + ", " + self.getCreateExpandedNodeIDMacro(reference.target()) + ", " + self.getCreateNodeIDMacro(reference.referenceType()) + ", UA_FALSE);")

    return code

  def getCreateNode(self, node):
    nodetype = ""
    code = []

    code.append("// Node: " + str(node) + ", " + str(node.browseName()))

    if node.nodeClass() == NODE_CLASS_OBJECT:
      nodetype = "UA_ObjectNode"
    elif node.nodeClass() == NODE_CLASS_VARIABLE:
      nodetype = "UA_VariableNode"
    elif node.nodeClass() == NODE_CLASS_METHOD:
      nodetype = "UA_MethodNode"
    elif node.nodeClass() == NODE_CLASS_OBJECTTYPE:
      nodetype = "UA_ObjectTypeNode"
    elif node.nodeClass() == NODE_CLASS_REFERENCETYPE:
      nodetype = "UA_ReferenceTypeNode"
    elif node.nodeClass() == NODE_CLASS_VARIABLETYPE:
      nodetype = "UA_VariableTypeNode"
    elif node.nodeClass() == NODE_CLASS_DATATYPE:
      nodetype = "UA_DataTypeNode"
    elif node.nodeClass() == NODE_CLASS_VIEW:
      nodetype = "UA_ViewNode"
    elif node.nodeClass() == NODE_CLASS_METHODTYPE:
      nodetype = "UA_MethodTypeNode"
    else:
      nodetype = "UA_NodeTypeNotFoundorGeneric"

    code.append(nodetype + " *" + node.getCodePrintableID() + " = " + nodetype + "_new();")
    if not "browsename" in self.supressGenerationOfAttribute:
      extrNs = node.browseName().split(":")
      if len(extrNs) > 1:
        code.append(node.getCodePrintableID() + "->browseName = UA_QUALIFIEDNAME_ALLOC(" +  str(extrNs[0]) + ", \"" + extrNs[1] + "\");")
      else:
        code.append(node.getCodePrintableID() + "->browseName = UA_QUALIFIEDNAME_ALLOC(0, \"" + node.browseName() + "\");")
    if not "displayname" in self.supressGenerationOfAttribute:
      code.append(node.getCodePrintableID() + "->displayName = UA_LOCALIZEDTEXT_ALLOC(\"en_US\", \"" +  node.displayName() + "\");")
    if not "description" in self.supressGenerationOfAttribute:
      code.append(node.getCodePrintableID() + "->description = UA_LOCALIZEDTEXT_ALLOC(\"en_US\", \"" +  node.description() + "\");")

    if not "writemask" in self.supressGenerationOfAttribute:
        if node.__node_writeMask__ != 0:
          code.append(node.getCodePrintableID() + "->writeMask = (UA_Int32) " +  str(node.__node_writeMask__) + ";")
    if not "userwritemask" in self.supressGenerationOfAttribute:
        if node.__node_userWriteMask__ != 0:
          code.append(node.getCodePrintableID() + "->userWriteMask = (UA_Int32) " + str(node.__node_userWriteMask__) + ";")
    #FIXME: Allocate descriptions, etc.

    if not "nodeid" in self.supressGenerationOfAttribute:
      if node.id().ns != 0:
        code.append(node.getCodePrintableID() + "->nodeId.namespaceIndex = " + str(node.id().ns) + ";")
      if node.id().i != None:
        code.append(node.getCodePrintableID() + "->nodeId.identifier.numeric = " + str(node.id().i) + ";")
      elif node.id().b != None:
        code.append(node.getCodePrintableID() + "->nodeId.identifierType = UA_NODEIDTYPE_BYTESTRING;")
        log(self, "ByteString IDs for nodes has not been implemented yet.", LOG_LEVEL_ERROR)
        return []
      elif node.id().g != None:
        #<jpfr> the string is sth like { .length = 111, .data = <ptr> }
        #<jpfr> there you _may_ alloc the <ptr> on the heap
        #<jpfr> for the guid, just set it to {.data1 = 111, .data2 = 2222, ....
        code.append(node.getCodePrintableID() + "->nodeId.identifierType = UA_NODEIDTYPE_GUID;")
        log(self, "GUIDs for nodes has not been implemented yet.", LOG_LEVEL_ERROR)
        return []
      elif node.id().s != None:
        code.append(node.getCodePrintableID() + "->nodeId.identifierType = UA_NODEIDTYPE_STRING;")
        code.append(node.getCodePrintableID() + "->nodeId.identifier.numeric = UA_STRING_ALLOC(\"" + str(node.id().i) + "\");")
      else:
        log(self, "Node ID is not numeric, bytestring, guid or string. I do not know how to create c code for that...", LOG_LEVEL_ERROR)
        return []

    return code
