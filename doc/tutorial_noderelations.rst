4. Generating an OPC UA Information Model from XML Descriptions
===============================================================

In the past tutorials you have learned to compile the stack in various configurations, create/delete nodes and manage variables. The core of OPC UA is its data modelling capabilities, and you will invariably find yourself confronted to investigate these relations during runtime. This tutorial will show you how to interact with object and type hierarchies and how to create your own.

Compile XML Namespaces
----------------------

So far we have made due with the hardcoded mini-namespace in the server stack. When writing an application, it is more then likely that you will want to create your own data models using some comfortable GUI based tools like UA Modeler. Most tools can export data to the OPC UA XML specification. open62541 contains a python based namespace compiler that can embed datamodels contained in XML files into the server stack.

Note beforehand that the pyUANamespace compiler you can find in the *tools* subfolder is *not* a XML transformation tool but a compiler. That means that it will create an internal representation (dAST) when parsing the XML files and attempt to understand this representation in order to generate C Code. In consequence, the compiler will refuse to print any inconsistencies or invalid nodes.

As an example, we will create a simple object model using UA Modeler and embed this into the servers nodeset, which is exported to the following XML file:

.. code-block:: xml

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

Or, more consiscly, this::

   +------------------+
   |  <<ObjectType>>  |
   |   FieldDevice    |
   +------------------+
             |              +------------------+
             |              |   <<Variable>>   |
             |------------->| ManufacturerName |
             | hasComponent +------------------+
             |              +------------------+
             |              |   <<Variable>>   |
             |------------->|    ModelName     |
             | hasComponent +------------------+
             |              +----------------+
             |              | <<ObjectType>> |
             '------------->|      Pump      |
                hasSubtype  +----------------+
                                     |
                                     |
                                     |                +------------------+
                                     |                |   <<Variable>>   |
                                     |--------------->|     MotorRPM     |
                                     |  hasComponent  +------------------+
                                     |                +------------------+
                                     |                |   <<Variable>>   |
                                     |--------------->|       isOn       |
                                     |  hasComponent  +------------------+
                                     |                +------------------+    +------------------+
                                     |                |    <<Method>>    |    |   <<Variable>>   |
                                     |--------------->|    startPump     |--->| outputArguments  |
                                     |  hasProperty   +------------------+    +------------------+
                                     |                +------------------+    +------------------+
                                     |                |    <<Method>>    |    |   <<Variable>>   |
                                     '--------------->|     stopPump     |--->| outputArguments  |
                                        hasProperty   +------------------+    +------------------+

                 
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

Albeit a bit more complicated then the previous description, you can see that a the namespace 0 XML file is loaded in the line before the last, and that the output will be in ``ua_namespaceinit_generated.c/h``. In order to take advantage of the namespace compiler, we will simply append our nodeset to this call and have cmake care for the rest. Modify the CMakeLists.txt line above to contain the relative path to your own XML file like this::

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

After adding your XML file to CMakeLists.txt, rerun cmake in your build directory and enable ``DENABLE_GENERATE_NAMESPACE0``. Make especially sure that you are using the option ``CMAKE_BUILD_TYPE=Debug``. The generated namespace contains more than 30000 lines of code and many strings. Optimizing this amount of code with -O2 or -Os options will require several hours on most PCs! Also make sure to enable ``-DENABLE_METHODCALLS``, as namespace 0 does contain methods that need to be encoded::
  
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

At this point, the make process will most likely hang for 30-60s until the namespace is parsed, checked, linked and finally generated (be patient). It should continue as follows::
  
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

If you open the header ``src_generated/ua_namespaceinit_generated.h`` and take a short look at the generated defines, you will notice the following definitions have been created:

.. code-block:: c
  
  #define UA_NS1ID_PROVIDESINPUTTO
  #define UA_NS1ID_FIELDDEVICE
  #define UA_NS1ID_PUMP
  #define UA_NS1ID_STARTPUMP
  #define UA_NS1ID_STOPPUMP

These definitions are generated for all types, but not variables, objects or views (as their names may be ambiguous and may not a be unique identifier). You can use these definitions in your code as you already used the ``UA_NS0ID_`` equivalents.
  
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

Defining an object type is only usefull if it ends up making our lives easier in some way (though it is always the proper thing to do). One of the key benefits of defining object types is being able to create object instances fairly easily. Object instantiation is handled automatically when the typedefinition NodeId points to a valid ObjectType node. All Attributes and Methods contained in the objectType definition will be instantiated along with the object node. 

While variables are copied from the objetType definition (allowing the user for example to attach new dataSources to them), methods are always only linked. This paradigm is identical to languages like C++: The method called is always the same piece of code, but the first argument is a pointer to an object. Likewise, in OPC UA, only one methodCallback can be attached to a specific methodNode. If that methodNode is called, the parent objectId will be passed to the method - it is the methods job to derefence which object instance it belongs to in that moment.

One of the problems arising from the server internally "building" new nodes as described in the type is that the user does not know which template creates which instance. This can be a problem - for example if a specific dataSource should be attached to each variableNode called "samples" later on. Unfortunately, we only know which template variable's Id the dataSource will be attached to - we do not know the nodeId of the instance of that variable. To easily cover usecases where variable instances Y derived from a definition template X should need to be manipulated in some maner, the stack provides an instantiation callback: Each time a new node is instantiated, the callback gets notified about the relevant data; the callback can then either manipulate the new node itself or just create a map/record for later use.

Let's look at an example that will create a pump instance given the newly defined objectType:

.. code-block:: c

    #include <stdio.h>
    #include <signal.h>

    #include "ua_types.h"
    #include "ua_server.h"
    #include "ua_namespaceinit_generated.h"
    #include "logger_stdout.h"
    #include "networklayer_tcp.h"

    UA_Boolean running;
    UA_Int32 global_accessCounter = 0;

    void stopHandler(int signal) {
      running = 0;
    }

    UA_StatusCode pumpInstantiationCallback(UA_NodeId objectId, UA_NodeId definitionId, void *handle);
    UA_StatusCode pumpInstantiationCallback(UA_NodeId objectId, UA_NodeId definitionId, void *handle) {
      printf("Created new node ns=%d;i=%d according to template ns=%d;i=%d (handle was %d)\n", objectId.namespaceIndex, objectId.identifier.numeric,
              definitionId.namespaceIndex, definitionId.identifier.numeric, *((UA_Int32 *) handle));
      return UA_STATUSCODE_GOOD;
    }

    int main(void) {
      signal(SIGINT,  stopHandler);
      signal(SIGTERM, stopHandler);

      UA_Server *server = UA_Server_new(UA_ServerConfig_standard);
      UA_Server_addNetworkLayer(server, ServerNetworkLayerTCP_new(UA_ConnectionConfig_standard, 16664));
      running = true;

      UA_NodeId createdNodeId;
      UA_Int32 myHandle = 42;
      UA_ObjectAttributes object_attr;
      UA_ObjectAttributes_init(&object_attr);
      
      object_attr.description = UA_LOCALIZEDTEXT("en_US","A pump!");
      object_attr.displayName = UA_LOCALIZEDTEXT("en_US","Pump1");
      
      UA_InstantiationCallback theAnswerCallback = {.method=pumpInstantiationCallback, .handle=(void*) &myHandle};
      
      UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, DEMOID),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(1, "Pump1"),
                              UA_NODEID_NUMERIC(0, UA_NS1ID_PUMPTYPE), object_attr, theAnswerCallback, &createdNodeId);
                              
      UA_Server_run(server, 1, &running);
      UA_Server_delete(server);

      printf("Bye\n");
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

If you start the server and inspect the nodes with UA Expert, you will find two pumps in the objects folder, which look like this::

       +------------+
       | <<Object>> |
       |   Pump1    |
       +------------+
              |
              |  +------------------+
              |->|   <<Variable>>   |
              |  | ManufacturerName |
              |  +------------------+
              |  +------------------+
              |  |   <<Variable>>   |
              |->|    ModelName     |
              |  +------------------+
              |  +------------------+
              |  |   <<Variable>>   |
              |->|     MotorRPM     |
              |  +------------------+
              |  +------------------+
              |  |   <<Variable>>   |
              |->|       isOn       |
              |  +------------------+
              |  +------------------+    +------------------+
              |  |    <<Method>>    |    |   <<Variable>>   |
              |->|    startPump     |--->| outputArguments  |
              |  +------------------+    +------------------+
              |  +------------------+    +------------------+
              |  |    <<Method>>    |    |   <<Variable>>   |
              '->|     stopPump     |--->| outputArguments  |
                 +------------------+    +------------------+

As you can see the pump has inherited it's parents attributes (ManufacturerName and ModelName). You may also notice that the callback was not called for the methods, even though they are obviously where they are supposed to be. Methods, in contrast to objects and variables, are never cloned but instead only linked. The reason is that you will quite propably attach a method callback to a central method, not each object. Objects are instantiated if they are *below* the object you are creating, so any object (like an object called associatedServer of ServerType) that is part of pump will be instantiated as well. Objects *above* you object are never instantiated, so the same ServerType object in Fielddevices would have been ommitted (the reason is that the recursive instantiation function protects itself from infinite recursions, which are hard to track when first ascending, then redescending into a tree).

For each object and variable created by the call, the callback was invoked. The callback gives you the nodeId of the new node along with the Id of the Type template used to create it. You can thereby effectively use setAttributeValue() functions (or others) to adapt the properties of these new nodes, as they can be identified by there templates.

If you want to overwrite an attribute of the parent definition, you will have to delete the node instantiated by the parent's template (this as a **FIXME** for developers).
    
Iterating over Child nodes
--------------------------

A common usecase is wanting to perform something akin to ``for each node referenced by X, call ...``; you may for example be searching for a specific browseName or instance which was created with a dynamic nodeId. There is no way of telling what you are searching for beforehand (inverse hasComponents, typedefinitions, etc.), but all usescases of "searching for" basically means iterating over each reference of a node.

Since searching in nodes is a common operation, the high-level branch provides a function to help you perform this operation:  ``UA_(Server|Client)_forEachChildNodeCall();``. These functions will iterate over all references of a given node, invoking a callback (with a handle) for every found reference. Since in our last tutorial we created a server that instantiates two pumps, we are now going to build a client that will search for pumps in all object instances on the server.

.. code-block:: c

    #include <stdio.h>

    #include "ua_types.h"
    #include "ua_server.h"
    #include "ua_client.h"
    #include "ua_namespaceinit_generated.h"
    #include "logger_stdout.h"
    #include "networklayer_tcp.h"

    UA_StatusCode nodeIter(UA_NodeId childId, UA_Boolean isInverse, UA_NodeId referenceTypeId, void *handle);
    UA_StatusCode nodeIter(UA_NodeId childId, UA_Boolean isInverse, UA_NodeId referenceTypeId, void *handle) {  
      struct {
        UA_Client *client;
        UA_Boolean isAPump;
        UA_NodeId PumpId;
      } *nodeIterParam = handle;
      
      if (isInverse == true)
        return UA_STATUSCODE_GOOD;
      if (childId.namespaceIndex != 1)
        return UA_STATUSCODE_GOOD;
      if (nodeIterParam == NULL)
        return UA_STATUSCODE_GOODNODATA;
      
      UA_QualifiedName *childBrowseName = NULL;
      UA_Client_getAttributeValue(nodeIterParam->client, childId, UA_ATTRIBUTEID_BROWSENAME, (void**) &childBrowseName);
      
      UA_String pumpName = UA_STRING("Pump");
      if (childBrowseName != NULL) {
        if (childBrowseName->namespaceIndex == 1) {
          if (!strncmp(childBrowseName->name.data, pumpName.data, pumpName.length))
            printf("Found %s with NodeId ns=1,i=%d\n", childBrowseName->name.data, childId.identifier.numeric);
            inodeIterParam->isAPump = true;
            UA_NodeId_copy(&childId, &nodeIterParam->PumpId);
        }
      }
      
      UA_QualifiedName_delete(childBrowseName);
      return UA_STATUSCODE_GOOD;
    }

    int main(void) {
      UA_Client *client = UA_Client_new(UA_ClientConfig_standard, Logger_Stdout_new());
      UA_StatusCode retval = UA_Client_connect(client, ClientNetworkLayerTCP_connect, "opc.tcp://localhost:16664");
      if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return retval;
      }
      
      struct {
        UA_Client *client;
        UA_Boolean isAPump;
        UA_NodeId PumpId;
      } nodeIterParam;
      nodeIterParam.client = client;
      nodeIterParam.isAPump = false;
      
      UA_Client_forEachChildNodeCall(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), nodeIter, (void *) &nodeIterParam);
      if (nodeIterParam.isAPump == true)
        printf("Found at least one pump\n");
        
      UA_Client_disconnect(client);
      UA_Client_delete(client);
      return 0;
    } 

If the client is run while the example server is running in the background, it will produce the following output::

    Found Pump1 with NodeId ns=1,i=1504
    Found Pump2 with NodeId ns=1,i=1509

How does it work? The nodeIter function is called by UA_Client_forEachChildNodeCall() for each reference contained in the objectsFolder. The iterator is passed the id of the target and the type of the reference, along with the references directionality. Since we are browsing the Object node, this iterator will be called mutliple times, indicating links to the root node, server, the two pump instances and the nodes type definition. 

We are only interested in nodes in namespace 1 that are referenced forwardly, so the iterator returns early if these conditions are not met.

We are searching the nodes by name, so we are comparing the name of the nodes to a string; We could also (in a more complicated example) repeat the node iteration inside the iterator, ie inspect the references of each node to see if it has the dataType "Pump", which would be a more reliable way to operate this sort of search. In either case we need to pass parameters to and from the iterator(s). Note the plural.

You can use the handle to contain a pointer to a struct, which can hold multiple arguments as in the example above. In a more thorough example, the field PumpId could have been an array or a linked list. That struct could also be defined as a global dataType instead of using in-function definitions. Since the handle can be passed between multiple calls of iterators (or any other function that accept handles), the data contents can be communicated between different functions easily.
    
Examining node copies
---------------------

So far we have always used the getAttribute() functions to inspect node contents. There may be isolated cases where these are insuficient because you want to examine the properties of a node "in bulk". As mentioned in the first tutorials, the user can not directly interact with the servers nodestore; but the userspace may request a copy of a node, including all its attributes and references. The following functions server the purpose of getting and getting rid of node copies.

.. code-block:: c
  
    UA_(Server|Client)_getNodeCopy()
    UA_(Server|Client)_destroyNodeCopy()

Since you are trying to see a struct (node types) that are usually hidden from userspace, you will have to include ``include/ua_nodes.h``, ``src/ua_types_encoding_binary.h`` and ``deps/queue.h`` in addition to the previous includes (link them into the includes folder).

Let's suppose we wanted to do something elaborate with our pump instance that was returned by the iterator of the previous example, or simply "print" all its fields. We could modify the above client's main function like so:

.. code-block:: c

    int main(void) {
      UA_Client *client = UA_Client_new(UA_ClientConfig_standard, Logger_Stdout_new());
      UA_StatusCode retval = UA_Client_connect(client, ClientNetworkLayerTCP_connect, "opc.tcp://localhost:16664");
      if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return retval;
      }
      
      struct {
        UA_Client *client;
        UA_Boolean isAPump;
        UA_NodeId PumpId;
      } nodeIterParam;
      nodeIterParam.client = client;
      nodeIterParam.isAPump = false;
      
      UA_Client_forEachChildNodeCall(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), nodeIter, (void *) &nodeIterParam);
      if (nodeIterParam.isAPump == true) {
        UA_ObjectNode *aPump;
        UA_Client_getNodeCopy(client, nodeIterParam.PumpId, (void **) &aPump);
        printf("The pump %s with NodeId ns=1,i=%d was returned\n", aPump->browseName.name.data, aPump->nodeId.identifier.numeric);
        UA_Client_deleteNodeCopy(client, (void **) &aPump);
      }
        
      UA_Client_disconnect(client);
      UA_Client_delete(client);
      return 0;
    } 

**Warning** in both examples, we are printing strings contained in UA_String types. These are fundamentaly different from strings in C in that they are *not* necessarlity NULL terminated; they are exactly as long as the string length indicates. It is quite possible that printf() will keep printing trailing data after the UA_String until it finds a NULL. If you intend to really print strings in an application, use the "length" field of the UA_String struct to allocate a null-initialized buffer, then copy the string data into that buffer before printing it.

Conclusion
----------

In this tutorial, you have learned how to compile your own namespaces, instantiate data and examine the relations of the new nodes. You have learned about node iterators and how to pack multiple pass-through parameters into handles; a technique that is by no means limited to iterators but can also be applied to any other callback, such as methods or value sources.
