#!/usr/bin/env python

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Check how generate_nodeid_header.py handles CSV rows that result in the same
# macro name. Usage: check_generate_nodeid_header.py path/to/generate_nodeid_header.py

import os
import shutil
import subprocess
import sys
import tempfile

generator = sys.argv[1]
tmpdir = tempfile.mkdtemp()
try:
    csv = os.path.join(tmpdir, "ids.csv")
    out = os.path.join(tmpdir, "ids")
    with open(csv, "w") as f:
        f.write("Unique,1,Object\n"
                "Same,2,Variable\n"
                "Same,2,Variable\n"        # identical duplicate
                "Conflict,3,Variable\n"
                "CONFLICT,4,Variable\n"    # same name after upper casing
                "Other,5,Object\n")

    p = subprocess.Popen([sys.executable, generator, csv, out, "TEST"],
                         stderr=subprocess.PIPE, universal_newlines=True)
    _, stderr = p.communicate()
    assert p.returncode == 0, stderr

    with open(out + ".h") as f:
        defines = [l.split()[1:3] for l in f if l.startswith("#define UA_TESTID_")]

    # Every macro exactly once. The unique names are unchanged.
    assert defines == [["UA_TESTID_UNIQUE", "1"],
                       ["UA_TESTID_SAME", "2"],
                       ["UA_TESTID_CONFLICT", "4"],
                       ["UA_TESTID_OTHER", "5"]], defines

    # The conflict is reported. The identical duplicate is not.
    assert "UA_TESTID_CONFLICT" in stderr, stderr
    assert "3, 4" in stderr, stderr
    assert "UA_TESTID_SAME" not in stderr, stderr
finally:
    shutil.rmtree(tmpdir)
