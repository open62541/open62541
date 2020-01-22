#!/usr/bin/env python

# coding: UTF-8
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this 
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

from __future__ import print_function
import argparse
import os.path
import re
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
include_re = re.compile("^#[\s]*include (\".*\").*$|^#[\s]*include (<open62541/.*>).*$")
guard_re = re.compile("^#(?:(?:ifndef|define)\s*[A-Z_]+_H_|endif /\* [A-Z_]+_H_ \*/|endif // [A-Z_]+_H_|endif\s*/\*\s*!?[A-Z_]+_H[_]+\s*\*/)")

print ("Starting amalgamating file "+ args.outfile)

file = io.open(args.outfile, 'wt', encoding='utf8', errors='replace')
file.write(u"""/* THIS IS A SINGLE-FILE DISTRIBUTION CONCATENATED FROM THE OPEN62541 SOURCES
 * visit http://open62541.org/ for information about this software
 * Git-Revision: %s
 */

/*
 * Copyright (C) 2014-2018 the contributors as stated in the AUTHORS file
 *
 * This file is part of open62541. open62541 is free software: you can
 * redistribute it and/or modify it under the terms of the Mozilla Public
 * License v2.0 as stated in the LICENSE file provided with open62541.
 *
 * open62541 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.
 */\n\n""" % args.version)

if is_c:
    file.write(u'''#ifndef UA_DYNAMIC_LINKING_EXPORT
# define UA_DYNAMIC_LINKING_EXPORT
# define MDNSD_DYNAMIC_LINKING
#endif

/* Disable security warnings for BSD sockets on MSVC */
#ifdef _MSC_VER
# define _CRT_SECURE_NO_WARNINGS
#endif

#include "%s.h"
''' % outname)
else:
    file.write(u'''#ifndef %s
#define %s
''' % (outname.upper() + u"_H_", outname.upper() + u"_H_"))

for fname in args.inputs:
    with io.open(fname, encoding='utf8', errors='replace') as infile:
        file.write(u"\n/*********************************** amalgamated original file \"" + fname + u"\" ***********************************/\n\n")
        print ("Integrating file '" + fname + "'...", end=""),
        for line in infile:
            inc_res = include_re.match(line)
            guard_res = guard_re.match(line)
            if not inc_res and not guard_res:
                file.write(line)
        # Ensure file is written to disk.
        file.flush()
        os.fsync(file.fileno())
        print ("done."),

if not is_c:
    file.write(u"#endif /* %s */\n" % (outname.upper() + u"_H_"))

# Ensure file is written to disk.
# See https://stackoverflow.com/questions/13761961/large-file-not-flushed-to-disk-immediately-after-calling-close
file.flush()
os.fsync(file.fileno())
file.close()

print ("The size of "+args.outfile+" is "+ str(os.path.getsize(args.outfile))+" Bytes.")
