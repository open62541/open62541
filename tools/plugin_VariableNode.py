from __future__ import print_function

def setup():
	config = dict()
	config["pluginType"] = "structuredObject";
	config["tagName"] = "UA_VariableNode";
	return config

def structInsert(element, fc, fh):
	print("\t//I will not be seen on the wire, I will not be freed by _destroy", end='\n', file=fh)
	print("\tUA_Open62541Data* Open62541Data;", end='\n', file=fh)
	return

def initInsert(element, fc, fh):
    print("\tp->Open62541Data = UA_NULL;", end='\n', file=fc)
    return

def calcSizeBinaryInsert(element, fc, fh):
    print("\tif(ptr==UA_NULL) return sizeof(UA_"+element.get("Name")+")-sizeof(void*);", end='\n', file=fc)
    return
