#!/usr/bin/env python

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this 
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import sys
import re

# Converts a header file to restructured text documentation
#
# All text in /** */ comments becomes restructured text. Everything else is
# included as a code-block with C syntax highlighting.
#
# The beginning and end of the header are removed.
# - Find the first /** */ comment -> start of the documentation
# - Find the last line beginning with "#ifdef" -> end of the documentation

remove_keyword = [" UA_EXPORT", " UA_FUNC_ATTR_WARN_UNUSED_RESULT",
                  " UA_FUNC_ATTR_MALLOC", " UA_RESTRICT "]

def clean_comment(line):
    m = re.search("^\s*(\* |/\*\* )(.*?)( \*/)?$", line)
    if not m:
        return "\n"
    return m.group(2) + "\n"

def clean_line(line):
    for keyword in remove_keyword:
        line = line.replace(keyword, "")
    return line

def comment_start(line):
    m = re.search("^\s*/\*\*[ \n]", line)
    if not m:
        return False
    return True

def comment_end(line):
    m = re.search(" \*/$", line)
    if not m:
        return False
    return True

def first_line(c):
    "Searches for the first comment"
    for i in range(len(c)):
        if comment_start(c[i]):
            return i
    return -1

def last_line(c):
    "Searches for the latest ifdef (closing the include guard)"
    reg = re.compile("^#ifdef")
    for i in range(len(c)-1,1,-1):
        if "_UA_END_DECLS" in c[i]:
            reg = re.compile("^_UA_END_DECLS")
    last = 1
    for i in range(len(c)-1,1,-1):
        m = reg.match(c[i])
        if m:
            last = i
            break
    # skip empty lines at the end
    for i in range(last-1,1,-1):
        if len(c[i].strip()) > 0:
            return i
    return len(c)-1

if len(sys.argv) < 2:
    print("Usage: python c2rst.py input.c/h output.rst")
    exit(0)

with open(sys.argv[1]) as f:
    c = f.readlines()

with open(sys.argv[2], 'w') as rst:
    in_doc = False
    last = last_line(c)
    for i in range(first_line(c), last+1):
        line = c[i]
        doc_start = False
        doc_end = False
        if in_doc:
            doc_end = comment_end(line)
            line = clean_comment(line)
        else:
            doc_start = comment_start(line)
            if doc_start:
                doc_end = comment_end(line)
                line = clean_comment(line)

        if doc_start:
            in_doc = True

        if not ((doc_start or doc_end) and line == "\n"):
            if not in_doc:
                line = "   " + line
            rst.write(clean_line(line))

        if doc_end and i < last:
            rst.write("\n.. code-block:: c\n\n")
            in_doc = False
