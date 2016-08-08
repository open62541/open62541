#!/usr/bin/env/python
# -*- coding: utf-8 -*-

###
### Author:  Chris Iatrou (ichrispa@core-vector.net)
### Version: rev 13
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

NODE_CLASS_GENERERIC        = 0
NODE_CLASS_OBJECT           = 1
NODE_CLASS_VARIABLE         = 2
NODE_CLASS_METHOD           = 4
NODE_CLASS_OBJECTTYPE       = 8
NODE_CLASS_VARIABLETYPE     = 16
NODE_CLASS_REFERENCETYPE    = 32
NODE_CLASS_DATATYPE         = 64
NODE_CLASS_VIEW             = 128

# Not in OPC-UA, but exists in XML
NODE_CLASS_METHODTYPE       = 256

##
## Numeric codes used to encode binary type fields:
##
BUILTINTYPE_TYPEID_EXTENSIONOBJECT = 1
BUILTINTYPE_TYPEID_LOCALIZEDTEXT = 2
BUILTINTYPE_TYPEID_EXPANDEDNODEID = 3
BUILTINTYPE_TYPEID_NODEID = 4
BUILTINTYPE_TYPEID_DATETIME = 5
BUILTINTYPE_TYPEID_QUALIFIEDNAME = 6
BUILTINTYPE_TYPEID_STATUSCODE = 7
BUILTINTYPE_TYPEID_GUID = 8
BUILTINTYPE_TYPEID_BOOLEAN = 9
BUILTINTYPE_TYPEID_BYTE = 10
BUILTINTYPE_TYPEID_SBYTE = 11
BUILTINTYPE_TYPEID_INT16 = 12
BUILTINTYPE_TYPEID_UINT16 = 13
BUILTINTYPE_TYPEID_INT32 = 14
BUILTINTYPE_TYPEID_UINT32 = 15
BUILTINTYPE_TYPEID_INT64 = 16
BUILTINTYPE_TYPEID_UINT64 = 17
BUILTINTYPE_TYPEID_FLOAT = 18
BUILTINTYPE_TYPEID_DOUBLE = 19
BUILTINTYPE_TYPEID_STRING = 20
BUILTINTYPE_TYPEID_XMLELEMENT = 21
BUILTINTYPE_TYPEID_BYTESTRING = 22
BUILTINTYPE_TYPEID_DIAGNOSTICINFO = 23
