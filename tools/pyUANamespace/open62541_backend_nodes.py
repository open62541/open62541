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

from ua_node_types import *

####################
# Helper Functions #
####################

def getCreateNodeIDMacro(node):
    if node.id().i != None:
      return "UA_NODEID_NUMERIC(%s, %s)" % (node.id().ns, node.id().i)
    elif node.id().s != None:
      return "UA_NODEID_STRING(%s, %s)" % (node.id().ns, node.id().s)
    elif node.id().b != None:
      logger.debug("NodeID Generation macro for bytestrings has not been implemented.")
      return ""
    elif node.id().g != None:
      logger.debug("NodeID Generation macro for guids has not been implemented.")
      return ""
    else:
      return ""

def getCreateExpandedNodeIDMacro(node):
    if node.id().i != None:
      return "UA_EXPANDEDNODEID_NUMERIC(%s, %s)" % (str(node.id().ns),str(node.id().i))
    elif node.id().s != None:
      return "UA_EXPANDEDNODEID_STRING(%s, %s)" % (str(node.id().ns), node.id().s)
    elif node.id().b != None:
      logger.debug("NodeID Generation macro for bytestrings has not been implemented.")
      return ""
    elif node.id().g != None:
      logger.debug("NodeID Generation macro for guids has not been implemented.")
      return ""
    else:
      return ""

def getCreateStandaloneReference(sourcenode, reference):
    code = []
    if reference.isForward():
      code.append("UA_Server_addReference(server, %s, %s, %s, true);" % \
                  (getCreateNodeIDMacro(sourcenode), getCreateNodeIDMacro(reference.referenceType()), \
                   getCreateExpandedNodeIDMacro(reference.target())))
    else:
      code.append("UA_Server_addReference(server, %s, %s, %s, false);" % \
                  (getCreateNodeIDMacro(sourcenode), getCreateNodeIDMacro(reference.referenceType()), \
                   getCreateExpandedNodeIDMacro(reference.target())))
    return code


#################
# Subtype Early #
#################

def Node_printOpen62541CCode_SubtypeEarly(node, bootstrapping = True):
    """ Initiate code segments for the nodes instantiotion that preceed
        the actual UA_Server_addNode or UA_NodeStore_insert calls.
    """
    code = []
    if isinstance(node, opcua_node_variable_t) or isinstance(node, opcua_node_variableType_t):
        # If we have an encodable value, try to encode that
        if node.dataType() != None and isinstance(node.dataType().target(), opcua_node_dataType_t):
          # Delegate the encoding of the datavalue to the helper if we have
          # determined a valid encoding
          if node.dataType().target().isEncodable():
            if node.value() != None:
              code.extend(node.value().printOpen62541CCode(bootstrapping))
              return code
        if(bootstrapping):
          code.append("UA_Variant *" + node.getCodePrintableID() + "_variant = UA_alloca(sizeof(UA_Variant));")
          code.append("UA_Variant_init(" + node.getCodePrintableID() + "_variant);")

    return code

###########
# Subtype #
###########

def ReferenceTypeNode_printOpen62541CCode_Subtype(node, unPrintedReferences=[], bootstrapping = True):
    code = []

    # Detect if this is bootstrapping or if we are attempting to use userspace...
    if bootstrapping == False:
        typeDefs = node.getNamespace().getSubTypesOf() # defaults to TypeDefinition
        myTypeRef = None
        for ref in node.getReferences():
          if ref.referenceType() in typeDefs:
            myTypeRef = ref
            break
        if myTypeRef==None:
          for ref in node.getReferences():
            if ref.referenceType().browseName() == "HasSubtype" and ref.isForward() == False:
              myTypeRef = ref
              break
        if myTypeRef==None:
          logger.warn(str(self) + " failed to locate a type definition, assuming BaseDataType.")
          code.append("       // No valid typeDefinition found; assuming BaseDataType")
          code.append("       UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE),")
        else:
          code.append("       " + getCreateExpandedNodeIDMacro(myTypeRef.target()) + ",")
          while myTypeRef in unPrintedReferences:
            unPrintedReferences.remove(myTypeRef)

        code.append("       UA_LOCALIZEDTEXT(\"\",\"" + str(node.inverseName()) + "\"),");
        code.append("       // FIXME: Missing, isAbstract")
        code.append("       // FIXME: Missing, symmetric")
        return code

    if node.isAbstract():
        code.append(node.getCodePrintableID() + "->isAbstract = true;")
    if node.symmetric():
        code.append(node.getCodePrintableID() + "->symmetric  = true;")
    if node.__reference_inverseName__ != "":
        code.append(node.getCodePrintableID() + "->inverseName  = UA_LOCALIZEDTEXT_ALLOC(\"en_US\", \"" + \
                    node.__reference_inverseName__ + "\");")
    return code;

def ObjectNode_printOpen62541CCode_Subtype(node, unPrintedReferences=[], bootstrapping = True):
    code = []

    # Detect if this is bootstrapping or if we are attempting to use userspace...
    if bootstrapping == False:
      typeDefs = self.getNamespace().getSubTypesOf() # defaults to TypeDefinition
      myTypeRef = None
      for ref in self.getReferences():
        if ref.referenceType() in typeDefs:
          myTypeRef = ref
          break
      if myTypeRef==None:
        for ref in self.getReferences():
          if ref.referenceType().browseName() == "HasSubtype" and ref.isForward() == False:
            myTypeRef = ref
            break
      if myTypeRef==None:
        logger.warn(str(self) + " failed to locate a type definition, assuming BaseObjectType.")
        code.append("       // No valid typeDefinition found; assuming BaseObjectType")
        code.append("       UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),")
      else:
        code.append("       " + getCreateExpandedNodeIDMacro(myTypeRef.target()) + ",")
        while myTypeRef in unPrintedReferences:
          unPrintedReferences.remove(myTypeRef)

      #FIXME: No event notifier in UA_Server_addNode call!
      return code

    # We are being bootstrapped! Add the raw attributes to the node.
    code.append(node.getCodePrintableID() + "->eventNotifier = (UA_Byte) " + str(node.eventNotifier()) + ";")
    return code


def ObjectTypeNode_printOpen62541CCode_Subtype(node, unPrintedReferences=[], bootstrapping = True):
    code = []

    # Detect if this is bootstrapping or if we are attempting to use userspace...
    if bootstrapping == False:
      typeDefs = node.getNamespace().getSubTypesOf() # defaults to TypeDefinition
      myTypeRef = None
      for ref in node.getReferences():
        if ref.referenceType() in typeDefs:
          myTypeRef = ref
          break
      if myTypeRef==None:
        for ref in node.getReferences():
          if ref.referenceType().browseName() == "HasSubtype" and ref.isForward() == False:
            myTypeRef = ref
            break
      if myTypeRef==None:
        logger.warn(str(node) + " failed to locate a type definition, assuming BaseObjectType.")
        code.append("       // No valid typeDefinition found; assuming BaseObjectType")
        code.append("       UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),")
      else:
        code.append("       " + getCreateExpandedNodeIDMacro(myTypeRef.target()) + ",")
        while myTypeRef in unPrintedReferences:
          code.append("       // removed " + str(myTypeRef))
          unPrintedReferences.remove(myTypeRef)

      if (node.isAbstract()):
        code.append("       true,")
      else:
        code.append("       false,")

    # Fallback mode for bootstrapping
    if (node.isAbstract()):
      code.append(node.getCodePrintableID() + "->isAbstract = true;")

    return code

def VariableNode_printOpen62541CCode_Subtype(node, unPrintedReferences=[], bootstrapping = True):
    code = []

    # Detect if this is bootstrapping or if we are attempting to use userspace...
    if bootstrapping == False:
      code.append("       " + node.getCodePrintableID() + "_variant, ")
      code.append("       // FIXME: missing minimumSamplingInterval")
      code.append("       // FIXME: missing accessLevel")
      code.append("       // FIXME: missing userAccessLevel")
      code.append("       // FIXME: missing valueRank")
      return code

    if node.historizing():
      code.append(node.getCodePrintableID() + "->historizing = true;")

    code.append(node.getCodePrintableID() + "->minimumSamplingInterval = (UA_Double) " + \
                str(node.minimumSamplingInterval()) + ";")
    code.append(node.getCodePrintableID() + "->userAccessLevel = (UA_Int32) " + str(node.userAccessLevel()) + ";")
    code.append(node.getCodePrintableID() + "->accessLevel = (UA_Int32) " + str(node.accessLevel()) + ";")
    code.append(node.getCodePrintableID() + "->valueRank = (UA_Int32) " + str(node.valueRank()) + ";")
    # The variant is guaranteed to exist by SubtypeEarly()
    code.append(node.getCodePrintableID() + "->value.variant.value = *" + node.getCodePrintableID() + "_variant;")
    code.append(node.getCodePrintableID() + "->valueSource = UA_VALUESOURCE_VARIANT;")
    return code

def VariableTypeNode_printOpen62541CCode_Subtype(node, unPrintedReferences=[], bootstrapping = True):
    code = []
    if bootstrapping == False:
      code.append("       " + node.getCodePrintableID() + "_variant, ")
      code.append("       " + str(node.valueRank()) + ",")
      if node.isAbstract():
        code.append("       true,")
      else:
        code.append("       false,")
      return code

    if (node.isAbstract()):
      code.append(node.getCodePrintableID() + "->isAbstract = true;")
    else:
      code.append(node.getCodePrintableID() + "->isAbstract = false;")

    # The variant is guaranteed to exist by SubtypeEarly()
    code.append(node.getCodePrintableID() + "->value.variant.value = *" + node.getCodePrintableID() + "_variant;")
    code.append(node.getCodePrintableID() + "->valueSource = UA_VALUESOURCE_VARIANT;")
    return code

def MethodNode_printOpen62541CCode_Subtype(node, unPrintedReferences=[], bootstrapping = True):
    code = []

    # Detect if this is bootstrapping or if we are attempting to use userspace...
    if bootstrapping == False:
      code.append("       // Note: in/outputArguments are added by attaching the variable nodes,")
      code.append("       //       not by including the in the addMethodNode() call.")
      code.append("       NULL,")
      code.append("       NULL,")
      code.append("       0, NULL,")
      code.append("       0, NULL,")
      code.append("       // FIXME: Missing executable")
      code.append("       // FIXME: Missing userExecutable")
      return code

    # UA_False is default for booleans on _init()
    if node.executable():
      code.append(node.getCodePrintableID() + "->executable = true;")
    if node.userExecutable():
      code.append(node.getCodePrintableID() + "->userExecutable = true;")
    return code

def DataTypeNode_printOpen62541CCode_Subtype(node, unPrintedReferences=[], bootstrapping = True):
    code = []

    # Detect if this is bootstrapping or if we are attempting to use userspace...
    if bootstrapping == False:
      typeDefs = node.getNamespace().getSubTypesOf() # defaults to TypeDefinition
      myTypeRef = None
      for ref in node.getReferences():
        if ref.referenceType() in typeDefs:
          myTypeRef = ref
          break
      if myTypeRef==None:
        for ref in node.getReferences():
          if ref.referenceType().browseName() == "HasSubtype" and ref.isForward() == False:
            myTypeRef = ref
            break
      if myTypeRef==None:
        logger.warn(str(node) + " failed to locate a type definition, assuming BaseDataType.")
        code.append("       // No valid typeDefinition found; assuming BaseDataType")
        code.append("       UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE),")
      else:
        code.append("       " + getCreateExpandedNodeIDMacro(myTypeRef.target()) + ",")
        while myTypeRef in unPrintedReferences:
          unPrintedReferences.remove(myTypeRef)

      if (node.isAbstract()):
        code.append("       true,")
      else:
        code.append("       false,")
      return code

    if (node.isAbstract()):
      code.append(node.getCodePrintableID() + "->isAbstract = true;")
    else:
      code.append(node.getCodePrintableID() + "->isAbstract = false;")
    return code

def ViewNode_printOpen62541CCode_Subtype(node, unPrintedReferences=[], bootstrapping = True):
    code = []

    # Detect if this is bootstrapping or if we are attempting to use userspace...
    if bootstrapping == False:
      typeDefs = node.getNamespace().getSubTypesOf() # defaults to TypeDefinition
      myTypeRef = None
      for ref in node.getReferences():
        if ref.referenceType() in typeDefs:
          myTypeRef = ref
          break
      if myTypeRef==None:
        for ref in node.getReferences():
          if ref.referenceType().browseName() == "HasSubtype" and ref.isForward() == False:
            myTypeRef = ref
            break
      if myTypeRef==None:
        logger.warn(str(node) + " failed to locate a type definition, assuming BaseViewType.")
        code.append("       // No valid typeDefinition found; assuming BaseViewType")
        code.append("       UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEViewTYPE),")
      else:
        code.append("       " + getCreateExpandedNodeIDMacro(myTypeRef.target()) + ",")
        while myTypeRef in unPrintedReferences:
          unPrintedReferences.remove(myTypeRef)

      code.append("       // FIXME: Missing eventNotifier")
      code.append("       // FIXME: Missing containsNoLoops")
      return code

    if node.containsNoLoops():
      code.append(node.getCodePrintableID() + "->containsNoLoops = true;")
    else:
      code.append(node.getCodePrintableID() + "->containsNoLoops = false;")
    code.append(node.getCodePrintableID() + "->eventNotifier = (UA_Byte) " + str(node.eventNotifier()) + ";")
    return code

def Node_printOpen62541CCode_Subtype(node, unPrintedReferences=[], bootstrapping = True):
    """ Appends node type specific information to the nodes  UA_Server_addNode
        or UA_NodeStore_insert calls.
    """

    if isinstance(node, opcua_node_referenceType_t):
      return ReferenceTypeNode_printOpen62541CCode_Subtype(node, unPrintedReferences, bootstrapping)
    elif isinstance(node, opcua_node_object_t):
      return ObjectNode_printOpen62541CCode_Subtype(node, unPrintedReferences, bootstrapping)
    elif isinstance(node, opcua_node_objectType_t):
      return ObjectTypeNode_printOpen62541CCode_Subtype(node, unPrintedReferences, bootstrapping)
    elif isinstance(node, opcua_node_variable_t):
      return VariableNode_printOpen62541CCode_Subtype(node, unPrintedReferences, bootstrapping)
    elif isinstance(node, opcua_node_variableType_t):
      return VariableTypeNode_printOpen62541CCode_Subtype(node, unPrintedReferences, bootstrapping)
    elif isinstance(node, opcua_node_method_t):
      return MethodNode_printOpen62541CCode_Subtype(node, unPrintedReferences, bootstrapping)
    elif isinstance(node, opcua_node_dataType_t):
      return DataTypeNode_printOpen62541CCode_Subtype(node, unPrintedReferences, bootstrapping)
    elif isinstance(node, opcua_node_view_t):
      return ViewNode_printOpen62541CCode_Subtype(node, unPrintedReferences, bootstrapping)
    
    raise Exception('Unknown node type', node)

###############
# Entry Point #
###############

def getCreateNodeNoBootstrap(node, parentNode, parentReference, unprintedNodes=[]):
    code = []
    code.append("// Node: %s, %s" % (str(node), str(node.browseName())))

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
                if r.target() != None and r.target().nodeClass() == NODE_CLASS_VARIABLE and \
                   r.target().browseName() == 'InputArguments':
                    while r.target() in unprintedNodes:
                        unprintedNodes.remove(r.target())
                    if r.target().value() != None:
                        inArgVal = r.target().value().value
                elif r.target() != None and r.target().nodeClass() == NODE_CLASS_VARIABLE and \
                     r.target().browseName() == 'OutputArguments':
                    while r.target() in unprintedNodes:
                        unprintedNodes.remove(r.target())
                    if r.target().value() != None:
                        outArgVal = r.target().value().value
        if len(inArgVal)>0:
            code.append("")
            code.append("inputArguments = (UA_Argument *) malloc(sizeof(UA_Argument) * " + \
                        str(len(inArgVal)) + ");")
            code.append("int inputArgumentCnt;")
            code.append("for (inputArgumentCnt=0; inputArgumentCnt<" + str(len(inArgVal)) + \
                        "; inputArgumentCnt++) UA_Argument_init(&inputArguments[inputArgumentCnt]); ")
            argumentCnt = 0
            for inArg in inArgVal:
                if inArg.getValueFieldByAlias("Description") != None:
                    code.append("inputArguments[" + str(argumentCnt) + "].description = UA_LOCALIZEDTEXT(\"" + \
                                str(inArg.getValueFieldByAlias("Description")[0]) + "\",\"" + \
                                str(inArg.getValueFieldByAlias("Description")[1]) + "\");")
                if inArg.getValueFieldByAlias("Name") != None:
                    code.append("inputArguments[" + str(argumentCnt) + "].name = UA_STRING(\"" + \
                                str(inArg.getValueFieldByAlias("Name")) + "\");")
                if inArg.getValueFieldByAlias("ValueRank") != None:
                    code.append("inputArguments[" + str(argumentCnt) + "].valueRank = " + \
                                str(inArg.getValueFieldByAlias("ValueRank")) + ";")
                if inArg.getValueFieldByAlias("DataType") != None:
                    code.append("inputArguments[" + str(argumentCnt) + "].dataType = " + \
                                str(getCreateNodeIDMacro(inArg.getValueFieldByAlias("DataType"))) + ";")
                #if inArg.getValueFieldByAlias("ArrayDimensions") != None:
                #  code.append("inputArguments[" + str(argumentCnt) + "].arrayDimensions = " + \
                #                  str(inArg.getValueFieldByAlias("ArrayDimensions")) + ";")
                argumentCnt += 1
        if len(outArgVal)>0:
            code.append("")
            code.append("outputArguments = (UA_Argument *) malloc(sizeof(UA_Argument) * " + \
                        str(len(outArgVal)) + ");")
            code.append("int outputArgumentCnt;")
            code.append("for (outputArgumentCnt=0; outputArgumentCnt<" + str(len(outArgVal)) + \
                        "; outputArgumentCnt++) UA_Argument_init(&outputArguments[outputArgumentCnt]); ")
            argumentCnt = 0
            for outArg in outArgVal:
                if outArg.getValueFieldByAlias("Description") != None:
                    code.append("outputArguments[" + str(argumentCnt) + "].description = UA_LOCALIZEDTEXT(\"" + \
                                str(outArg.getValueFieldByAlias("Description")[0]) + "\",\"" + \
                                str(outArg.getValueFieldByAlias("Description")[1]) + "\");")
                if outArg.getValueFieldByAlias("Name") != None:
                    code.append("outputArguments[" + str(argumentCnt) + "].name = UA_STRING(\"" + \
                                str(outArg.getValueFieldByAlias("Name")) + "\");")
                if outArg.getValueFieldByAlias("ValueRank") != None:
                    code.append("outputArguments[" + str(argumentCnt) + "].valueRank = " + \
                                str(outArg.getValueFieldByAlias("ValueRank")) + ";")
                if outArg.getValueFieldByAlias("DataType") != None:
                    code.append("outputArguments[" + str(argumentCnt) + "].dataType = " + \
                                str(getCreateNodeIDMacro(outArg.getValueFieldByAlias("DataType"))) + ";")
                #if outArg.getValueFieldByAlias("ArrayDimensions") != None:
                #  code.append("outputArguments[" + str(argumentCnt) + "].arrayDimensions = " + \
                #              str(outArg.getValueFieldByAlias("ArrayDimensions")) + ";")
                argumentCnt += 1

    # print the attributes struct
    code.append("UA_%sAttributes attr;" % nodetype)
    code.append("UA_%sAttributes_init(&attr);" %  nodetype);
    code.append("attr.displayName = UA_LOCALIZEDTEXT(\"\", \"%s\");" % node.displayName().replace("\"", "\\\""))
    code.append("attr.description = UA_LOCALIZEDTEXT(\"\", \"%s\");" % node.description().replace("\"", "\\\""))

    if nodetype == "Variable":
      code.append("attr.accessLevel = %s;"     % str(node.accessLevel()))
      code.append("attr.userAccessLevel = %s;" % str(node.userAccessLevel()))

    if nodetype in ["Variable", "VariableType"]:
      code.extend(Node_printOpen62541CCode_SubtypeEarly(node, bootstrapping = False))
    elif nodetype == "Method":
      if node.executable():
        code.append("attr.executable = true;")
      if node.userExecutable():
        code.append("attr.userExecutable = true;")

    code.append("UA_NodeId nodeId = %s;" % str(getCreateNodeIDMacro(node)))
    if nodetype in ["Object", "Variable"]:
      #due to the current API we cannot set types here since the API will
      #generate nodes with random IDs
      code.append("UA_NodeId typeDefinition = UA_NODEID_NULL;")
    code.append("UA_NodeId parentNodeId = %s;" % str(getCreateNodeIDMacro(parentNode)))
    code.append("UA_NodeId parentReferenceNodeId = %s;" % \
                str(getCreateNodeIDMacro(parentReference.referenceType())))
    extrNs = node.browseName().split(":")
    if len(extrNs) > 1:
      code.append("UA_QualifiedName nodeName = UA_QUALIFIEDNAME(%s, \"%s\");" % (str(extrNs[0]), extrNs[1]))
    else:
      code.append("UA_QualifiedName nodeName = UA_QUALIFIEDNAME(0, \"%s\");" % str(node.browseName()))

    # In case of a MethodNode: Add in|outArg struct generation here. Mandates
    # that namespace reordering was done using Djikstra (check that arguments
    # have not been printed). (@ichrispa)
    code.append("UA_Server_add%sNode(server, nodeId, parentNodeId, parentReferenceNodeId, nodeName" % nodetype)

    if nodetype in ["Object", "Variable"]:
      code.append("       , typeDefinition")
    if nodetype != "Method":
      code.append("       , attr, NULL, NULL);")
    else:
      code.append("       , attr, (UA_MethodCallback) NULL, NULL, %s, inputArguments,  %s, outputArguments, NULL);" % (str(len(inArgVal)), str(len(outArgVal))))

    #Adding a Node with typeDefinition = UA_NODEID_NULL will create a
    #HasTypeDefinition reference to BaseDataType - remove it since a real
    #Reference will be add in a later step (a single HasTypeDefinition reference
    #is assumed here) The current API does not let us specify IDs of Object's
    #subelements.
    if nodetype is "Object":
      code.append("UA_Server_deleteReference(server, nodeId, UA_NODEID_NUMERIC(0, 40), true, UA_EXPANDEDNODEID_NUMERIC(0, 58), true); //remove HasTypeDefinition refs generated by addObjectNode");
    if nodetype is "Variable":
      code.append("UA_Server_deleteReference(server, nodeId, UA_NODEID_NUMERIC(0, 40), true, UA_EXPANDEDNODEID_NUMERIC(0, 62), true); //remove HasTypeDefinition refs generated by addVariableNode");
    return code

##################
# Node Bootstrap #
##################

def getCreateNodeBootstrap(node, supressGenerationOfAttribute=[]):
    nodetype = ""
    code = []

    code.append("// Node: " + str(node) + ", " + str(node.browseName()))

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
      raise Exception('Undefined NodeClass')

    code.append("UA_" + nodetype + "Node *" + node.getCodePrintableID() + \
                " = UA_NodeStore_new" + nodetype + "Node();")
    if not "browsename" in supressGenerationOfAttribute:
      extrNs = node.browseName().split(":")
      if len(extrNs) > 1:
        code.append(node.getCodePrintableID() + "->browseName = UA_QUALIFIEDNAME_ALLOC(" + \
                    str(extrNs[0]) + ", \"" + extrNs[1] + "\");")
      else:
        code.append(node.getCodePrintableID() + "->browseName = UA_QUALIFIEDNAME_ALLOC(0, \"" + \
                    node.browseName() + "\");")
    if not "displayname" in supressGenerationOfAttribute:
      code.append(node.getCodePrintableID() + "->displayName = UA_LOCALIZEDTEXT_ALLOC(\"en_US\", \"" + \
                  node.displayName() + "\");")
    if not "description" in supressGenerationOfAttribute:
      code.append(node.getCodePrintableID() + "->description = UA_LOCALIZEDTEXT_ALLOC(\"en_US\", \"" + \
                  node.description() + "\");")

    if not "writemask" in supressGenerationOfAttribute:
        if node.__node_writeMask__ != 0:
          code.append(node.getCodePrintableID() + "->writeMask = (UA_Int32) " + \
                      str(node.__node_writeMask__) + ";")
    if not "userwritemask" in supressGenerationOfAttribute:
        if node.__node_userWriteMask__ != 0:
          code.append(node.getCodePrintableID() + "->userWriteMask = (UA_Int32) " + \
                      str(node.__node_userWriteMask__) + ";")
    if not "nodeid" in supressGenerationOfAttribute:
      if node.id().ns != 0:
        code.append(node.getCodePrintableID() + "->nodeId.namespaceIndex = " + str(node.id().ns) + ";")
      if node.id().i != None:
        code.append(node.getCodePrintableID() + "->nodeId.identifier.numeric = " + str(node.id().i) + ";")
      elif node.id().b != None:
        code.append(node.getCodePrintableID() + "->nodeId.identifierType = UA_NODEIDTYPE_BYTESTRING;")
        logger.error("ByteString IDs for nodes has not been implemented yet.")
        return []
      elif node.id().g != None:
        #<jpfr> the string is sth like { .length = 111, .data = <ptr> }
        #<jpfr> there you _may_ alloc the <ptr> on the heap
        #<jpfr> for the guid, just set it to {.data1 = 111, .data2 = 2222, ....
        code.append(node.getCodePrintableID() + "->nodeId.identifierType = UA_NODEIDTYPE_GUID;")
        logger.error("GUIDs for nodes has not been implemented yet.")
        return []
      elif node.id().s != None:
        code.append(node.getCodePrintableID() + "->nodeId.identifierType = UA_NODEIDTYPE_STRING;")
        code.append(node.getCodePrintableID() + "->nodeId.identifier.numeric = UA_STRING_ALLOC(\"" + str(node.id().i) + "\");")
      else:
        logger.error("Node ID is not numeric, bytestring, guid or string. I do not know how to create c code for that...")
        return []
    return code

def Node_printOpen62541CCode(node, unPrintedNodes=[], unPrintedReferences=[],
                             supressGenerationOfAttribute=[]):
    """ Returns a list of strings containing the C-code necessary to intialize
        this node for the open62541 OPC-UA Stack.

        Note that this function will fail if the nodeid is non-numeric, as
        there is no UA_EXPANDEDNNODEID_[STRING|GUID|BYTESTRING] macro.
    """
    code = []
    code.append("")
    code.append("do {")

    # Just to be sure...
    if not (node in unPrintedNodes):
      logger.warn(str(node) + " attempted to reprint already printed node " + str(node)+ ".")
      return []

    # If we are being passed a parent node by the namespace, use that for registering ourselves in the namespace
    # Note: getFirstParentNode will return [parentNode, referenceToChild]
    (parentNode, parentRef) = node.getFirstParentNode()
    if not (parentNode in unPrintedNodes) and (parentNode != None) and (parentRef.referenceType() != None):
      code.append("// Referencing node found and declared as parent: " + str(parentNode .id()) + "/" +
                  str(parentNode .__node_browseName__) + " using " + str(parentRef.referenceType().id()) +
                  "/" + str(parentRef.referenceType().__node_browseName__))
      code.extend(getCreateNodeNoBootstrap(node, parentNode, parentRef, unPrintedNodes))
      # Parent to child reference is added by the server, do not reprint that reference
      if parentRef in unPrintedReferences:
        unPrintedReferences.remove(parentRef)
      # the UA_Server_addNode function will use addReference which creates a
      # bidirectional reference; remove any inverse references to our parent to
      # avoid duplicate refs
      for ref in node.getReferences():
        if ref.target() == parentNode and ref.referenceType() == parentRef.referenceType() and \
           ref.isForward() == False:
          while ref in unPrintedReferences:
            unPrintedReferences.remove(ref)
    # Otherwise use the "Bootstrapping" method and we will get registered with other nodes later.
    else:
      code.extend(Node_printOpen62541CCode_SubtypeEarly(node, bootstrapping = True))
      code.extend(getCreateNodeBootstrap(node, supressGenerationOfAttribute))
      code.extend(Node_printOpen62541CCode_Subtype(node, unPrintedReferences = unPrintedReferences,
                                                   bootstrapping = True))
      code.append("// Parent node does not exist yet. This node will be bootstrapped and linked later.")
      code.append("UA_RCU_LOCK();")
      code.append("UA_NodeStore_insert(server->nodestore, (UA_Node*) " + node.getCodePrintableID() + ");")
      code.append("UA_RCU_UNLOCK();")

    # Try to print all references to nodes that already exist
    # Note: we know the reference types exist, because the namespace class made sure they were
    #       the first ones being printed
    tmprefs = []
    for r in node.getReferences():
      #logger.debug("Checking if reference from " + str(r.parent()) + "can be created...")
      if not (r.target() in unPrintedNodes):
        if r in unPrintedReferences:
          if (len(tmprefs) == 0):
            code.append("// This node has the following references that can be created:")
          code.extend(getCreateStandaloneReference(node, r))
          tmprefs.append(r)
    # Remove printed refs from list
    for r in tmprefs:
      unPrintedReferences.remove(r)

    # Again, but this time check if other nodes deffered their node creation
    # because this node did not exist...
    tmprefs = []
    for r in unPrintedReferences:
      #logger.debug("Checking if another reference " + str(r.target()) + "can be created...")
      if (r.target() == node) and not (r.parent() in unPrintedNodes):
        if not isinstance(r.parent(), opcua_node_t):
          logger.debug("Reference has no parent!")
        elif not isinstance(r.parent().id(), opcua_node_id_t):
          logger.debug("Parents nodeid is not a nodeID!")
        else:
          if (len(tmprefs) == 0):
            code.append("//  Creating this node has resolved the following open references:")
          code.extend(getCreateStandaloneReference(r.parent(), r))
          tmprefs.append(r)
    # Remove printed refs from list
    for r in tmprefs:
      unPrintedReferences.remove(r)

    # Again, just to be sure...
    if node in unPrintedNodes:
      # This is necessery to make printing work at all!
      unPrintedNodes.remove(node)

    code.append("} while(0);")
    return code

def Node_printOpen62541CCode_HL_API(node, reference, supressGenerationOfAttribute=[]):
    """ Returns a list of strings containing the C-code necessary to intialize
        this node for the open62541 OPC-UA Stack using only the high level API

        Note that this function will fail if the nodeid is non-numeric, as
        there is no UA_EXPANDEDNNODEID_[STRING|GUID|BYTESTRING] macro.
    """
    code = []
    code.append("")
    code.append("do {")
    # If we are being passed a parent node by the namespace, use that for registering ourselves in the namespace
    # Note: getFirstParentNode will return [parentNode, referenceToChild]
    parentNode = reference.target()
    parentRefType = reference.referenceType()
    code.append("// Referencing node found and declared as parent: " + str(parentNode .id()) + "/" +
                  str(parentNode .__node_browseName__) + " using " + str(parentRefType.id()) +
                  "/" + str(parentRefType.__node_browseName__))
    code.extend(getCreateNodeNoBootstrap(node, parentNode, reference))
    code.append("} while(0);")
    return code
