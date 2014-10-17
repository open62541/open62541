from __future__ import print_function
import sys
import time
import platform
import getpass
from collections import OrderedDict
import re
from lxml import etree
import inspect
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--with-xml', action='store_true', help='generate xml encoding')
parser.add_argument('--with-json', action='store_true', help='generate json encoding')
parser.add_argument('--only-nano', action='store_true', help='generate only the types for the nano profile')
parser.add_argument('--only-needed', action='store_true', help='generate only types needed for compile')
parser.add_argument('--additional-includes', action='store', help='include additional header files (separated by comma)')
parser.add_argument('types', help='path/to/Opc.Ua.Types.bsd')
parser.add_argument('outfile', help='outfile w/o extension')
args = parser.parse_args()
        
ns = {"opc": "http://opcfoundation.org/BinarySchema/"}
tree = etree.parse(args.types)
types = tree.xpath("/opc:TypeDictionary/*[not(self::opc:Import)]", namespaces=ns)

fh = open(args.outfile + ".h",'w')
fc = open(args.outfile + ".c",'w')

# dirty hack. we go up the call frames to access local variables of the calling
# function. this allows to shorten code and get %()s replaces with less clutter.
def printh(string):
    print(string % inspect.currentframe().f_back.f_locals, end='\n', file=fh)

def printc(string):
    print(string % inspect.currentframe().f_back.f_locals, end='\n', file=fc)


# types that are coded manually 
from type_lists import existing_types

# whitelist for "only needed" profile
from type_lists import only_needed_types

# some types are omitted (pretend they exist already)
existing_types.add("NodeIdType")

fixed_size = set(["UA_Boolean", "UA_SByte", "UA_Byte", "UA_Int16", "UA_UInt16",
                  "UA_Int32", "UA_UInt32", "UA_Int64", "UA_UInt64", "UA_Float",
                  "UA_Double", "UA_DateTime", "UA_Guid", "UA_StatusCode"])

# types we do not want to autogenerate
def skipType(name):
    if name in existing_types:
        return True
    if "Test" in name: #skip all Test types
        return True
    if re.search("NodeId$", name) != None:
        return True
    if args.only_needed and not(name in only_needed_types):
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
    printh("typedef UA_Int32 " + name + ";")
    printh("enum " + name + "_enum { \n\t" +
           ",\n\t".join(map(lambda (key, value) : key.upper() + " = " + value, valuemap.iteritems())) +
           "\n};")
    printh("UA_TYPE_PROTOTYPES (" + name + ")")
    printh("UA_TYPE_BINARY_ENCODING(" + name + ")")
    printc("UA_TYPE_AS(" + name + ", UA_Int32)")
    printc("UA_TYPE_BINARY_ENCODING_AS(" + name + ", UA_Int32)")
    if args.with_xml:
        printh("UA_TYPE_XML_ENCODING(" + name + ")\n")
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
    printc("UA_TYPE_AS(%(name)s, UA_ByteString)")
    printc("UA_TYPE_BINARY_ENCODING_AS(%(name)s, UA_ByteString)")
    if args.with_xml:
        printh("UA_TYPE_XML_ENCODING(" + name + ")\n")
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
    if args.with_xml:
        printh("UA_TYPE_XML_ENCODING(" + name + ")\n")

    # 4) CalcSizeBinary
    printc('''UA_UInt32 %(name)s_calcSizeBinary(%(name)s const * ptr) {
    return 0''')
    has_fixed_size = True
    for n,t in membermap.iteritems():
        if t in fixed_size:
            printc('\t + sizeof(%(t)s) // %(n)s')
        elif t.find("*") != -1:
            printc('\t + UA_Array_calcSizeBinary(ptr->%(n)sSize,&UA_['+ t[0:t.find("*")].upper() +
                   "],ptr->%(n)s)")
            has_fixed_size = False
        else:
            printc('\t + %(t)s_calcSizeBinary(&ptr->%(n)s)')
            has_fixed_size = False
    printc("\t;\n}\n")
    if has_fixed_size:
        fixed_size.add(name)

    # 5) EncodeBinary
    printc('''UA_StatusCode %(name)s_encodeBinary(%(name)s const * src, UA_ByteString* dst, UA_UInt32 *offset) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;''')
    for n,t in membermap.iteritems():
        if t.find("*") != -1:
            printc("\tretval |= UA_Array_encodeBinary(src->%(n)s,src->%(n)sSize,&UA_[" + t[0:t.find("*")].upper() + "],dst,offset);")
        else:
            printc('\tretval |= %(t)s_encodeBinary(&src->%(n)s,dst,offset);')
    printc("\treturn retval;\n}\n")

    # 6) DecodeBinary
    printc('''UA_StatusCode %(name)s_decodeBinary(UA_ByteString const * src, UA_UInt32 *offset, %(name)s * dst) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;''')
    printc('\t'+name+'_init(dst);')
    for n,t in membermap.iteritems():
        if t.find("*") != -1:
            printc('\tretval |= UA_Int32_decodeBinary(src,offset,&dst->%(n)sSize);')
            printc('\tif(!retval) { retval |= UA_Array_decodeBinary(src,offset,dst->%(n)sSize,&UA_[' + t[0:t.find("*")].upper() + '],(void**)&dst->%(n)s); }')
            printc('\tif(retval) { dst->%(n)sSize = -1; }') # arrays clean up internally. But the size needs to be set here for the eventual deleteMembers.
        else:
            printc('\tretval |= %(t)s_decodeBinary(src,offset,&dst->%(n)s);')
    printc("\tif(retval) %(name)s_deleteMembers(dst);")
    printc("\treturn retval;\n}\n")

    # 7) Xml
    if args.with_xml:
        printc('''UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(%(name)s)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(%(name)s)
UA_TYPE_METHOD_DECODEXML_NOTIMPL(%(name)s)''')
    
    # 8) Delete
    printc('''void %(name)s_delete(%(name)s *p) {
	%(name)s_deleteMembers(p);
	UA_free(p);\n}\n''')
	
    # 9) DeleteMembers
    printc('''void %(name)s_deleteMembers(%(name)s *p) {''')
    for n,t in membermap.iteritems():
        if not t in fixed_size: # dynamic size on the wire
            if t.find("*") != -1:
		printc("\tUA_Array_delete((void*)p->%(n)s,p->%(n)sSize,&UA_[" +
                       t[0:t.find("*")].upper()+"]);")
            else:
		printc('\t%(t)s_deleteMembers(&p->%(n)s);')
    printc("\n}\n")

    # 10) Init
    printc('''void %(name)s_init(%(name)s *p) {
    if(!p) return;''')
    for n,t in membermap.iteritems():
        if t.find("*") != -1:
            printc('\tp->%(n)sSize = -1;')
            printc('\tp->%(n)s = UA_NULL;')
        else:
            printc('\t%(t)s_init(&p->%(n)s);')
    printc("\n}\n")

    # 11) New
    printc("UA_TYPE_NEW_DEFAULT(%(name)s)")

    # 12) Copy
    printc('''UA_StatusCode %(name)s_copy(const %(name)s *src,%(name)s *dst) {
	UA_StatusCode retval = UA_STATUSCODE_GOOD;''')
    printc("\t%(name)s_init(dst);")
    for n,t in membermap.iteritems():
        if t.find("*") != -1:
            printc('\tdst->%(n)sSize = src->%(n)sSize;')
            printc("\tretval |= UA_Array_copy(src->%(n)s, src->%(n)sSize,&UA_[" + t[0:t.find("*")].upper() + "],(void**)&dst->%(n)s);")
            continue
        if not t in fixed_size: # there are members of variable size    
            printc('\tretval |= %(t)s_copy(&src->%(n)s,&dst->%(n)s);')
            continue
        printc("\tdst->%(n)s = src->%(n)s;")
    printc('''if(retval) {
    %(name)s_deleteMembers(dst);
    %(name)s_init(dst);
    }''')
    printc("\treturn retval;\n}\n")

    # 13) Print
    printc('''#ifdef DEBUG''') 
    printc('''void %(name)s_print(const %(name)s *p, FILE *stream) {
    fprintf(stream, "(%(name)s){");''')
    for i,(n,t) in enumerate(membermap.iteritems()):
        if t.find("*") != -1:
            printc('\tUA_Int32_print(&p->%(n)sSize, stream);')
            printc("\tUA_Array_print(p->%(n)s, p->%(n)sSize, &UA_[" + t[0:t.find("*")].upper()+"], stream);")
        else:
            printc('\t%(t)s_print(&p->%(n)s,stream);')
        if i == len(membermap)-1:
            continue
	printc('\tfprintf(stream, ",");')
    printc('''\tfprintf(stream, "}");\n}''')
    printc('#endif');
    printc('''\n''')
    
	
shortname = args.outfile.split("/")[-1] 
print(shortname)
printh('''/**
 * @file %(shortname)s.h
 *
 * @brief Autogenerated data types defined in the UA standard
 *
 * Generated from '''+sys.argv[1]+''' with script '''+sys.argv[0]+'''
 * on host '''+platform.uname()[1]+''' by user '''+getpass.getuser()+''' at '''+ time.strftime("%Y-%m-%d %I:%M:%S")+'''
 */

#ifndef ''' + shortname.upper() + '''_H_
#define ''' + shortname.upper() + '''_H_

#include "ua_types.h"
#include "ua_types_encoding_binary.h"

/** @ingroup ''' + shortname.split("_")[1] + '''
 *
 * @defgroup ''' + shortname + ''' Generated Types
 *
 * @brief Data structures that are autogenerated from an XML-Schema.
 * @{
 */''')
if args.with_xml:
	printh('#include "ua_types_encoding_xml.h"')
if args.additional_includes:
    for incl in args.additional_includes.split(","):
        printh("#include \"" + incl + "\"")
printh("") # newline

printc('''/**
 * @file '''+sys.argv[2]+'''.c
 *
 * @brief Autogenerated function implementations to manage the data types defined in the UA standard
 *
 * Generated from '''+sys.argv[1]+''' with script '''+sys.argv[0]+'''
 * on host '''+platform.uname()[1]+''' by user '''+getpass.getuser()+''' at '''+ time.strftime("%Y-%m-%d %I:%M:%S")+'''
 */
 
#include "''' + args.outfile.split("/")[-1] + '''.h"
#include "ua_namespace_0.h"
#include "ua_util.h"\n''')

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
printh('/// @} /* end of group */')
printh('#endif')

fh.close()
fc.close()
