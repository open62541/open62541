.. _introduction:

Introduction to OPC UA
======================

OPC UA, a collection of services
--------------------------------

In OPC UA, all communication is based on service calls, each consisting of a request and a response
message. Be careful to note the difference between services and methods. Services are pre-defined in
the standard and cannot be changed. But you can use the *Call* service to invoke user-defined
methods on the server.

For completeness, the following tables contain all services defined in the standard. Do not bother
with their details yet. We will introduce the different services later in the text. In open62541,
each service is implemented in a single function. See the :ref:`services` section for details.

**Establishing communication**

+-----------------------------+-----------------------------+------------------------------+
| Discovery Service Set       | SecureChannel Service Set   | Session Service Set          |
+=============================+=============================+==============================+
| FindServers                 | OpenSecureChannel           | CreateSession                |
+-----------------------------+-----------------------------+------------------------------+
| GetEndpoints                | CloseSecureChannel          | ActivateSession              |
+-----------------------------+-----------------------------+------------------------------+
| RegisterServer              |                             | CloseSession                 |
+-----------------------------+-----------------------------+------------------------------+
|                             |                             | Cancel                       |
+-----------------------------+-----------------------------+------------------------------+

**Interaction with the information model**

+-----------------------------+-------------------------------+------------------------------+------------------------------+----------------------+
| Attribute Service Set       | View Service Set              | Method Service Set           | NodeManagement Service Set   | Query Service Set    |
+=============================+===============================+==============================+==============================+======================+
| Read                        | Browse                        | Call                         | AddNodes                     | QueryFirst           |
+-----------------------------+-------------------------------+------------------------------+------------------------------+----------------------+
| HistoryRead                 | BrowseNext                    |                              | AddReferences                | QueryNext            |
+-----------------------------+-------------------------------+------------------------------+------------------------------+----------------------+
| Write                       | TranslateBrowsePathsToNodeids |                              | DeleteNodes                  |                      |
+-----------------------------+-------------------------------+------------------------------+------------------------------+----------------------+
| HistoryUpdate               | RegisterNodes                 |                              | DeleteReferences             |                      |
+-----------------------------+-------------------------------+------------------------------+------------------------------+----------------------+
|                             | UnregisterNodes               |                              |                              |                      |
+-----------------------------+-------------------------------+------------------------------+------------------------------+----------------------+

**Notifications**

+-----------------------------+-------------------------------+
| MonitoredItem Service Set   | Subscription Service Set      |
+=============================+===============================+
| CreateMonitoredItems        | CreateSubscription            |
+-----------------------------+-------------------------------+
| ModifyMonitoreditems        | ModifySubscription            |
+-----------------------------+-------------------------------+
| SetMonitoringMode           | SetPublishingMode             |
+-----------------------------+-------------------------------+
| SetTriggering               | Publish                       |
+-----------------------------+-------------------------------+
| DeleteMonitoredItems        | Republish                     |
+-----------------------------+-------------------------------+
|                             | TransferSubscription          |
+-----------------------------+-------------------------------+
|                             | DeleteSubscription            |
+-----------------------------+-------------------------------+

OPC UA, a web of nodes
----------------------

The information model in each OPC UA server is a web of interconnected nodes.
There are eight different types of nodes.

+-----------------------+-------------------+
| ReferenceTypeNode     | MethodNode        |
+-----------------------+-------------------+
| DataTypeNode          | ObjectTypeNode    |
+-----------------------+-------------------+
| VariableTypeNode      | ObjectNode        |
+-----------------------+-------------------+
| VariableNode          | ViewNode          |
+-----------------------+-------------------+

Depending on its type, every node contains different attributes. Some
attributes, are contained in all node types:

+----------------+---------------+
| Name           | Type          |
+================+===============+
| nodeID         | NodeId        |
+----------------+---------------+
| nodeClass      | NodeClass     |
+----------------+---------------+
| browseName     | QualifiedName |
+----------------+---------------+
| displayName    | LocalizedText |
+----------------+---------------+
| description    | LocalizedText |
+----------------+---------------+
| writeMask      | UInt32        |
+----------------+---------------+
| userWriteMask  | UInt32        |
+----------------+---------------+
| referencesSize | Int32         |
+----------------+---------------+
| references     |ReferenceNode[]|
+----------------+---------------+

Nodes are interconnected by directed reference-triples of the form ``(nodeid,
referencetype, target-nodeid)``. Therefore an OPC UA information model is
easiest imagined as a *web of nodes*. Reference types can be

- standard- or user-defined and
- either non-hierarchical (e.g., indicating the type of a variable-node) or
  hierarchical (e.g., indicating a parent-child relationship).

OPC UA, a protocol
------------------

The OPC UA protocol (both binary and XML-based) is based on 25 *built-in*
datatypes. In open62541, these are defined in ua_types.h.

The builtin datatypes are combined to more complex structures. When the structure contains an array,
then the size of the array is stored in an Int32 value just before the array itself. A size of -1
indicates an undefined array. Positive sizes (and zero) have the usual semantics.

Most importantly, every service has a request and a response message defined as such a data
structure. The entire OPC UA protocol revolves around the exchange of these request and response
messages. Their exact definitions can be looked up here:
https://opcfoundation.org/UA/schemas/Opc.Ua.Types.bsd.xml. In open62541, we autogenerate the
C-structs to handle the standard-defined structures automatically. See ua_types_generated.h for
comparison.
