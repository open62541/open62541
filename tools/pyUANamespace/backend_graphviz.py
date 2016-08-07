from nodeset import *

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
          dot = dot + cleanname + " -> " + tgtname + \
                " [label=\"" + str(r.referenceType.inverseName) + "\"]\n"
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
        if ref.isForward:
          refs.append(ref)
    while iter > 0:
      tmp = []
      for ref in refs:
        if isinstance(ref.target, Node):
          tgt = ref.target
          if not str(tgt.id) in excludeNodeIds:
            if not tgt in processed:
              file.write(tgt.NodePrintDot())
              processed.append(tgt)
              if ref.isForward == False and followInverse == True:
                tmp = tmp + tgt.references; # + tgt.getInverseReferences()
              elif ref.isForward == True :
                tmp = tmp + tgt.references;
      refs = tmp
      iter = iter - 1

    file.write("}\n")
    file.close()
