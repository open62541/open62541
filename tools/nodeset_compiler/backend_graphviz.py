from nodeset import *
import graphviz as gv
import codecs

def nodePrintDot(node):
    cleanname = "node_" + str(node.id).replace(";", "").replace("=", "")
    dot = cleanname + " [label = \"{" + str(node.id) + "|" + str(node.browseName) + \
          "}\", shape=\"record\"]"
    for r in node.references:
        if isinstance(r.target, Node):
            tgtname = "node_" + str(r.target.id).replace(";", "").replace("=", "")
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
                      followInverse=False, excludeNodeIds=[]):
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
    if rootNode is None or not isinstance(rootNode, Node) or not rootNode in nodeset.nodes:
        root = nodeset.getRoot()
    else:
        root = rootNode

    file = codecs.open(filename, 'w+', encoding='utf-8')

    if root is None:
        return

    file.write("digraph ns {\n")
    file.write(nodePrintDot(root))
    refs = []
    if followInverse == True:
        refs = root.references  # + root.getInverseReferences()
    else:
        for ref in root.references:
            if ref.isForward:
                refs.append(ref)
    while iter > 0:
        tmp = []
        for ref in refs:
            if isinstance(ref.target, NodeId):
                tgt = nodeset.nodes[ref.target]
                if not str(tgt.id) in excludeNodeIds:
                    if not tgt in processed:
                        file.write(nodePrintDot(tgt))
                        processed.append(tgt)
                        if ref.isForward is False and followInverse is True:
                            for inverseRef in tgt.inverseReferences:
                                refs.append(inverseRef)
                            tmp = tmp + tgt.references  # + tgt.getInverseReferences()
                        elif ref.isForward:
                            for targetRef in tgt.references:
                                refs.append(targetRef)
        refs = tmp
        iter = iter - 1

    file.write("}\n")
    file.close()

def getNodeString(node):
    return node.browseName.name + " (" + str(node.id) + ")"

def getReferenceString(nodeset, ref):
    refNode = nodeset.nodes[ref.referenceType]
    return refNode.browseName.name

def getNodeStyle(node):
    if isinstance(node, ReferenceTypeNode):
        return {'shape': 'box', 'style': 'filled', 'fillcolor': '1', 'colorscheme': "pastel19"}
    if isinstance(node, VariableTypeNode):
        return {'shape': 'box', 'style': 'filled', 'fillcolor': '2', 'colorscheme': "pastel19"}
    if isinstance(node, ObjectTypeNode):
        return {'shape': 'box', 'style': 'filled', 'fillcolor': '3', 'colorscheme': "pastel19"}
    if isinstance(node, DataTypeNode):
        return {'shape': 'box', 'style': 'filled', 'fillcolor': '4', 'colorscheme': "pastel19"}
    if isinstance(node, VariableNode):
        return {'shape': 'ellipse', 'style': 'rounded,filled', 'fillcolor': '5', 'colorscheme': "pastel19"}
    if isinstance(node, ObjectNode):
        return {'shape': 'box', 'style': 'rounded,filled', 'fillcolor': '6', 'colorscheme': "pastel19"}
    if isinstance(node, MethodNode):
        return {'shape': 'box', 'style': 'rounded,filled', 'fillcolor': '7', 'colorscheme': "pastel19"}
    if isinstance(node, ViewNode):
        return {'shape': 'box', 'style': 'rounded,filled', 'fillcolor': '8', 'colorscheme': "pastel19"}

def add_edges(graph, edges):
    for e in edges:
        if isinstance(e[0], tuple):
            graph.edge(*e[0], **e[1])
        else:
            graph.edge(*e)
    return graph

def add_nodes(graph, nodes):
    for n in nodes:
        if isinstance(n, tuple):
            graph.node(n[0], **n[1])
        else:
            graph.node(n)
    return graph

def addReferenceToGraph(nodeset, nodeFrom, nodeTo, reference, graph):
    add_edges(graph, [((getNodeString(nodeFrom), getNodeString(nodeTo)), {'label': getReferenceString(nodeset, reference)})])


def addNodeToGraph(nodeset, node, graph, alreadyAdded=set(), relevantReferences=set(), ignoreNodes=set(), isRoot=False, depth = 0):
    if node.id in alreadyAdded or node.id in ignoreNodes:
        return
    alreadyAdded.add(node.id)
    add_nodes(graph, [(getNodeString(node), getNodeStyle(node))])
    for ref in node.references:
        if ref.referenceType in relevantReferences and ref.isForward:
            targetNode = nodeset.nodes[ref.target]
            if targetNode.id in ignoreNodes:
                continue
            addNodeToGraph(nodeset, targetNode, graph, alreadyAdded, depth=depth+1, relevantReferences=relevantReferences,
                           ignoreNodes = ignoreNodes)
            addReferenceToGraph(nodeset, node, targetNode, ref, graph)


def generateGraphvizCode(nodeset, filename="dependencies", rootNode=None, excludeNodeIds=[]):
    if rootNode is None or not isinstance(rootNode, Node) or not rootNode in nodeset.nodes:
        root = nodeset.getRoot()
    else:
        root = rootNode

    if root is None:
        return

    g = gv.dot.Digraph(name="NodeSet Dependency", format='pdf', )

    alreadyAdded = set()
    ignoreNodes = set()
    # Ignore some nodes since almost all nodes will point to that which messes up the graph
    ignoreNodes.add(NodeId("i=68")) # PropertyType
    ignoreNodes.add(NodeId("i=63")) # BaseDataVariableType
    ignoreNodes.add(NodeId("i=61")) # FolderType
    addNodeToGraph(nodeset, root, g, alreadyAdded, isRoot=True,
                   relevantReferences=nodeset.getRelevantOrderingReferences(),
                   ignoreNodes=ignoreNodes)

    g.render(filename)
