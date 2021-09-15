# open62541 nodeset Compiler

The nodeset compiler is a collection of Python scripts that can parse OPC UA XML Namespace definition files and transform them into a class representation. This facilitates both reprinting the namespace in a different non-XML format (such as C-Code or DOT) and analysis of the namespace structure.

The initial implementation has been contributed by a research project of the chair for Process Control Systems Engineering of the TU Dresden. It was not strictly speaking created as a C generator, but could be easily modified to fulfill this role for open62541. Later on it was extended and improved by the core developers of open62541.

# open62541 nodeset Resolver

The nodeset resolver can be used to identify dependencies between nodesets. The main purpose of this tool is to generate custom tailored, stripped-down namespace-zero nodesets.

## Usage
Use ``nodeset_resolver.py --help`` for a help output:
````
usage: nodeset_resolver.py [-h] [-e <existingNodeSetXML>] [-x <nodeSetXML>]
                           [-r <referenceNodeSetXML>] [-p] [-m] [-v]

optional arguments:
  -h, --help            show this help message and exit
  -e <existingNodeSetXML>, --existing <existingNodeSetXML>
                        NodeSet XML files with nodes that are already present
                        on the server.
  -x <nodeSetXML>, --xml <nodeSetXML>
                        NodeSet XML files with nodes that dependencies shall
                        be resolved for.
  -r <referenceNodeSetXML>, --ref <referenceNodeSetXML>
                        NodeSet XML file where missing dependencies are
                        resolved from.
  -p, --pull            Pull in and output missing Nodes from reference XML
  -m, --merge           Merge missing Nodes from reference NodeSet into
                        (first) existing-NodeSet
  -v, --verbose         Make the script more verbose. Can be applied up to 4
                        times

````

## Showing missing dependencies
Suppose you want to implement a OPC UA PA-DIM nodeset on an embedded platform.
Due to memory constraints, you are using the default reduced OPC UA Namespace-Zero nodeset.

````
nodeset_resolver.py -e ../../tools/schema/Opc.Ua.NodeSet2.Reduced.xml \
  -x .../deps/ua-nodeset/PADIM/Opc.Ua.IRDI.NodeSet2.xml \
  -x .../deps/ua-nodeset/PADIM/Opc.Ua.PADIM.NodeSet2.xml
````

The output shows a list of missing NodeIds, that can not be resolved from the supplied XML files. Ignoring ``ns=0``, the missing NodeIds can be solved by including the OPC UA DI model.
````
nodeset_resolver.py -e ../../tools/schema/Opc.Ua.NodeSet2.Reduced.xml \
  -x .../deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml \
  -x .../deps/ua-nodeset/PADIM/Opc.Ua.IRDI.NodeSet2.xml \
  -x .../deps/ua-nodeset/PADIM/Opc.Ua.PADIM.NodeSet2.xml
````

This time, only NodeIds with ``ns=0`` are list as unresolvable, because they are missing from the default reduced namespace-zero nodeset.
We can now include the full OPC UA namespace-zero nodeset as a reference file to resolve all missing dependencies.

````
nodeset_resolver.py -e ../../tools/schema/Opc.Ua.NodeSet2.Reduced.xml \
  -x .../deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml \
  -x .../deps/ua-nodeset/PADIM/Opc.Ua.IRDI.NodeSet2.xml \
  -x .../deps/ua-nodeset/PADIM/Opc.Ua.PADIM.NodeSet2.xml \
  -r .../deps/ua-nodeset/Schema/Opc.Ua.NodeSet2.xml
````

The output now shows all resolved NodeIds, that are required for namespace zero.

## Generating Custom NodeSets
You can now automatically generate an XML file with these resolved NodeIds by pulling in their definition from the reference XML file.
Additionally, you can automatically merge the resolved node definitions with the existing XML nodeset given with the ``-e`` parameter.

Using this feature, you can create a custom tailored namespace-zero XML file using the default reduced nodeset as a basis.

````
nodeset_resolver.py -e ../../tools/schema/Opc.Ua.NodeSet2.Reduced.xml \
  -x .../deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml \
  -x .../deps/ua-nodeset/PADIM/Opc.Ua.IRDI.NodeSet2.xml \
  -x .../deps/ua-nodeset/PADIM/Opc.Ua.PADIM.NodeSet2.xml \
  -r .../deps/ua-nodeset/Schema/Opc.Ua.NodeSet2.xml \
  -p -m > Opc.Ua.NodeSet2.Custom.xml
````

_Note_: In this scenario, the nodesets given with the ``-x`` parameter will later be used with the ``nodeset_compiler`` to produce the compiled code for implementing the OPC UA PA-DIM nodeset.

````
nodeset_compiler.py -e path/to/generated/Opc.Ua.NodeSet2.Custom.xml \
  -x .../deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml \
  -x .../deps/ua-nodeset/PADIM/Opc.Ua.IRDI.NodeSet2.xml \
  -x .../deps/ua-nodeset/PADIM/Opc.Ua.PADIM.NodeSet2.xml \
  PADIM_NodeSet
````

## Documentation

Usage documentation and How-Tos can be found on the webpage: <https://open62541.org/doc/current/nodeset_compiler.html>
