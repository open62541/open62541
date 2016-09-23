Namespace Compiler
------------------

Generating an OPC UA Information Model from XML Descriptions


This tutorial will show you how to generate an OPC UA Information Model from XML descritions defined in the OPC UA Nodeset XML schema.

OPC UA XML Namespaces
^^^^^^^^^^^^^^^^^^^^^^

Writing all the code for generating namespace is quite boring and error-prone.
When writing an application, it is more comfortable to create information models using some comfortable GUI tools. Most tools can export data according the (OPC UA Nodeset XML schema)[https://opcfoundation.org/UA/schemas/Opc.Ua.NodeSet.xml]. These XML files allow to persistent and transfer OPC UA information models. Furthermore, most servers can somehow embed or include those node XMLs.

The :ref:`example information model <example-information-model>` from the previous tutorial can be represented in XML as follows:


.. literalinclude:: server_nodeset.xml
   :language: xml


open62541 contains a python based namespace compiler that can transform these information model definitions into a working server.
Note that the namespace compiler you can find in the *tools* subfolder is *not* an XML transformation tool but a compiler. That means that it will create an internal representation when parsing the XML files and attempt to understand and verify the correctness of this representation in order to generate C Code.


Compile the namespace into C-code
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In its simplest form, an invokation of the namespace compiler will look like this:

.. code-block:: bash

   $ python ./generate_open62541CCode.py <Opc.Ua.NodeSet2.xml> myNS.xml myNS

The first argument points to the XML definition of the standard-defined namespace 0. Namespace 0 is assumed to be loaded beforehand and provides definitions for data type, reference types, and so. The second argument points to the user-defined information model (``myNS.xml``), whose nodes will be added to the abstract syntax tree. The script will then creates the files ``myNS.c`` and ``myNS.h`` containing the C code and its header necessary to instantiate those namespaces. The code for actually creating the namespace can then be called with ``myNS(UA_Server *server);``.

**TODO** Some modelers prepends the namespace qualifier "uax:" to some fields - this is not supported by the namespace compiler, who has strict aliasing rules concerning field names. If a datatype defines a field called "Argument", the compiler expects to find "<Argument>" tags, not "<uax:Argument>".


Integration into CMake
^^^^^^^^^^^^^^^^^^^^^^
Although it is possible to run the compiler this way, it is highly discouraged. If you care to examine the CMakeLists.txt (toplevel directory), you will find that compiling the stack with ``-DUA_ENABLE_GENERATE_NAMESPACE0`` will execute the following command::

  COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/generate_open62541CCode.py
    -i ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/NodeID_AssumeExternal.txt
    -s description -b ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/NodeID_Blacklist.txt
    ${PROJECT_SOURCE_DIR}/tools/schema/namespace0/${GENERATE_NAMESPACE0_FILE}
    ${PROJECT_BINARY_DIR}/src_generated/ua_namespaceinit_generated

Albeit a bit more complicated than the previous description, you can see that the namespace 0 XML file is loaded in the line before the last, and that the output will be in ``ua_namespaceinit_generated.c/h``. In order to take advantage of the namespace compiler, we will simply append our nodeset to this call and have CMake care for the rest. Modify the CMakeLists.txt line above to contain the relative path to your own XML file like this::

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
  -- Git version: v0.1.0-RC4-403-g198597c-dirty
  -- Configuring done
  -- Generating done
  -- Build files have been written to: /home/ichrispa/work/svn/working_copies/open62541/build

  $ make
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

  #define UA_NS1ID_FIELDDEVICETYPE
  #define UA_NS1ID_PUMPTYPE
  #define UA_NS1ID_PUMPAX2500TYPE

These definitions are generated for all types, but not variables, objects or views (as their names may be ambiguous and may not a be unique identifier). You can use these definitions in your code as you already used the ``UA_NS0ID_`` equivalents.

Now switch back to your own source directory and update your libopen62541 library (in case you have not linked it into the build folder). Compile our example server as follows::

  ichrispa@Cassandra:open62541/build-tutorials> gcc -g -std=c99 -Wl,-rpath,`pwd` -I ./include -L . -DUA_ENABLE_METHODCALLS -o server ./server.c -lopen62541

Note that we need to also define the method-calls here, as the header files may choose to ommit functions such as UA_Server_addMethodNode() if they believe you do not use them. If you run the server, you should now see a new dataType in the browse path ``/Types/ObjectTypes/BaseObjectType/FieldDevice`` when viewing the nodes in UAExpert.

If you take a look at any of the variables, like ``ManufacturerName``, you will notice it is shown as a Boolean; this is not an error. The node does not include a variant and as you learned in our previous tutorial, it is that variant that would hold the dataType ID.


Possible pitfalls
^^^^^^^^^^^^^^^^^

A minor list of some of the miriad things that can go wrong:
  * Your file was not found. The namespace compiler will complain, print a help message, and exit.
  * A structure/DataType you created with a value was not encoded. The namespace compiler can currently not handle nested extensionObjects.
  * Nodes are not or wrongly encoded or you get nodeId errors.  The namespace compiler can currently not encode bytestring or guid node id's and external server uris are not supported either.
  * You get compiler complaints for non-existant variants. Check that you have removed any namespace qualifiers (like "uax:") from the XML file.
  * You get "invalid reference to addMethodNode" style errors. Make sure ``-DUA_ENABLE_METHODCALLS=On`` is defined.
