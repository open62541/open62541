.. _introduction:

Core Concepts of OPC UA
=======================

In one sentence, OPC UA (ISO 62541) defines a framework for object-oriented
information models (typically representing a physical device) that live in an
OPC UA server and a protocol with which a client can interact with the
information model over the network (read and write variables, call methods,
instantiate and delete objects, subscribe to change notifications, and so on).

This Section introduces the core concepts of OPC UA. For the full specification
see the OPC UA standard at `https://reference.opcfoundation.org/
<https://reference.opcfoundation.org/>`_.

.. _protocol:

Protocol
--------

We focus on the TCP-based binary protocol since it is by far the most common
transport layer for OPC UA. The general concepts also translate to HTTP and
SOAP-based communication defined in the standard. Communication in OPC UA is
best understood by starting with the following key principles:

Request / Response
  All communication is based on the Request/Response pattern. Only clients can
  send a request to a server. And servers can only send responses to a matching
  request. Often the server is hosted close to a (physical) device, such as a
  sensor or a machine tool.

Asynchronous Responses
  A server does not have to immediately respond to requests and responses may be
  sent in a different order. This keeps the server responsive when it takes time
  until a specific request has been processed (e.g. a method call or when
  reading from a sensor with delay). Subscriptions (push-notifications) are
  implemented via special requests where the response is delayed until a
  notification is published.

.. note::
   *OPC UA PubSub* (Part 14 of the standard) is an extension for the integration
   of many-to-many communication with OPC UA. PubSub does not use the
   client-server protocol. Rather, OPC UA PubSub integrates with either existing
   broker-based protocols such as MQTT, UDP-multicast or Ethernet-based
   communication. Typically an OPC UA server (accessed via the client-server
   protocol) is used to configured PubSub communication.

   Note that the client-server protocol also supports Subscriptions for
   one-to-one communication and does not depend on PubSub for this feature.

A client-server connection for the OPC UA binary protocol consists of three
nested levels: The stateful TCP connection, a SecureChannel and the Session. For
full details, see Part 6 of the OPC UA standard.

TCP Connection
  The TCP connection is opened to the corresponding hostname and port with an
  initial handshake of HEL/ACK messages. The handshake establishes the basic
  settings of the connection, such as the maximum message length. The *Reverse
  Connect* extension of OPC UA allows the server to initiate the underlying TCP
  connection.

SecureChannel
  SecureChannels are created on top of the raw TCP connection. A SecureChannel
  is established with an *OpenSecureChannel* request and response message pair.
  **Attention!** Even though a SecureChannel is mandatory, encryption might
  still be disabled. The *SecurityMode* of a SecureChannel can be either
  ``None``, ``Sign``, or ``SignAndEncrypt``. As of version 0.2 of open62541,
  message signing and encryption is still under ongoing development.

  With message signing or encryption enabled, the *OpenSecureChannel* messages
  are encrypted using an asymmetric encryption algorithm (public-key
  cryptography) [#key-mgmnt]_. As part of the *OpenSecureChannel* messages,
  client and server establish a common secret over an initially unsecure
  channel. For subsequent messages, the common secret is used for symmetric
  encryption, which has the advantage of being much faster.

  Different *SecurityPolicies* -- defined in part 7 of the OPC UA standard --
  specify the algorithms for asymmetric and symmetric encryption, encryption key
  lengths, hash functions for message signing, and so on. Example
  SecurityPolicies are ``None`` for transmission of cleartext and
  ``Basic256Sha256`` which mandates a variant of RSA with SHA256 certificate
  hashing for asymmetric encryption and AES256 for symmetric encryption.

  The possible SecurityPolicies of a server are described with a list of
  *Endpoints*. An endpoint jointly defines the SecurityMode, SecurityPolicy and
  means for authenticating a session (discussed in the next section) in order to
  connect to a certain server. The *GetEndpoints* service returns a list of
  available endpoints. This service can usually be invoked without a session and
  from an unencrypted SecureChannel. This allows clients to first discover
  available endpoints and then use an appropriate SecurityPolicy that might be
  required to open a session.

Session
  Sessions are created on top of a SecureChannel. This ensures that users may
  authenticate without sending their credentials, such as username and password,
  in cleartext. Currently defined authentication mechanisms are anonymous login,
  username/password, Kerberos and x509 certificates. The latter requires that
  the request message is accompanied by a signature to prove that the sender is
  in possession of the private key with which the certificate was created.

  There are two message exchanges required to establish a session:
  *CreateSession* and *ActivateSession*. The ActivateSession service can be used
  to switch an existing session to a different SecureChannel. This is important,
  for example when the connection broke down and the existing session is
  reused with a new SecureChannel.

.. [#key-mgmnt] This entails that the client and server exchange so-called
   public keys. The public keys might come with a certificate from a key-signing
   authority or be verified against an external key repository. But we will not
   discuss certificate management in detail in this section.

Structure of a protocol message
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Consider the example OPC UA binary conversation in Figure
:numref:`ua-wireshark`, recorded and displayed with `Wireshark
<https://www.wireshark.org/>`_.

.. _ua-wireshark:

.. figure:: ua-wireshark.png
   :figwidth: 100 %
   :alt: OPC UA conversation in Wireshark

   OPC UA conversation displayed in Wireshark

The top part of the Wireshark window shows the messages from the conversation in
order. The green line contains the applied filter. Here, we want to see the OPC
UA protocol messages only. The first messages (from TCP packets 49 to 56) show
the client opening an unencrypted SecureChannel and retrieving the server's
endpoints. Then, starting with packet 63, a new connection and SecureChannel are
created in conformance with one of the endpoints. On top of this SecureChannel,
the client can then create and activate a session. The following *ReadRequest*
message is selected and covered in more detail in the bottom windows.

The bottom left window shows the structure of the selected *ReadRequest*
message. The purpose of the message is invoking the *Read* :ref:`service
<services>`. The message is structured into a header and a message body. Note
that we do not consider encryption or signing of messages here.

Message Header
  As stated before, OPC UA defines an asynchronous protocol. So responses may be
  out of order. The message header contains some basic information, such as the
  length of the message, as well as necessary information to relate messages to
  a SecureChannel and each request to the corresponding response. "Chunking"
  refers to the splitting and reassembling of messages that are longer than the
  maximum network packet size.

Message Body
  Every OPC UA :ref:`service <services>` has a signature in the form of a
  request and response data structure. These are defined according to the OPC UA
  protocol :ref:`type system <types>`. See especially the :ref:`auto-generated
  type definitions<generated-types>` for the data types corresponding to service
  requests and responses. The message body begins with the identifier of the
  following data type. Then, the main payload of the message follows.

The bottom right window shows the binary payload of the selected *ReadRequest*
message. The message header is highlighted in light-grey. The message body in
blue highlighting shows the encoded *ReadRequest* data structure.

.. _services:

Services
--------

In OPC UA, all communication is based on service calls, each consisting of a
request and a response message. These messages are defined as data structures
with a binary encoding and listed in :ref:`generated-types`. Since all
Services are pre-defined in the standard, they cannot be modified by the
user. But you can use the :ref:`Call <method-services>` service to invoke
user-defined methods on the server.

Please refer to the :ref:`client` and :ref:`server` API where the services
are exposed to end users. Please see part 4 of the OPC UA standard for the
authoritative definition of the services and their behaviour.
   
Discovery Service Set
~~~~~~~~~~~~~~~~~~~~~
This Service Set defines Services used to discover the Endpoints implemented
by a Server and to read the security configuration for those Endpoints.

FindServers Service
   Returns the Servers known to a Server or Discovery Server. The Client may
   reduce the number of results returned by specifying filter criteria

GetEndpoints Service
   Returns the Endpoints supported by a Server and all of the configuration
   information required to establish a SecureChannel and a Session.

FindServersOnNetwork Service
   Returns the Servers known to a Discovery Server. Unlike FindServer,
   this Service is only implemented by Discovery Servers. It additionally
   returns servers which may have been detected through Multicast.

RegisterServer
   Registers a remote server in the local discovery service.

RegisterServer2
   This Service allows a Server to register its DiscoveryUrls and capabilities
   with a Discovery Server. It extends the registration information from
   RegisterServer with information necessary for FindServersOnNetwork.

SecureChannel Service Set
~~~~~~~~~~~~~~~~~~~~~~~~~
This Service Set defines Services used to open a communication channel that
ensures the confidentiality and Integrity of all Messages exchanged with the
Server.

OpenSecureChannel Service
   Open or renew a SecureChannel that can be used to ensure Confidentiality and
   Integrity for Message exchange during a Session.

CloseSecureChannel Service
   Used to terminate a SecureChannel.

Session Service Set
   This Service Set defines Services for an application layer connection
   establishment in the context of a Session.

CreateSession Service
   Used by an OPC UA Client to create a Session and the Server returns two
   values which uniquely identify the Session. The first value is the sessionId
   which is used to identify the Session in the audit logs and in the Server's
   address space. The second is the authenticationToken which is used to
   associate an incoming request with a Session.

ActivateSession
   Used by the Client to submit its SoftwareCertificates to the Server for
   validation and to specify the identity of the user associated with the
   Session. This Service request shall be issued by the Client before it issues
   any other Service request after CreateSession. Failure to do so shall cause
   the Server to close the Session.

CloseSession
   Used to terminate a Session.
   
Cancel Service
   Used to cancel outstanding Service requests. Successfully cancelled service
   requests shall respond with Bad_RequestCancelledByClient.

NodeManagement Service Set
~~~~~~~~~~~~~~~~~~~~~~~~~~
This Service Set defines Services to add and delete AddressSpace Nodes and
References between them. All added Nodes continue to exist in the
AddressSpace even if the Client that created them disconnects from the
Server.

AddNodes Service
   Used to add one or more Nodes into the AddressSpace hierarchy.
   If the type or one of the supertypes has any HasInterface references
   (see OPC 10001-7 - Amendment 7, 4.9.2), the child nodes of the interfaces
   are added to the new object.

AddReferences Service
   Used to add one or more References to one or more Nodes.

DeleteNodes Service
   Used to delete one or more Nodes from the AddressSpace.

DeleteReferences
   Used to delete one or more References of a Node.

.. _view-services:

View Service Set
~~~~~~~~~~~~~~~~
Clients use the browse Services of the View Service Set to navigate through
the AddressSpace or through a View which is a subset of the AddressSpace.

Browse Service
   Used to discover the References of a specified Node. The browse can be
   further limited by the use of a View. This Browse Service also supports a
   primitive filtering capability.

BrowseNext Service
   Used to request the next set of Browse or BrowseNext response information
   that is too large to be sent in a single response. "Too large" in this
   context means that the Server is not able to return a larger response or that
   the number of results to return exceeds the maximum number of results to
   return that was specified by the Client in the original Browse request.

TranslateBrowsePathsToNodeIds Service
   Used to translate textual node paths to their respective ids.

RegisterNodes Service
   Used by Clients to register the Nodes that they know they will access
   repeatedly (e.g. Write, Call). It allows Servers to set up anything needed so
   that the access operations will be more efficient.

UnregisterNodes Service
   This Service is used to unregister NodeIds that have been obtained via the
   RegisterNodes service.

Query Service Set
~~~~~~~~~~~~~~~~~
This Service Set is used to issue a Query to a Server. OPC UA Query is
generic in that it provides an underlying storage mechanism independent Query
capability that can be used to access a wide variety of OPC UA data stores
and information management systems. OPC UA Query permits a Client to access
data maintained by a Server without any knowledge of the logical schema used
for internal storage of the data. Knowledge of the AddressSpace is
sufficient.

QueryFirst Service (not implemented)
   This Service is used to issue a Query request to the Server.

QueryNext Service (not implemented)
   This Service is used to request the next set of QueryFirst or QueryNext
   response information that is too large to be sent in a single response.

Attribute Service Set
~~~~~~~~~~~~~~~~~~~~~
This Service Set provides Services to access Attributes that are part of
Nodes.

Read Service
   Used to read attributes of nodes. For constructed attribute values whose
   elements are indexed, such as an array, this Service allows Clients to read
   the entire set of indexed values as a composite, to read individual elements
   or to read ranges of elements of the composite.

Write Service
   Used to write attributes of nodes. For constructed attribute values whose
   elements are indexed, such as an array, this Service allows Clients to write
   the entire set of indexed values as a composite, to write individual elements
   or to write ranges of elements of the composite.

HistoryRead Service
   Used to read historical values or Events of one or more Nodes. Servers may
   make historical values available to Clients using this Service, although the
   historical values themselves are not visible in the AddressSpace.

HistoryUpdate Service
   Used to update historical values or Events of one or more Nodes. Several
   request parameters indicate how the Server is to update the historical value
   or Event. Valid actions are Insert, Replace or Delete.

.. _method-services:

Method Service Set
~~~~~~~~~~~~~~~~~~
The Method Service Set defines the means to invoke methods. A method shall be
a component of an Object. See the section on :ref:`MethodNodes <methodnode>`
for more information.

Call Service
   Used to call (invoke) a methods. Each method call is invoked within the
   context of an existing Session. If the Session is terminated, the results of
   the method's execution cannot be returned to the Client and are discarded.

MonitoredItem Service Set
~~~~~~~~~~~~~~~~~~~~~~~~~
Clients define MonitoredItems to subscribe to data and Events. Each
MonitoredItem identifies the item to be monitored and the Subscription to use
to send Notifications. The item to be monitored may be any Node Attribute.

CreateMonitoredItems Service
   Used to create and add one or more MonitoredItems to a Subscription. A
   MonitoredItem is deleted automatically by the Server when the Subscription is
   deleted. Deleting a MonitoredItem causes its entire set of triggered item
   links to be deleted, but has no effect on the MonitoredItems referenced by
   the triggered items.

DeleteMonitoredItems Service
   Used to remove one or more MonitoredItems of a Subscription. When a
   MonitoredItem is deleted, its triggered item links are also deleted.

ModifyMonitoredItems Service
   Used to modify MonitoredItems of a Subscription. Changes to the MonitoredItem
   settings shall be applied immediately by the Server. They take effect as soon
   as practical but not later than twice the new revisedSamplingInterval.

   Illegal request values for parameters that can be revised do not generate
   errors. Instead the server will choose default values and indicate them in
   the corresponding revised parameter.

SetMonitoringMode Service
   Used to set the monitoring mode for one or more MonitoredItems of a
   Subscription.

SetTriggering Service
   Used to create and delete triggering links for a triggering item.

Subscription Service Set
~~~~~~~~~~~~~~~~~~~~~~~~
Subscriptions are used to report Notifications to the Client.

CreateSubscription Service
   Used to create a Subscription. Subscriptions monitor a set of MonitoredItems
   for Notifications and return them to the Client in response to Publish
   requests.

ModifySubscription Service
   Used to modify a Subscription.

SetPublishingMode Service
   Used to enable sending of Notifications on one or more Subscriptions.

Publish Service
   Used for two purposes. First, it is used to acknowledge the receipt of
   NotificationMessages for one or more Subscriptions. Second, it is used to
   request the Server to return a NotificationMessage or a keep-alive
   Message.

Republish Service
   Requests the Subscription to republish a NotificationMessage from its
   retransmission queue.

DeleteSubscriptions Service
   Invoked to delete one or more Subscriptions that belong to the Client's
   Session.

TransferSubscription Service
   Used to transfer a Subscription and its MonitoredItems from one Session to
   another. For example, a Client may need to reopen a Session and then transfer
   its Subscriptions to that Session. It may also be used by one Client to take
   over a Subscription from another Client by transferring the Subscription to
   its Session.

.. _information-modelling:

Information Modelling
---------------------

Information modelling in OPC UA combines concepts from object-orientation and
semantic modelling. At the core, an OPC UA information model is a graph
consisting of Nodes and References between them.

Nodes
  There are eight possible NodeClasses for Nodes (Variable, VariableType,
  Object, ObjectType, ReferenceType, DataType, Method, View). The NodeClass
  defines the attributes a Node can have.

References
  References are links between Nodes. References are typed (refer to a
  ReferenceType) and directed.

The original source for the following information is Part 3 of the OPC UA
specification (https://reference.opcfoundation.org/Core/Part3/).

Each Node is identified by a unique (within the server) :ref:`nodeid` and
carries different attributes depending on the NodeClass. These attributes can be
read (and sometimes also written) via the OPC UA protocol. The protocol further
allows the creation and deletion of Nodes and References at runtime. But this is
not supported by all servers.

Reference are triples of the form ``(source-nodeid, referencetype-nodeid,
target-nodeid)``. (The ``target-nodeid`` is actually an :ref:`expandednodeid`
which is a NodeId that can additionally point to a remote server.) An example
reference between nodes is a ``hasTypeDefinition`` reference between a Variable
and its VariableType. Some ReferenceTypes are *hierarchical* and must not form
*directed loops*. See the section on :ref:`ReferenceTypes <referencetypenode>`
for more details on possible references and their semantics.

The following table (adapted from Part 3 of the specification) shows which
attributes are mandatory (``M``), optional (``O``) or not defined for each
NodeClass. In open62541 all optional attributes are defined - with sensible
defaults if users do not change them.

.. table:: Node attributes for the different NodeClasses
   :width: 100%

   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+
   | Attribute               | DataType                    | Variable | Variable­Type | Object | Object­Type | Reference­Type | Data­Type | Method | View  |
   +=========================+=============================+==========+===============+========+=============+================+===========+========+=======+
   | NodeId                  | NodeId                      |   ``M``  |     ``M``     |  ``M`` |    ``M``    |     ``M``      |   ``M``   |  ``M`` | ``M`` |
   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+
   | NodeClass               | NodeClass                   |   ``M``  |     ``M``     |  ``M`` |    ``M``    |     ``M``      |   ``M``   |  ``M`` | ``M`` |
   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+
   | BrowseName              | QualifiedName               |   ``M``  |     ``M``     |  ``M`` |    ``M``    |     ``M``      |   ``M``   |  ``M`` | ``M`` |
   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+
   | DisplayName             | LocalizedText               |   ``M``  |     ``M``     |  ``M`` |    ``M``    |     ``M``      |   ``M``   |  ``M`` | ``M`` |
   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+
   | Description             | LocalizedText               |   ``O``  |     ``O``     |  ``O`` |    ``O``    |     ``O``      |   ``O``   |  ``O`` | ``O`` |
   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+
   | WriteMask               | UInt32                      |   ``O``  |     ``O``     |  ``O`` |    ``O``    |     ``O``      |   ``O``   |  ``O`` | ``O`` |
   |                         | (:ref:`write-mask`)         |          |               |        |             |                |           |  ``M`` |       |
   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+
   | UserWriteMask           | UInt32                      |   ``O``  |     ``O``     |  ``O`` |    ``O``    |     ``O``      |   ``O``   |  ``O`` | ``O`` |
   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+
   | IsAbstract              | Boolean                     |          |     ``M``     |        |    ``M``    |     ``M``      |   ``M``   |        |       |
   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+
   | Symmetric               | Boolean                     |          |               |        |             |     ``M``      |           |        |       |
   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+
   | InverseName             | LocalizedText               |          |               |        |             |     ``O``      |           |        |       |
   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+
   | ContainsNoLoops         | Boolean                     |          |               |        |             |                |           |        | ``M`` |
   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+
   | EventNotifier           | Byte                        |          |               |  ``M`` |             |                |           |        | ``M`` |
   |                         | (:ref:`eventnotifier`)      |          |               |        |             |                |           |  ``M`` |       |
   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+
   | Value                   | Variant                     |   ``M``  |     ``O``     |        |             |                |           |        |       |
   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+
   | DataType                | NodeId                      |   ``M``  |     ``M``     |        |             |                |           |        |       |
   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+
   | ValueRank               | Int32                       |   ``M``  |     ``M``     |        |             |                |           |        |       |
   |                         | (:ref:`valuerank-defines`)  |          |               |        |             |                |           |  ``M`` |       |
   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+
   | ArrayDimensions         | [UInt32]                    |   ``O``  |     ``O``     |        |             |                |           |        |       |
   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+
   | AccessLevel             | Byte                        |   ``M``  |               |        |             |                |           |        |       |
   |                         | (:ref:`access-level-mask`)  |          |               |        |             |                |           |  ``M`` |       |
   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+
   | UserAccessLevel         | Byte                        |   ``M``  |               |        |             |                |           |        |       |
   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+
   | MinimumSamplingInterval | Double                      |   ``O``  |               |        |             |                |           |        |       |
   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+
   | Historizing             | Boolean                     |   ``M``  |               |        |             |                |           |        |       |
   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+
   | Executable              | Boolean                     |          |               |        |             |                |           |  ``M`` |       |
   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+
   | UserExecutable          | Boolean                     |          |               |        |             |                |           |  ``M`` |       |
   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+
   | DataTypeDefinition      | DataTypeDefinition          |          |               |        |             |                |   ``O``   |        |       |
   +-------------------------+-----------------------------+----------+---------------+--------+-------------+----------------+-----------+--------+-------+

Each attribute is referenced by a numerical :ref:`attribute-id`.

Some numerical attributes are used as bitfields or come with special semantics.
In particular, see the sections on :ref:`access-level-mask`, :ref:`write-mask`,
:ref:`valuerank-defines` and :ref:`eventnotifier`.

New attributes in the standard that are still unsupported in open62541 are
RolePermissions, UserRolePermissions, AccessRestrictions and AccessLevelEx.

VariableNode
~~~~~~~~~~~~

Variables store values in a :ref:`datavalue` together with
metadata for introspection. Most notably, the attributes data type, value
rank and array dimensions constrain the possible values the variable can take
on.

Variables come in two flavours: properties and datavariables. Properties are
related to a parent with a ``hasProperty`` reference and may not have child
nodes themselves. Datavariables may contain properties (``hasProperty``) and
also datavariables (``hasComponents``).

All variables are instances of some :ref:`variabletypenode` in return
constraining the possible data type, value rank and array dimensions
attributes.

Data Type
^^^^^^^^^

The (scalar) data type of the variable is constrained to be of a specific
type or one of its children in the type hierarchy. The data type is given as
a NodeId pointing to a :ref:`datatypenode` in the type hierarchy. See the
Section :ref:`datatypenode` for more details.

If the data type attribute points to ``UInt32``, then the value attribute
must be of that exact type since ``UInt32`` does not have children in the
type hierarchy. If the data type attribute points ``Number``, then the type
of the value attribute may still be ``UInt32``, but also ``Float`` or
``Byte``.

Consistency between the data type attribute in the variable and its
:ref:`VariableTypeNode` is ensured.

ValueRank
^^^^^^^^^

This attribute indicates whether the value attribute of the variable is an
array and how many dimensions the array has. It may have the following
values:

- ``n >= 1``: the value is an array with the specified number of dimensions
- ``n =  0``: the value is an array with one or more dimensions
- ``n = -1``: the value is a scalar
- ``n = -2``: the value can be a scalar or an array with any number of dimensions
- ``n = -3``: the value can be a scalar or a one dimensional array

Some helper macros for ValueRanks are defined :ref:`here <valuerank-defines>`.

The consistency between the value rank attribute of a VariableNode and its
:ref:`variabletypenode` is tested within the server.

Array Dimensions
^^^^^^^^^^^^^^^^

If the value rank permits the value to be a (multi-dimensional) array, the
exact length in each dimensions can be further constrained with this
attribute.

- For positive lengths, the variable value must have a dimension length less
  or equal to the array dimension length defined in the VariableNode.
- The dimension length zero is a wildcard and the actual value may have any
  length in this dimension. Note that a value (variant) must have array
  dimensions that are positive (not zero).

Consistency between the array dimensions attribute in the variable and its
:ref:`variabletypenode` is ensured. However, we consider that an array of
length zero (can also be a null-array with undefined length) has implicit
array dimensions ``[0,0,...]``. These always match the required array
dimensions.

.. _variabletypenode:

VariableTypeNode
~~~~~~~~~~~~~~~~

VariableTypes are used to provide type definitions for variables.
VariableTypes constrain the data type, value rank and array dimensions
attributes of variable instances. Furthermore, instantiating from a specific
variable type may provide semantic information. For example, an instance from
``MotorTemperatureVariableType`` is more meaningful than a float variable
instantiated from ``BaseDataVariable``.

ObjectNode
~~~~~~~~~~

Objects are used to represent systems, system components, real-world objects
and software objects. Objects are instances of an :ref:`object type<objecttypenode>`
and may contain variables, methods and further objects.

.. _objecttypenode:

ObjectTypeNode
~~~~~~~~~~~~~~

ObjectTypes provide definitions for Objects. Abstract objects cannot be
instantiated. See :ref:`node-lifecycle` for the use of constructor and
destructor callbacks.

.. _referencetypenode:

ReferenceTypeNode
~~~~~~~~~~~~~~~~~

Each reference between two nodes is typed with a ReferenceType that gives
meaning to the relation. The OPC UA standard defines a set of ReferenceTypes
as a mandatory part of OPC UA information models.

- Abstract ReferenceTypes cannot be used in actual references and are only
  used to structure the ReferenceTypes hierarchy
- Symmetric references have the same meaning from the perspective of the
  source and target node

The figure below shows the hierarchy of the standard ReferenceTypes (arrows
indicate a ``hasSubType`` relation). Refer to Part 3 of the OPC UA
specification for the full semantics of each ReferenceType.

.. graphviz::

   digraph tree {

   node [height=0, shape=box, fillcolor="#E5E5E5", concentrate=true]

   references [label="References\n(Abstract, Symmetric)"]
   hierarchical_references [label="HierarchicalReferences\n(Abstract)"]
   references -> hierarchical_references

   nonhierarchical_references [label="NonHierarchicalReferences\n(Abstract, Symmetric)"]
   references -> nonhierarchical_references

   haschild [label="HasChild\n(Abstract)"]
   hierarchical_references -> haschild

   aggregates [label="Aggregates\n(Abstract)"]
   haschild -> aggregates

   organizes [label="Organizes"]
   hierarchical_references -> organizes

   hascomponent [label="HasComponent"]
   aggregates -> hascomponent

   hasorderedcomponent [label="HasOrderedComponent"]
   hascomponent -> hasorderedcomponent

   hasproperty [label="HasProperty"]
   aggregates -> hasproperty

   hassubtype [label="HasSubtype"]
   haschild -> hassubtype

   hasmodellingrule [label="HasModellingRule"]
   nonhierarchical_references -> hasmodellingrule

   hastypedefinition [label="HasTypeDefinition"]
   nonhierarchical_references -> hastypedefinition

   hasencoding [label="HasEncoding"]
   nonhierarchical_references -> hasencoding

   hasdescription [label="HasDescription"]
   nonhierarchical_references -> hasdescription

   haseventsource [label="HasEventSource"]
   hierarchical_references -> haseventsource

   hasnotifier [label="HasNotifier"]
   hierarchical_references -> hasnotifier

   generatesevent [label="GeneratesEvent"]
   nonhierarchical_references -> generatesevent

   alwaysgeneratesevent [label="AlwaysGeneratesEvent"]
   generatesevent -> alwaysgeneratesevent

   {rank=same hierarchical_references nonhierarchical_references}
   {rank=same generatesevent haseventsource hasmodellingrule
              hasencoding hassubtype}
   {rank=same alwaysgeneratesevent hasproperty}

   }

The ReferenceType hierarchy can be extended with user-defined ReferenceTypes.
Many Companion Specifications for OPC UA define new ReferenceTypes to be used
in their domain of interest.

For the following example of custom ReferenceTypes, we attempt to model the
structure of a technical system. For this, we introduce two custom
ReferenceTypes. First, the hierarchical ``contains`` ReferenceType indicates
that a system (represented by an OPC UA object) contains a component (or
subsystem). This gives rise to a tree-structure of containment relations. For
example, the motor (object) is contained in the car and the crankshaft is
contained in the motor. Second, the symmetric ``connectedTo`` ReferenceType
indicates that two components are connected. For example, the motor's
crankshaft is connected to the gear box. Connections are independent of the
containment hierarchy and can induce a general graph-structure. Further
subtypes of ``connectedTo`` could be used to differentiate between physical,
electrical and information related connections. A client can then learn the
layout of a (physical) system represented in an OPC UA information model
based on a common understanding of just two custom reference types.

.. _datatypenode:

DataTypeNode
~~~~~~~~~~~~

DataTypes represent simple and structured data types. DataTypes may contain
arrays. But they always describe the structure of a single instance. In
open62541, DataTypeNodes in the information model hierarchy are matched to
``UA_DataType`` type descriptions for :ref:`generic-types` via their NodeId.

Abstract DataTypes (e.g. ``Number``) cannot be the type of actual values.
They are used to constrain values to possible child DataTypes (e.g.
``UInt32``).

.. _methodnode:

MethodNode
~~~~~~~~~~

Methods define callable functions and are invoked using the :ref:`Call <method-services>`
service. MethodNodes may have special properties (variable
children with a ``hasProperty`` reference) with the :ref:`qualifiedname` ``(0, "InputArguments")``
and ``(0, "OutputArguments")``. The input and output
arguments are both described via an array of ``UA_Argument``. While the Call
service uses a generic array of :ref:`variant` for input and output, the
actual argument values are checked to match the signature of the MethodNode.

Note that the same MethodNode may be referenced from several objects (and
object types). For this, the NodeId of the method *and of the object
providing context* is part of a Call request message.

ViewNode
~~~~~~~~

Each View defines a subset of the Nodes in the AddressSpace. Views can be
used when browsing an information model to focus on a subset of nodes and
references only. ViewNodes can be created and be interacted with. But their
use in the :ref:`Browse<view-services>` service is currently unsupported in
open62541.
