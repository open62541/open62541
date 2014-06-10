from __future__ import print_function
import sys
import time
import platform
import getpass
from collections import OrderedDict
import re
from lxml import etree
import inspect

if len(sys.argv) != 3:
	print("Usage: python generate_builtin.py <path/to/Opc.Ua.Types.bsd> <outfile w/o extension>", file=sys.stdout)
	exit(0)

ns = {"opc": "http://opcfoundation.org/BinarySchema/"}
tree = etree.parse(sys.argv[1])
types = tree.xpath("/opc:TypeDictionary/*[not(self::opc:Import)]", namespaces=ns)

fh = open(sys.argv[2] + ".h",'w');
fc = open(sys.argv[2] + ".c",'w');

# dirty hack. we go up the call frames to access local variables of the calling
# function. this allows to shorten code and get %()s replaces with less clutter.
def printh(string):
    print(string % inspect.currentframe().f_back.f_locals, end='\n', file=fh)

def printc(string):
    print(string % inspect.currentframe().f_back.f_locals, end='\n', file=fc)

# types that are coded manually 
existing_types = set(["Boolean", "SByte", "Byte", "Int16", "UInt16", "Int32", "UInt32",
                     "Int64", "UInt64", "Float", "Double", "String", "DateTime", "Guid",
                     "ByteString", "XmlElement", "NodeId", "ExpandedNodeId", "StatusCode", 
                     "QualifiedName", "LocalizedText", "ExtensionObject", "DataValue",
                     "Variant", "DiagnosticInfo"])

fixed_size = set(["UA_Boolean", "UA_SByte", "UA_Byte", "UA_Int16", "UA_UInt16",
                  "UA_Int32", "UA_UInt32", "UA_Int64", "UA_UInt64", "UA_Float",
                  "UA_Double", "UA_DateTime", "UA_Guid", "UA_StatusCode"])

# types we do not want to autogenerate
def skipType(name):
    if name in existing_types:
        return True
    if re.search("NodeId$", name) != None:
        return True
    return False
    
def stripTypename(tn):
	return tn[tn.find(":")+1:]

def camlCase2CCase(item):
	if item in ["Float","Double"]:
		return "my" + item
	return item[:1].lower() + item[1:] if item else ''

# are the types we need already in place? if not, postpone.
def printableStructuredType(element):
    for child in element:
        if child.tag == "{http://opcfoundation.org/BinarySchema/}Field":
            typename = stripTypename(child.get("TypeName"))
            if typename not in existing_types:
                return False
    return True

def createEnumerated(element):	
    valuemap = OrderedDict()
    name = "UA_" + element.get("Name")
    fixed_size.add(name)
    for child in element:
        if child.tag == "{http://opcfoundation.org/BinarySchema/}Documentation":
            printh("/** @brief " + child.text + " */")
        if child.tag == "{http://opcfoundation.org/BinarySchema/}EnumeratedValue":
            valuemap[name + "_" + child.get("Name")] = child.get("Value")
    valuemap = OrderedDict(sorted(valuemap.iteritems(), key=lambda (k,v): int(v)))
    printh("typedef UA_UInt32 " + name + ";")
    printh("enum " + name + "_enum { \n\t" +
           ",\n\t".join(map(lambda (key, value) : key.upper() + " = " + value, valuemap.iteritems())) +
           "\n};")
    printh("UA_TYPE_PROTOTYPES (" + name + ")")
    printh("UA_TYPE_BINARY_ENCODING(" + name + ")")
    printh("UA_TYPE_XML_ENCODING(" + name + ")\n")
    printc("UA_TYPE_AS(" + name + ", UA_UInt32)")
    printc("UA_TYPE_BINARY_ENCODING_AS(" + name + ", UA_UInt32)")
    printc('''UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(%(name)s)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(%(name)s)
UA_TYPE_METHOD_DECODEXML_NOTIMPL(%(name)s\n)''')

def createOpaque(element):
    name = "UA_" + element.get("Name")
    for child in element:
        if child.tag == "{http://opcfoundation.org/BinarySchema/}Documentation":
            printh("/** @brief " + child.text + " */")
    printh("typedef UA_ByteString %(name)s;")
    printh("UA_TYPE_PROTOTYPES(%(name)s)")
    printh("UA_TYPE_BINARY_ENCODING(%(name)s)")
    printh("UA_TYPE_XML_ENCODING(" + name + ")\n")
    printc("UA_TYPE_AS(%(name)s, UA_ByteString)")
    printc("UA_TYPE_BINARY_ENCODING_AS(%(name)s, UA_ByteString)")
    printc('''UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(%(name)s)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(%(name)s)
UA_TYPE_METHOD_DECODEXML_NOTIMPL(%(name)s)\n''')

def createStructured(element):
    name = "UA_" + element.get("Name")

    # 1) Are there arrays in the type?
    lengthfields = set()
    for child in element:
        if child.get("LengthField"):
            lengthfields.add(child.get("LengthField"))

    # 2) Store members in membermap (name->type).
    membermap = OrderedDict()
    for child in element:
        if child.tag == "{http://opcfoundation.org/BinarySchema/}Documentation":
            printh("/** @brief " + child.text + " */")
        elif child.tag == "{http://opcfoundation.org/BinarySchema/}Field":
            if child.get("Name") in lengthfields:
                continue
            childname = camlCase2CCase(child.get("Name"))
            typename = stripTypename(child.get("TypeName"))
            if child.get("LengthField"):
                membermap[childname] = "UA_" + typename + "*"
            else:
                membermap[childname] = "UA_" + typename

    # 3) Print structure
    if len(membermap) > 0:
        printh("typedef struct %(name)s {")
        for n,t in membermap.iteritems():
	    if t.find("*") != -1:
	        printh("\t" + "UA_Int32 " + n + "Size;")
            printh("\t%(t)s %(n)s;")
        printh("} %(name)s;")
    else:
        printh("typedef void* %(name)s;")
        

    # 3) function prototypes
    printh("UA_TYPE_PROTOTYPES(" + name + ")")
    printh("UA_TYPE_BINARY_ENCODING(" + name + ")")
    printh("UA_TYPE_XML_ENCODING(" + name + ")\n")

    # 4) CalcSizeBinary
    printc('''UA_Int32 %(name)s_calcSizeBinary(%(name)s const * ptr) {
    \tif(ptr==UA_NULL) return sizeof(%(name)s);
    \treturn 0''')
    has_fixed_size = True
    for n,t in membermap.iteritems():
        if t in fixed_size:
            printc('\t + sizeof(%(t)s) // %(n)s')
        elif t.find("*") != -1:
            printc('\t + UA_Array_calcSizeBinary(ptr->%(n)sSize,'+ t[0:t.find("*")].upper() + ",ptr->%(n)s)")
            has_fixed_size = False
        else:
            printc('\t + %(t)s_calcSizeBinary(&ptr->%(n)s)')
            has_fixed_size = False
    printc("\t;\n}\n")
    if has_fixed_size:
        fixed_size.add(name)

    # 5) EncodeBinary
    printc('''UA_Int32 %(name)s_encodeBinary(%(name)s const * src, UA_ByteString* dst, UA_UInt32 *offset) {
    \tUA_Int32 retval = UA_SUCCESS;''')
    for n,t in membermap.iteritems():
        if t.find("*") != -1:
            printc("\tretval |= UA_Array_encodeBinary(src->%(n)s,src->%(n)sSize," +
                   t[0:t.find("*")].upper() + ",dst,offset);")
        else:
            printc('\tretval |= %(t)s_encodeBinary(&src->%(n)s,dst,offset);')
    printc("\treturn retval;\n}\n")

    # 6) DecodeBinary
    printc('''UA_Int32 %(name)s_decodeBinary(UA_ByteString const * src, UA_UInt32 *offset, %(name)s * dst) {
    \tUA_Int32 retval = UA_SUCCESS;''')
    printc('\t'+name+'_init(dst);')
    for n,t in membermap.iteritems():
        if t.find("*") != -1:
            printc('\tCHECKED_DECODE(UA_Int32_decodeBinary(src,offset,&dst->%(n)sSize),' +
                   '%(name)s_deleteMembers(dst));')
            printc('\tCHECKED_DECODE(UA_Array_decodeBinary(src,offset,dst->%(n)sSize,' + t[0:t.find("*")].upper() +
                   ',(void**)&dst->%(n)s), %(name)s_deleteMembers(dst));')
        else:
            printc('\tCHECKED_DECODE(%(t)s_decodeBinary(src,offset,&dst->%(n)s), %(name)s_deleteMembers(dst));')
    printc("\treturn retval;\n}\n")

    # 7) Xml
    printc('''UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(%(name)s)
    UA_TYPE_METHOD_ENCODEXML_NOTIMPL(%(name)s)
    UA_TYPE_METHOD_DECODEXML_NOTIMPL(%(name)s)''')
    
    # 8) Delete
    printc('''UA_Int32 %(name)s_delete(%(name)s *p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= %(name)s_deleteMembers(p);
	retval |= UA_free(p);
	return retval;\n}\n''')
	
    # 9) DeleteMembers
    printc('''UA_Int32 %(name)s_deleteMembers(%(name)s *p) {
    \tUA_Int32 retval = UA_SUCCESS;''')
    for n,t in membermap.iteritems():
        if not t in fixed_size: # dynamic size on the wire
            if t.find("*") != -1:
		printc("\tretval |= UA_Array_delete((void*)p->%(n)s,p->%(n)sSize," +
                       t[0:t.find("*")].upper()+");")
            else:
		printc('\tretval |= %(t)s_deleteMembers(&p->%(n)s);')
    printc("\treturn retval;\n}\n")

    # 10) Init
    printc('''UA_Int32 %(name)s_init(%(name)s *p) {
    \tUA_Int32 retval = UA_SUCCESS;''')
    for n,t in membermap.iteritems():
        if t.find("*") != -1:
            printc('\tp->%(n)sSize = 0;')
            printc('\tp->%(n)s = UA_NULL;')
        else:
            printc('\tretval |= %(t)s_init(&p->%(n)s);')
    printc("\treturn retval;\n}\n")

    # 11) New
    printc("UA_TYPE_NEW_DEFAULT(%(name)s)")

    # 12) Copy
    printc('''UA_Int32 %(name)s_copy(const %(name)s *src,%(name)s *dst) {
    \tif(src == UA_NULL || dst == UA_NULL) return UA_ERROR;
    \tUA_Int32 retval = UA_SUCCESS;''')
    printc("\tmemcpy(dst, src, sizeof(%(name)s));")
    for n,t in membermap.iteritems():
        if t.find("*") != -1:
            printc('\tdst->%(n)s = src->%(n)s;')
            printc("\tretval |= UA_Array_copy(src->%(n)s, src->%(n)sSize," +
                      t[0:t.find("*")].upper()+",(void**)&dst->%(n)s);")
            continue
        if not t in fixed_size: # there are members of variable size    
            printc('\tretval |= %(t)s_copy(&src->%(n)s,&dst->%(n)s);')
    printc("\treturn retval;\n}\n")
	
printh('''/**
 * @file '''+sys.argv[2]+'''.h
 *
 * @brief Autogenerated data types defined in the UA standard
 *
 * Generated from '''+sys.argv[1]+''' with script '''+sys.argv[0]+'''
 * on host '''+platform.uname()[1]+''' by user '''+getpass.getuser()+''' at '''+ time.strftime("%Y-%m-%d %I:%M:%S")+'''
 */

#ifndef ''' + sys.argv[2].upper() + '''_H_
#define ''' + sys.argv[2].upper() + '''_H_

#include "ua_types.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_encoding_xml.h"
#include "ua_namespace_0.h"\n''')

printc('''/**
 * @file '''+sys.argv[2]+'''.c
 *
 * @brief Autogenerated function implementations to manage the data types defined in the UA standard
 *
 * Generated from '''+sys.argv[1]+''' with script '''+sys.argv[0]+'''
 * on host '''+platform.uname()[1]+''' by user '''+getpass.getuser()+''' at '''+ time.strftime("%Y-%m-%d %I:%M:%S")+'''
 */
 
#include "''' + sys.argv[2] + '.h"\n')
#include "ua_types_encoding_binary.h"
#include "ua_util.h"

# types for which we create a vector type
arraytypes = set()
fields = tree.xpath("//opc:Field", namespaces=ns)
for field in fields:
	if field.get("LengthField"):
		arraytypes.add(stripTypename(field.get("TypeName")))

deferred_types = OrderedDict()

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
		existing_types.add(name)
	elif element.tag == "{http://opcfoundation.org/BinarySchema/}StructuredType":
		if printableStructuredType(element):
			createStructured(element)
			existing_types.add(name)
		else: # the record contains types that were not yet detailed
			deferred_types[name] = element
			continue
	elif element.tag == "{http://opcfoundation.org/BinarySchema/}OpaqueType":
		createOpaque(element)
		existing_types.add(name)

for name, element in deferred_types.iteritems():
    if name in plugin_types:
        #execute plugin if registered
	exec "ret = " + packageForType[name][0]+"."+packageForType[name][1]["functionCall"]
	if ret == "default":
            createStructured(element)
            existing_types.add(name)
    else:
	createStructured(element)
        existing_types.add(name)

printh('#endif')

fh.close()
fc.close()
