The companion compiler tests use the same pinned NodeSet repository and the
same `catalog.txt` inventory as `check_nodeset_loader_all`. The inventory
explicitly distinguishes NodeSets from profile documents, which declare no
address-space nodes and therefore have no compiled model.

Configure with `UA_BUILD_UNIT_TESTS=ON`, `UA_BUILD_COMPANION_TESTS=ON` and
`UA_NAMESPACE_ZERO=FULL`. Build `check_companion_specs`, then run
`ctest --output-on-failure -L companion`. The Linux companion-spec CI job runs
these tests and the runtime loader's complete catalog test.

For each NodeSet, configuration resolves its RequiredModel dependency closure
against the bundled providers and creates:

* A generated datatype header/source and a namespace header/source.
* A separate C adapter and thin test header. Only the adapter includes that
  model's generated headers, so unrelated C typedefs cannot collide.
* One object library, reused by a standalone executable and the monster binary.
* Copy-mode and in-place standalone tests, and a copy-mode test using the
  monster binary with the same model selected.

The monster binary links every catalog module. Its shared-server scenarios load
one preferred provider per model URI (the newest bundled publication date).
Other revisions remain covered by the individual executables and monster
`--case` tests. A required revision newer than the available provider is an
error. `dependency_overrides.json` records narrowly scoped upstream metadata
inconsistencies, including exact dates and versions; stale exceptions fail
configuration.

Every scenario loads the generated model and an XML reference server. Source
node availability and declared attributes, variable shape, values and datatype
definitions with explicit fields are compared after normalizing namespace
indexes by URI. Their native memory layouts are compared too. Simple aliases
retain their builtin C representation and have separate metadata descriptors.
Empty abstract definitions have no exported C structure and are checked as
namespace nodes. Nodes absent in both loaders are reported, since successful
XML loading can include rejected nodes. Both modes additionally load a second
live generated server with the selected modules requested in reverse order.
Copy mode inserts unrelated namespaces;
static registration preloads the common compile-time namespace plan in both
servers. All descriptor arrays are const and can be shared.

Legal structure field ranks that the SDK cannot represent natively generate a
warning. Their datatype namespace nodes remain present, while their native
descriptors and native layouts depending on them are omitted. Polymorphic
ExtensionObject members remain representable.
