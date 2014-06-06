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
input_str = f.read() + "\nInvalidType,0,DataType"
input_str = input_str.replace('\r','')
rows = map(lambda x:tuple(x.split(',')), input_str.split('\n'))
f.close()

fh = open(sys.argv[2] + ".h",'w')
fc = open(sys.argv[2] + ".c",'w')

print('''/**********************************************************
 * '''+sys.argv[2]+'''.hgen -- do not modify
 **********************************************************
 * Generated from '''+sys.argv[1]+''' with script '''+sys.argv[0]+'''
 * on host '''+platform.uname()[1]+''' by user '''+getpass.getuser()+''' at '''+ time.strftime("%Y-%m-%d %I:%M:%S")+'''
 **********************************************************/
 
#ifndef ''' + sys.argv[2].upper() + '''_H_
#define ''' + sys.argv[2].upper() + '''_H_

#include "ua_util.h"
#include "ua_types.h"  // definition of UA_VTable and basic UA_Types
#include "ua_types_generated.h"

/**
 * @brief maps namespace zero nodeId to index into UA_VTable
 *
 * @param[in] ns0Id The namespace zero nodeId
 *
 * @retval UA_ERR_INVALID_VALUE whenever ns0Id could not be mapped
 * @retval the corresponding index into UA_VTable
 */

UA_Int32 UA_ns0ToVTableIndex(const UA_NodeId *id);

extern UA_VTable UA_; 

static UA_Int32 phantom_delete(void * p) { return UA_SUCCESS; }
extern UA_VTable UA_borrowed_;

/**
 * @brief the set of possible indices into UA_VTable
 *
 * Enumerated type to define the types that the open62541 stack can handle
 */
enum UA_VTableIndex_enum {''', end='\n', file=fh)

print('''/**********************************************************
 * '''+sys.argv[2]+'''.cgen -- do not modify
 **********************************************************
 * Generated from '''+sys.argv[1]+''' with script '''+sys.argv[0]+'''
 * on host '''+platform.uname()[1]+''' by user '''+getpass.getuser()+''' at '''+ time.strftime("%Y-%m-%d %I:%M:%S")+'''
 **********************************************************/
 
#include "''' + sys.argv[2] + '''.h"

UA_Int32 UA_ns0ToVTableIndex(const UA_NodeId *id) {
	UA_Int32 retval = UA_ERR_INVALID_VALUE;
        if(id->namespace != 0) return retval;
	switch (id->identifier.numeric) { ''', end='\n',file=fc)

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

UA_VTable UA_ = {
\t.getTypeIndex = UA_ns0ToVTableIndex,
\t.types = (UA_VTable_Entry[]){''', file=fc)

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

    print("\t{.typeId={UA_NODEIDTYPE_FOURBYTE,0,.identifier.numeric=" + row[1] +"}"+ 
          ",.name=(UA_Byte*)&\""+name+"\""+
          ",.new=(UA_Int32(*)(void **))"+name+"_new"+
          ",.init=(UA_Int32(*)(void *))"+name+"_init"+
          ",.copy=(UA_Int32(*)(void const * ,void*))"+name+"_copy"+
          ",.delete=(UA_Int32(*)(void *))"+name+"_delete"+
          ",.deleteMembers=(UA_Int32(*)(void *))"+name+"_deleteMembers"+
          ",.memSize=" + ("sizeof("+name+")" if (name != "UA_InvalidType") else "0") +
          ",.encodings={(UA_Encoding){.calcSize=(UA_calcSize)"+ name +"_calcSizeBinary" +
          ",.encode=(UA_encode)"+name+ "_encodeBinary" +
          ",.decode=(UA_decode)"+name+"_decodeBinary}"+
          ",(UA_Encoding){.calcSize=(UA_calcSize)"+ name +"_calcSizeXml" +
          ",.encode=(UA_encode)"+name+ "_encodeXml" +
          ",.decode=(UA_decode)"+name+"_decodeXml}"+
          "}},",
          end='\n',file=fc) 

print('''}};

UA_VTable UA_noDelete_ = {
\t.getTypeIndex=UA_ns0ToVTableIndex,
\t.types = (UA_VTable_Entry[]){''', end='\n', file=fc)

for row in rows:
    if row[0] == "" or row[0] in exclude_types or row[2] in exclude_kinds:
        continue
    if row[0] == "BaseDataType":
        name = "UA_Variant"
    elif row[0] == "Structure":
        name = "UA_ExtensionObject"
    else:	
	name = "UA_" + row[0]

    print("\t{.typeId={UA_NODEIDTYPE_FOURBYTE,0,.identifier.numeric=" + row[1] +"}"+ 
          ",.name=(UA_Byte*)&\""+name+"\""+
          ",.new=(UA_Int32(*)(void **))"+name+"_new"+
          ",.init=(UA_Int32(*)(void *))"+name+"_init"+
          ",.copy=(UA_Int32(*)(void const * ,void*))"+name+"_copy"+
          ",.delete=(UA_Int32(*)(void *))phantom_delete"+
          ",.deleteMembers=(UA_Int32(*)(void *))phantom_delete"+
          ",.memSize=" + ("sizeof("+name+")" if (name != "UA_InvalidType") else "0") +
          ",.encodings={(UA_Encoding){.calcSize=(UA_calcSize)"+ name +"_calcSizeBinary" +
          ",.encode=(UA_encode)"+name+ "_encodeBinary" +
          ",.decode=(UA_decode)"+name+"_decodeBinary}"+
          ",(UA_Encoding){.calcSize=(UA_calcSize)"+ name +"_calcSizeXml" +
          ",.encode=(UA_encode)"+name+ "_encodeXml" +
          ",.decode=(UA_decode)"+name+"_decodeXml}"+
          "}},",
          end='\n',file=fc) 


print("}};", end='\n', file=fc) 

print('\n#endif /* OPCUA_NAMESPACE_0_H_ */', end='\n', file=fh)

fh.close()
fc.close()
