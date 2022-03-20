#!/usr/bin/env python3

# Load into gdb with 'source <path-to>/tools/gdb-prettyprint.py'
# Make sure to have 'set print pretty on' to get nice structure printouts

import base64
import gdb

def findType(name):
    tt = None
    try:
        tt = gdb.lookup_type("UA_" + name)
    except Exception:
        try:
            tt = gdb.lookup_type(name)
        except Exception:
            pass
    return tt

class String:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        data = self.val['data']
        if int(data) == 0:
            return "UA_STRING_NULL"
        length = int(self.val['length'])
        inferior = gdb.selected_inferior()
        return "\"%s\"" % inferior.read_memory(data, length).tobytes().decode(errors='replace')

class ByteString:
    def __init__(self, val):
        self.val = val

    @staticmethod
    def print(s):
        length = int(s['length'])
        data = s['data']
        if int(data) == 0:
            return "UA_BYTESTRING_NULL"
        inferior = gdb.selected_inferior()
        encoded = base64.b64encode(inferior.read_memory(data, length).tobytes())
        return "\"%s\"" % encoded.decode(errors='replace')

    def to_string(self):
        data = self.val['data']
        if int(data) == 0:
            return "UA_BYTESTRING_NULL"
        return "UA_ByteString(%s)" % ByteString.print(self.val)

class LocalizedText:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "UA_LocalizedText(%s, %s)" % (self.val['locale'], self.val['text'])

class QualifiedName:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "UA_QualifiedName(%i, %s)" % (self.val['namespaceIndex'], self.val['name'])

class Guid:
    def __init__(self, val):
        self.val = val

    @staticmethod
    def print(g):
        data = (g['data1'], g['data2'], g['data3'], g['data4'][0],
                g['data4'][1], g['data4'][2], g['data4'][3], g['data4'][4],
                g['data4'][5], g['data4'][6], g['data4'][7])
        return "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x" % data

    def to_string(self):
        return "UA_Guid(%s)" % Guid.print(self.val)

class NodeId:
    def __init__(self, val):
        self.val = val

    @staticmethod
    def print(val):
        s = ""
        ns = int(val['namespaceIndex'])
        idType = int(val['identifierType'])
        if ns > 0:
            s += "ns=%i;" % ns
        if idType < 3:
            s += "i=%i" % val['identifier']['numeric']
        elif idType == 3:
            s += "s=%s" % val['identifier']['string']
        elif idType == 4:
            s += "g=%s" % Guid.print(val['identifier']['guid'])
        elif idType == 5:
            s += "b=%s" % ByteString.print(val['identifier']['byteString'])
        else:
            raise gdb.GdbError("Invalid NodeId Identifier Type")
        return s

    def to_string(self):
        return "UA_NodeId(%s)" % NodeId.print(self.val)

class ExpandedNodeId:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        s = ""
        if self.val['serverIndex'] > 0:
            s += "svr=%i;" % self.val['serverIndex']
        if self.val['namespaceUri']['data'] > 0:
            s += "nsu=%s;" % self.val['namespaceUri']
        s += NodeId.print(self.val['nodeId'])
        return "UA_ExpandedNodeId(%s)" % s

class ExtensionObject:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        encoding = self.val['encoding']
        content = self.val['content']
        if encoding == 0:
            return "UA_ExtensionObject()"
        elif encoding == 1 or encoding == 2:
            encoded = content['encoded']
            return "UA_ExtensionObject(%s, %s)" % (encoded['typeId'], encoded['body'])
        decoded = content['decoded']
        if int(decoded['type']) == 0:
            return "UA_ExtensionObject()"
        datatype = decoded['type'].dereference()
        typeName = datatype['typeName'].string()
        tt = findType(typeName)
        if not tt:
            return "UA_ExtensionObject<%s>((void*) %s)" % (typeName, decoded['data'])
        value = decoded['data'].cast(tt.pointer()).dereference()
        return "UA_ExtensionObject<%s>(%s)" % (tt, value)

class Variant:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        if int(self.val['type']) == 0:
            return "UA_Variant()"
        datatype = self.val['type'].dereference()
        tt = findType(datatype['typeName'].string())
        if not tt:
            return "UA_Variant()"
        data_ptr = int(self.val['data'])
        array_length = int(self.val['arrayLength'])
        content = None
        if array_length == 0 and data_ptr > 1:
            content = self.val['data'].cast(tt.pointer()).dereference()
        else:
            content = self.val['data'].cast(tt.array(array_length - 1).pointer()).dereference()
        dims_length = int(self.val['arrayDimensionsSize'])
        if dims_length == 0:
            return "UA_Variant<%s>(%s)" % (tt, content)
        dims_type = findType("UInt32").array(dims_length - 1).pointer()
        dims = self.val['arrayDimensions'].cast(dims_type).dereference()
        return "UA_Variant<%s[%i]>(%s, arrayDimensions = %s)" % (tt, array_length, content, dims)

def build_open62541_printer():
   pp = gdb.printing.RegexpCollectionPrettyPrinter("open62541")
   pp.add_printer('UA_String', '^UA_String$', String)
   pp.add_printer('UA_ByteString', '^UA_ByteString$', ByteString)
   pp.add_printer('UA_LocalizedText', '^UA_LocalizedText$', LocalizedText)
   pp.add_printer('UA_QualifiedName', '^UA_QualifiedName$', QualifiedName)
   pp.add_printer('UA_Guid', '^UA_Guid$', Guid)
   pp.add_printer('UA_NodeId', '^UA_NodeId$', NodeId)
   pp.add_printer('UA_ExpandedNodeId', '^UA_ExpandedNodeId$', ExpandedNodeId)
   pp.add_printer('UA_ExtensionObject', '^UA_ExtensionObject$', ExtensionObject)
   pp.add_printer('UA_Variant', '^UA_Variant$', Variant)
   return pp

gdb.printing.register_pretty_printer(gdb.current_objfile(),
                                     build_open62541_printer())
