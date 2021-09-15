import logging
import argparse
import sys
from datatypes import NodeId
from lxml import etree
from nodeset import NodeSet
import nodes

# Parse the arguments
parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument('-e', '--existing',
                    metavar="<existingNodeSetXML>",
                    type=argparse.FileType('rb'),
                    dest="existing",
                    action='append',
                    default=[],
                    help='NodeSet XML files with nodes that are already present on the server.')

parser.add_argument('-x', '--xml',
                    metavar="<nodeSetXML>",
                    type=argparse.FileType('rb'),
                    action='append',
                    dest="infiles",
                    default=[],
                    help='NodeSet XML files with nodes that dependencies shall be resolved for.')

parser.add_argument('-r', '--ref',
                    metavar="<referenceNodeSetXML>",
                    type=argparse.FileType('rb'),
                    dest="ref",
                    default=None,
                    help='NodeSet XML file where missing dependencies are resolved from.')

parser.add_argument('-p', '--pull',
                    action='store_true',
                    dest='pull',
                    default=False,
                    help='Pull in and output missing Nodes from reference XML')

parser.add_argument('-m', '--merge',
                    action='store_true',
                    dest='merge',
                    default=False,
                    help='Merge missing Nodes from reference NodeSet into (first) existing-NodeSet')

parser.add_argument('-v', '--verbose', action='count',
                    default=1,
                    help='Make the script more verbose. Can be applied up to 4 times')



args = parser.parse_args()

# Set up logging
# Leave logging output on sys.stderr so that we can output XML data on stdout
logging.basicConfig(stream=sys.stderr)
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
verbosity = 0
if args.verbose:
    verbosity = int(args.verbose)
if verbosity == 1:
    logging.basicConfig(level=logging.ERROR)
elif verbosity == 2:
    logging.basicConfig(level=logging.WARNING)
elif verbosity == 3:
    logging.basicConfig(level=logging.INFO)
elif verbosity >= 4:
    logging.basicConfig(level=logging.DEBUG)
else:
    logging.basicConfig(level=logging.CRITICAL)

# Create a new nodeset. The nodeset name is not significant.
# Parse the XML files
ns = NodeSet()
nsCount = 0
loadedFiles = list()

for xmlfile in args.existing:
    if xmlfile.name in loadedFiles:
        logger.info("Skipping Nodeset since it is already loaded: {} ".format(xmlfile.name))
        continue
    loadedFiles.append(xmlfile.name)
    logger.info("Preprocessing (existing) " + str(xmlfile.name))
    ns.addNodeSet(xmlfile, True)
    nsCount +=1
for xmlfile in args.infiles:
    if xmlfile.name in loadedFiles:
        logger.info("Skipping Nodeset since it is already loaded: {} ".format(xmlfile.name))
        continue
    loadedFiles.append(xmlfile.name)
    logger.info("Preprocessing " + str(xmlfile.name))
    ns.addNodeSet(xmlfile)
    nsCount +=1

for n in ns.nodes.values():
    if n.sanitize() == False:
        raise Exception("Failed to sanitize node " + str(n))

# Function for walking a tree of nodeIds in the nodeSet and recording a list
# of nodes encountered during the walk
def walkNodes(nodeSet, nodeIds, nodeList=[]):
    for nodeId in nodeIds:
        if nodeId not in nodeSet.nodes:
            # Can not follow unresolved dependency
            continue

        n = nodeSet.nodes[nodeId]
        candidateNodes = []

        # Gather candidateNodes from various node attributes
        if type(n) == nodes.DataTypeNode and n.__isEnum__ == False and n.__isOptionSet__ == False:
            # DataType contains other DataType fields. __definition__ is list of (Name, DataTypeNode) tuples
            for definition in n.__definition__:
                candidateNodes.append(definition[1].id)

        if type(n) == nodes.VariableNode or type(n) == nodes.VariableTypeNode:
            if n.dataType is not None:
                candidateNodes.append(n.dataType)

        for ref in n.references:
            if not ref.source == n.id: raise Exception("Reference " + str(ref) + " has an invalid source")
            candidateNodes.append(ref.referenceType)
            candidateNodes.append(ref.target)

        # Uniquify candidateNodes and exclude nodes already present in nodeList
        candidateNodes = list(set(candidateNodes) - set(nodeList))

        # Add remaining candidateNodes to nodeList
        nodeList.extend(candidateNodes)

        # Inquire candidateNodes recursively if there are candidates left
        if len(candidateNodes) > 0:
            walkNodes(nodeSet, candidateNodes, nodeList)

    return nodeList

# Function for printing a set of nodeIds as Xml by filtering the referenceXml
# file. If existingXml is given, the generated Xml is merged with it
def printXML(nodeIds, referenceXml, existingXml=None):
    referenceRoot = etree.parse(referenceXml.name).getroot()

    # Filter XML for required nodeIds
    for node in referenceRoot:
        if ('NodeId' not in node.attrib) or NodeId(node.attrib['NodeId']) not in nodeIds:
            # This node is not required
            referenceRoot.remove(node)

    if existingXml is not None:
        # Merge with existing Xml
        existingRoot = etree.parse(existingXml.name).getroot()

        for node in referenceRoot:
            existingRoot.append(node)

        print(etree.tostring(existingRoot).decode('utf-8'))
    else:
        print(etree.tostring(referenceRoot).decode('utf-8'))

logger.info("Collecting missing nodes...".format(xmlfile.name))
usedNodes = walkNodes(ns, ns.nodes)
missingNodes = [node for node in usedNodes if node not in ns.nodes]
logger.info("Collected {} missing nodes out of {} used nodes".format(len(missingNodes), len(usedNodes)))

# Load reference nodeset if given on command line
if args.ref is not None:
    referenceNodeSet = NodeSet()

    for xmlfile in [args.ref]:
        if xmlfile.name in loadedFiles:
            logger.info("Skipping Nodeset since it is already loaded: {} ".format(xmlfile.name))
            continue
        loadedFiles.append(xmlfile.name)
        logger.info("Preprocessing (reference) " + str(xmlfile.name))
        referenceNodeSet.addNodeSet(xmlfile, True, typesArray="UA_TYPES")

    logger.info("Resolving all dependencies from {}...".format(xmlfile.name))

    # Walk entire tree to find all dependencies
    dependentNodes = walkNodes(referenceNodeSet, missingNodes)

    # Remove dependencies already satisfied in existing NodeSet
    requiredNodes = [node for node in dependentNodes if node not in ns.nodes]
    unresolvedNodes = [node for node in requiredNodes if node not in referenceNodeSet.nodes]

    logger.info("Resolved {} required nodes out of {} dependent nodes ({} unresolved)"
        .format(len(requiredNodes), len(dependentNodes), len(unresolvedNodes)))

    if args.pull:
        # Run printXML when pull is specified. This will gather the requiredNodes
        # from the reference file and output it as XML or merge it into the
        # (first) existing XML file (if merge is specified)
        if args.merge and len(args.existing) > 0:
            logger.info("Pulling in required nodes from {} and merge with {}...".format(args.ref.name, args.existing[0].name))
            printXML(requiredNodes, args.ref, args.existing[0])
        else:
            logger.info("Pulling in required nodes from {}...".format(args.ref.name))
            printXML(requiredNodes, args.ref)
    else:
        print(requiredNodes)
else:
    print(missingNodes)
