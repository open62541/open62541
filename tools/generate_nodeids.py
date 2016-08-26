from __future__ import print_function
import inspect
import sys
import platform
import getpass
import time
import re
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('nodeids', help='path/to/NodeIds.csv')
parser.add_argument('outfile', help='outfile w/o extension')
args = parser.parse_args()

def useNodeId(row):
    if row[0] == "":
        return False
    if "Test" in row[0]:
        return False
    if row[0].startswith("OpcUa_"):
        return False
    if row[0].startswith("SessionsDiagnosticsSummaryType_"):
        return False
    if "Type_" in row[0]:
        return False
    return True

f = open(args.nodeids)
input_str = f.read() + "\nHasModelParent,50,ReferenceType"
f.close()
input_str = input_str.replace('\r','')
rows = map(lambda x:tuple(x.split(',')), input_str.split('\n'))

fh = open(args.outfile + ".h",'w')
def printh(string):
    print(string, end='\n', file=fh)

printh('''/**********************************************************
 * '''+args.outfile+'''.hgen -- do not modify
 **********************************************************
 * Generated from '''+args.nodeids+''' with script '''+sys.argv[0]+'''
 * on host '''+platform.uname()[1]+''' by user '''+getpass.getuser()+''' at '''+
       time.strftime("%Y-%m-%d %I:%M:%S")+'''
 **********************************************************/\n 
#ifndef ''' + args.outfile.upper().split("/")[-1] + '''_H_
#define ''' + args.outfile.upper().split("/")[-1] + '''_H_
''')

for row in rows:
    if useNodeId(row):
        printh("#define UA_NS0ID_%s %s // %s" % (row[0].upper(), row[1], row[2]))

printh('\n#endif /* ' + args.outfile.upper().split("/")[-1] + '_H_ */')

fh.close()
