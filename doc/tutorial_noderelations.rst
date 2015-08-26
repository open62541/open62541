Examining and interacting with node relations
=============================================

In the past tutorials you learned to compile the stack in various configurations, create/delete nodes and manage variables. The core of OPC UA are it's data modelling capabilities, and you will invariably find yourself confronted to investigate these relations during runtime. This tutorial will show you how to interact with object and types hierarchies and how to create your own.

Compile XML Namespaces
----------------------

So far we have made due with the hardcoded mini-namespace in the server stack. When writing an application, it is more then likely that you will want to create your own data models using some comfortable GUI based tools like UA Modeller. Most tools can export data to the OPC UA XML specification. open62541 contains a python based namespace compiler that can embed datamodels contained in XML files into the server stack.

Note beforehand that the pyUANamespace compiler you can find in the *tools* subfolder is *not* a XML transformation tool but a compiler. That means that it will create an internal representation (dAST) when parsing the XML files and attempt to understand this representation in order to generate C Code. In consequence, the compiler will refuse to print any inconsistencies or invalid nodes.

As an example, we will create a simple object model using UA Modeller and embed this into the servers nodeset, which is exported to the following XML file::

    <UANodeSet xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:uax="http://opcfoundation.org/UA/2008/02/Types.xsd" xmlns="http://opcfoundation.org/UA/2011/03/UANodeSet.xsd" xmlns:s1="http://yourorganisation.org/example_nodeset/" xmlns:xsd="http://www.w3.org/2001/XMLSchema">
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
                <ModelInfo Tool="UaModeler" Hash="Zs8w1AQI71W8P/GOk3k/xQ==" Version="1.3.4"/>
            </Extension>
        </Extensions>
        <UAReferenceType NodeId="ns=1;i=4001" BrowseName="1:providesInputTo">
            <DisplayName>providesInputTo</DisplayName>
            <References>
                <Reference ReferenceType="HasSubtype" IsForward="false">i=33</Reference>
            </References>
            <InverseName Locale="en_US">inputProcidedBy</InverseName>
        </UAReferenceType>
        <UAObjectType IsAbstract="true" NodeId="ns=1;i=1001" BrowseName="1:FieldDevice">
            <DisplayName>FieldDevice</DisplayName>
            <References>
                <Reference ReferenceType="HasSubtype" IsForward="false">i=58</Reference>
                <Reference ReferenceType="HasComponent">ns=1;i=6001</Reference>
                <Reference ReferenceType="HasComponent">ns=1;i=6002</Reference>
            </References>
        </UAObjectType>
        <UAVariable DataType="String" ParentNodeId="ns=1;i=1001" NodeId="ns=1;i=6001" BrowseName="1:ManufacturerName" UserAccessLevel="3" AccessLevel="3">
            <DisplayName>ManufacturerName</DisplayName>
            <References>
                <Reference ReferenceType="HasTypeDefinition">i=63</Reference>
                <Reference ReferenceType="HasModellingRule">i=78</Reference>
                <Reference ReferenceType="HasComponent" IsForward="false">ns=1;i=1001</Reference>
            </References>
        </UAVariable>
        <UAVariable DataType="String" ParentNodeId="ns=1;i=1001" NodeId="ns=1;i=6002" BrowseName="1:ModelName" UserAccessLevel="3" AccessLevel="3">
            <DisplayName>ModelName</DisplayName>
            <References>
                <Reference ReferenceType="HasTypeDefinition">i=63</Reference>
                <Reference ReferenceType="HasModellingRule">i=78</Reference>
                <Reference ReferenceType="HasComponent" IsForward="false">ns=1;i=1001</Reference>
            </References>
        </UAVariable>
        <UAObjectType NodeId="ns=1;i=1002" BrowseName="1:Pump">
            <DisplayName>Pump</DisplayName>
            <References>
                <Reference ReferenceType="HasComponent">ns=1;i=6003</Reference>
                <Reference ReferenceType="HasComponent">ns=1;i=6004</Reference>
                <Reference ReferenceType="HasSubtype" IsForward="false">ns=1;i=1001</Reference>
                <Reference ReferenceType="HasComponent">ns=1;i=7001</Reference>
                <Reference ReferenceType="HasComponent">ns=1;i=7002</Reference>
            </References>
        </UAObjectType>
        <UAVariable DataType="Boolean" ParentNodeId="ns=1;i=1002" NodeId="ns=1;i=6003" BrowseName="1:isOn" UserAccessLevel="3" AccessLevel="3">
            <DisplayName>isOn</DisplayName>
            <References>
                <Reference ReferenceType="HasTypeDefinition">i=63</Reference>
                <Reference ReferenceType="HasModellingRule">i=78</Reference>
                <Reference ReferenceType="HasComponent" IsForward="false">ns=1;i=1002</Reference>
            </References>
        </UAVariable>
        <UAVariable DataType="UInt32" ParentNodeId="ns=1;i=1002" NodeId="ns=1;i=6004" BrowseName="1:MotorRPM" UserAccessLevel="3" AccessLevel="3">
            <DisplayName>MotorRPM</DisplayName>
            <References>
                <Reference ReferenceType="HasTypeDefinition">i=63</Reference>
                <Reference ReferenceType="HasModellingRule">i=78</Reference>
                <Reference ReferenceType="HasComponent" IsForward="false">ns=1;i=1002</Reference>
            </References>
        </UAVariable>
        <UAMethod ParentNodeId="ns=1;i=1002" NodeId="ns=1;i=7001" BrowseName="1:startPump">
            <DisplayName>startPump</DisplayName>
            <References>
                <Reference ReferenceType="HasModellingRule">i=78</Reference>
                <Reference ReferenceType="HasProperty">ns=1;i=6005</Reference>
                <Reference ReferenceType="HasComponent" IsForward="false">ns=1;i=1002</Reference>
            </References>
        </UAMethod>
        <UAVariable DataType="Argument" ParentNodeId="ns=1;i=7001" ValueRank="1" NodeId="ns=1;i=6005" ArrayDimensions="1" BrowseName="OutputArguments">
            <DisplayName>OutputArguments</DisplayName>
            <References>
                <Reference ReferenceType="HasModellingRule">i=78</Reference>
                <Reference ReferenceType="HasProperty" IsForward="false">ns=1;i=7001</Reference>
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
        <UAMethod ParentNodeId="ns=1;i=1002" NodeId="ns=1;i=7002" BrowseName="1:stopPump">
            <DisplayName>stopPump</DisplayName>
            <References>
                <Reference ReferenceType="HasModellingRule">i=78</Reference>
                <Reference ReferenceType="HasProperty">ns=1;i=6006</Reference>
                <Reference ReferenceType="HasComponent" IsForward="false">ns=1;i=1002</Reference>
            </References>
        </UAMethod>
        <UAVariable DataType="Argument" ParentNodeId="ns=1;i=7002" ValueRank="1" NodeId="ns=1;i=6006" ArrayDimensions="1" BrowseName="OutputArguments">
            <DisplayName>OutputArguments</DisplayName>
            <References>
                <Reference ReferenceType="HasModellingRule">i=78</Reference>
                <Reference ReferenceType="HasProperty" IsForward="false">ns=1;i=7002</Reference>
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

UA Modeler prepends the namespace qualifier "uax:" to some fields - this is not supported by the namespace compiler, who has strict aliasing rules concerning field names. If a datatype defines a field called "Argument", the compiler expects to find "<Argument>" tags, not "<uax:Argument>". Remove/Substitute such fields to remove namespace qualifiers.

The namespace compiler can be invoked manually and has numerous options. In its simplest form, an invokation will look like this::

    python ./generate_open62541CCode.py ../schema/namespace0/Opc.Ua.NodeSet2.xml <path>/<to>/<more>/<files>.xml <path>/<to>/<evenmore>/<files>.xml myNamespace

The above call first parses Namespace 0, which provides all dataTypes, referenceTypes, etc.. An arbitrary amount of further xml files can be passed as options, whose nodes will be added to the abstract syntax tree. The script will then create the files ``myNamespace.c`` and ``myNamespace.h`` containing the C code necessary to instantiate those namespaces.

Although it is possible to run the compiler this way, it is highly discouraged. If you care to examine the CMakeLists.txt (toplevel directory), you will find that compiling the stack with ``DENABLE_GENERATE_NAMESPACE0`` will execute the following command::

  COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/generate_open62541CCode.py 
    -i ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/NodeID_AssumeExternal.txt
    -s description -b ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/NodeID_Blacklist.txt 
    ${PROJECT_SOURCE_DIR}/tools/schema/namespace0/${GENERATE_NAMESPACE0_FILE} 
    ${PROJECT_BINARY_DIR}/src_generated/ua_namespaceinit_generated

Albeit a bit more complicates then the previous description, you can see that a the namespace 0 XML file is loaded in the line before the last, and that the output will be in ``ua_namespaceinit_generated.c/h``. In order to take advantage of the namespace compiler, we will simply append our nodeset to this call and have cmake care for the rest. Modify the CMakeLists.txt line above to contain the relative path to your own XML file like this::

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

After adding you XML file to CMakeLists.txt, rerun cmake in your build directory and enable ``DENABLE_GENERATE_NAMESPACE0``. Make especially sure that you are using the option ``CMAKE_BUILD_TYPE=Debug``. The generated namespace contains more than 30000 lines of code and many strings. Optimizing this amount of code with -O2 or -Os options will require several hours on most PCs! Also make sure to enable ``-DENABLE_METHODCALLS``, as namespace 0 does contain methods that need to be encoded::
  
  ichrispa@Cassandra:open62541/build> cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_METHODCALLS=On -BUILD_EXAMPLECLIENT=On -BUILD_EXAMPLESERVER=On -DENABLE_GENERATE_NAMESPACE0=On ../
  -- Git version: v0.1.0-RC4-403-g198597c-dirty
  -- Configuring done
  -- Generating done
  -- Build files have been written to: /home/ichrispa/work/svn/working_copies/open62541/build
  ichrispa@Cassandra:open62541/build> make
  [  3%] Generating src_generated/ua_nodeids.h
  [  6%] Generating src_generated/ua_types_generated.c, src_generated/ua_types_generated.h
  [ 10%] Generating src_generated/ua_transport_generated.c, src_generated/ua_transport_generated.h
  [ 13%] Generating src_generated/ua_namespaceinit_generated.c, src_generated/ua_namespaceinit_generated.h

At this point, the make process will most likely hang for 30-60s until the namespace is parsed, checked, linked and finally generated (be pacient). It sould continue as follows::
  
  Scanning dependencies of target open62541-object
  [ 17%] Building C object CMakeFiles/open62541-object.dir/src/ua_types.c.o
  [ 20%] Building C object CMakeFiles/open62541-object.dir/src/ua_types_encoding_binary.c.o                                                                                                 
  [ 24%] Building C object CMakeFiles/open62541-object.dir/src_generated/ua_types_generated.c.o                                                                                             
  [ 27%] Building C object CMakeFiles/open62541-object.dir/src_generated/ua_transport_generated.c.o                                                                                         
  [ 31%] Building C object CMakeFiles/open62541-object.dir/src/ua_connection.c.o                                                                                                            
  [ 34%] Building C object CMakeFiles/open62541-object.dir/src/ua_securechannel.c.o                                                                                                         
  [ 37%] Building C object CMakeFiles/open62541-object.dir/src/ua_session.c.o                                                                                                               
  [ 41%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_server.c.o                                                                                                         
  [ 44%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_server_addressspace.c.o                                                                                            
  [ 48%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_server_binary.c.o                                                                                                  
  [ 51%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_nodes.c.o                                                                                                          
  [ 55%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_server_worker.c.o                                                                                                  
  [ 58%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_securechannel_manager.c.o                                                                                          
  [ 62%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_session_manager.c.o                                                                                                
  [ 65%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_services_discovery.c.o                                                                                             
  [ 68%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_services_securechannel.c.o                                                                                         
  [ 72%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_services_session.c.o                                                                                               
  [ 75%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_services_attribute.c.o                                                                                             
  [ 79%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_services_nodemanagement.c.o                                                                                        
  [ 82%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_services_view.c.o                                                                                                  
  [ 86%] Building C object CMakeFiles/open62541-object.dir/src/client/ua_client.c.o                                                                                                         
  [ 89%] Building C object CMakeFiles/open62541-object.dir/examples/networklayer_tcp.c.o                                                                                                    
  [ 93%] Building C object CMakeFiles/open62541-object.dir/examples/logger_stdout.c.o                                                                                                       
  [ 96%] Building C object CMakeFiles/open62541-object.dir/src_generated/ua_namespaceinit_generated.c.o 

And at this point, you are going to see the compiler hanging again. If you specified ``-DCMAKE_BUILD_TYPE=Debug``, you are looking at about 5-10 seconds of waiting. If you forgot, you can now drink a cup of coffee, go to the movies or take a loved one out for dinner (or abort the build with CTRL+C). Shortly after::

  [ 83%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_services_call.c.o
  [ 86%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_nodestore.c.o
  [100%] Built target open62541-object
  Scanning dependencies of target open62541
  Linking C shared library libopen62541.so
  [100%] Built target open62541

Now switch back to your own source directory and update your libopen62541 library (in case you have not linked it into the build folder). Compile our example server as follows::
  
  ichrispa@Cassandra:open62541/build-tutorials> gcc -g -std=c99 -Wl,-rpath,`pwd` -I ./include -L . -DENABLE_METHODCALLS -o server ./server.c -lopen62541

Note that we need to also define the method-calls here, as the header files may choose to ommit functions such as UA_Server_addMethodNode() if they believe you do not use them. If you run the server, you should now see a new dataType in the browse path ``/Types/ObjectTypes/BaseObjectType/FieldDevice`` when viewing the nodes in UAExpert.

If you take a look at any of the variables, like ``ManufacturerName``, you will notice it is shown as a Boolean; this is not an error. The node does not include a variant and as you learned in our previous tutorial, it is that variant that would hold the dataType ID.
  
A minor list of some of the miriad things that can go wrong:
  * Your file was not found. The namespace compiler will complain, print a help message, and exit.
  * A structure/DataType you created with a value was not encoded. The namespace compiler can currently not handle nested extensionObjects.
  * Nodes are not or wrongly encoded or you get nodeId errors.  The namespace compiler can currently not encode bytestring or guid node id's and external server uris are not supported either.
  * You get compiler complaints for non-existant variants. Check that you have removed any namespace qualifiers (like "uax:") from the XML file.
  * You get "invalid reference to addMethodNode" style errors. Make sure ``-DDENABLE_METHODCALLS=On`` is defined.

Creating object instances
-------------------------

``` UA_(Server|Client)_createInstanceOf()```


Iterating over Child nodes
--------------------------

``` UA_(Server|Client)_forEachChildNodeCall();```

Examining node copies
---------------------

``` UA_(Server|Client)_getNodeCopy()```

``` UA_(Server|Client)_destroyNodeCopy()```

