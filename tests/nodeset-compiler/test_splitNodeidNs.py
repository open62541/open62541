#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS_DIR = os.path.join(HERE, "..", "..", "tools")
sys.path.insert(0, TOOLS_DIR)

from nodeset_compiler.backend_open62541_datatypes import splitNodeidNs


def main():
    failures = 0
    tests = [
        ("i=1234",          ("0", "i=1234")),
        ("s=MyNode",        ("0", "s=MyNode")),
        ("1234",            ("0", "1234")),
        ("ns=0;i=1234",     ("0", "i=1234")),
        ("ns=2;i=5678",     ("2", "i=5678")),
        ("ns=10;s=MyNode",  ("10", "s=MyNode")),
        ("ns=1;s=A;B",      ("1", "s=A;B")),
        ("",                ("0", "")),
        (None,              ("0", None)),
        ("g=72962B91-FA75-4AE6-9D28-B404DC7DAF63",
         ("0", "g=72962B91-FA75-4AE6-9D28-B404DC7DAF63")),
        ("ns=3;g=72962B91-FA75-4AE6-9D28-B404DC7DAF63",
         ("3", "g=72962B91-FA75-4AE6-9D28-B404DC7DAF63")),
        ("b=M/RbKBsRVkePCePcx24oRA==",
         ("0", "b=M/RbKBsRVkePCePcx24oRA==")),
        ("ns=5;b=M/RbKBsRVkePCePcx24oRA==",
         ("5", "b=M/RbKBsRVkePCePcx24oRA==")),
    ]

    for value, expected in tests:
        result = splitNodeidNs(value)
        if result != expected:
            print(f"FAIL: splitNodeidNs({value!r}) = {result!r}, "
                  f"expected {expected!r}")
            failures += 1
        else:
            print(f"PASS: splitNodeidNs({value!r}) = {result!r}")

    if failures == 0:
        print(f"\nAll {len(tests)} tests passed.")
    else:
        print(f"\n{failures}/{len(tests)} tests FAILED.")
    return failures


if __name__ == "__main__":
    sys.exit(main())
