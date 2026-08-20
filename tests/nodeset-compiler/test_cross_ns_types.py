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
TOOLS_DIR = os.path.join(HERE, "..", "..", "tools")
GENERATOR = os.path.join(TOOLS_DIR, "generate_datatypes.py")
sys.path.insert(0, TOOLS_DIR)

from nodeset_compiler.backend_open62541_datatypes import portable_basename

BASE_BSD = os.path.join(HERE, "cross_ns_base.bsd")
SAME_BSD = os.path.join(HERE, "cross_ns_same.bsd")
SAME_CSV = os.path.join(HERE, "cross_ns_same.csv")
DIFF_BSD = os.path.join(HERE, "cross_ns_diff.bsd")
DIFF_CSV = os.path.join(HERE, "cross_ns_diff.csv")


def run_generator(spec_bsd, spec_csv, outdir, outname):
    """Run the public legacy generator for one cross-namespace fixture."""
    cmd = [
        sys.executable, GENERATOR,
        f"--import=CROSS_NS_BASE#{BASE_BSD}",
        f"--type-bsd={spec_bsd}",
        f"--type-csv={spec_csv}",
        "--no-builtin",
        os.path.join(outdir, outname),
    ]
    return subprocess.run(cmd, capture_output=True, text=True)


def require_generated_legacy_output(outdir, outname):
    """Check the public declarations that must survive backend refactoring."""
    header_path = os.path.join(outdir, f"{outname}_generated.h")
    source_path = os.path.join(outdir, f"{outname}_generated.c")
    if not os.path.exists(header_path) or not os.path.exists(source_path):
        return "legacy generator did not create both output files"
    with open(header_path, encoding="utf-8") as f:
        header = f.read()
    with open(source_path, encoding="utf-8") as f:
        source = f.read()
    expected = [
        ("UA_CROSS_NS_SAME_SHAREDENUM", header),
        ("UA_DataType UA_CROSS_NS_SAME", source),
        ('UA_TYPENAME("Val0")', source),
        ('UA_TYPENAME("Val1")', source),
        ("UA_DATATYPEKIND_ENUM", source),
    ]
    missing = [text for text, contents in expected if text not in contents]
    if missing:
        return "legacy BSD output changed; missing: " + ", ".join(missing)
    return None


def main():
    failures = 0

    # Output names must not inherit a Windows drive or parent directories.
    windows_output = r"C:\Temp\generated\cross_ns_same"
    if portable_basename(windows_output) != "cross_ns_same":
        print("FAIL: generated output basename is not portable")
        failures += 1

    with tempfile.TemporaryDirectory() as tmpdir:

        # --- Test 1: same definition → must succeed ---
        r = run_generator(SAME_BSD, SAME_CSV, tmpdir, "cross_ns_same")
        if r.returncode != 0:
            print("FAIL: same-definition cross-namespace type should be accepted, "
                  f"but generator exited {r.returncode}")
            print(r.stderr)
            failures += 1
        else:
            error = require_generated_legacy_output(
                tmpdir, "cross_ns_same")
            if error:
                print(f"FAIL: {error}")
                failures += 1
            else:
                print("PASS: complete legacy BSD output is compatible")

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
