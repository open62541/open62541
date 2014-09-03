from __future__ import print_function

def setup():
	config = dict()
	config["pluginType"] = "structuredObject";
	config["tagName"] = "UA_VariableNode";
	return config

def structInsert(element, fc, fh):
	print("\tUA_Open62541Data Open62541Data;", end='\n', file=fh)
	return

def initInsert(element, fc, fh):
    print("\tUA_Open62541Data_init(&(p->Open62541Data));", end='\n', file=fc)
    return

def calcSizeBinaryInsert(element, fc, fh):
    print("\tif(ptr==UA_NULL) return sizeof(UA_"+element.get("Name")+")-sizeof(UA_Open62541Data);", end='\n', file=fc)
    return
