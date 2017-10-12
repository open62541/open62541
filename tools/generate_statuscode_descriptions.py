#!/usr/bin/env python

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
rows = list(map(lambda x:tuple(x.split(',')), input_str.split('\n')))

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

count = 2 + len(rows)

printc('''

/* Definition for the deprecated StatusCode description API */
const UA_StatusCodeDescription statusCodeExplanation_default = {0xffffffff, "", ""};

typedef struct {
    UA_StatusCode code;
    const char *name;
} UA_StatusCodeName;

#ifndef UA_ENABLE_STATUSCODE_DESCRIPTIONS
static const char * emptyStatusCodeName = "";
const char * UA_StatusCode_name(UA_StatusCode code) {
    return emptyStatusCodeName;
}
#else
static const size_t statusCodeDescriptionsSize = %s;
static const UA_StatusCodeName statusCodeDescriptions[%i] = {''' % (count, count))

printc("    {UA_STATUSCODE_GOOD, \"Good\"},")
for row in rows:
    printc("    {UA_STATUSCODE_%s, \"%s\",}," % (row[0].upper(), row[0]))
printc('''    {0xffffffff, "Unknown StatusCode"}
};

const char * UA_StatusCode_name(UA_StatusCode code) {
    for(size_t i = 0; i < statusCodeDescriptionsSize; ++i) {
        if(statusCodeDescriptions[i].code == code)
            return statusCodeDescriptions[i].name;
    }
    return statusCodeDescriptions[statusCodeDescriptionsSize-1].name;
}

#endif''')

fc.close()
