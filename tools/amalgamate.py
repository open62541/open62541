from __future__ import print_function
import re
import argparse
import os.path
import io

parser = argparse.ArgumentParser()
parser.add_argument('version', help='file version')
parser.add_argument('outfile', help='outfile with extension .c/.h')
parser.add_argument('inputs', nargs='*', action='store', help='input filenames')
args = parser.parse_args()

outname = args.outfile.split("/")[-1]
is_c = False
if outname[-2:] == ".c":
    is_c = True
pos = outname.find(".")
if pos > 0:
    outname = outname[:pos]
include_re = re.compile("^#include (\".*\").*$")
guard_re = re.compile("^#(?:(?:ifndef|define) [A-Z_]+_H_|endif /\* [A-Z_]+_H_ \*/|endif // [A-Z_]+_H_)")

print ("Starting amalgamating file "+ args.outfile)

file = io.open(args.outfile, 'w')
file.write(u"""/* THIS IS A SINGLE-FILE DISTRIBUTION CONCATENATED FROM THE OPEN62541 SOURCES
 * visit http://open62541.org/ for information about this software
 * Git-Revision: %s
 */

/*
 * Copyright (C) 2014-2016 the contributors as stated in the AUTHORS file
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
 */\n\n""" % args.version)

if is_c:
    file.write(u'''#ifndef UA_DYNAMIC_LINKING_EXPORT
# define UA_DYNAMIC_LINKING_EXPORT
#endif

#include "%s.h"
''' % outname)
else:
    file.write(u'''#ifndef %s
#define %s

#ifdef __cplusplus
extern "C" {
#endif\n''' % (outname.upper() + u"_H_", outname.upper() + u"_H_") )

for fname in args.inputs:
    with io.open(fname, encoding="utf8") as infile:
        file.write(u"\n/*********************************** amalgamated original file \"" + fname + u"\" ***********************************/\n\n")
        print ("Integrating file '" + fname + "'...", end=""),
        for line in infile:
            inc_res = include_re.match(line)
            guard_res = guard_re.match(line)
            if not inc_res and not guard_res:
                file.write(line)
        print ("done."),

if not is_c:
    file.write(u'''
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* %s */''' % (outname.upper() + u"_H_"))
file.close()

print ("The size of "+args.outfile+" is "+ str(os.path.getsize(args.outfile))+" Bytes.")
