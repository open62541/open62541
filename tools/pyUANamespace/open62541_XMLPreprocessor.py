#!/usr/bin/env/python
# -*- coding: utf-8 -*-

###
### Author:  Chris Iatrou (ichrispa@core-vector.net)
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

import logging
from ua_constants import *
import tempfile
import xml.dom.minidom as dom
import os
import string
from collections import Counter
import re

from ua_namespace import opcua_node_id_t


logger = logging.getLogger(__name__)

class preProcessDocument:
  originXML = '' # Original XML passed to the preprocessor
  targetXML = () # tuple of (fileHandle, fileName)
  nodeset   = '' # Parsed DOM XML object
  parseOK   = False;
  containedNodes  = [] # contains tuples of (opcua_node_id_t, xmlelement)
  referencedNodes = [] # contains tuples of (opcua_node_id_t, xmlelement)
  namespaceOrder  = [] # contains xmlns:sX attributed as tuples (int ns, string name)
  namespaceQualifiers = []      # contains all xmlns:XYZ qualifiers that might prefix value aliases (like "<uax:Int32>")
  referencedNamesSpaceUris = [] # contains <NamespaceUris> URI elements

  def __init__(self, originXML):
    self.originXML = originXML
    self.targetXML = tempfile.mkstemp(prefix=os.path.basename(originXML)+"_preProcessed-" ,suffix=".xml")
    self.parseOK   = True
    self.containedNodes  = []
    self.referencedNodes = []
    self.namespaceOrder  = []
    self.referencedNamesSpaceUris = []
    self.namespaceQualifiers = []
    try:
      self.nodeset = dom.parse(originXML)
      if len(self.nodeset.getElementsByTagName("UANodeSet")) == 0 or len(self.nodeset.getElementsByTagName("UANodeSet")) > 1:
        logger.error(self, "Document " + self.targetXML[1] + " contains no or more then 1 nodeset", LOG_LEVEL_ERROR)
        self.parseOK   = False
    except:
      self.parseOK   = False
    logger.debug("Adding new document to be preprocessed " + os.path.basename(originXML) + " as " + self.targetXML[1])

  def clean(self):
    #os.close(self.targetXML[0]) Don't -> done to flush() after finalize()
    os.remove(self.targetXML[1])

  def getTargetXMLName(self):
    if (self.parseOK):
      return self.targetXML[1]
    return None

  def extractNamespaceURIs(self):
    """ extractNamespaceURIs

        minidom gobbles up <NamespaceUris></NamespaceUris> elements, without a decent
        way to reliably access this dom2 <uri></uri> elements (only attribute xmlns= are
        accessible using minidom).  We need them for dereferencing though... This
        function attempts to do just that.

        returns: Nothing
    """
    infile = open(self.originXML)
    foundURIs = False
    nsline = ""
    line = infile.readline()
    for line in infile:
      if "<namespaceuris>" in line.lower():
        foundURIs = True
      elif "</namespaceuris>" in line.lower():
        foundURIs = False
        nsline = nsline + line
        break
      if foundURIs:
        nsline = nsline + line

    if len(nsline) > 0:
      ns = dom.parseString(nsline).getElementsByTagName("NamespaceUris")
      for uri in ns[0].childNodes:
        if uri.nodeType != uri.ELEMENT_NODE:
          continue
        self.referencedNamesSpaceUris.append(uri.firstChild.data)

    infile.close()

  def analyze(self):
    """ analyze()

        analyze will gather information about the nodes and references contained in a XML File
        to facilitate later preprocessing stages that adresss XML dependency issues

        returns: No return value
    """
    nodeIds = []
    ns = self.nodeset.getElementsByTagName("UANodeSet")

    # We need to find out what the namespace calls itself and other referenced, as numeric id's are pretty
    # useless sans linked nodes. There is two information sources...
    self.extractNamespaceURIs() # From <URI>...</URI> definitions

    for key in ns[0].attributes.keys(): # from xmlns:sX attributes
      if "xmlns:" in key:  # Any key: we will be removing these qualifiers from Values later
        self.namespaceQualifiers.append(key.replace("xmlns:",""))
      if "xmlns:s" in key: # get a numeric nsId and modelname/uri
        self.namespaceOrder.append((int(key.replace("xmlns:s","")), re.sub("[A-Za-z0-9-_\.]+\.[xXsSdD]{3}$","",ns[0].getAttribute(key))))

    # Get all nodeIds contained in this XML
    for nd in ns[0].childNodes:
      if nd.nodeType != nd.ELEMENT_NODE:
        continue
      if nd.hasAttribute(u'NodeId'):
        self.containedNodes.append( (opcua_node_id_t(nd.getAttribute(u'NodeId')), nd) )
        refs = nd.getElementsByTagName(u'References')[0]
        for ref in refs.childNodes:
          if ref.nodeType == ref.ELEMENT_NODE:
            self.referencedNodes.append( (opcua_node_id_t(ref.firstChild.data), ref) )

    logger.debug("Nodes: " + str(len(self.containedNodes)) + " References: " + str(len(self.referencedNodes)))

  def getNamespaceId(self):
    """ namespaceId()

        Counts the namespace IDs in all nodes of this XML and picks the most used
        namespace as the numeric identifier of this data model.

        returns: Integer ID of the most propable/most used namespace in this XML
    """
    max = 0;
    namespaceIdGuessed = 0;
    idDict = {}

    for ndid in self.containedNodes:
      if not ndid[0].ns in idDict.keys():
        idDict[ndid[0].ns] = 1
      else:
        idDict[ndid[0].ns] = idDict[ndid[0].ns] + 1

    for entry in idDict:
      if idDict[entry] > max:
        max = idDict[entry]
        namespaceIdGuessed = entry
    #logger.debug("XML Contents are propably in namespace " + str(entry) + " (used by " + str(idDict[entry]) + " Nodes)")
    return namespaceIdGuessed

  def getReferencedNamespaceUri(self, nsId):
    """ getReferencedNamespaceUri

        returns an URL that hopefully corresponds to the nsId that was used to reference this model

        return: URI string corresponding to nsId
    """
    # Might be the more reliable method: Get the URI from the xmlns attributes (they have numers)
    if len(self.namespaceOrder) > 0:
      for el in self.namespaceOrder:
        if el[0] == nsId:
          return el[1]

    # Fallback:
    #  Some models do not have xmlns:sX attributes, but still <URI>s (usually when they only reference NS0)
    if len(self.referencedNamesSpaceUris) > 0  and len(self.referencedNamesSpaceUris) >= nsId-1:
      return self.referencedNamesSpaceUris[nsId-1]

    #Nope, not found.
    return ""

  def getNamespaceDependencies(self):
    deps = []
    for ndid in self.referencedNodes:
      if not ndid[0].ns in deps:
        deps.append(ndid[0].ns)
    return deps

  def finalize(self):
    outfile = self.targetXML[0]
    outline = self.nodeset.toxml()
    for qualifier in self.namespaceQualifiers:
      rq = qualifier+":"
      outline = outline.replace(rq, "")
    os.write(outfile, outline.encode('UTF-8'))
    os.close(outfile)

  def reassignReferencedNamespaceId(self, currentNsId, newNsId):
    """ reassignReferencedNamespaceId

        Iterates over all references in this document, find references to currentNsId and changes them to newNsId.
        NodeIds themselves are not altered.

        returns: nothing
    """
    for refNd in self.referencedNodes:
      if refNd[0].ns == currentNsId:
        refNd[1].firstChild.data = refNd[1].firstChild.data.replace("ns="+str(currentNsId), "ns="+str(newNsId))
        refNd[0].ns = newNsId
        refNd[0].toString()

  def reassignNamespaceId(self, currentNsId, newNsId):
    """ reassignNamespaceId

        Iterates over all nodes in this document, find those in namespace currentNsId and changes them to newNsId.

        returns: nothing
    """

    #change ids in aliases
    ns = self.nodeset.getElementsByTagName("Alias")
    for al in ns:
      if al.nodeType == al.ELEMENT_NODE:
        if al.hasAttribute("Alias"):
          al.firstChild.data = al.firstChild.data.replace("ns=" + str(currentNsId), "ns=" + str(newNsId))

    logger.debug("Migrating nodes /w ns index " + str(currentNsId) + " to " + str(newNsId))
    for nd in self.containedNodes:
      if nd[0].ns == currentNsId:
        # In our own document, update any references to this node
        for refNd in self.referencedNodes:
          if refNd[0].ns == currentNsId and refNd[0] == nd[0]:
            refNd[1].firstChild.data = refNd[1].firstChild.data.replace("ns="+str(currentNsId), "ns="+str(newNsId))
            refNd[0].ns = newNsId
            refNd[0].toString()
        nd[1].setAttribute(u'NodeId', nd[1].getAttribute(u'NodeId').replace("ns="+str(currentNsId), "ns="+str(newNsId)))
        nd[0].ns = newNsId
        nd[0].toString()

class open62541_XMLPreprocessor:
  preProcDocuments = []

  def __init__(self):
      self.preProcDocuments = []

  def addDocument(self, documentPath):
    self.preProcDocuments.append(preProcessDocument(documentPath))

  def removePreprocessedFiles(self):
    for doc in self.preProcDocuments:
      doc.clean()

  def getPreProcessedFiles(self):
    files = []
    for doc in self.preProcDocuments:
      if (doc.parseOK):
        files.append(doc.getTargetXMLName())
    return files

  def testModelCongruencyAgainstReferences(self, doc, refs):
    """ testModelCongruencyAgainstReferences

        Counts how many of the nodes referencef in refs can be found in the model
        doc.

        returns: double corresponding to the percentage of hits
    """
    sspace = len(refs)
    if sspace == 0:
      return float(0)
    found   = 0
    for ref in refs:
      for n in doc.containedNodes:
        if str(ref) == str(n[0]):
          print(ref, n[0])
          found = found + 1
          break
    return float(found)/float(sspace)

  def preprocess_assignUniqueNsIds(self):
    nsdep  = []
    docLst = []
    # Search for namespace 0('s) - plural possible if user is overwriting NS0 defaults
    # Remove them from the list of namespaces, zero does not get demangled
    for doc in self.preProcDocuments:
      if doc.getNamespaceId() == 0:
        docLst.append(doc)
    for doc in docLst:
      self.preProcDocuments.remove(doc)

    # Reassign namespace id's to be in ascending order
    nsidx = 1 # next namespace id to assign on collision (first one will be "2")
    for doc in self.preProcDocuments:
      nsidx = nsidx + 1
      nsid = doc.getNamespaceId()
      doc.reassignNamespaceId(nsid, nsidx)
      docLst.append(doc)
      logger.info("Document " + doc.originXML + " is now namespace " + str(nsidx))
    self.preProcDocuments = docLst

  def getUsedNamespaceArrayNames(self):
    """ getUsedNamespaceArrayNames

        Returns the XML xmlns:s1 or <URI>[0] of each XML document (if contained/possible)

        returns: dict of int:nsId -> string:url
    """
    nsName = {}
    for doc in self.preProcDocuments:
      uri = doc.getReferencedNamespaceUri(1)
      if uri == None:
        uri = "http://modeluri.not/retrievable/from/xml"
      nsName[doc.getNamespaceId()] = doc.getReferencedNamespaceUri(1)
    return nsName

  def preprocess_linkDependantModels(self):
    revertToStochastic = [] # (doc, int id), where id was not resolvable using model URIs

    # Attemp to identify the model relations by using model URIs in xmlns:sX or <URI> contents
    for doc in self.preProcDocuments:
      nsid = doc.getNamespaceId()
      dependencies = doc.getNamespaceDependencies()
      for d in dependencies:
        if d != nsid and d != 0:
          # Attempt to identify the namespace URI this d referes to...
          nsUri = doc.getReferencedNamespaceUri(d) # FIXME: This could actually fail and return ""!
          logger.info("Need a namespace referenced as " + str(d) + ". Which hopefully is " + nsUri)
          targetDoc = None
          for tgt in self.preProcDocuments:
            # That model, whose URI is known but its current id is not, will
            #   refer have referred to itself as "1"
            if tgt.getReferencedNamespaceUri(1) == nsUri:
              targetDoc = tgt
              break
          if not targetDoc == None:
            # Found the model... relink the references
            doc.reassignReferencedNamespaceId(d, targetDoc.getNamespaceId())
            continue
          else:
            revertToStochastic.append((doc, d))
            logger.warn("Failed to reliably identify which XML/Model " + os.path.basename(doc.originXML) + " calls ns=" +str(d))

    for (doc, d) in revertToStochastic:
      logger.warn("Attempting to find stochastic match for target namespace ns=" + str(d) + " of " + os.path.basename(doc.originXML))
      # Copy all references to the given namespace
      refs = []
      matches = [] # list of (match%, targetDoc) to pick from later
      for ref in doc.referencedNodes:
        if ref[0].ns == d:
          refs.append(opcua_node_id_t(str(ref[0])))
      for tDoc in self.preProcDocuments:
        tDocId = tDoc.getNamespaceId()
        # Scenario: If these references did target this documents namespace...
        for r in refs:
          r.ns = tDocId
          r.toString()
        # ... how many of them would be found!?
        c = self.testModelCongruencyAgainstReferences(tDoc, refs)
        print(c)
        if c>0:
          matches.append((c, tDoc))
      best = (0, None)
      for m in matches:
        print(m[0])
        if m[0] > best[0]:
          best = m
      if best[1] != None:
        logger.warn("Best match (" + str(best[1]*100) + "%) for what " + os.path.basename(doc.originXML) + " refers to as ns="+str(d)+" was " + os.path.basename(best[1].originXML))
        doc.reassignReferencedNamespaceId(d, best[1].getNamespaceId())
      else:
        logger.error("Failed to find a match for what " +  os.path.basename(doc.originXML) + " refers to as ns=" + str(d))

  def preprocessAll(self):
    ##
    ## First: Gather statistics about the namespaces:
    for doc in self.preProcDocuments:
      doc.analyze()

    # Preprocess step: Remove XML specific Naming scheme ("uax:")
    # FIXME: Not implemented

    ##
    ## Preprocess step: Check namespace ID multiplicity and reassign IDs if necessary
    ##
    self.preprocess_assignUniqueNsIds()
    self.preprocess_linkDependantModels()


    ##
    ## Prep step: prevent any XML from using namespace 1 (reserved for instances)
    ## FIXME: Not implemented

    ##
    ## Final: Write modified XML tmp files
    for doc in self.preProcDocuments:
      doc.finalize()

    return True
