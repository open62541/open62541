from __future__ import print_function
import sys
from collections import OrderedDict
import re
from lxml import etree

if len(sys.argv) != 3:
    print("Usage: python generate_builtin.py <path/to/Opc.Ua.Types.bsd> <outfile w/o extension>", file=sys.stdout)
    exit(0)

# types that are coded manually 
exclude_types = set(["Boolean", "SByte", "Byte", "Int16", "UInt16", "Int32", "UInt32",
                    "Int64", "UInt64", "Float", "Double", "String", "DateTime", "Guid",
                    "ByteString", "XmlElement", "NodeId", "ExpandedNodeId", "StatusCode", 
                    "QualifiedName", "LocalizedText", "ExtensionObject", "DataValue",
                     "Variant", "DiagnosticInfo", "IntegerId"])

elementary_size = dict()
elementary_size["Boolean"] = 1;
elementary_size["SByte"] = 1;
elementary_size["Byte"] = 1;
elementary_size["Int16"] = 2;
elementary_size["UInt16"] = 2;
elementary_size["Int32"] = 4;
elementary_size["UInt32"] = 4;
elementary_size["Int64"] = 8;
elementary_size["UInt64"] = 8;
elementary_size["Float"] = 4;
elementary_size["Double"] = 8;
elementary_size["DateTime"] = 8;
elementary_size["StatusCode"] = 4;

# indefinite_types = ["NodeId", "ExpandedNodeId", "QualifiedName", "LocalizedText", "ExtensionObject", "DataValue", "Variant", "DiagnosticInfo"]
indefinite_types = ["ExpandedNodeId", "QualifiedName", "ExtensionObject", "DataValue", "Variant", "DiagnosticInfo"]
enum_types = []
structured_types = []
                   
# indefinite types cannot be directly contained in a record as they don't have a definite size
printed_types = exclude_types # types that were already printed and which we can use in the structures to come

# types we do not want to autogenerate
def skipType(name):
    if name in exclude_types:
        return True
    if re.search("NodeId$", name) != None:
        return True
    return False

def stripTypename(tn):
    return tn[tn.find(":")+1:]

def camlCase2AdaCase(item):
    (newitem, n) = re.subn("(?<!^)(?<![A-Z])([A-Z])", "_\\1", item)
    return newitem
    
def camlCase2CCase(item):
    if item in ["Float","Double"]:
        return "my" + item
    return item[:1].lower() + item[1:] if item else ''

# are the prerequisites in place? if not, postpone.
def printableStructuredType(element):
    for child in element:
        if child.tag == "{http://opcfoundation.org/BinarySchema/}Field":
            typename = stripTypename(child.get("TypeName"))
            if typename not in printed_types:
                return False
    return True

# There three types of types in the bsd file:
# StructuredType, EnumeratedType OpaqueType

def createEnumerated(element):
    valuemap = OrderedDict()
    name = "UA_" + element.get("Name")
    enum_types.append(name)
    print("\n/*** " + name + " ***/", end='\n', file=fh)
    for child in element:
        if child.tag == "{http://opcfoundation.org/BinarySchema/}Documentation":
            print("/* " + child.text + " */", end='\n', file=fh)
        if child.tag == "{http://opcfoundation.org/BinarySchema/}EnumeratedValue":
            valuemap[name + "_" + child.get("Name")] = child.get("Value")
    valuemap = OrderedDict(sorted(valuemap.iteritems(), key=lambda (k,v): int(v)))
    print("typedef UA_UInt32 " + name + ";", end='\n', file=fh);
    print("enum " + name + "_enum { \n\t" + ",\n\t".join(map(lambda (key, value) : key.upper() + " = " + value, valuemap.iteritems())) + "\n};", end='\n', file=fh)
    print("UA_TYPE_METHOD_PROTOTYPES (" + name + ")", end='\n', file=fh)
    print("UA_TYPE_METHOD_CALCSIZE_AS("+name+", UA_UInt32)", end='\n', file=fc)
    print("UA_TYPE_METHOD_ENCODE_AS("+name+", UA_UInt32)", end='\n', file=fc)
    print("UA_TYPE_METHOD_DECODE_AS("+name+", UA_UInt32)", end='\n', file=fc)
    print("UA_TYPE_METHOD_DELETE_AS("+name+", UA_UInt32)", end='\n', file=fc)
    print("UA_TYPE_METHOD_DELETEMEMBERS_AS("+name+", UA_UInt32)", end='\n', file=fc)
    print("UA_TYPE_METHOD_INIT_AS("+name+", UA_UInt32)", end='\n', file=fc)
    print("UA_TYPE_METHOD_NEW_DEFAULT("+name+")\n", end='\n', file=fc)
    
    return
    
def createStructured(element):
    valuemap = OrderedDict()
    name = "UA_" + element.get("Name")
    print("\n/*** " + name + " ***/", end='\n', file=fh)

    lengthfields = set()
    for child in element:
        if child.get("LengthField"):
            lengthfields.add(child.get("LengthField"))
    
    for child in element:
        if child.tag == "{http://opcfoundation.org/BinarySchema/}Documentation":
            print("/* " + child.text + " */", end='\n', file=fh)
        elif child.tag == "{http://opcfoundation.org/BinarySchema/}Field":
            if child.get("Name") in lengthfields:
                continue
            childname = camlCase2CCase(child.get("Name"))
            #if childname in printed_types:
            #    childname = childname + "_Value" # attributes may not have the name of a type
            typename = stripTypename(child.get("TypeName"))
            if typename in structured_types:
                valuemap[childname] = typename + "*"
                if child.get("LengthField"):
                    valuemap[childname] = typename + "**"
            elif typename in indefinite_types:
                valuemap[childname] = typename + "*"
                if child.get("LengthField"):
                    valuemap[childname] = typename + "**"
            elif child.get("LengthField"):
                valuemap[childname] = typename + "**"
            else:
                valuemap[childname] = typename

    # if "Response" in name[len(name)-9:]:
    #    print("type " + name + " is new Response_Base with "),
    # elif "Request" in name[len(name)-9:]:
    #    print ("type " + name + " is new Request_Base with "),
    # else:
    #    print ("type " + name + " is new UA_Builtin with "),
    print("typedef struct T_" + name + " {", end='\n', file=fh)
    if len(valuemap) > 0:
        for n,t in valuemap.iteritems():
            if t.find("**") != -1:
	        print("\t" + "UA_Int32 " + n + "Size;", end='\n', file=fh)
            print("\t" + "UA_" + t + " " + n + ";", end='\n', file=fh)
    else:
        print("\t/* null record */", end='\n', file=fh)
        print("\tUA_Int32 NullRecord; /* avoiding warnings */", end='\n', file=fh)
    print("} " + name + ";", end='\n', file=fh)

    print("UA_Int32 " + name + "_calcSize(" + name + " const * ptr);", end='\n', file=fh)
    print("UA_Int32 " + name + "_encode(" + name + " const * src, UA_Int32* pos, UA_Byte* dst);", end='\n', file=fh)
    print("UA_Int32 " + name + "_decode(UA_Byte const * src, UA_Int32* pos, " + name + "* dst);", end='\n', file=fh)
    print("UA_Int32 " + name + "_delete("+ name + "* p);", end='\n', file=fh)
    print("UA_Int32 " + name + "_deleteMembers(" + name + "* p);", end='\n', file=fh)
    print("UA_Int32 " + name + "_init("+ name + " * p);", end='\n', file=fh)
    print("UA_Int32 " + name + "_new(" + name + " ** p);", end='\n', file=fh)

    print("UA_Int32 "  + name + "_calcSize(" + name + " const * ptr) {", end='', file=fc)
    print("\n\tif(ptr==UA_NULL){return sizeof("+ name +");}", end='', file=fc)
    print("\n\treturn 0", end='', file=fc)

    # code _calcSize
    for n,t in valuemap.iteritems():
        if t in elementary_size:
            print('\n\t + sizeof(UA_' + t + ") // " + n, end='', file=fc)
        else:
            if t in enum_types:
                print('\n\t + 4 //' + n, end='', file=fc) # enums are all 32 bit
            elif t.find("**") != -1:
		print("\n\t + 0 //" + n + "Size is included in UA_Array_calcSize", end='', file=fc),
		print("\n\t + UA_Array_calcSize(ptr->" + n + "Size, UA_" + t[0:t.find("*")].upper() + ", (void const**) ptr->" + n +")", end='', file=fc)
            elif t.find("*") != -1:
                print('\n\t + ' + "UA_" + t[0:t.find("*")] + "_calcSize(ptr->" + n + ')', end='', file=fc)
            else:
                print('\n\t + ' + "UA_" + t + "_calcSize(&(ptr->" + n + '))', end='', file=fc)

    print("\n\t;\n}\n", end='\n', file=fc)

    print("UA_Int32 "+name+"_encode("+name+" const * src, UA_Int32* pos, UA_Byte* dst) {\n\tUA_Int32 retval = UA_SUCCESS;", end='\n', file=fc)
    # code _encode
    for n,t in valuemap.iteritems():
        if t in elementary_size:
            print('\tretval |= UA_'+t+'_encode(&(src->'+n+'),pos,dst);', end='\n', file=fc)
        else:
            if t in enum_types:
                print('\tretval |= UA_'+t+'_encode(&(src->'+n+'),pos,dst);', end='\n', file=fc)
            elif t.find("**") != -1:
                print('\t//retval |= UA_Int32_encode(&(src->'+n+'Size),pos,dst); // encode size managed by UA_Array_encode', end='\n', file=fc)
		print("\tretval |= UA_Array_encode((void const**) (src->"+n+"),src->"+n+"Size, UA_" + t[0:t.find("*")].upper()+",pos,dst);", end='\n', file=fc)
            elif t.find("*") != -1:
                print('\tretval |= UA_' + t[0:t.find("*")] + "_encode(src->" + n + ',pos,dst);', end='\n', file=fc)
            else:
                print('\tretval |= UA_'+t+"_encode(&(src->"+n+"),pos,dst);", end='\n', file=fc)
    print("\treturn retval;\n}\n", end='\n', file=fc)

    # code _decode
    print("UA_Int32 "+name+"_decode(UA_Byte const * src, UA_Int32* pos, " + name + "* dst) {\n\tUA_Int32 retval = UA_SUCCESS;", end='\n', file=fc)
    for n,t in valuemap.iteritems():
        if t in elementary_size:
            print('\tretval |= UA_'+t+'_decode(src,pos,&(dst->'+n+'));', end='\n', file=fc)
        else:
            if t in enum_types:
                print('\tretval |= UA_'+t+'_decode(src,pos,&(dst->'+n+'));', end='\n', file=fc)
            elif t.find("**") != -1:
				print('\tretval |= UA_Int32_decode(src,pos,&(dst->'+n+'Size)); // decode size', end='\n', file=fc)
				#allocate memory for array
				print('\tretval |= UA_alloc((void**)&(dst->' + n + "),dst->" + n + "Size*sizeof(void*));", end='\n', file=fc)
				print("\tretval |= UA_Array_decode(src,dst->"+n+"Size, UA_" + t[0:t.find("*")].upper()+",pos,(void const**) (dst->"+n+"));", end='\n', file=fc) #not tested
            elif t.find("*") != -1:
				#allocate memory using new
				print('\tretval |= UA_'+ t[0:t.find("*")] +"_new(&(dst->" + n + "));", end='\n', file=fc)
				print('\tretval |= UA_' + t[0:t.find("*")] + "_decode(src,pos,dst->"+ n +");", end='\n', file=fc)
            else:
                print('\tretval |= UA_'+t+"_decode(src,pos,&(dst->"+n+"));", end='\n', file=fc)
    print("\treturn retval;\n}\n", end='\n', file=fc)
    
    # code _delete and _deleteMembers
    print('UA_Int32 '+name+'_delete('+name+'''* p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= '''+name+'''_deleteMembers(p);
	retval |= UA_free(p);
	return retval;
    }''', end='\n', file=fc)
    
    print("UA_Int32 "+name+"_deleteMembers(" + name + "* p) {\n\tUA_Int32 retval = UA_SUCCESS;", end='\n', file=fc)
    for n,t in valuemap.iteritems():
        if t not in elementary_size:
            if t.find("**") != -1:
		print("\tretval |= UA_Array_delete((void**)p->"+n+",p->"+n+"Size);", end='\n', file=fc) #not tested
            elif t.find("*") != -1:
		print('\tretval |= UA_' + t[0:t.find("*")] + "_delete(p->"+n+");", end='\n', file=fc)
            else:
		print('\tretval |= UA_' + t + "_deleteMembers(&(p->"+n+"));", end='\n', file=fc)
		
    print("\treturn retval;\n}\n", end='\n', file=fc)

    # code _init
    print("UA_Int32 "+name+"_init(" + name + " * p) {\n\tUA_Int32 retval = UA_SUCCESS;", end='\n', file=fc)
    for n,t in valuemap.iteritems():
        if t in elementary_size:
            print('\tretval |= UA_'+t+'_init(&(p->'+n+'));', end='\n', file=fc)
        else:
            if t in enum_types:
                print('\tretval |= UA_'+t+'_init(&(p->'+n+'));', end='\n', file=fc)
            elif t.find("**") != -1:
				print('\tp->'+n+'Size=0;', end='\n', file=fc)
				print("\tp->"+n+"=UA_NULL;", end='\n', file=fc)
            elif t.find("*") != -1:
				print("\tp->"+n+"=UA_NULL;", end='\n', file=fc)
            else:
                print('\tretval |= UA_'+t+"_init(&(p->"+n+"));", end='\n', file=fc)
    print("\treturn retval;\n}\n", end='\n', file=fc)

    # code _new
    print("UA_TYPE_METHOD_NEW_DEFAULT(" + name + ")", end='\n', file=fc)
        
def createOpaque(element):
    name = "UA_" + element.get("Name")
    print("\n/*** " + name + " ***/", end='\n', file=fh)
    for child in element:
        if child.tag == "{http://opcfoundation.org/BinarySchema/}Documentation":
            print("/* " + child.text + " */", end='\n', file=fh)

    print("typedef UA_ByteString " + name + ";", end='\n', file=fh)
    print("UA_TYPE_METHOD_PROTOTYPES (" + name + ")", end='\n', file=fh)
    print("UA_TYPE_METHOD_CALCSIZE_AS("+name+", UA_ByteString)", end='\n', file=fc)
    print("UA_TYPE_METHOD_ENCODE_AS("+name+", UA_ByteString)", end='\n', file=fc)
    print("UA_TYPE_METHOD_DECODE_AS("+name+", UA_ByteString)", end='\n', file=fc)
    print("UA_TYPE_METHOD_DELETE_AS("+name+", UA_ByteString)", end='\n', file=fc)
    print("UA_TYPE_METHOD_DELETEMEMBERS_AS("+name+", UA_ByteString)", end='\n', file=fc)
    print("UA_TYPE_METHOD_INIT_AS("+name+", UA_ByteString)", end='\n', file=fc)
    print("UA_TYPE_METHOD_NEW_DEFAULT("+name+")\n", end='\n', file=fc)
    return

ns = {"opc": "http://opcfoundation.org/BinarySchema/"}
tree = etree.parse(sys.argv[1])
types = tree.xpath("/opc:TypeDictionary/*[not(self::opc:Import)]", namespaces=ns)

fh = open(sys.argv[2] + ".h",'w');
fc = open(sys.argv[2] + ".c",'w');
print('#include "' + sys.argv[2] + '.h"', end='\n', file=fc);

# types for which we create a vector type
arraytypes = set()
fields = tree.xpath("//opc:Field", namespaces=ns)
for field in fields:
    if field.get("LengthField"):
        arraytypes.add(stripTypename(field.get("TypeName")))

deferred_types = OrderedDict()

print('#ifndef OPCUA_H_', end='\n', file=fh)
print('#define OPCUA_H_', end='\n', file=fh)
print('#include "opcua_basictypes.h"', end='\n', file=fh)
print('#include "opcua_namespace_0.h"', end='\n', file=fh);


#plugin handling
import os
files = [f for f in os.listdir('.') if os.path.isfile(f) and f[-3:] == ".py" and f[:7] == "plugin_"]
plugin_types = []
packageForType = OrderedDict()

for f in files:
	package = f[:-3]
	exec "import " + package
	exec "pluginSetup = " + package + ".setup()"
	if pluginSetup["pluginType"] == "structuredObject":
		plugin_types.append(pluginSetup["tagName"])
		packageForType[pluginSetup["tagName"]] = [package,pluginSetup]
		print("Custom object creation for tag " + pluginSetup["tagName"] + " imported from package " + package)
#end plugin handling

for element in types:
    name = element.get("Name")
    if skipType(name):
        continue
        
    if element.tag == "{http://opcfoundation.org/BinarySchema/}EnumeratedType":
        createEnumerated(element)
        printed_types.add(name)
    elif element.tag == "{http://opcfoundation.org/BinarySchema/}StructuredType":
        if printableStructuredType(element):
            createStructured(element)
            structured_types.append(name)
            printed_types.add(name)
        else: # the record contains types that were not yet detailed
            deferred_types[name] = element
            continue
    elif element.tag == "{http://opcfoundation.org/BinarySchema/}OpaqueType":
        createOpaque(element)
        printed_types.add(name)

    #if name in arraytypes:
    #    print "package ListOf" + name + " is new Types.Arrays.UA_Builtin_Arrays(" + name + ");\n"

for name, element in deferred_types.iteritems():
	if name in plugin_types:
		#execute plugin if registered
		exec "ret = " + packageForType[name][0]+"."+packageForType[name][1]["functionCall"]
		if ret == "default":
			createStructured(element)
	else:
		createStructured(element)
    # if name in arraytypes:
    #    print "package ListOf" + name + " is new Types.Arrays.UA_Builtin_Arrays(" + name + ");\n"

print('#endif /* OPCUA_H_ */', end='\n', file=fh)
fh.close()
fc.close()

