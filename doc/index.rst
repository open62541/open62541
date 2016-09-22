Introduction
============

open62541 (http://open62541.org) is an open source implementation of OPC UA (short for OPC Unified Architecture). open62541 is a C-based library (linking with C++ projects is possible) with all necessary tools to implement dedicated OPC UA clients and servers, or to integrate OPC UA-based communication into existing applications. open62541 is licensed under the LGPL with a static linking exception. So the *open62541 library can be used in projects that are not open source*. However, changes to the open62541 library itself need to published under the same license. The plugins, as well as the provided example servers and clients are in the public domain (CC0 license). They can be reused under any license and changes do not have to be published.

OPC Unified Architecture
------------------------

`OPC UA <http://en.wikipedia.org/wiki/OPC_Unified_Architecture>`_ is a protocol for industrial communication and has been standardized in the IEC 62541 series. At its core, OPC UA defines a set of services to interact with a server-side object-oriented information model. Besides the service-calls initiated by the client, push-notification (discrete events and data changes with a sampling interval) can be negotiated with the server. The client/server interaction is mapped either to a binary encoding and TCP-based transmission or to SOAP-based webservices. As of late, OPC UA is marketed as the one standard for non-realtime industrial communication.

In OPC UA, all communication is based on a set of predefined :ref:`services`, each consisting of a request and a response message. The services are designed for remote interaction with an object-oriented :ref:`information model <information-modelling>`. Note that services responses are asynchronous. So a server can decide to answer requests in a different order. Servers may also respond to requests after considerable delay. But this is commonly used only for subscriptions, i.e. push-notification of data changes and events.

The standard itself can be purchased from IEC or downloaded for free from the website of the OPC Foundation at https://opcfoundation.org/ (you only need to register with a valid email).

open62541 Features
------------------

open62541 implements the OPC UA binary protocol stack as well as a client and server SDK. It currently supports the Micro Embedded Device Server Profile plus some additional features. The final server binaries can be well under 100kb, depending on the size of the information model.

open62541 adheres to the OPC UA specification as closely as possible and the released features pass the official Conformance Testing Tools (CTT). However, the library comes without any warranty. If you intend to use OPC UA in a mission-critical product, please consider talking to a commercial vendor of OPC UA SDKs and services.

- Communication Stack

  - OPC UA binary protocol
  - Chunking (splitting of large messages)
  - Exchangeable network layer (plugin) for using custom networking APIs (e.g. on embedded targets)

- Information model

  - Support for all OPC UA node types (including method nodes)
  - Support for adding and removing nodes and references also at runtime.
  - Support for inheritance and instantiation of object- and variable-types (custom constructor/destructor, instantiation of child nodes)

- Subscriptions

  - Support for subscriptions/monitoreditems for data change notifications
  - Very low resource consumption for each monitored value (event-based server architecture)

- Code-Generation

  - Support for generating data types from standard XML definitions
  - Support for generating server-side information models (nodesets) from standard XML definitions

Features still missing in the 0.2 release are:

- Encryption
- Access control for individual nodes
- Events (notifications emitted by objects, data change notifications are implemented)
- Event-loop (background tasks) and asynchronous service requests in the client

Getting Help
------------

For discussion and help besides this documentation, you can reach the open62541 community via

- the `mailing list <https://groups.google.com/d/forum/open62541>`_
- our `IRC channel <http://webchat.freenode.net/?channels=%23open62541>`_
- the `bugtracker <https://github.com/open62541/open62541/issues>`_

Contributing
------------

As an open source project, we invite new contributors to help improve open62541. Issue reports, bugfixes and new features are very welcome. Note that there are ways to begin contributing without deep knowledge of the OPC UA standard:

- `Report bugs <https://github.com/open62541/open62541/issues>`_
- Improve the `documentation <http://open62541.org/doc/current>`_
- Work on issues marked as `easy hacks <https://github.com/open62541/open62541/labels/easy%20hack>`_
