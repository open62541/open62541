from ua_nodeset import *

def getSubTypesOf(nodeset, tdNodes = None, currentNode = None, hasSubtypeRefNode = None):
    # If this is a toplevel call, collect the following information as defaults
    if tdNodes == None:
      tdNodes = []
    if currentNode == None:
      currentNode = nodeset.getNodeByBrowseName("HasTypeDefinition")
      tdNodes.append(currentNode)
      if len(tdNodes) < 1:
        return []
    if hasSubtypeRefNode == None:
      hasSubtypeRefNode = nodeset.getNodeByBrowseName("HasSubtype")
      if hasSubtypeRefNode == None:
        return tdNodes

    # collect all subtypes of this node
    for ref in currentNode.references():
      if ref.isForward and ref.referenceType.id == hasSubtypeRefNode.id:
        tdNodes.append(ref.target)
        nodeset.getTypeDefinitionNodes(tdNodes=tdNodes, currentNode = ref.target,
                                    hasSubtypeRefNode=hasSubtypeRefNode)

    return tdNodes


def NodePrintDot(self):
    cleanname = "node_" + str(self.id).replace(";","").replace("=","")
    dot = cleanname + " [label = \"{" + str(self.id) + "|" + str(self.browseName) + \
                                               "}\", shape=\"record\"]"
    for r in self.references:
      if isinstance(r.target, Node):
        tgtname = "node_" + str(r.target.id).replace(";","").replace("=","")
        dot = dot + "\n"
        if r.isForward == True:
          dot = dot + cleanname + " -> " + tgtname + " [label=\"" + \
                  str(r.referenceType.browseName) + "\"]\n"
        else:
          if len(r.referenceType.inverseName) == 0:
            logger.warn("Inverse name of reference is null " + str(r.referenceType.id))
          dot = dot + cleanname + " -> " + tgtname + " [label=\"" + str(r.referenceType.inverseName) + "\"]\n"
    return dot


def printDotGraphWalk(nodeset, depth=1, filename="out.dot", rootNode=None,
                        followInverse = False, excludeNodeIds=[]):
    """ Outputs a graphiz/dot description the nodes centered around rootNode.

        References beginning from rootNode will be followed for depth steps. If
        "followInverse = True" is passed, then inverse (not Forward) references
        will also be followed.

        Nodes can be excluded from the graph by passing a list of NodeIds as
        string representation using excludeNodeIds (ex ["i=53", "ns=2;i=453"]).

        Output is written into filename to be parsed by dot/neato/srfp...
    """
    iter = depth
    processed = []
    if rootNode == None or not isinstance(rootNode, Node) or not rootNode in nodeset.nodes:
      root = nodeset.getRoot()
    else:
      root = rootNode

    file=open(filename, 'w+')

    if root == None:
      return

    file.write("digraph ns {\n")
    file.write(root.NodePrintDot())
    refs=[]
    if followInverse == True:
      refs = root.references; # + root.getInverseReferences()
    else:
      for ref in root.references:
        if ref.isForward():
          refs.append(ref)
    while iter > 0:
      tmp = []
      for ref in refs:
        if isinstance(ref.target(), Node):
          tgt = ref.target()
          if not str(tgt.id()) in excludeNodeIds:
            if not tgt in processed:
              file.write(tgt.NodePrintDot())
              processed.append(tgt)
              if ref.isForward() == False and followInverse == True:
                tmp = tmp + tgt.references; # + tgt.getInverseReferences()
              elif ref.isForward() == True :
                tmp = tmp + tgt.references;
      refs = tmp
      iter = iter - 1

    file.write("}\n")
    file.close()

class testing:
  def __init__(self):
    self.ns = NodeSet("testing")

    logger.debug("Phase 1: Reading XML file nodessets")
    self.ns.parseXML("Opc.Ua.NodeSet2.xml")
    #self.ns.parseXML("Opc.Ua.NodeSet2.Part4.xml")
    #self.ns.parseXML("Opc.Ua.NodeSet2.Part5.xml")
    #self.ns.parseXML("Opc.Ua.SimulationNodeSet2.xml")

    logger.debug("Phase 2: Linking address space references and datatypes")
    self.ns.linkOpenPointers()
    self.ns.sanitize()

    logger.debug("Phase 3: Comprehending DataType encoding rules")
    self.ns.buildEncodingRules()

    logger.debug("Phase 4: Allocating variable value data")
    self.ns.allocateVariables()

    bin = self.ns.buildBinary()
    f = open("binary.base64","w+")
    f.write(bin.encode("base64"))
    f.close()

    allnodes = self.ns.nodes;
    ns = [self.ns.getRoot()]

    i = 0
    #print "Starting depth search on " + str(len(allnodes)) + " nodes starting with from " + str(ns)
    while (len(ns) < len(allnodes)):
      i = i + 1;
      tmp = [];
      print("Iteration: " + str(i))
      for n in ns:
        tmp.append(n)
        for r in n.getReferences():
          if (not r.target() in tmp):
           tmp.append(r.target())
      print("...tmp, " + str(len(tmp)) + " nodes discovered")
      ns = []
      for n in tmp:
        ns.append(n)
      print("...done, " + str(len(ns)) + " nodes discovered")

class testing_open62541_header:
  def __init__(self):
    self.ns = opcua_ns("testing")

    logger.debug("Phase 1: Reading XML file nodessets")
    self.ns.parseXML("Opc.Ua.NodeSet2.xml")
    #self.ns.parseXML("Opc.Ua.NodeSet2.Part4.xml")
    #self.ns.parseXML("Opc.Ua.NodeSet2.Part5.xml")
    #self.ns.parseXML("Opc.Ua.SimulationNodeSet2.xml")

    logger.debug("Phase 2: Linking address space references and datatypes")
    self.ns.linkOpenPointers()
    self.ns.sanitize()

    logger.debug("Phase 3: Calling C Printers")
    code = self.ns.printOpen62541Header()

    codeout = open("./open62541_nodeset.c", "w+")
    for line in code:
      codeout.write(line + "\n")
    codeout.close()
    return

# Call testing routine if invoked standalone.
# For better debugging, it is advised to import this file using an interactive
# python shell and instantiating a nodeset.
#
# import ua_types.py as ua; ns=ua.testing().nodeset
if __name__ == '__main__':
  tst = testing_open62541_header()
