# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
"""Generate separate NodeSet modules under one frozen namespace plan."""
import json
from pathlib import Path
import re
import subprocess
import sys

from .namespace_plan import parse_namespace_plan
from .nodeset import extractNamespaces


def generate_batch(args):
    if (args.infiles or args.typesOutput or args.bsdFile or args.blacklistFiles or
            args.ignoreFiles or args.types_array_for):
        raise ValueError("--batch cannot be combined with per-module input or output options")
    if args.backend != "open62541":
        raise ValueError("--batch requires the open62541 backend")
    manifest = Path(args.batch).resolve()
    modules = json.loads(manifest.read_text(encoding="utf-8"))["modules"]
    if not modules:
        raise ValueError("Batch manifest has no modules")
    by_name = {}
    for module in modules:
        name = module["name"]
        if not re.fullmatch(r"[a-z][a-z0-9_]*", name) or name in by_name:
            raise ValueError("Invalid or duplicate module name: " + name)
        module["xml"] = [(manifest.parent / path).resolve() for path in module["xml"]]
        if not module["xml"]:
            raise ValueError("Module has no XML inputs: " + name)
        by_name[name] = module

    # Validate the graph before writing any generated output.
    ordered, active, done = [], set(), set()

    def visit(name):
        if name in active:
            raise ValueError("NodeSet dependency cycle at " + name)
        if name in done:
            return
        if name not in by_name:
            raise ValueError("Unknown NodeSet dependency: " + name)
        active.add(name)
        for dep in by_name[name].get("depends", []):
            visit(dep)
        active.remove(name)
        done.add(name)
        ordered.append(by_name[name])

    for name in by_name:
        visit(name)

    plan = parse_namespace_plan(args.namespace_map)
    used = set(plan.values())
    inputs = [Path(f.name).resolve() for f in args.existing]
    inputs += [path for module in ordered for path in module["xml"]]
    for path in dict.fromkeys(inputs):
        with path.open("rb") as source:
            for uri in extractNamespaces(source):
                if not uri:
                    raise ValueError("Empty namespace URI in " + str(path))
                if uri in plan:
                    continue
                index = next((i for i in range(1, 65536) if i not in used), None)
                if index is None:
                    raise ValueError("Too many namespaces in batch")
                plan[uri] = index
                used.add(index)

    output = Path(args.outputFile).resolve()
    output.mkdir(parents=True, exist_ok=True)
    (output / "namespace_plan.json").write_text(
        json.dumps(plan, indent=2) + "\n", encoding="utf-8")
    common = [sys.executable, str(Path(__file__).with_name("nodeset_compiler.py"))]
    common += ["--namespaceMap=%d:%s" % (index, uri) for uri, index in plan.items()]
    if args.typesSchema:
        common += ["--types-schema", args.typesSchema]
    if args.internal_headers:
        common.append("--internal-headers")
    for source in args.existing:
        common += ["--existing", str(Path(source.name).resolve())]
    for array in args.typesArray:
        common += ["--types-array", array]

    for module in ordered:
        dependencies = set()

        def collect(name):
            for dep in by_name[name].get("depends", []):
                if dep not in dependencies:
                    dependencies.add(dep)
                    collect(dep)

        collect(module["name"])
        command = list(common)
        for dep in ordered:
            if dep["name"] not in dependencies:
                continue
            for path in dep["xml"]:
                command += ["--existing", str(path)]
                command += ["--types-array-for", str(path),
                            "UA_TYPES_" + dep["name"].upper()]
        for path in module["xml"]:
            command += ["--xml", str(path)]
        command += ["--types-output", str(output / ("types_" + module["name"])),
                    str(output / ("namespace_" + module["name"] + "_generated"))]
        subprocess.run(command, check=True)
