Examining and interacting with node relations
=============================================

In the past tutorials you learned to compile the stack in various configurations, create/delete nodes and manage variables. The core of OPC UA are it's data modelling capabilities, and you will invariably find yourself confronted to investigate these relations during runtime. This tutorial will show you how to interact with object and types hierarchies and how to create your own.




Iterating over Child nodes
--------------------------

``` UA_(Server|Client)_forEachChildNodeCall();```

Examining node copies
---------------------

``` UA_(Server|Client)_getNodeCopy()```

``` UA_(Server|Client)_destroyNodeCopy()```

Compile XML Namespaces
----------------------

Stasiks comfy CMake method.

Manual script call.

Creating object instances
-------------------------

``` UA_(Server|Client)_createInstanceOf()```