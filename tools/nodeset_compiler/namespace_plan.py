# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
"""Compile-time namespace identities shared by nodes and native datatypes."""
import hashlib
import json

UA_NAMESPACE = "http://opcfoundation.org/UA/"


def parse_namespace_plan(entries):
    plan = {UA_NAMESPACE: 0}
    indexes = {0: UA_NAMESPACE}
    for entry in entries:
        index, uri = entry.split(":", 1)
        index = int(index)
        if not uri or not 0 <= index <= 65535:
            raise ValueError("Invalid namespace mapping: " + entry)
        if uri in plan and plan[uri] != index:
            raise ValueError("Conflicting namespace index for " + uri)
        if index in indexes and indexes[index] != uri:
            raise ValueError("Namespace index %d belongs to more than one URI" % index)
        plan[uri], indexes[index] = index, uri
    return plan


def namespace_plan_checks(plan):
    """Reject conflicting imported plans at compile time, without runtime data."""
    lines = []
    for uri, index in sorted(plan.items(), key=lambda item: item[1]):
        if index == 0:
            continue
        macro = "UA_NODESET_NS_" + hashlib.sha256(uri.encode("utf-8")).hexdigest().upper()
        lines.extend(("#ifndef " + macro,
                      "#define " + macro + " " + str(index),
                      "#elif " + macro + " != " + str(index),
                      "#error " + json.dumps("Inconsistent generated namespace plan for " + uri),
                      "#endif"))
    return "\n".join(lines)
