from __future__ import print_function
import sys
import platform
import getpass
import time
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('statuscodes', help='path/to/Opc.Ua.StatusCodes.csv')
parser.add_argument('outfile', help='outfile w/o extension')
args = parser.parse_args()

f = open(args.statuscodes)
input_str = f.read()
f.close()
input_str = input_str.replace('\r','')
rows = map(lambda x:tuple(x.split(',')), input_str.split('\n'))

fh = open(args.outfile + ".c",'w')
def printh(string):
    print(string, end='\n', file=fh)

printh('''/**********************************************************
 * '''+args.outfile+'''.hgen -- do not modify
 **********************************************************
 * Generated from '''+args.statuscodes+''' with script '''+sys.argv[0]+'''
 * on host '''+platform.uname()[1]+''' by user '''+getpass.getuser()+''' at '''+
       time.strftime("%Y-%m-%d %I:%M:%S")+'''
 **********************************************************/\n

#include "ua_statuscode_msg.h"

const struct UA_StatusCode_msg_info UA_StatusCode_msg_table[] =
{''')

#for row in rows:
#    printh("#define UA_STATUSCODE_%s %s // %s" % (row[0].upper(), row[1].lower(), row[2]))

count = 1
printh(" {UA_STATUSCODE_GOOD, \"Good\", \"No error\"},")

for row in rows:
    printh(" {UA_STATUSCODE_%s, \"%s\", \"%s\"}," % (row[0].upper(), row[0], row[2]))
    count += 1

printh('\n};\n')

printh('const unsigned int UA_StatusCode_msg_table_size = ' + str(count) + ';')


fh.close()
