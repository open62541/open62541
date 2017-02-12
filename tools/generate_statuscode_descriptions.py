# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this 
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

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

fc = open(args.outfile + ".c",'w')
def printc(string):
    print(string, end='\n', file=fc)

printc('''/**********************************************************
 * '''+args.outfile+'''.hgen -- do not modify
 **********************************************************
 * Generated from '''+args.statuscodes+''' with script '''+sys.argv[0]+'''
 * on host '''+platform.uname()[1]+''' by user '''+getpass.getuser()+''' at '''+
       time.strftime("%Y-%m-%d %I:%M:%S")+'''
 **********************************************************/\n

#include "ua_types.h"''')

count = 2
for row in rows:
    count += 1

printc('''
#ifndef UA_ENABLE_STATUSCODE_DESCRIPTIONS
static const size_t statusCodeDescriptionsSize = 1;
static const UA_StatusCodeDescription statusCodeDescriptions[1] = {
{0xffffffff, \"StatusCode descriptions not available\", \"open62541 was compiled without support for statuscode descriptions\"}
};
#else
static const size_t statusCodeDescriptionsSize = %s;
static const UA_StatusCodeDescription statusCodeDescriptions[%i] =
{''' % (count, count))

printc(" {UA_STATUSCODE_GOOD, \"Good\", \"Success / No error\"},")
for row in rows:
    printc(" {UA_STATUSCODE_%s, \"%s\", \"%s\"}," % (row[0].upper(), row[0], row[2]))
printc(" {0xffffffff, \"Unknown\", \"Unknown StatusCode\"},")
printc('''\n};
#endif''')

printc('''
const UA_StatusCodeDescription * UA_StatusCode_description(UA_StatusCode code) {
    for(size_t i = 0; i < statusCodeDescriptionsSize; ++i) {
        if(statusCodeDescriptions[i].code == code)
            return &statusCodeDescriptions[i];
    }
    return &statusCodeDescriptions[statusCodeDescriptionsSize-1];
}
''')

fc.close()
