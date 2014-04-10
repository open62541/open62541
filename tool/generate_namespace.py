from __future__ import print_function
import sys
import platform
import getpass
from collections import OrderedDict
import time
import re
import csv
from itertools import tee

if len(sys.argv) != 3:
    print("Usage: python generate_namespace.py <path/to/NodeIds.csv> <outfile w/o extension>", file=sys.stdout)
    exit(0)

# types that are to be excluded
exclude_kind = set(["Object","ObjectType","Variable","Method","ReferenceType"])
exclude_types = set(["Number", 
    "Integer", "UInteger", "Enumeration",
	"Image", "ImageBMP", "ImageGIF", "ImageJPG", "ImagePNG",
	"References", "BaseVariableType", "BaseDataVariableType", 
	"PropertyType", "DataTypeDescriptionType", "DataTypeDictionaryType", "NamingRuleType",
	"IntegerId","Counter","Duration","NumericRange","Time","Date",
	"UtcTime", "LocaleId","UserTokenType",
	"ApplicationType","ApplicationInstanceCertificate",
	"ServerVendorCapabilityType","ServerStatusType","ServerDiagnosticsSummaryType",
	"SamplingIntervalDiagnosticsArrayType", "SamplingIntervalDiagnosticsType", 
	"SubscriptionDiagnosticsArrayType", "SubscriptionDiagnosticsType",
	"SessionDiagnosticsArrayType", "SessionDiagnosticsVariableType", 
	"SessionSecurityDiagnosticsArrayType", "SessionSecurityDiagnosticsType", 
	"DataItemType", "AnalogItemType", "DiscreteItemType", "TwoStateDiscreteType",
	"MultiStateDiscreteType", "ProgramDiagnosticType", "StateVariableType", "FiniteStateVariableType",
	"TransitionVariableType", "FiniteTransitionVariableType", "BuildInfoType", "TwoStateVariableType",
	"ConditionVariableType", "MultiStateValueDiscreteType", "OptionSetType", "ArrayItemType",
	"YArrayItemType", "XYArrayItemType", "ImageItemType", "CubeItemType", "NDimensionArrayItemType"
	])
	
def skipKind(name):
    if name in exclude_kind:
        return True
    return False
    
def skipType(name):
    if name in exclude_types:
        return True
    return False

f = open(sys.argv[1])
rows1, rows2, rows3 = tee(csv.reader(f), 3)

fh = open(sys.argv[2] + ".hgen",'w');
fc = open(sys.argv[2] + ".cgen",'w');

print('''/**********************************************************
 * '''+sys.argv[2]+'''.hgen -- do not modify
 **********************************************************
 * Generated from '''+sys.argv[1]+''' with script '''+sys.argv[0]+'''
 * on host '''+platform.uname()[1]+''' by user '''+getpass.getuser()+''' at '''+ time.strftime("%Y-%m-%d %I:%M:%S")+'''
 **********************************************************/
 
#ifndef OPCUA_NAMESPACE_0_H_
#define OPCUA_NAMESPACE_0_H_

#include "opcua.h"  // definition of UA_VTable and basic UA_Types

UA_Int32 UA_toIndex(UA_Int32 id);
extern UA_VTable UA_[]; 

enum UA_VTableIndex_enum {''', end='\n', file=fh)

print('''/**********************************************************
 * '''+sys.argv[2]+'''.cgen -- do not modify
 **********************************************************
 * Generated from '''+sys.argv[1]+''' with script '''+sys.argv[0]+'''
 * on host '''+platform.uname()[1]+''' by user '''+getpass.getuser()+''' at '''+ time.strftime("%Y-%m-%d %I:%M:%S")+'''
 **********************************************************/
 
#include "opcua.h"

UA_Int32 UA_toIndex(UA_Int32 id) {
    UA_Int32 retval = -1;
    switch (id) { ''', end='\n',file=fc)

i = 0
for row in rows1:
    if skipKind(row[2]):
	continue

    if skipType(row[0]):
	continue
	
    if row[0] == "BaseDataType":
    	name = "UA_Variant"
    elif row[0] == "Structure":
    	name = "UA_ExtensionObject" 
    else:	
	name = "UA_" + row[0]
	
    print("\t"+name.upper()+"="+str(i)+",", file=fh)
    print('\tcase '+row[1]+': retval='+name.upper()+'; break; //'+row[2], file=fc)
    i = i+1

print('\tUA_NS0_VTABLE_MAX = 0\n};\n', file=fh)
print('''\t}\n\treturn retval;
}
UA_VTable UA_[] = {''', file=fc)

for row in rows2:
    if skipKind(row[2]):
	continue

    if skipType(row[0]):
	continue

    if row[0] == "BaseDataType":
    	name = "UA_Variant"
    elif row[0] == "Structure":
    	name = "UA_ExtensionObject" 
    else:	
	name = "UA_" + row[0]

    print('#define '+name.upper()+'_NS0 '+row[1], file=fh)

    print("\t{" + row[1] + 
          ",(UA_Int32(*)(void const*))"+name+"_calcSize" + 
          ",(UA_Int32(*)(UA_ByteString const*,UA_Int32*,void*))"+name+ "_decodeBinary" +
          ",(UA_Int32(*)(void const*,UA_Int32*,UA_ByteString*))"+name+"_encodeBinary"+
          ",(UA_Int32(*)(void **))"+name+"_new"+
          ",(UA_Int32(*)(void *))"+name+"_delete"+
          ',(UA_Byte*)"'+name+'"},',end='\n',file=fc) 

print('\t{0,UA_NULL,UA_NULL,UA_NULL,UA_NULL,UA_NULL,(UA_Byte*)"undefined"}\n};',file=fc)
print('#endif /* OPCUA_NAMESPACE_0_H_ */', end='\n', file=fh)
fh.close()
fc.close()
f.close()

2222