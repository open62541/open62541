#!/usr/bin/env python3

### This Source Code Form is subject to the terms of the Mozilla Public
### License, v. 2.0. If a copy of the MPL was not distributed with this
### file, You can obtain one at http://mozilla.org/MPL/2.0/.

###    Copyright 2023 (c) Fraunhofer IOSB (Author: Noel Graf)

import argparse
import re
import os
import platform

parser = argparse.ArgumentParser()
parser.add_argument('outfile', help='outfile w/o extension')
parser.add_argument('namespace', help='namespace')
args = parser.parse_args()

include_line_counter = 0
code_line_counter = 0
existing_namespaces = []
data = []


def is_unix():
    try:
        # Check if the os.uname() function is available
        os.uname()
        return True
    except:
        return False


def windows_exec():
    import msvcrt

    global include_line_counter
    global code_line_counter
    global existing_namespaces
    global data

    # Open the file in read-write mode
    with open(args.outfile + ".c", "r+", encoding='utf8') as file:
        # Lock the file
        msvcrt.locking(file.fileno(), msvcrt.LK_LOCK, 1)

        # Write to the file
        write_code_generation(file)

        # Unlock the file
        msvcrt.locking(file.fileno(), msvcrt.LK_UNLCK, 1)


def unix_exec():
    import fcntl

    global include_line_counter
    global code_line_counter
    global existing_namespaces
    global data

    with open(args.outfile + ".c", "r+", encoding='utf8') as file:
            # Acquire a lock on the file
            fcntl.flock(file, fcntl.LOCK_EX)

            # Write to the file
            write_code_generation(file)

            # Release the lock on the file
            fcntl.flock(file, fcntl.LOCK_UN)


def print_include(string):
    data[include_line_counter] = string


def print_function_call(string):
    data[code_line_counter] = string


def generate_code():
    #########################
    # Print the header file #
    #########################

    print_include(u'''
#include <open62541/{namespace}.h>
//<
'''.format(namespace=args.namespace))

    #########################
    # Print the source file #
    #########################

    print_function_call(u'''
/* {namespace} */
retval |= {namespace}(server);
if(retval != UA_STATUSCODE_GOOD) {{
UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Adding the {namespace} failed. Please check previous error output.");
UA_Server_delete(server);
return EXIT_FAILURE;
}}
UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "The {namespace} successfully added.");
//>
'''.format(namespace=args.namespace))


def write_code_generation(file):
    global include_line_counter
    global code_line_counter
    global existing_namespaces
    global data

    # Write to the file
    lines = file.readlines()
    line_counter = 0
    # List which contains the existing namespaces
    existing_namespaces = []
    for line in lines:
        namespaces = re.findall("namespace_.*_generated\(.*\)", line)
        if namespaces:
            namespaces = namespaces[0].split("(")[0]
            existing_namespaces.append(namespaces)
        if re.search("//<", line):
            include_line_counter = line_counter
        if re.search("//>", line):
            code_line_counter = line_counter
        line_counter += 1

    if args.namespace not in existing_namespaces:
        # Set the file descriptor to the beginning of the file
        file.seek(0)
        # read a list of lines into data
        data = file.readlines()
        generate_code()
        # Set the file descriptor to the beginning of the file and delete content
        file.seek(0)
        file.truncate(0)
        file.writelines(data)


if platform.system == "Linux":
    unix_exec()
else:
    if is_unix():
        unix_exec()
    else:
        windows_exec()
