from __future__ import print_function
import sys
import platform
import getpass
import time

if len(sys.argv) != 3:
    print("Usage: python generate_namespace.py <path/to/NodeIds.csv> <outfile w/o extension>", file=sys.stdout)
    exit(0)

# types that are to be excluded
exclude_kinds = set(["Object","ObjectType","Variable","Method","ReferenceType"])
exclude_types = set(["Number", "Integer", "UInteger", "Enumeration",
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
	"YArrayItemType", "XYArrayItemType", "ImageItemType", "CubeItemType", "NDimensionArrayItemType"])

f = open(sys.argv[1])
input_str = "InvalidType,0,DataType\n" + f.read()
input_str = input_str.replace('\r','')
rows = map(lambda x:tuple(x.split(',')), input_str.split('\n'))
f.close()

fh = open(sys.argv[2] + ".hgen",'w')
fc = open(sys.argv[2] + ".cgen",'w')

print('''/**********************************************************
 * '''+sys.argv[2]+'''.hgen -- do not modify
 **********************************************************
 * Generated from '''+sys.argv[1]+''' with script '''+sys.argv[0]+'''
 * on host '''+platform.uname()[1]+''' by user '''+getpass.getuser()+''' at '''+ time.strftime("%Y-%m-%d %I:%M:%S")+'''
 **********************************************************/
 
#ifndef OPCUA_NAMESPACE_0_H_
#define OPCUA_NAMESPACE_0_H_

#include "ua_basictypes.h"  // definition of UA_VTable and basic UA_Types

UA_Int32 UA_ns0ToVTableIndex(UA_Int32 id);
extern UA_VTable UA_[]; 

static UA_Int32 phantom_delete(void * p) { return UA_SUCCESS; }
extern UA_VTable UA_noDelete_[];

enum UA_VTableIndex_enum {''', end='\n', file=fh)

print('''/**********************************************************
 * '''+sys.argv[2]+'''.cgen -- do not modify
 **********************************************************
 * Generated from '''+sys.argv[1]+''' with script '''+sys.argv[0]+'''
 * on host '''+platform.uname()[1]+''' by user '''+getpass.getuser()+''' at '''+ time.strftime("%Y-%m-%d %I:%M:%S")+'''
 **********************************************************/
 
#include "opcua.h"

UA_Int32 UA_ns0ToVTableIndex(UA_Int32 id) {
    UA_Int32 retval = UA_ERR_INVALID_VALUE;
    switch (id) { ''', end='\n',file=fc)

i = 0
for row in rows:
    if row[0] == "" or row[0] in exclude_types or row[2] in exclude_kinds:
        continue
    if row[0] == "BaseDataType":
    	name = "UA_Variant"
    elif row[0] == "Structure":
        name = "UA_ExtensionObject"
    else:
	name = "UA_" + row[0]
	
    print("\t"+name.upper()+" = "+str(i)+",", file=fh)
    print('\tcase '+row[1]+': retval='+name.upper()+'; break; //'+row[2], file=fc)
    i = i+1

print("};\n", file=fh)
print('''
    }
    return retval;
}

UA_VTable UA_[] = {''', file=fc)

for row in rows:
    if row[0] == "" or row[0] in exclude_types or row[2] in exclude_kinds:
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
          ",(UA_Int32(*)(void *))"+name+"_init"+
          ",(UA_Int32(*)(void **))"+name+"_new"+
	  ",(UA_Int32(*)(void const * ,void*))"+name+"_copy"+
          ",(UA_Int32(*)(void *))"+name+"_delete"+
          ',(UA_Byte*)"'+name+'"},',end='\n',file=fc) 

print("};\n\nUA_VTable UA_noDelete_[] = {", end='\n', file=fc)

for row in rows:
    if row[0] == "" or row[0] in exclude_types or row[2] in exclude_kinds:
        continue
    if row[0] == "BaseDataType":
        name = "UA_Variant"
    elif row[0] == "Structure":
        name = "UA_ExtensionObject"
    else:	
	name = "UA_" + row[0]

    print("\t{" + row[1] +
          ",(UA_Int32(*)(void const*))"+name+"_calcSize" + 
          ",(UA_Int32(*)(UA_ByteString const*,UA_Int32*,void*))"+name+ "_decodeBinary" +
          ",(UA_Int32(*)(void const*,UA_Int32*,UA_ByteString*))"+name+"_encodeBinary"+
          ",(UA_Int32(*)(void *))"+name+"_init"+
          ",(UA_Int32(*)(void **))"+name+"_new"+
	  ",(UA_Int32(*)(void const * ,void*))"+name+"_copy"+
          ",(UA_Int32(*)(void *))phantom_delete"+
          ',(UA_Byte*)"'+name+'"},',end='\n',file=fc)
print("};", end='\n', file=fc) 

print('\n#endif /* OPCUA_NAMESPACE_0_H_ */', end='\n', file=fh)

fh.close()
fc.close()
