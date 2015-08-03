Manipulating node attributes
============================

In our last tutorial, we created some nodes using both the server and the client side API. In this tutorial, we will explore how to manipulate the contents of nodes.

Getting and setting node attributes
-----------------------------------

```UA_(Server|Client)_(get|set)AttributeValue( ..., UA_AttributeId attributeId, void* value);```
  


+----------------------------------------+-----+-----+-------------------------+
| Attribute Name                         | get | set | Expected type for void* |
+========================================+=====+=====+=========================+
| UA_ATTRIBUTEID_NODEID                  |  ✔  |  ✘  | UA_NodeId               |
+----------------------------------------+-----+-----+-------------------------+
| UA_ATTRIBUTEID_NODECLASS               |  ✔  |  ✘  | UA_NodeClass | UA_UInt32|
+----------------------------------------+-----+-----+-------------------------+
| UA_ATTRIBUTEID_BROWSENAME              |  ✔  |  ✔  | UA_QualifiedName        |
+----------------------------------------+-----+-----+-------------------------+
| UA_ATTRIBUTEID_DISPLAYNAME             |  ✔  |  ✔  | UA_LocalizedText        |
+----------------------------------------+-----+-----+-------------------------+
| UA_ATTRIBUTEID_DESCRIPTION             |  ✔  |  ✔  | UA_LocalizedText        |
+----------------------------------------+-----+-----+-------------------------+
| UA_ATTRIBUTEID_WRITEMASK               |  ✔  |  ✔  | UA_UInt32               |
+----------------------------------------+-----+-----+-------------------------+
| UA_ATTRIBUTEID_USERWRITEMASK           |  ✔  |  ✔  | UA_UInt32               |
+----------------------------------------+-----+-----+-------------------------+
| UA_ATTRIBUTEID_ISABSTRACT              |  ✔  |  ✔  | UA_Boolean              |
+----------------------------------------+-----+-----+-------------------------+
| UA_ATTRIBUTEID_SYMMETRIC               |  ✔  |  ✔  | UA_Boolean              |
+----------------------------------------+-----+-----+-------------------------+
| UA_ATTRIBUTEID_INVERSENAME             |  ✔  |  ✔  | UA_LocalizedText        |
+----------------------------------------+-----+-----+-------------------------+
| UA_ATTRIBUTEID_CONTAINSNOLOOPS         |  ✔  |  ✔  | UA_Boolean              |
+----------------------------------------+-----+-----+-------------------------+
| UA_ATTRIBUTEID_EVENTNOTIFIER           |  ✔  |  ✔  | UA_Byte                 |
+----------------------------------------+-----+-----+-------------------------+
| UA_ATTRIBUTEID_VALUE                   |  ✔  |  ✔  | UA_Variant              |
+----------------------------------------+-----+-----+-------------------------+
| UA_ATTRIBUTEID_DATATYPE                |  ✔  |  ✘  | UA_NodeId               |
+----------------------------------------+-----+-----+-------------------------+
| UA_ATTRIBUTEID_VALUERANK               |  ✔  |  ✘  | UA_Int32                |
+----------------------------------------+-----+-----+-------------------------+
| UA_ATTRIBUTEID_ARRAYDIMENSIONS         |  ✔  |  ✘  | UA_UInt32               |
+----------------------------------------+-----+-----+-------------------------+
| UA_ATTRIBUTEID_ACCESSLEVEL             |  ✔  |  ✔  | UA_UInt32               |
+----------------------------------------+-----+-----+-------------------------+
| UA_ATTRIBUTEID_USERACCESSLEVEL         |  ✔  |  ✔  | UA_UInt32               |
+----------------------------------------+-----+-----+-------------------------+
| UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL |  ✔  |  ✔  | UA_Double               |
+----------------------------------------+-----+-----+-------------------------+
| UA_ATTRIBUTEID_HISTORIZING             |  ✔  |  ✔  | UA_Boolean              |
+----------------------------------------+-----+-----+-------------------------+
| UA_ATTRIBUTEID_EXECUTABLE              |  ✔  |  ✔  | UA_Boolean              |
+----------------------------------------+-----+-----+-------------------------+
| UA_ATTRIBUTEID_USEREXECUTABLE          |  ✔  |  ✔  | UA_Boolean              |
+----------------------------------------+-----+-----+-------------------------+

The Basenode attributes NodeId and NodeClass uniquely identify that node and cannot be changed (changing them is equal to creating a new node). The DataType, ValueRank and ArrayDimensions are not part of the node attributes in open62541, but instead contained in the UA_Variant data value of that a variable or variableType node (change the value to change these as well).

Setting Variable contents
-------------------------

DataSource nodes and callbacks
------------------------------

```UA_Server_addDataSourceVariableNode()```

Iterating over Child nodes
--------------------------

``` UA_(Server|Client)_forEachChildNodeCall();```

Examining node copies
---------------------

``` UA_(Server|Client)_getNodeCopy()```

``` UA_(Server|Client)_destroyNodeCopy()```

Creating object instances
-------------------------

``` UA_(Server|Client)_createInstanceOf()```
