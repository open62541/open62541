#!/usr/bin/env python3

### This Source Code Form is subject to the terms of the Mozilla Public
### License, v. 2.0. If a copy of the MPL was not distributed with this
### file, You can obtain one at http://mozilla.org/MPL/2.0/.

###    Copyright 2014-2015 (c) TU-Dresden (Author: Chris Iatrou)
###    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
###    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH

import codecs
from nodeset import *

class testing:
    def __init__(self):
        self.ns = NodeSet()

        logger.debug("Phase 1: Reading XML file nodessets")
        self.ns.parseXML("Opc.Ua.NodeSet2.xml")
        # self.ns.parseXML("Opc.Ua.NodeSet2.Part4.xml")
        # self.ns.parseXML("Opc.Ua.NodeSet2.Part5.xml")
        # self.ns.parseXML("Opc.Ua.SimulationNodeSet2.xml")

        logger.debug("Phase 2: Linking address space references and datatypes")
        self.ns.linkOpenPointers()
        self.ns.sanitize()

        logger.debug("Phase 3: Allocating variable value data")
        self.ns.allocateVariables()

        bin = self.ns.buildBinary()
        f = codecs.open("binary.base64", "w+", encoding='utf-8')
        f.write(bin.encode("base64"))
        f.close()

        allnodes = self.ns.nodes
        ns = [self.ns.getRoot()]

        i = 0
        # print "Starting depth search on " + str(len(allnodes)) + " nodes starting
        # with from " + str(ns)
        while (len(ns) < len(allnodes)):
            i = i + 1
            tmp = []
            print("Iteration: " + str(i))
            for n in ns:
                tmp.append(n)
                for r in n.getReferences():
                    if r.target() not in tmp:
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
        # self.ns.parseXML("Opc.Ua.NodeSet2.Part4.xml")
        # self.ns.parseXML("Opc.Ua.NodeSet2.Part5.xml")
        # self.ns.parseXML("Opc.Ua.SimulationNodeSet2.xml")

        logger.debug("Phase 2: Linking address space references and datatypes")
        self.ns.linkOpenPointers()
        self.ns.sanitize()

        logger.debug("Phase 3: Calling C Printers")
        code = self.ns.printOpen62541Header()

        codeout = codecs.open("./open62541_nodeset.c", "w+", encoding='utf-8')
        for line in code:
            codeout.write(line + "\n")
        codeout.close()
        return

if __name__ == '__main__':
    tst = testing_open62541_header()
