from __future__ import print_function

def setup():
	config = dict()
	config["pluginType"] = "structuredObject";
	config["tagName"] = "UA_VariableNode";
	return config

#UA_Int32 readCallback(const UA_NodeId* nodeid, UA_Int32 typeId, const void* value);
#UA_Int32 writeCallback(const UA_NodeId* nodeid, UA_Int32 typeId, const void const * value);
def structInsert(element, fc, fh):
	print("\tUA_Int32(*NONSTANDARDreadCallback)(const UA_NodeId* nodeid, UA_Int32 typeId, const UA_Variant* value);", end='\n', file=fh)
	print("\tUA_Int32(*NONSTANDARDwriteCallback)(const UA_NodeId* nodeid, UA_Int32 typeId, const UA_Variant* value);", end='\n', file=fh)
	return

def initInsert(element, fc, fh):
    print("\tp->NONSTANDARDreadCallback = UA_NULL;", end='\n', file=fc)
    print("\tp->NONSTANDARDwriteCallback = UA_NULL;", end='\n', file=fc)
    return

def calcSizeBinaryInsert(element, fc, fh):
    print("\tif(ptr==UA_NULL) return sizeof(UA_"+element.get("Name")+")-2*sizeof(void*);", end='\n', file=fc)
    return