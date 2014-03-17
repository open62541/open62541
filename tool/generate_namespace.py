from __future__ import print_function
import sys
from collections import OrderedDict
import re
import csv
from itertools import tee

if len(sys.argv) != 3:
    print("Usage: python generate_namespace.py <path/to/NodeIds.csv> <outfile w/o extension>", file=sys.stdout)
    exit(0)

# types that are to be excluded
exclude_types = set(["Object","ObjectType","Variable","Method"])

def skipType(name):
    if name in exclude_types:
        return True
    return False

f = open(sys.argv[1])
rows1, rows2, rows3 = tee(csv.reader(f), 3)

fh = open(sys.argv[2] + ".h",'w');
fc = open(sys.argv[2] + ".c",'w');

print('''/* struktur vTable und enumerierung*/
#ifndef OPCUA_NAMESPACE_0_H_
#define OPCUA_NAMESPACE_0_H_

#include "opcua.h"  // definition of UA_VTable and basic UA_Types

Int32 UA_namespace_zero_to_index(Int32 id);
extern UA_VTable UA_namespace_zero[]; 

enum UA_VTableIndex_enum {''', end='\n', file=fh)

print('''/* Mapping and vTable of Namespace Zero */
#include "opcua.h"
Int32 UA_namespace_zero_to_index(Int32 id) {
    Int32 retval = -1;
    switch (id) { ''', end='\n',file=fc)

i = 0
for row in rows1:
    if skipType(row[2]):
	continue

    name = "UA_" + row[0]
    print("\t"+name.upper()+"="+str(i)+",", file=fh)
    print('\tcase '+row[1]+': retval='+name.upper()+'; break; //'+row[2], file=fc)
    i = i+1

print('\tUA_NS0_VTABLE_MAX = 0\n};\n', file=fh)
print('''\t}\n\treturn retval;
}
UA_VTable UA_namespace_zero[] = {''', file=fc)

for row in rows2:
    if skipType(row[2]):
	continue

    name = "UA_" + row[0]
    print('#define '+name.upper()+'_NS0 (UA_namespace_zero['+name.upper()+'].Id)', file=fh)

    print("\t{" + row[1] + ", &" + name + "_calcSize, &" + name + "_decode, &" + name + "_encode},",end='\n',file=fc) 

print("\t{0,NULL,NULL,NULL}\n};",file=fc)
print('#endif /* OPCUA_NAMESPACE_0_H_ */', end='\n', file=fh)
fh.close()
fc.close()
f.close()

