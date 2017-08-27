XML Nodeset Compiler
--------------------

When writing an application, it is more comfortable to create information models using some GUI tools. Most tools can export data according the OPC UA Nodeset XML schema. open62541 contains a python based namespace compiler that can transform these information model definitions into a working server.

Note that the namespace compiler you can find in the *tools* subfolder is *not* an XML transformation tool but a compiler. That means that it will create an internal representation when parsing the XML files and attempt to understand and verify the correctness of this representation in order to generate C Code.

We take the following information model snippet as the starting point of the following tutorial:

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
            <InverseName Locale="en_US">inputProcidedBy</InverseName>
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

**TODO** Some modelers prepends the namespace qualifier "uax:" to some fields - this is not supported by the namespace compiler, who has strict aliasing rules concerning field names. If a datatype defines a field called "Argument", the compiler expects to find "<Argument>" tags, not "<uax:Argument>".

In its simplest form, an invokation of the namespace compiler will look like this:

.. code-block:: bash

   $ python ./generate_open62541CCode.py <Opc.Ua.NodeSet2.xml> myNS.xml myNS

The first argument points to the XML definition of the standard-defined namespace 0. Namespace 0 is assumed to be loaded beforehand and provides defintions for data type, reference types, and so. The second argument points to the user-defined information model, whose nodes will be added to the abstract syntax tree. The script will then creates the files ``myNS.c`` and ``myNS.h`` containing the C code necessary to instantiate those namespaces.

Although it is possible to run the compiler this way, it is highly discouraged. If you care to examine the CMakeLists.txt (toplevel directory), you will find that compiling the stack with ``DUA_ENABLE_GENERATE_NAMESPACE0`` will execute the following command::

  COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/generate_open62541CCode.py 
    -i ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/NodeID_AssumeExternal.txt
    -s description -b ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/NodeID_Blacklist.txt 
    ${PROJECT_SOURCE_DIR}/tools/schema/namespace0/${GENERATE_NAMESPACE0_FILE} 
    ${PROJECT_BINARY_DIR}/src_generated/ua_namespaceinit_generated

Albeit a bit more complicated than the previous description, you can see that a the namespace 0 XML file is loaded in the line before the last, and that the output will be in ``ua_namespaceinit_generated.c/h``. In order to take advantage of the namespace compiler, we will simply append our nodeset to this call and have cmake care for the rest. Modify the CMakeLists.txt line above to contain the relative path to your own XML file like this::

  COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/generate_open62541CCode.py 
    -i ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/NodeID_AssumeExternal.txt
    -s description -b ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/NodeID_Blacklist.txt 
    ${PROJECT_SOURCE_DIR}/tools/schema/namespace0/${GENERATE_NAMESPACE0_FILE} 
    ${PROJECT_SOURCE_DIR}/<relative>/<path>/<to>/<your>/<namespace>.xml
    ${PROJECT_BINARY_DIR}/src_generated/ua_namespaceinit_generated

Always make sure that your XML file comes *after* namespace 0. Also, take into consideration that any node ID's you specify that already exist in previous files will overwrite the previous file (yes, you could intentionally overwrite the NS0 Server node if you wanted to). The namespace compiler will now automatically embedd you namespace definitions into the namespace of the server. So in total, all that was necessary was:
  
  * Creating your namespace XML description
  * Adding the relative path to the file into CMakeLists.txt
  * Compiling the stack

After adding your XML file to CMakeLists.txt, rerun cmake in your build directory and enable ``DUA_ENABLE_GENERATE_NAMESPACE0``. Make especially sure that you are using the option ``CMAKE_BUILD_TYPE=Debug``. The generated namespace contains more than 30000 lines of code and many strings. Optimizing this amount of code with -O2 or -Os options will require several hours on most PCs! Also make sure to enable ``-DUA_ENABLE_METHODCALLS``, as namespace 0 does contain methods that need to be encoded

.. code-block:: bash
  
  $ cmake -DCMAKE_BUILD_TYPE=Debug -DUA_ENABLE_METHODCALLS=On \
          -BUILD_EXAMPLECLIENT=On -BUILD_EXAMPLESERVER=On \
          -DUA_ENABLE_GENERATE_NAMESPACE0=On ../
  $ make

At this point, the make process will most likely hang for 30-60s until the namespace is parsed, checked, linked and finally generated (be patient). If you specified ``-DCMAKE_BUILD_TYPE=Debug``, you are looking at about 5-10 seconds of waiting. A release build can take a lot longer.

If you open the header ``src_generated/ua_namespaceinit_generated.h`` and take a short look at the generated defines, you will notice the following definitions have been created:

.. code-block:: c
  
  #define UA_NS1ID_PROVIDESINPUTTO
  #define UA_NS1ID_FIELDDEVICE
  #define UA_NS1ID_PUMP
  #define UA_NS1ID_STARTPUMP
  #define UA_NS1ID_STOPPUMP

These definitions are generated for all types, but not variables, objects or views (as their names may be ambiguous and may not a be unique identifier). You can use these definitions in your code for numeric nodeids in namespace 1.
  
Now switch back to your own source directory and update your libopen62541 library (in case you have not linked it into the build folder). Compile our example server as follows:

.. code-block:: bash
  
   $ gcc -g -std=c99 -Wl,-rpath,`pwd` -I ./include -L . -DUA_ENABLE_METHODCALLS -o server ./server.c -lopen62541

Note that we need to also define the method-calls here, as the header files may choose to ommit functions such as UA_Server_addMethodNode() if they believe you do not use them. If you run the server, you should now see a new dataType in the browse path ``/Types/ObjectTypes/BaseObjectType/FieldDevice`` when viewing the nodes in UAExpert.

A minor list of some of the things that can go wrong:
  * Your file was not found. The namespace compiler will complain, print a help message, and exit.
  * A structure/DataType you created with a value was not encoded. The namespace compiler can currently not handle nested extensionObjects.
  * Nodes are not or wrongly encoded or you get nodeId errors.  The namespace compiler can currently not encode bytestring or guid node id's and external server uris are not supported either.
  * You get compiler complaints for non-existant variants. Check that you have removed any namespace qualifiers (like "uax:") from the XML file.
  * You get "invalid reference to addMethodNode" style errors. Make sure ``-DUA_ENABLE_METHODCALLS=On`` is defined.

Creating object instances
^^^^^^^^^^^^^^^^^^^^^^^^^

One of the key benefits of defining object types is being able to create object instances fairly easily. Object instantiation is handled automatically when the typedefinition NodeId points to a valid ObjectType node. All Attributes and Methods contained in the objectType definition will be instantiated along with the object node. 

While variables are copied from the objetType definition (allowing the user for example to attach new dataSources to them), methods are always only linked. This paradigm is identical to languages like C++: The method called is always the same piece of code, but the first argument is a pointer to an object. Likewise, in OPC UA, only one methodCallback can be attached to a specific methodNode. If that methodNode is called, the parent objectId will be passed to the method - it is the methods job to derefence which object instance it belongs to in that moment.

One of the problems arising from the server internally "building" new nodes as described in the type is that the user does not know which template creates which instance. This can be a problem - for example if a specific dataSource should be attached to each variableNode called "samples" later on. Unfortunately, we only know which template variable's Id the dataSource will be attached to - we do not know the nodeId of the instance of that variable. To easily cover usecases where variable instances Y derived from a definition template X should need to be manipulated in some maner, the stack provides an instantiation callback: Each time a new node is instantiated, the callback gets notified about the relevant data; the callback can then either manipulate the new node itself or just create a map/record for later use.

Let's look at an example that will create a pump instance given the newly defined objectType:

.. code-block:: c

    #include <stdio.h>
    #include <signal.h>

    #include "open62541.h"

    UA_Boolean running = true;

    void stopHandler(int signal) {
      running = false;
    }

    static UA_StatusCode
    pumpInstantiationCallback(UA_NodeId objectId, UA_NodeId definitionId,
                              void *handle) {
      printf("Created new node ns=%d;i=%d according to template ns=%d;i=%d "
             "(handle was %d)\n", objectId.namespaceIndex,
             objectId.identifier.numeric, definitionId.namespaceIndex,
             definitionId.identifier.numeric, *((UA_Int32 *) handle));
      return UA_STATUSCODE_GOOD;
    }

    int main(void) {
      signal(SIGINT,  stopHandler);
      signal(SIGTERM, stopHandler);

      UA_ServerConfig config = UA_ServerConfig_standard;
      UA_ServerNetworkLayer nl;
      nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 4840);
      config.networkLayers = &nl;
      config.networkLayersSize = 1;
      UA_Server *server = UA_Server_new(config);

      UA_NodeId createdNodeId;
      UA_Int32 myHandle = 42;
      UA_ObjectAttributes object_attr;
      UA_ObjectAttributes_init(&object_attr);
      
      object_attr.description = UA_LOCALIZEDTEXT("en_US","A pump!");
      object_attr.displayName = UA_LOCALIZEDTEXT("en_US","Pump1");
      
      UA_InstantiationCallback theAnswerCallback;
      theAnswerCallback.method = pumpInstantiationCallback;
      theAnswerCallback.handle = (void*) &myHandle};
      
      UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, DEMOID),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                              UA_QUALIFIEDNAME(1, "Pump1"),
                              UA_NODEID_NUMERIC(0, UA_NS1ID_PUMPTYPE),
                              object_attr, theAnswerCallback, &createdNodeId);
                              
      UA_Server_run(server, 1, &running);
      UA_Server_delete(server);
      return 0;
    }


Make sure you have updated the headers and libs in your project, then recompile and run the server. Make especially sure you have added ``ua_namespaceinit_generated.h`` to your include folder and that you have removed any references to header in ``server``. The only include you are going to need is ``ua_types.h``.

As you can see instantiating an object is not much different from creating an object node. The main difference is that you *must* use an objectType node as typeDefinition and you (may) pass a callback function (``pumpInstantiationCallback``) and a handle (``myHandle``). You should already be familiar with callbacks and handles from our previous tutorial and you can easily derive how the callback is used by running the server binary, which produces the following output::

    Created new node ns=1;i=1505 according to template ns=1;i=6001 (handle was 42)
    Created new node ns=1;i=1506 according to template ns=1;i=6002 (handle was 42)
    Created new node ns=1;i=1507 according to template ns=1;i=6003 (handle was 42)
    Created new node ns=1;i=1508 according to template ns=1;i=6004 (handle was 42)
    Created new node ns=1;i=1510 according to template ns=1;i=6001 (handle was 42)
    Created new node ns=1;i=1511 according to template ns=1;i=6002 (handle was 42)
    Created new node ns=1;i=1512 according to template ns=1;i=6003 (handle was 42)
    Created new node ns=1;i=1513 according to template ns=1;i=6004 (handle was 42)

If you start the server and inspect the nodes with UA Expert, you will find two pumps in the objects folder, which look like this:

As you can see the pump has inherited it's parents attributes (ManufacturerName and ModelName). You may also notice that the callback was not called for the methods, even though they are obviously where they are supposed to be. Methods, in contrast to objects and variables, are never cloned but instead only linked. The reason is that you will quite propably attach a method callback to a central method, not each object. Objects are instantiated if they are *below* the object you are creating, so any object (like an object called associatedServer of ServerType) that is part of pump will be instantiated as well. Objects *above* you object are never instantiated, so the same ServerType object in Fielddevices would have been ommitted (the reason is that the recursive instantiation function protects itself from infinite recursions, which are hard to track when first ascending, then redescending into a tree).

For each object and variable created by the call, the callback was invoked. The callback gives you the nodeId of the new node along with the Id of the Type template used to create it. You can thereby effectively use setAttributeValue() functions (or others) to adapt the properties of these new nodes, as they can be identified by there templates.
