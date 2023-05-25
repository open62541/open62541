#!/usr/bin/env python3

### This Source Code Form is subject to the terms of the Mozilla Public
### License, v. 2.0. If a copy of the MPL was not distributed with this
### file, You can obtain one at http://mozilla.org/MPL/2.0/.

###    Copyright 2023 (c) Fraunhofer IOSB (Author: Noel Graf)

import argparse
from io import open

parser = argparse.ArgumentParser()
parser.add_argument('outfile', help='outfile w/o extension')
args = parser.parse_args()

fh = open(args.outfile + ".h", "wt", encoding='utf8')
fc = open(args.outfile + ".c", "wt", encoding='utf8')

def printh(string):
    print(string, end=u'\n', file=fh)
def printc(string):
    print(string, end=u'\n', file=fc)

#########################
# Print the header file #
#########################

printh(u'''
/* WARNING: This is a generated file.
 * Any manual changes will be overwritten. */
 
#ifndef NODESETINJECTOR_H_
#define NODESETINJECTOR_H_

#include <open62541/server.h>
#include <open62541/plugin/log_stdout.h>

_UA_BEGIN_DECLS

extern UA_StatusCode UA_Server_injectNodesets(UA_Server *server);

_UA_END_DECLS

#endif /* NODESETINJECTOR_H_ */
''')

#########################
# Print the source file #
#########################

printc(u'''
/* WARNING: This is a generated file.
 * Any manual changes will be overwritten. */

#include "nodesetinjector.h"
//<

UA_StatusCode UA_Server_injectNodesets(UA_Server *server) {
UA_StatusCode retval = UA_STATUSCODE_GOOD;
UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Attaching the AUTOLOAD Nodesets to the server!");
//>

return retval;
}
''')

fc.close()
fh.close()
