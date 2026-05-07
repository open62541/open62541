#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Unit tests for the splitNodeidNs() helper in generate_datatypes.py.
# Covers all four OPC UA NodeId encoding types (OPC UA Part 6 §5.2.2):
#   i=  numeric      s=  string      g=  GUID      b=  ByteString (opaque)
# Because generate_datatypes.py calls argparse at module level, we
# cannot import it directly.  Instead we extract just the function
# definition and test the extracted callable.

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
GEN_SCRIPT = os.path.join(HERE, "..", "..", "tools", "generate_datatypes.py")


def _load_splitNodeidNs():
    """Extract the splitNodeidNs function from generate_datatypes.py
    without running the full script (which calls argparse)."""
    with open(GEN_SCRIPT, "r") as f:
        source = f.read()
    # Locate the function source
    start = source.index("def splitNodeidNs(")
    # Find the next top-level def/class or end of file
    rest = source[start:]
    lines = rest.split("\n")
    func_lines = [lines[0]]
    for line in lines[1:]:
        # Stop at the next non-indented, non-blank, non-comment line
        if line and not line[0].isspace() and not line.startswith("#"):
            break
        func_lines.append(line)
    ns = {}
    exec("\n".join(func_lines), ns)
    return ns["splitNodeidNs"]


def main():
    splitNodeidNs = _load_splitNodeidNs()
    failures = 0

    tests = [
        # (input, expected_output)
        ("i=1234",          ("0", "i=1234")),
        ("s=MyNode",        ("0", "s=MyNode")),
        ("1234",            ("0", "1234")),
        ("ns=0;i=1234",     ("0", "i=1234")),
        ("ns=2;i=5678",     ("2", "i=5678")),
        ("ns=10;s=MyNode",  ("10", "s=MyNode")),
        ("ns=1;s=A;B",      ("1", "s=A;B")),    # only first semicolon splits
        ("",                ("0", "")),
        (None,              ("0", None)),
        # GUID NodeIds  (OPC UA Part 6 §5.2.2, type g=)
        ("g=72962B91-FA75-4AE6-9D28-B404DC7DAF63",
         ("0", "g=72962B91-FA75-4AE6-9D28-B404DC7DAF63")),
        ("ns=3;g=72962B91-FA75-4AE6-9D28-B404DC7DAF63",
         ("3", "g=72962B91-FA75-4AE6-9D28-B404DC7DAF63")),
        # ByteString / Opaque NodeIds  (OPC UA Part 6 §5.2.2, type b=)
        ("b=M/RbKBsRVkePCePcx24oRA==",
         ("0", "b=M/RbKBsRVkePCePcx24oRA==")),
        ("ns=5;b=M/RbKBsRVkePCePcx24oRA==",
         ("5", "b=M/RbKBsRVkePCePcx24oRA==")),
    ]

    for inp, expected in tests:
        result = splitNodeidNs(inp)
        if result != expected:
            print(f"FAIL: splitNodeidNs({inp!r}) = {result!r}, expected {expected!r}")
            failures += 1
        else:
            print(f"PASS: splitNodeidNs({inp!r}) = {result!r}")

    if failures == 0:
        print(f"\nAll {len(tests)} tests passed.")
    else:
        print(f"\n{failures}/{len(tests)} tests FAILED.")

    sys.exit(failures)


if __name__ == "__main__":
    main()
