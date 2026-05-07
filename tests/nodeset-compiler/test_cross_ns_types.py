#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Test cross-namespace type-name collision handling in generate_datatypes.py.
#
# Two scenarios are exercised:
#   1. Same-name type with IDENTICAL definition across namespaces
#      -> generator must succeed (exit 0).
#   2. Same-name type with DIFFERENT definition across namespaces
#      -> generator must fail (non-zero exit), because the same C typedef
#         cannot represent two different structures/enums.

import os
import sys
import subprocess
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
GENERATOR = os.path.join(HERE, "..", "..", "tools", "generate_datatypes.py")

BASE_BSD = os.path.join(HERE, "cross_ns_base.bsd")
SAME_BSD = os.path.join(HERE, "cross_ns_same.bsd")
SAME_CSV = os.path.join(HERE, "cross_ns_same.csv")
DIFF_BSD = os.path.join(HERE, "cross_ns_diff.bsd")
DIFF_CSV = os.path.join(HERE, "cross_ns_diff.csv")


def run_generator(spec_bsd, spec_csv, outdir, outname):
    cmd = [
        sys.executable, GENERATOR,
        f"--import=CROSS_NS_BASE#{BASE_BSD}",
        f"--type-bsd={spec_bsd}",
        f"--type-csv={spec_csv}",
        "--no-builtin",
        os.path.join(outdir, outname),
    ]
    return subprocess.run(cmd, capture_output=True, text=True)


def main():
    failures = 0

    with tempfile.TemporaryDirectory() as tmpdir:

        # --- Test 1: same definition → must succeed ---
        r = run_generator(SAME_BSD, SAME_CSV, tmpdir, "cross_ns_same")
        if r.returncode != 0:
            print("FAIL: same-definition cross-namespace type should be accepted, "
                  f"but generator exited {r.returncode}")
            print(r.stderr)
            failures += 1
        else:
            print("PASS: same-definition cross-namespace type accepted")

        # --- Test 2: different definition → must fail ---
        r = run_generator(DIFF_BSD, DIFF_CSV, tmpdir, "cross_ns_diff")
        if r.returncode == 0:
            print("FAIL: different-definition cross-namespace type should be "
                  "rejected, but generator succeeded")
            failures += 1
        else:
            print("PASS: different-definition cross-namespace type rejected")

    sys.exit(failures)


if __name__ == "__main__":
    main()
