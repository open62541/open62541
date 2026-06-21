#!/usr/bin/env python3

# coding: UTF-8
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# This script checks the valgrind output for errors.
# The track-fds does not cause an error if there are too many FDs open,
# therefore we parse the output and fail if there are open FDs

import sys
import subprocess
import os.path
import re
import os

logfile = sys.argv[1]

valgrind_command = ' '.join('"' + item + '"' for item in sys.argv[2:])

# Execute a command and output its stdout text
def execute(command):

    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    # Poll process for new output until finished
    while True:
        nextline = process.stdout.readline().decode('utf-8')
        if nextline == '' and process.poll() is not None:
            break
        sys.stdout.write(nextline)
        sys.stdout.flush()

    return process.returncode

ret_code = execute(valgrind_command)

if not os.path.isfile(logfile):
    print("### PYTHON ERROR: Valgrind logfile does not exist: " + logfile)
    exit(1)

log_content = ""
with open(logfile) as content_file:
    log_content = content_file.read()

# Valgrind 3.26 with --quiet writes an empty logfile on success. If
# valgrind itself returned 0 (no errors, no leaks), treat the run as
# successful regardless of whether the descriptor block was printed.
# The descriptor block is only informative for --track-fds=yes.
if len(log_content) == 0:
    if ret_code == 0:
        exit(0)
    print("### PYTHON ERROR: Valgrind logfile is empty and valgrind"
          " exited with code " + str(ret_code) + ": " + logfile)
    exit(1)

# Remove output of possible bug in OSX
# --16672-- run: /usr/bin/dsymutil "/Users/travis/build/Pro/open62541/build/bin/tests/check_utils"
# --16672-- WARNING: unhandled amd64-darwin syscall: unix:464
# --16672-- You may be able to write your own handler.
# --16672-- Read the file README_MISSING_SYSCALL_OR_IOCTL.
# --16672-- Nevertheless we consider this a bug.  Please report
# --16672-- it at http://valgrind.org/support/bug_reports.html.
replace_re = re.compile(r"^--(\d+)--\s+run: .*-- it at http://valgrind.org/support/bug_reports.html\.$\n", re.MULTILINE | re.DOTALL)
log_content = replace_re.sub('', log_content)

# Try to parse the output. Look for the following line:
# ==17054== FILE DESCRIPTORS: 5 open at exit.
# or in newer valgrind:
# ==17054== FILE DESCRIPTORS: 5 open (3 inherited) at exit.
# With newer valgrind and --quiet, this block may be missing even on
# a clean run. Treat as a pass when valgrind returned 0.
descriptors_re = re.compile(r"==(\d+)==\s+FILE DESCRIPTORS:\s+(\d+) open(?:\s+\(\d+\s+\w+\))?\s+at exit\.")
m = descriptors_re.search(log_content)

if not m:
    if ret_code == 0:
        exit(0)
    print("### PYTHON ERROR: File descriptors header not found: " + logfile)
    print(log_content)
    exit(1)

log_content = descriptors_re.sub('', log_content)

valgrind_number = m.group(1)
open_count = int(m.group(2))

# Remove the open file descriptors which are inherited from parent. they look like this:
#==21343== Open file descriptor 3: /home/user/open62541/build/bin/tests/discovery.log
#==21343==    <inherited from parent>
#==21343==
#==21343== Open file descriptor 2:
#==21343==    <inherited from parent>
#==21343==
#==21343== Open file descriptor 1:
#==21343==    <inherited from parent>
#==21343==
#==21343== Open file descriptor 0: /dev/pts/1
#==21343==    <inherited from parent>

replace_re = re.compile(r"^==" + str(valgrind_number) + r"==\s+Open .*$\n" +
                        r"^==" + str(valgrind_number) + r"==\s+<inherited from parent>$\n" +
                        r"(^==" + str(valgrind_number) + r"==\s+$\n)*", re.MULTILINE)
log_content = replace_re.sub('', log_content)

# The check library opens a tmp file (e.g. /tmp/check_XXXXXX) for
# per-test IPC and never closes it. Newer valgrind with --track-fds=yes
# reports it as an "Open file descriptor" entry that is NOT marked as
# inherited. Strip the whole block (header + frame lines).
log_content = re.sub(
    r"^==\d+==\s+Open file descriptor \d+: /tmp/check_\w+$\n"
    r"(?:^==\d+==\s+.*\n)*"
    r"(?:^==\d+==\s*$\n)*",
    "", log_content, flags=re.MULTILINE)

# Valgrind detected a memleak if ret_code != 0
if ret_code != 0:
    print(log_content)
    exit(ret_code)

# No issues by valgrind
if len(log_content) == 0 or log_content.isspace():
    exit(0)

# There is something fishy in the valgrind output, so error-exit
print(log_content)
exit(1)
