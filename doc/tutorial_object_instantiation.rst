Instantiation of objects
------------------------

Creating object instances
^^^^^^^^^^^^^^^^^^^^^^^^^
Defining an object type is only useful if it ends up making our lives easier in some way (though it is always the proper thing to do). One of the key benefits of defining object types is being able to create object instances fairly easily. Object instantiation is handled automatically when the typedefinition NodeId points to a valid ObjectType node. All Attributes and Methods contained in the objectType definition will be instantiated along with the object node.

While variables are copied from the objectType definition (allowing the user for example to attach new dataSources to them), methods are always only linked. This paradigm is identical to languages like C++: The method called is always the same piece of code, but the first argument is a pointer to an object. Likewise, in OPC UA, only one methodCallback can be attached to a specific methodNode. If that methodNode is called, the parent objectId will be passed to the method - it is the methods job to derefence which object instance it belongs to in that moment.


Instantiation Callbacks
^^^^^^^^^^^^^^^^^^^^^^^
One of the problems arising from the server internally "building" new nodes as described in the type is that the user does not know which template creates which instance. This can be a problem - for example if a specific dataSource should be attached to each variableNode called "samples" later on. Unfortunately, we only know which template variable's Id the dataSource will be attached to - we do not know the nodeId of the instance of that variable. To easily cover usecases where variable instances Y derived from a definition template X should need to be manipulated in some maner, the stack provides an instantiation callback: Each time a new node is instantiated, the callback gets notified about the relevant data; the callback can then either manipulate the new node itself or just create a map/record for later use.


Instantiation Example
^^^^^^^^^^^^^^^^^^^^^
Let's look at an example that will create a pump instance given the pumptype as specified in the prior tutorial:

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

As you can see instantiating an object is not much different from creating an object node. The main difference is that you *must* use an objectType node as typeDefinition and you (may) pass a callback function (``pumpInstantiationCallback``) and a handle (``myHandle``). You should already be familiar with callbacks and handles from our previous tutorial and you can easily derive how the callback is used by running the server binary, which produces the following output:

    Created new node ns=1;i=1505 according to template ns=1;i=6001 (handle was 42)
    Created new node ns=1;i=1506 according to template ns=1;i=6002 (handle was 42)
    Created new node ns=1;i=1507 according to template ns=1;i=6003 (handle was 42)
    Created new node ns=1;i=1508 according to template ns=1;i=6004 (handle was 42)
    Created new node ns=1;i=1510 according to template ns=1;i=6001 (handle was 42)
    Created new node ns=1;i=1511 according to template ns=1;i=6002 (handle was 42)
    Created new node ns=1;i=1512 according to template ns=1;i=6003 (handle was 42)
    Created new node ns=1;i=1513 according to template ns=1;i=6004 (handle was 42)

If you start the server and inspect the nodes with UA Expert, you will find two pumps in the objects folder, which look like this:

.. graphviz::

    digraph tree {

    fixedsize=true;
    node [width=2, height=0, shape=box, fillcolor="#E5E5E5", concentrate=true]

    node_root [label="<<Object>>\nPump"]

    { rank=same
      point_1 [shape=point]
      node_1 [label="<<Variable>>\nManufacturerName"] }
    node_root -> point_1 [arrowhead=none]
    point_1 -> node_1 [label="hasComponent"]

    { rank=same
      point_2 [shape=point]
      node_2 [label="<<Variable>>\nModelName"] }
    point_1 -> point_2 [arrowhead=none]
    point_2 -> node_2 [label="hasComponent"]

    {  rank=same
       point_4 [shape=point]
       node_4 [label="<<Variable>>\nMotorRPM"] }
    point_2 -> point_4 [arrowhead=none]
    point_4 -> node_4 [label="hasComponent"]

    {  rank=same
       point_5 [shape=point]
       node_5 [label="<<Variable>>\nisOn"] }
    point_4 -> point_5 [arrowhead=none]
    point_5 -> node_5 [label="hasComponent"]

    {  rank=same
       point_6 [shape=point]
       node_6 [label="<<Method>>\nstartPump"]
       node_8 [label="<<Variable>>\nOutputArguments"] }
    point_5 -> point_6 [arrowhead=none]
    point_6 -> node_6 [label="hasComponent"]
    node_6 -> node_8 [label="hasProperty"]

    {  rank=same
       point_7 [shape=point]
       node_7 [label="<<Method>>\nstopPump"]
       node_9 [label="<<Variable>>\nOutputArguments"] }
    point_6 -> point_7 [arrowhead=none]
    point_7 -> node_7 [label="hasComponent"]
    node_7 -> node_9 [label="hasProperty"]

    }

As you can see the pump has inherited it's parents attributes (ManufacturerName and ModelName). You may also notice that the callback was not called for the methods, even though they are obviously where they are supposed to be. Methods, in contrast to objects and variables, are never cloned but instead only linked. The reason is that you will quite propably attach a method callback to a central method, not each object. Objects are instantiated if they are *below* the object you are creating, so any object (like an object called associatedServer of ServerType) that is part of pump will be instantiated as well. Objects *above* you object are never instantiated, so the same ServerType object in Fielddevices would have been ommitted (the reason is that the recursive instantiation function protects itself from infinite recursions, which are hard to track when first ascending, then redescending into a tree).

For each object and variable created by the call, the callback was invoked. The callback gives you the nodeId of the new node along with the Id of the Type template used to create it. You can thereby effectively use setAttributeValue() functions (or others) to adapt the properties of these new nodes, as they can be identified by there templates.


Inheritance
^^^^^^^^^^^


* We can instantiate object nodes from typedefs (obviously)
* We can instantiate superTypes (inheritance) along with objects:

.. code-block:: c

    Type:
    + MamalType
       v- Class = "mamalia"
       v- Species
       + DogType
          v Name

    Instance of Dog:
    + Dog
      v- Class = "mamalia"
      v- Genus
      v= Name


* We can mask superType attributes

.. code-block:: c

    Type:
    + MamalType
       v- Class  = "mamalia"
       v- Species
       + DogType
          v- Species = "Canis"
          v- Name

    Instance of Dog:
    + Dog
      v- Class  = "mamalia"
      v- Species  = "Canis"
      v= Name

    * We can do both of these things in nested variable and object definitions, which are merged in the instance

.. code-block:: c

    Type:
    + MamalType
       v- Class  = "mamalia"
       v- Species
       o- Abilities
           v- MakeSound
           v- Breathe = True
       + DogType
          v- Species = "Canis"
          v- Name
          o- Abilities
             v- MakeSound = "Wuff"
             v- FetchNewPaper

    Instance of Dog:
    + Dog
      v- Class  = "mamalia"
      v- Species  = "Canis"
      v- Name
      o- Abilities // Merged nested definition
          v- Breathe = True // Inherited in nested object from SuperType
          v- MakeSound = "Wuff" // Overwrite/Mask SuperType
          v- FetchNewPaper // Add to superType


.. literalinclude:: server_inheritance.c
   :language: c
   :linenos:
   :lines: 4,12,14-
