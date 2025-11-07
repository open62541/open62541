#!/usr/bin/env python3

### This Source Code Form is subject to the terms of the Mozilla Public
### License, v. 2.0. If a copy of the MPL was not distributed with this
### file, You can obtain one at http://mozilla.org/MPL/2.0/.

###    Copyright 2023 (c) Fraunhofer IOSB (Author: Noel Graf)

import argparse

parser = argparse.ArgumentParser()
parser.add_argument('outfile', help='outfile w/o extension')
parser.add_argument('nodesets', nargs='+', help='List of Nodesets')
args = parser.parse_args()

# Normalize to lower case letters
nodesets = [ns.lower().replace('-', '_') for ns in args.nodesets]

fh = open(args.outfile + ".h", "w", encoding='utf8')
fc = open(args.outfile + ".c", "w", encoding='utf8')

def printh(string):
    print(string, end='\n', file=fh)
def printc(string):
    print(string, end='\n', file=fc)

#########################
# Print the header file #
#########################

printh('''
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

printc('''
/* WARNING: This is a generated file.
 * Any manual changes will be overwritten. */

#include "nodesetinjector.h"
''')

# Includes for each nodeset
for ns in nodesets:
    printc(f'#include <open62541/namespace_{ns}_generated.h>')

# Special case: PADIM requires IRDI beforehand
if 'padim' in nodesets:
    printc('#include <open62541/namespace_irdi_generated.h>')

printc('''
UA_StatusCode UA_Server_injectNodesets(UA_Server *server) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Attaching the AUTOLOAD Nodesets to the server!");
''')

# Function calls for each nodeset
for ns in nodesets:
    # Special handling: Insert IRDI before PADIM
    if ns == 'padim':
        printc('''
    /* namespace_irdi_generated */
    retval |= namespace_irdi_generated(server);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Adding the namespace_irdi_generated failed. Please check previous error output.");
        return retval;
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "The namespace_irdi_generated successfully added.");
        ''')

    printc(f'''
    /* namespace_{ns}_generated */
    retval |= namespace_{ns}_generated(server);
    if(retval != UA_STATUSCODE_GOOD) {{
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Adding the namespace_{ns}_generated failed. Please check previous error output.");
        return retval;
    }}
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "The namespace_{ns}_generated successfully added.");
    ''')

printc('''
    return retval;
}
''')

fc.close()
fh.close()
