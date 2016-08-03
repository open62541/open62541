from ua_namespace import *

class testing:
  def __init__(self):
    self.namespace = opcua_namespace("testing")

    logger.debug("Phase 1: Reading XML file nodessets")
    self.namespace.parseXML("Opc.Ua.NodeSet2.xml")
    #self.namespace.parseXML("Opc.Ua.NodeSet2.Part4.xml")
    #self.namespace.parseXML("Opc.Ua.NodeSet2.Part5.xml")
    #self.namespace.parseXML("Opc.Ua.SimulationNodeSet2.xml")

    logger.debug("Phase 2: Linking address space references and datatypes")
    self.namespace.linkOpenPointers()
    self.namespace.sanitize()

    logger.debug("Phase 3: Comprehending DataType encoding rules")
    self.namespace.buildEncodingRules()

    logger.debug("Phase 4: Allocating variable value data")
    self.namespace.allocateVariables()

    bin = self.namespace.buildBinary()
    f = open("binary.base64","w+")
    f.write(bin.encode("base64"))
    f.close()

    allnodes = self.namespace.nodes;
    ns = [self.namespace.getRoot()]

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
    self.namespace = opcua_namespace("testing")

    logger.debug("Phase 1: Reading XML file nodessets")
    self.namespace.parseXML("Opc.Ua.NodeSet2.xml")
    #self.namespace.parseXML("Opc.Ua.NodeSet2.Part4.xml")
    #self.namespace.parseXML("Opc.Ua.NodeSet2.Part5.xml")
    #self.namespace.parseXML("Opc.Ua.SimulationNodeSet2.xml")

    logger.debug("Phase 2: Linking address space references and datatypes")
    self.namespace.linkOpenPointers()
    self.namespace.sanitize()

    logger.debug("Phase 3: Calling C Printers")
    code = self.namespace.printOpen62541Header()

    codeout = open("./open62541_namespace.c", "w+")
    for line in code:
      codeout.write(line + "\n")
    codeout.close()
    return

# Call testing routine if invoked standalone.
# For better debugging, it is advised to import this file using an interactive
# python shell and instantiating a namespace.
#
# import ua_types.py as ua; ns=ua.testing().namespace
if __name__ == '__main__':
  tst = testing_open62541_header()
