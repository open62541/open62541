#!/usr/bin/env python

# WARNING: This is till work in progress
#
# Load into gdb with 'source ../tools/gdb-prettyprint.py'
# Make sure to also apply 'set print pretty on' to get nice structure printouts

class String:
    def __init__(self, val):
        self.val = val

    def to_string (self):
        length = int(self.val['length'])
        data = self.val['data']

        if int(data) == 0:
            return "UA_STRING_NULL"
        inferior = gdb.selected_inferior()
        text = inferior.read_memory(data, length).tobytes().decode(errors='replace')
        return "\"%s\"" % text

class LocalizedText:
    def __init__(self, val):
        self.val = val

    def to_string (self):
        return "UA_LocalizedText(%s, %s)" % (self.val['locale'], self.val['text'])

class QualifiedName:
    def __init__(self, val):
        self.val = val

    def to_string (self):
        return "UA_QualifiedName(%s, %s)" % (int(self.val['namespaceIndex']), self.val['name'])

class Guid:
    def __init__(self, val):
        self.val = val

    def to_string (self):
        return "UA_Guid()"

class NodeId:
    def __init__(self, val):
        self.val = val

    def to_string (self):
        return "UA_NodeId()"

class Variant:
    def __init__(self, val):
        self.val = val

    def to_string (self):
        return "UA_Variant()"

def lookup_type (val):
    if str(val.type) == 'UA_String':
        return String(val)
    if str(val.type) == 'UA_LocalizedText':
        return LocalizedText(val)
    if str(val.type) == 'UA_QualifiedName':
        return QualifiedName(val)
    if str(val.type) == 'UA_Guid':
        return Guid(val)
    if str(val.type) == 'UA_NodeId':
        return NodeId(val)
    if str(val.type) == 'UA_Variant':
        return Variant(val)
    return None

gdb.pretty_printers.append (lookup_type)
