#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Standalone tests for generate_datatypes.py. Does not need a built library.
#
# Cross-namespace type-name collisions:
#   1. Same-name type with IDENTICAL definition across namespaces
#      -> generator must succeed (exit 0).
#   2. Same-name type with DIFFERENT definition across namespaces
#      -> generator must fail (non-zero exit), because the same C typedef
#         cannot represent two different structures/enums.
#
# --namespaceMap (pins namespace indices, makes the type array const):
#   3. Pinning every namespace that contributes types is enough, even when
#      other namespaces are imported -> succeeds and the array is const.
#   4. A namespace uri that matches nothing -> must fail instead of silently
#      shifting the remaining indices.
#   5. A partially pinned array -> must fail, because the nodeset compiler
#      would overwrite the pinned indices at load time.
#   6. One index pinned to two namespaces -> must fail.

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

BASE_NS = "http://test.org/CrossNsBase/"
SAME_NS = "http://test.org/CrossNsSame/"
DIFF_NS = "http://test.org/CrossNsDiff/"


def run_generator(outdir, outname, type_bsd, spec_csv, extra_args=(), import_base=True):
    cmd = [sys.executable, GENERATOR]
    if import_base:
        cmd.append(f"--import=CROSS_NS_BASE#{BASE_BSD}")
    for f in type_bsd:
        cmd.append(f"--type-bsd={f}")
    cmd += [
        f"--type-csv={spec_csv}",
        "--no-builtin",
        *extra_args,
        os.path.join(outdir, outname),
    ]
    return subprocess.run(cmd, capture_output=True, text=True)


def is_const_array(outdir, outname):
    """Whether the generated type array was emitted const."""
    with open(os.path.join(outdir, outname + "_generated.h")) as f:
        return "_IS_CONST 1" in f.read()


def main():
    failures = 0

    def expect_ok(label, r):
        nonlocal failures
        if r.returncode != 0:
            print(f"FAIL: {label}, but generator exited {r.returncode}")
            print(r.stderr)
            failures += 1
            return False
        print(f"PASS: {label}")
        return True

    def expect_fail(label, r, expect_in_stderr):
        nonlocal failures
        if r.returncode == 0:
            print(f"FAIL: {label}, but generator succeeded")
            failures += 1
        elif expect_in_stderr not in r.stderr:
            print(f"FAIL: {label}, but the error did not mention "
                  f"{expect_in_stderr!r}:")
            print(r.stderr)
            failures += 1
        else:
            print(f"PASS: {label}")

    with tempfile.TemporaryDirectory() as tmpdir:

        # --- Test 1: same definition -> must succeed ---
        r = run_generator(tmpdir, "cross_ns_same", [SAME_BSD], SAME_CSV)
        expect_ok("same-definition cross-namespace type accepted", r)

        # --- Test 2: different definition -> must fail ---
        r = run_generator(tmpdir, "cross_ns_diff", [DIFF_BSD], DIFF_CSV)
        expect_fail("different-definition cross-namespace type rejected", r,
                    "different definition")

        # --- Test 3: pinning the contributing namespace is enough ---
        # The imported base namespace contributes no type to this array, so it
        # does not need to be pinned for the array to become const.
        r = run_generator(tmpdir, "ns_pinned", [SAME_BSD], SAME_CSV,
                          extra_args=[f"--namespaceMap=2:{SAME_NS}"])
        if expect_ok("pinning the contributing namespace accepted", r):
            if not is_const_array(tmpdir, "ns_pinned"):
                print("FAIL: fully pinned type array was not emitted const")
                failures += 1
            else:
                print("PASS: fully pinned type array is const")

        # Without pinning the same array stays mutable
        r = run_generator(tmpdir, "ns_unpinned", [SAME_BSD], SAME_CSV)
        if expect_ok("unpinned generation accepted", r):
            if is_const_array(tmpdir, "ns_unpinned"):
                print("FAIL: unpinned type array must not be const")
                failures += 1
            else:
                print("PASS: unpinned type array is mutable")

        # --- Test 4: namespace uri that matches nothing ---
        r = run_generator(tmpdir, "ns_unknown", [SAME_BSD], SAME_CSV,
                          extra_args=["--namespaceMap=2:http://test.org/Nope/"])
        expect_fail("unmatched --namespaceMap namespace rejected", r,
                    "do not appear in the generated type array")

        # --- Test 5: partially pinned array (two contributing namespaces) ---
        r = run_generator(tmpdir, "ns_partial", [BASE_BSD, DIFF_BSD], DIFF_CSV,
                          extra_args=[f"--namespaceMap=2:{BASE_NS}"],
                          import_base=False)
        expect_fail("partial --namespaceMap rejected", r,
                    "--namespaceMap is incomplete")

        # Pinning both contributing namespaces is accepted and const
        r = run_generator(tmpdir, "ns_both", [BASE_BSD, DIFF_BSD], DIFF_CSV,
                          extra_args=[f"--namespaceMap=2:{BASE_NS}",
                                      f"--namespaceMap=3:{DIFF_NS}"],
                          import_base=False)
        if expect_ok("fully pinned multi-namespace array accepted", r):
            if not is_const_array(tmpdir, "ns_both"):
                print("FAIL: fully pinned multi-namespace array was not const")
                failures += 1
            else:
                print("PASS: fully pinned multi-namespace array is const")

        # --- Test 6: one index pinned to two namespaces ---
        r = run_generator(tmpdir, "ns_dup", [BASE_BSD, DIFF_BSD], DIFF_CSV,
                          extra_args=[f"--namespaceMap=2:{BASE_NS}",
                                      f"--namespaceMap=2:{DIFF_NS}"],
                          import_base=False)
        expect_fail("duplicate pinned index rejected", r,
                    "more than one namespace")

        # --- Test 7: namespace 0 pinned somewhere other than index 0 ---
        r = run_generator(tmpdir, "ns_zero", [SAME_BSD], SAME_CSV,
                          extra_args=["--namespaceMap=5:http://opcfoundation.org/UA/"])
        expect_fail("relocating namespace 0 rejected", r,
                    "Namespace 0 is always at index 0")

        # Spelling out the default namespace 0 mapping stays valid
        r = run_generator(tmpdir, "ns_zero_ok", [SAME_BSD], SAME_CSV,
                          extra_args=["--namespaceMap=0:http://opcfoundation.org/UA/",
                                      f"--namespaceMap=2:{SAME_NS}"])
        if expect_ok("explicit namespace 0 mapping accepted", r):
            if not is_const_array(tmpdir, "ns_zero_ok"):
                print("FAIL: explicit namespace 0 mapping lost const")
                failures += 1
            else:
                print("PASS: explicit namespace 0 mapping is const")

    sys.exit(failures)


if __name__ == "__main__":
    main()
