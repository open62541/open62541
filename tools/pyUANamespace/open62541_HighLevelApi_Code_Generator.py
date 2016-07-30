import logging
from open62541_MacroHelper import open62541_MacroHelper


logger = logging.getLogger(__name__)

logger.setLevel(logging.INFO)

class CodeGenerator():
    
    def __init__(self, namespaceIdentifiers):
        self.namespaceIdentifiers = namespaceIdentifiers
        

    def printOpen62541CodeHighLevelApi(self, name, nodes, printedExternally=[]):
        logger.info("name: %s", name)
        unPrintedNodes = []
        unPrintedRefs = []
        code = []
        header = []
    
        # Some macros (UA_EXPANDEDNODEID_MACRO()...) are easily created, but
        # bulky. This class will help to offload some code.
        codegen = open62541_MacroHelper()
    
        # Populate the unPrinted-Lists with everything we have.
        for n in nodes:
            if not n in printedExternally:
                unPrintedNodes.append(n)
            else:
                logger.debug("Node " + str(n.id()) + " is being ignored.")
        for n in unPrintedNodes:
            for r in n.getReferences():
                unPrintedRefs.append(r)
    
        logger.debug("%d nodes and %d references need to get printed.", len(unPrintedNodes), len(unPrintedRefs))
        header.append("/* WARNING: This is a generated file.\n * Any manual changes will be overwritten.\n\n */")
        code.append("/* WARNING: This is a generated file.\n * Any manual changes will be overwritten.\n\n */")
    
        header.append("#ifndef %s_H_" % name.upper())
        header.append("#define %s_H_" % name.upper())
        header.append('#ifdef UA_NO_AMALGAMATION')
        header.append('#include "ua_types.h"')
        header.append('#include "ua_job.h"')
        header.append('#include "ua_server.h"')
        header.append('#else')
        header.append('#include "open62541.h"')
        header.append('#define NULL ((void *)0)')
        header.append('#endif')
    
        code.append("#include \"%s.h\"" % name)
        code.append("UA_INLINE void %s (UA_Server *server) {" % name)
    
        # Before printing nodes, we need to request additional namespace arrays from the server
        for nsid in self.namespaceIdentifiers:
            if nsid == 0 or nsid == 1:
                continue
            else:
                name_ns = self.namespaceIdentifiers[nsid]
                name_ns = name_ns.replace("\"", "\\\"")
                code.append("UA_Server_addNamespace(server, \"%s\");" % name_ns)
    
        logger.debug("Collecting all nodeids and define them in the header")
        for n in unPrintedNodes:
            header.append(codegen.getNodeIdDefineString(n))
    
        already_printed = list(printedExternally)
        while unPrintedNodes:
            node_found = False
            for node in unPrintedNodes:
                for ref in node.getReferences():
                    if ref.referenceType() in already_printed and ref.target() in already_printed:
                        node_found = True
                        code.append("\n do {")
                        code.extend(codegen.getCreateNodeNoBootstrap(node, ref.target(), ref))
                        code.append("} while(0);")
                        unPrintedRefs.remove(ref)
                        unPrintedNodes.remove(node)
                        already_printed.append(node)
                        break
            if not node_found:
                logger.critical("no complete code generation with high level API possible; not all nodes will be created")
                code.append(
                    "CRITICAL: no complete code generation with high level API possible; not all nodes will be created")
                break
        code.append("// creating references")
        for r in unPrintedRefs:
            code.extend(codegen.getCreateStandaloneReference(r.parent(), r))
    
        # finalizing source and header
        header.append("extern void %s (UA_Server *server);\n" % name)
        header.append("#endif /* %s_H_ */" % name.upper())
        code.append("} // closing nodeset()")
        return (header, code)
     