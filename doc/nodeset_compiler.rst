XML Nodeset Compiler
--------------------

When writing an application, it is more comfortable to create information models using some GUI tools. Most tools can export data according the OPC UA Nodeset XML schema. open62541 contains a Python based nodeset compiler that can transform these information model definitions into a working server.

Note that the nodeset compiler you can find in the *tools/nodeset_compiler* subfolder is *not* an XML transformation tool but a compiler. That means that it will create an internal representation when parsing the XML files and attempt to understand and verify the correctness of this representation in order to generate C Code.

Getting started
...............

We take the following information model snippet as the starting point of the following tutorial. A more detailed tutorial on how to create your own information model and NodeSet2.xml can be found in this blog post: https://opcua.rocks/custom-information-models/

.. code-block:: xml

    <UANodeSet xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
               xmlns:uax="http://opcfoundation.org/UA/2008/02/Types.xsd"
               xmlns="http://opcfoundation.org/UA/2011/03/UANodeSet.xsd"
               xmlns:s1="http://yourorganisation.org/example_nodeset/"
               xmlns:xsd="http://www.w3.org/2001/XMLSchema">
        <NamespaceUris>
            <Uri>http://yourorganisation.org/example_nodeset/</Uri>
        </NamespaceUris>
        <Aliases>
            <Alias Alias="Boolean">i=1</Alias>
            <Alias Alias="UInt32">i=7</Alias>
            <Alias Alias="String">i=12</Alias>
            <Alias Alias="HasModellingRule">i=37</Alias>
            <Alias Alias="HasTypeDefinition">i=40</Alias>
            <Alias Alias="HasSubtype">i=45</Alias>
            <Alias Alias="HasProperty">i=46</Alias>
            <Alias Alias="HasComponent">i=47</Alias>
            <Alias Alias="Argument">i=296</Alias>
        </Aliases>
        <Extensions>
            <Extension>
                <ModelInfo Tool="UaModeler" Hash="Zs8w1AQI71W8P/GOk3k/xQ=="
                           Version="1.3.4"/>
            </Extension>
        </Extensions>
        <UAReferenceType NodeId="ns=1;i=4001" BrowseName="1:providesInputTo">
            <DisplayName>providesInputTo</DisplayName>
            <References>
                <Reference ReferenceType="HasSubtype" IsForward="false">
                    i=33
                </Reference>
            </References>
            <InverseName Locale="en-US">inputProcidedBy</InverseName>
        </UAReferenceType>
        <UAObjectType IsAbstract="true" NodeId="ns=1;i=1001"
                      BrowseName="1:FieldDevice">
            <DisplayName>FieldDevice</DisplayName>
            <References>
                <Reference ReferenceType="HasSubtype" IsForward="false">
                    i=58
                </Reference>
                <Reference ReferenceType="HasComponent">ns=1;i=6001</Reference>
                <Reference ReferenceType="HasComponent">ns=1;i=6002</Reference>
            </References>
        </UAObjectType>
        <UAVariable DataType="String" ParentNodeId="ns=1;i=1001"
                    NodeId="ns=1;i=6001" BrowseName="1:ManufacturerName"
                    UserAccessLevel="3" AccessLevel="3">
            <DisplayName>ManufacturerName</DisplayName>
            <References>
                <Reference ReferenceType="HasTypeDefinition">i=63</Reference>
                <Reference ReferenceType="HasModellingRule">i=78</Reference>
                <Reference ReferenceType="HasComponent" IsForward="false">
                    ns=1;i=1001
                </Reference>
            </References>
        </UAVariable>
        <UAVariable DataType="String" ParentNodeId="ns=1;i=1001"
                    NodeId="ns=1;i=6002" BrowseName="1:ModelName"
                    UserAccessLevel="3" AccessLevel="3">
            <DisplayName>ModelName</DisplayName>
            <References>
                <Reference ReferenceType="HasTypeDefinition">i=63</Reference>
                <Reference ReferenceType="HasModellingRule">i=78</Reference>
                <Reference ReferenceType="HasComponent" IsForward="false">
                    ns=1;i=1001
                </Reference>
            </References>
        </UAVariable>
        <UAObjectType NodeId="ns=1;i=1002" BrowseName="1:Pump">
            <DisplayName>Pump</DisplayName>
            <References>
                <Reference ReferenceType="HasComponent">ns=1;i=6003</Reference>
                <Reference ReferenceType="HasComponent">ns=1;i=6004</Reference>
                <Reference ReferenceType="HasSubtype" IsForward="false">
                    ns=1;i=1001
                </Reference>
                <Reference ReferenceType="HasComponent">ns=1;i=7001</Reference>
                <Reference ReferenceType="HasComponent">ns=1;i=7002</Reference>
            </References>
        </UAObjectType>
        <UAVariable DataType="Boolean" ParentNodeId="ns=1;i=1002"
                    NodeId="ns=1;i=6003" BrowseName="1:isOn" UserAccessLevel="3"
                    AccessLevel="3">
            <DisplayName>isOn</DisplayName>
            <References>
                <Reference ReferenceType="HasTypeDefinition">i=63</Reference>
                <Reference ReferenceType="HasModellingRule">i=78</Reference>
                <Reference ReferenceType="HasComponent" IsForward="false">
                    ns=1;i=1002
                </Reference>
            </References>
        </UAVariable>
        <UAVariable DataType="UInt32" ParentNodeId="ns=1;i=1002"
                    NodeId="ns=1;i=6004" BrowseName="1:MotorRPM"
                    UserAccessLevel="3" AccessLevel="3">
            <DisplayName>MotorRPM</DisplayName>
            <References>
                <Reference ReferenceType="HasTypeDefinition">i=63</Reference>
                <Reference ReferenceType="HasModellingRule">i=78</Reference>
                <Reference ReferenceType="HasComponent" IsForward="false">
                    ns=1;i=1002
                </Reference>
            </References>
        </UAVariable>
        <UAMethod ParentNodeId="ns=1;i=1002" NodeId="ns=1;i=7001"
                  BrowseName="1:startPump">
            <DisplayName>startPump</DisplayName>
            <References>
                <Reference ReferenceType="HasModellingRule">i=78</Reference>
                <Reference ReferenceType="HasProperty">ns=1;i=6005</Reference>
                <Reference ReferenceType="HasComponent" IsForward="false">
                    ns=1;i=1002
                </Reference>
            </References>
        </UAMethod>
        <UAVariable DataType="Argument" ParentNodeId="ns=1;i=7001" ValueRank="1"
                    NodeId="ns=1;i=6005" ArrayDimensions="1"
                    BrowseName="OutputArguments">
            <DisplayName>OutputArguments</DisplayName>
            <References>
                <Reference ReferenceType="HasModellingRule">i=78</Reference>
                <Reference ReferenceType="HasProperty"
                           IsForward="false">ns=1;i=7001</Reference>
                <Reference ReferenceType="HasTypeDefinition">i=68</Reference>
            </References>
            <Value>
                <ListOfExtensionObject>
                    <ExtensionObject>
                        <TypeId>
                            <Identifier>i=297</Identifier>
                        </TypeId>
                        <Body>
                            <Argument>
                                <Name>started</Name>
                                <DataType>
                                    <Identifier>i=1</Identifier>
                                </DataType>
                                <ValueRank>-1</ValueRank>
                                <ArrayDimensions></ArrayDimensions>
                                <Description/>
                            </Argument>
                        </Body>
                    </ExtensionObject>
                </ListOfExtensionObject>
            </Value>
        </UAVariable>
        <UAMethod ParentNodeId="ns=1;i=1002" NodeId="ns=1;i=7002"
                  BrowseName="1:stopPump">
            <DisplayName>stopPump</DisplayName>
            <References>
                <Reference ReferenceType="HasModellingRule">i=78</Reference>
                <Reference ReferenceType="HasProperty">ns=1;i=6006</Reference>
                <Reference ReferenceType="HasComponent"
                           IsForward="false">ns=1;i=1002</Reference>
            </References>
        </UAMethod>
        <UAVariable DataType="Argument" ParentNodeId="ns=1;i=7002" ValueRank="1"
                    NodeId="ns=1;i=6006" ArrayDimensions="1"
                    BrowseName="OutputArguments">
            <DisplayName>OutputArguments</DisplayName>
            <References>
                <Reference ReferenceType="HasModellingRule">i=78</Reference>
                <Reference ReferenceType="HasProperty" IsForward="false">
                    ns=1;i=7002
                </Reference>
                <Reference ReferenceType="HasTypeDefinition">i=68</Reference>
            </References>
            <Value>
                <ListOfExtensionObject>
                    <ExtensionObject>
                        <TypeId>
                            <Identifier>i=297</Identifier>
                        </TypeId>
                        <Body>
                            <Argument>
                                <Name>stopped</Name>
                                <DataType>
                                    <Identifier>i=1</Identifier>
                                </DataType>
                                <ValueRank>-1</ValueRank>
                                <ArrayDimensions></ArrayDimensions>
                                <Description/>
                            </Argument>
                        </Body>
                    </ExtensionObject>
                </ListOfExtensionObject>
            </Value>
        </UAVariable>
    </UANodeSet>

Take the previous snippet and save it to a file ``myNS.xml``. To compile this nodeset into the corresponding C code, which can then be used by the open62541 stack, the nodeset compiler needs some arguments when you call it. The output of the help command gives you the following info:

.. code-block:: bash

    $ python ./nodeset_compiler.py -h
    usage: nodeset_compiler.py [-h] [-e <existingNodeSetXML>] [-x <nodeSetXML>]
                               [--internal-headers]
                               [-b <blacklistFile>] [-i <ignoreFile>]
                               [-t <typesArray>]
                               [-v]
                               <outputFile>

    positional arguments:
      <outputFile>          The path/basename for the <output file>.c and <output
                            file>.h files to be generated. This will also be the
                            function name used in the header and c-file.

    optional arguments:
      -h, --help            show this help message and exit
      -e <existingNodeSetXML>, --existing <existingNodeSetXML>
                            NodeSet XML files with nodes that are already present
                            on the server.
      -x <nodeSetXML>, --xml <nodeSetXML>
                            NodeSet XML files with nodes that shall be generated.
      --internal-headers    Include internal headers instead of amalgamated header
      -b <blacklistFile>, --blacklist <blacklistFile>
                            Loads a list of NodeIDs stored in blacklistFile (one
                            NodeID per line). Any of the nodeIds encountered in
                            this file will be removed from the nodeset prior to
                            compilation. Any references to these nodes will also
                            be removed
      -i <ignoreFile>, --ignore <ignoreFile>
                            Loads a list of NodeIDs stored in ignoreFile (one
                            NodeID per line). Any of the nodeIds encountered in
                            this file will be kept in the nodestore but not
                            printed in the generated code
      -t <typesArray>, --types-array <typesArray>
                            Types array for the given namespace. Can be used
                            mutliple times to define (in the same order as the
                            .xml files, first for --existing, then --xml) the type
                            arrays
      --max-string-length MAX_STRING_LENGTH
                            Maximum allowed length of a string literal. If longer,
                            it will be set to an empty string
      -v, --verbose         Make the script more verbose. Can be applied up to 4
                            times

So the resulting call looks like this:

.. code-block:: bash

    $ python ./nodeset_compiler.py --types-array=UA_TYPES --existing ../../deps/ua-nodeset/Schema/Opc.Ua.NodeSet2.xml --xml myNS.xml myNS

And the output of the command:

.. code-block:: bash

    INFO:__main__:Preprocessing (existing) ../../deps/ua-nodeset/Schema/Opc.Ua.NodeSet2.xml
    INFO:__main__:Preprocessing myNS.xml
    INFO:__main__:Generating Code
    INFO:__main__:NodeSet generation code successfully printed

The first argument ``--types-array=UA_TYPES`` defines the name of the global array in open62541 which contains the corresponding types used within the nodeset in ``NodeSet2.xml``. If you do not define your own datatypes, you can always use the ``UA_TYPES`` value. More on that later in this tutorial.
The next argument ``--existing ../../deps/ua-nodeset/Schema/Opc.Ua.NodeSet2.xml`` points to the XML definition of the standard-defined namespace 0 (NS0). Namespace 0 is assumed to be loaded beforehand and provides definitions for data type, reference types, and so. Since we reference nodes from NS0 in our myNS.xml we need to tell the nodeset compiler that it should also load that nodeset, but not compile it into the output.
Note that you may need to initialize the git submodule to get the ``deps/ua-nodeset`` folder (``git submodule update --init``) or download the full ``NodeSet2.xml`` manually.
The argument ``--xml myNS.xml`` points to the user-defined information model, whose nodes will be added to the abstract syntax tree. The script will then create the files ``myNS.c`` and ``myNS.h`` (indicated by the last argument ``myNS``) containing the C code necessary to instantiate those namespaces.

Although it is possible to run the compiler this way, it is highly discouraged. If you care to examine the CMakeLists.txt (examples/nodeset/CMakeLists.txt), you will find out that the file ``server_nodeset.xml`` is compiled using the following function::

    ua_generate_nodeset(
        NAME "example"
        FILE "${PROJECT_SOURCE_DIR}/examples/nodeset/server_nodeset.xml"
        DEPENDS_TYPES "UA_TYPES"
        DEPENDS_NS    "${UA_FILE_NS0}"
    )

If you look into the files generated by the nodeset compiler, you will see that it generated a method called ``extern UA_StatusCode myNS(UA_Server *server);``. You need to include the header and source file and then call the ``myNS(server)`` method right after creating the server instance with ``UA_Server_new``. This will automatically add all the nodes to the server and return ``UA_STATUSCODE_GOOD`` if there weren't any errors. Additionally you need to compile the open62541 stack with the full NS0 by setting ``UA_NAMESPACE_ZERO=FULL`` in CMake. Otherwise the stack uses a subset where many nodes are not included and thus adding a custom nodeset may fail.

This is how you can use the nodeset compiler to compile simple NodeSet XMLs to be used by the open62541 stack.

For your convenience and for simpler use we also provide a CMake function which simplifies the use of the ``ua_generate_datatypes`` and ``ua_generate_nodeset`` function even more.
It is highly recommended to use this function: ``ua_generate_nodeset_and_datatypes``. It uses some best practice settings and you only need to pass a name, the namespace mapping ``NAMESPACE_MAP`` (as described further below) and the nodeset files.
Passing the .csv and .bsd files is optional and if not given, generating datatypes for that noteset will be skipped. You can also define dependencies between nodesets using the ``DEPENDS`` argument.

Here are some examples for the ``DI`` and ``PLCOpen`` nodesets::

    # Generate types and namespace for DI
    ua_generate_nodeset_and_datatypes(
        NAME "di"
        FILE_CSV "${UA_NODESET_DIR}/DI/OpcUaDiModel.csv"
        FILE_BSD "${UA_NODESET_DIR}/DI/Opc.Ua.Di.Types.bsd"
        NAMESPACE_MAP "2:http://opcfoundation.org/UA/DI/"
        FILE_NS "${UA_NODESET_DIR}/DI/Opc.Ua.Di.NodeSet2.xml"
    )

    # generate PLCopen namespace which is using DI
    ua_generate_nodeset_and_datatypes(
        NAME "plc"
        # PLCopen does not define custom types. Only generate the nodeset
        FILE_NS "${UA_NODESET_DIR}/PLCopen/Opc.Ua.PLCopen.NodeSet2_V1.02.xml"
        # PLCopen depends on the di nodeset, which must be generated before
        DEPENDS "di"
    )

Creating object instances
.........................

One of the key benefits of defining object types is being able to create object instances fairly easily. Object instantiation is handled automatically when the typedefinition NodeId points to a valid ObjectType node. All Attributes and Methods contained in the objectType definition will be instantiated along with the object node.

While variables are copied from the objectType definition (allowing the user for example to attach new dataSources to them), methods are always only linked. This paradigm is identical to languages like C++: The method called is always the same piece of code, but the first argument is a pointer to an object. Likewise, in OPC UA, only one methodCallback can be attached to a specific methodNode. If that methodNode is called, the parent objectId will be passed to the method - it is the methods job to derefence which object instance it belongs to in that moment.

Let's look at an example that will create a pump instance given the newly defined objectType from myNS.xml:

.. code-block:: c

    /* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
     * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

    #include <signal.h>
    #include <stdio.h>
    #include "open62541.h"

    /* Files myNS.h and myNS.c are created from myNS.xml */
    #include "myNS.h"

    UA_Boolean running = true;

    static void stopHandler(int sign) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
        running = false;
    }

    int main(int argc, char **argv) {
        signal(SIGINT, stopHandler);
        signal(SIGTERM, stopHandler);

        UA_Server *server = UA_Server_new();
        UA_ServerConfig_setDefault(UA_Server_getConfig(server));

        UA_StatusCode retval = myNS(server);
        /* Create nodes from nodeset */
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Could not add the example nodeset. "
                "Check previous output for any error.");
            retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
        } else {
            UA_NodeId createdNodeId;
            UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;

            object_attr.description = UA_LOCALIZEDTEXT("en-US", "A pump!");
            object_attr.displayName = UA_LOCALIZEDTEXT("en-US", "Pump1");

            // we assume that the myNS nodeset was added in namespace 2.
            // You should always use UA_Server_addNamespace to check what the
            // namespace index is for a given namespace URI. UA_Server_addNamespace
            // will just return the index if it is already added.
            UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 0),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                    UA_QUALIFIEDNAME(1, "Pump1"),
                                    UA_NODEID_NUMERIC(2, 1002),
                                    object_attr, NULL, &createdNodeId);


            retval = UA_Server_run(server, &running);
        }

        UA_Server_delete(server);
        return (int) retval;
    }

Make sure you have updated the headers and libs in your project, then recompile and run the server. Make especially sure you have added ``myNS.h`` to your include folder.

As you can see instantiating an object is not much different from creating an object node. The main difference is that you *must* use an objectType node as typeDefinition.

If you start the server and inspect the nodes with UA Expert, you will find the pump in the objects folder, which look like this :numref:`nodeset-compiler-pump`.

.. _nodeset-compiler-pump:

.. figure:: nodeset_compiler_pump.png
   :alt: Instantiated Pump Object with inherited children

   Instantiated Pump Object with inherited children

As you can see the pump has inherited its parents attributes (ManufacturerName and ModelName). Methods, in contrast to objects and variables, are never cloned but instead only linked. The reason is that you will quite propably attach a method callback to a central method, not each object. Objects are instantiated if they are *below* the object you are creating, so any object (like an object called associatedServer of ServerType) that is part of pump will be instantiated as well. Objects *above* you object are never instantiated, so the same ServerType object in Fielddevices would have been omitted (the reason is that the recursive instantiation function protects itself from infinite recursions, which are hard to track when first ascending, then redescending into a tree).


Combination of multiple nodesets
................................

In the previous section you have seen how you can use the nodeset compiler with one single nodeset which depends on the default nodeset (NS0) ``Opc.Ua.NodeSet2.xml``. The nodeset compiler also supports nodesets which depend on more than one nodeset. We will show this use-case with the PLCopen nodeset. The PLCopen nodeset ``Opc.Ua.PLCopen.NodeSet2_V1.02.xml`` depends on the DI nodeset ``Opc.Ua.Di.NodeSet2.xml`` which then depends on NS0. This example is also shown in ``examples/nodeset/CMakeLists.txt``.

This DI nodeset makes use of some additional data types in ``deps/ua-nodeset/DI/Opc.Ua.Di.Types.bsd``. Since we also need these types within the generated code, we first need to compile the types into C code. The generated code is mainly a definition of the binary representation of the types required for encoding and decoding. The generation can be done using the ``ua_generate_datatypes`` CMake function, which uses the ``tools/generate_datatypes.py`` script::

    ua_generate_datatypes(
        NAME "ua_types_di"
        TARGET_SUFFIX "types-di"
        NAMESPACE_MAP "2:http://opcfoundation.org/UA/DI/"
        FILE_CSV "${UA_NODESET_DIR}/DI/OpcUaDiModel.csv"
        FILES_BSD "${UA_NODESET_DIR}/DI/Opc.Ua.Di.Types.bsd"
    )

The ``NAMESPACE_MAP`` parameter is an array of strings which indicates the mapping of specific namespace uris to the resulting namespace index.
This mapping is required for correct mapping of DataType nodes and their node ids. Currently we need to rely that the namespace is also added at this position in the final server. There is no automatic inferring yet (pull requests are warmly welcome).
If you are using the `DEPENDS` option on the ``ua_generate_nodeset_and_datatypes``, the ``NAMESPACE_MAP`` is also inherited and you do not need to pass all mappings for dependent types.
The CSV and BSD files contain the metadata and definition for the types. ``TARGET_SUFFIX`` is used to create a new target with the name ``open62541-generator-TARGET_SUFFIX``.

Now you can compile the DI nodeset XML using the following command::

    ua_generate_nodeset(
        NAME "di"
        FILE "${UA_NODESET_DIR}/DI/Opc.Ua.Di.NodeSet2.xml"
        TYPES_ARRAY "UA_TYPES_DI"
        INTERNAL
        DEPENDS_TYPES "UA_TYPES"
        DEPENDS_NS    "${UA_NODESET_DIR}/Schema/Opc.Ua.NodeSet2.xml"
        DEPENDS_TARGET "open62541-generator-types-di"
    )

There are now two new arguments: ``INTERNAL`` indicates that internal headers (and non public API) should be included within the generated source code. This is currently required for nodesets which use structures as data values, and will probably be fixed in the future.
The ``DEPENDS_TYPES`` types array argument is matched with the nodesets in the same order as they appear on the ``DEPENDS_TARGET`` parameter. It tells the nodeset compiler which types array it should use: ``UA_TYPES`` for ``Opc.Ua.NodeSet2.xml`` and ``UA_TYPES_DI`` for ``Opc.Ua.Di.NodeSet2.xml``. This is the type array generated by the ``generate_datatypes.py`` script. The rest is similar to the example in previous section: ``Opc.Ua.NodeSet2.xml`` is assumed to exist already and only needs to be loaded for consistency checks, ``Opc.Ua.Di.NodeSet2.xml`` will be generated in the output file ``ua_namespace_di.c/.h``

Next we can generate the PLCopen nodeset. Since it doesn't require any additional datatype definitions, we can immediately start with the nodeset compiler command::

    ua_generate_nodeset(
        NAME "plc"
        FILE "${UA_NODESET_DIR}/PLCopen/Opc.Ua.PLCopen.NodeSet2_V1.02.xml"
        INTERNAL
        DEPENDS_TYPES
            "UA_TYPES" "UA_TYPES_DI"
        DEPENDS_NS
            "${UA_NODESET_DIR}/Schema/Opc.Ua.NodeSet2.xml"
            "${UA_NODESET_DIR}/DI/Opc.Ua.Di.NodeSet2.xml"
        DEPENDS_TARGET "open62541-generator-ns-di"
    )
This call is quite similar to the compilation of the DI nodeset. As you can see, we do not define any specific types array for the PLCopen nodeset. Since the PLCopen nodeset depends on the NS0 and DI nodeset, we need to tell the nodeset compiler that these two nodesets should be seen as already existing. Make sure that the order is the same as in your XML file, e.g., in this case the order indicated in ``Opc.Ua.PLCopen.NodeSet2_V1.02.xml -> UANodeSet -> Models -> Model``.

As a result of the previous scripts you will have multiple source files:

* ua_types_di_generated.c
* ua_types_di_generated.h
* ua_types_di_generated_encoding_binary.h
* ua_types_di_generated_handling.h
* ua_namespace_di.c
* ua_namespace_di.h
* ua_namespace_plc.c
* ua_namespace_plc.h

Finally you need to include all these files in your build process and call the corresponding initialization methods for the nodesets. An example application could look like this:

.. code-block:: c

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    /* Create nodes from nodeset */
    UA_StatusCode retval = ua_namespace_di(server);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Adding the DI namespace failed. Please check previous error output.");
        UA_Server_delete(server);
        return (int)UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    retval |= ua_namespace_plc(server);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Adding the PLCopen namespace failed. Please check previous error output.");
        UA_Server_delete(server);
        return (int)UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    retval = UA_Server_run(server, &running);
