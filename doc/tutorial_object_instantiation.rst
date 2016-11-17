Instantiation of objects
------------------------


ObjectTypes in open62541
^^^^^^^^^^^^^^^^^^^^^^^^

We take the following information model snippet as the starting point of the following tutorial.
It consists of an objectType for building field devices.


.. graphviz::

    digraph tree {

    fixedsize=true;
    node [width=2, height=0, shape=box, fillcolor="#E5E5E5", concentrate=true]

    node_root [label="<<ObjectType>>\nFieldDeviceType"]

    { rank=same
      point_2 [shape=point]
      node_2 [label="<<Variable>>\nModelName"] }
    node_root -> point_2 [arrowhead=none]
    point_2 -> node_2 [label="hasProperty"]

    { rank=same
      point_2a [shape=point]
      node_2a [label="<<Variable>>\nSerialNumber"] }
    point_2 -> point_2a [arrowhead=none]
    point_2a -> node_2a [label="hasProperty"]

    }


Let's start to create this type and its variables:

.. literalinclude:: server_instantiation.c
   :language: c
   :linenos:
   :lines: 47-72




Creating object instances
^^^^^^^^^^^^^^^^^^^^^^^^^
Defining an object type is only useful if it ends up making our lives easier in some way (though it is always the proper thing to do). One of the key benefits of defining object types is being able to create object instances fairly easily. Object instantiation is handled automatically when the typedefinition NodeId points to a valid ObjectType node. All Attributes and Methods contained in the objectType definition will be instantiated along with the object node.

Now we can instantiate an object of this type by:

.. literalinclude:: server_instantiation.c
   :language: c
   :linenos:
   :lines: 141-150

As you can see instantiating an object is not much different from creating an object node. The main difference is that you *must* use an objectType node as typeDefinition.

If you start the server and inspect the nodes with UA Expert, you will find a field device in the objects folder, which look like this:

   .. graphviz::

       digraph tree {

       fixedsize=true;
       node [width=2, height=0, shape=box, fillcolor="#E5E5E5", concentrate=true]

       node_root [label="<<Object>>\nFD314"]

       { rank=same
         point_2 [shape=point]
         node_2 [label="<<Variable>>\nModelName"] }
       node_root -> point_2 [arrowhead=none]
       point_2 -> node_2 [label="hasComponent"]

       {  rank=same
          point_4 [shape=point]
          node_4 [label="<<Variable>>\nSerialNumber"] }
       point_2 -> point_4 [arrowhead=none]
       point_4 -> node_4 [label="hasComponent"]

       }

As you can see the device has inherited the properties of its type (`ModelName` and `SerialNumber`).
While variables are copied from the objectType definition (allowing the user for example to attach new dataSources to them), methods are always only linked. This paradigm is identical to languages like C++: The method called is always the same piece of code, but the first argument is a pointer to an object. Likewise, in OPC UA, only one methodCallback can be attached to a specific methodNode. If that methodNode is called, the parent objectId will be passed to the method - it is the methods job to derefence which object instance it belongs to in that moment.


Instantiation Callbacks
^^^^^^^^^^^^^^^^^^^^^^^
One of the problems arising from the server internally "building" new nodes as described in the type is that the user does not know which template creates which instance. This can be a problem - for example if a specific dataSource should be attached to each variableNode called "samples" later on. Unfortunately, we only know which template variable's Id the dataSource will be attached to - we do not know the nodeId of the instance of that variable. To easily cover usecases where variable instances Y derived from a definition template X should need to be manipulated in some maner, the stack provides an instantiation callback: Each time a new node is instantiated, the callback gets notified about the relevant data; the callback can then either manipulate the new node itself or just create a map/record for later use.

Thus, we first define a callback function

.. literalinclude:: server_instantiation.c
   :language: c
   :linenos:
   :lines: 21-29

And then bind this function to the node instantiation with (`instantiationCallback`) and a handle (`myHandle`). You should already be familiar with callbacks and handles from our previous tutorial.

.. literalinclude:: server_instantiation.c
  :language: c
  :linenos:
  :lines: 154-167


You can easily derive how the callback is used by running the server binary, which produces the following output:

.. code-block:: bash

    Created new node ns=1;i=1315 according to template ns=0;i=63 (handle was 42)
    Created new node ns=1;i=1315 according to template ns=1;i=10001 (handle was 42)
    Created new node ns=1;i=1316 according to template ns=0;i=63 (handle was 42)
    Created new node ns=1;i=1316 according to template ns=1;i=10002 (handle was 42)
    Created new node ns=1;i=10010 according to template ns=1;i=10000 (handle was 42)


For each object and variable created by the call, the callback was invoked. The callback gives you the nodeId of the new node along with the Id of the Type template used to create it. You can thereby effectively use setAttributeValue() functions (or others) to adapt the properties of these new nodes, as they can be identified by there templates.

You may also notice that the callback was not called for methods, even though they are obviously where they are supposed to be. Methods, in contrast to objects and variables, are never cloned but instead only linked. The reason is that you will quite propably attach a method callback to a central method, not each object.



Inheritance
^^^^^^^^^^^
open62541 supports inheritence of types. So, supertypes of an object are also instantiated along with the object.

We extend our information model to have some subclasses

.. _example-information-model:

.. graphviz::

    digraph tree {

    fixedsize=true;
    node [width=2, height=0, shape=box, fillcolor="#E5E5E5", concentrate=true]

    node_root [label="<<ObjectType>>\nFieldDeviceType"]

    { rank=same
      point_2 [shape=point]
      node_2 [label="<<Variable>>\nModelName"] }
    node_root -> point_2 [arrowhead=none]
    point_2 -> node_2 [label="hasProperty"]

    { rank=same
      point_2a [shape=point]
      node_2a [label="<<Variable>>\nSerialNumber"] }
    point_2 -> point_2a [arrowhead=none]
    point_2a -> node_2a [label="hasProperty"]

    {  rank=same
       point_3 [shape=point]
       node_3 [label="<<ObjectType>>\nPumpType"] }
    point_2a -> point_3 [arrowhead=none]
    point_3 -> node_3 [label="hasSubtype"]

    {  rank=same
       point_4 [shape=point]
       node_4 [label="<<Variable>>\nMotorRPM"] }
    node_3 -> point_4 [arrowhead=none]
    point_4 -> node_4 [label="hasProperty"]

    {  rank=same
       point_6 [shape=point]
       node_6 [label="<<Method>>\nstartPump"]
       node_8 [label="<<Variable>>\nOutputArgument"] }
    point_4 -> point_6 [arrowhead=none]
    point_6 -> node_6 [label="hasComponent"]
    node_6 -> node_8 [label="hasProperty"]

    {  rank=same
       point_7 [shape=point]
       node_7 [label="<<Method>>\nstopPump"]
       node_9 [label="<<Variable>>\nOutputArgument"] }
    point_6 -> point_7 [arrowhead=none]
    point_7 -> node_7 [label="hasComponent"]
    node_7 -> node_9 [label="hasProperty"]

    {  rank=same
       point_8 [shape=point]
       node_10 [label="<<ObjectType>>\nPumpAX2500Type"] }
    point_7 -> point_8 [arrowhead=none]
    point_8 -> node_10 [label="hasSubType"]

    { rank=same
      point_9 [shape=point]
      node_11 [label="<<Variable>>\nModelName=\"AX-2500\""] }
    node_10 -> point_9 [arrowhead=none]
    point_9 -> node_11 [label="hasProperty"]

    }


This is reflected by the following code:

.. literalinclude:: server_instantiation.c
   :language: c
   :linenos:
   :lines: 73-138


Now, we can instantiate a specific pump of type AX-2500:

.. literalinclude:: server_instantiation.c
  :language: c
  :linenos:
  :lines: 170-178


Now, also the properties of the supertype of `PumpAX2500Type` are instantiated for the new object. Note, that the empty `ModelName` of `FieldDeviceType` is masked by `PumpAX2500Type`, so that the object `T1.A3.P002` has a correct model name.

.. graphviz::

    digraph tree {

    fixedsize=true;
    node [width=2, height=0, shape=box, fillcolor="#E5E5E5", concentrate=true]

    node_root [label="<<Object>>\nT1.A3.P002"]

    { rank=same
      point_2 [shape=point]
      node_2 [label="<<Variable>>\nModelName=AX-2500"] }
    node_root -> point_2 [arrowhead=none]
    point_2 -> node_2 [label="hasProperty"]

    { rank=same
      point_2a [shape=point]
      node_2a [label="<<Variable>>\nSerialNumber"] }
    point_2 -> point_2a [arrowhead=none]
    point_2a -> node_2a [label="hasProperty"]

    {  rank=same
       point_4 [shape=point]
       node_4 [label="<<Variable>>\nMotorRPM"] }
    point_2a -> point_4 [arrowhead=none]
    point_4 -> node_4 [label="hasProperty"]

    {  rank=same
       point_6 [shape=point]
       node_6 [label="<<Method>>\nstartPump"]
       node_8 [label="<<Variable>>\nOutputArguments"] }
    point_4 -> point_6 [arrowhead=none]
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


The whole example can be found in ``examples/server_instantiation.c``.
