from __future__ import print_function
import re
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('version', help='version to include')
parser.add_argument('outfile', help='outfile w/o extension')
parser.add_argument('inputs', nargs='*', action='store', help='input filenames')
args = parser.parse_args()

outname = args.outfile.split("/")[-1]
pos = outname.find(".")
if pos > 0:
    outname = outname[:pos]
include_re = re.compile("^#include ([\"<].*[\">]).*$")
guard_re = re.compile("^#(?:(?:ifndef|define) [A-Z_]+_H_|endif /\* [A-Z_]+_H_ \*/)")
includes = []

is_c = False

for fname in args.inputs:
    if("util.h" in fname):
        is_c = True
        continue
    with open(fname) as infile:
        for line in infile:
            res = include_re.match(line)
            if res:
                inc = res.group(1)
                if not inc in includes and not inc[0] == '"':
                    includes.append(inc)

file = open(args.outfile, 'w')
file.write('''/* THIS IS A SINGLE-FILE DISTRIBUTION CONCATENATED FROM THE OPEN62541 SOURCES 
 * visit http://open62541.org/ for information about this software
 * Git-Revision: %s
 */
 
 /*
 * Copyright (C) 2015 the contributors as stated in the AUTHORS file
 *
 * This file is part of open62541. open62541 is free software: you can
 * redistribute it and/or modify it under the terms of the GNU Lesser General
 * Public License, version 3 (as published by the Free Software Foundation) with
 * a static linking exception as stated in the LICENSE file provided with
 * open62541.
 *
 * open62541 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 */\n\n''' % args.version)

if not is_c:
    file.write('''#ifndef %s
#define %s

#ifdef __cplusplus
extern "C" {
#endif\n\n''' % (outname.upper() + "_H_", outname.upper() + "_H_") )

if not is_c:
    for inc in includes:
        file.write("#include " + inc + "\n")
else:
    file.write("#define UA_AMALGAMATE\n")
    file.write('''#ifndef UA_DYNAMIC_LINKING
# define UA_DYNAMIC_LINKING
#endif\n\n''')
    for fname in args.inputs:
        if "ua_config.h" in fname or "ua_util.h" in fname:
            with open(fname) as infile:
                for line in infile:
                    file.write(line)
    file.write("#include \"" + outname + ".h\"\n")

for fname in args.inputs:
    if not "util.h" in fname:
        with open(fname) as infile:
            file.write("/*********************************** amalgamated original file \"" + fname + "\" ***********************************/\n")
            for line in infile:
                inc_res = include_re.match(line)
                guard_res = guard_re.match(line)
                if not inc_res and not guard_res:
                    file.write(line)

if not is_c:
    file.write('''
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* %s */''' % (outname.upper() + "_H_"))
file.close()
