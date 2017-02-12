Introduction
============

open62541 (http://open62541.org) is an open source implementation of OPC UA
(short for OPC Unified Architecture). open62541 is a C-based library (linking
with C++ projects is possible) with all necessary tools to implement dedicated
OPC UA clients and servers, or to integrate OPC UA-based communication into
existing applications. open62541 is licensed under Mozilla Public License v2.0.
So the *open62541 library can be used in projects that are
not open source*. However, changes to the open62541 library itself need to
published under the same license. The plugins, as well as the provided example
servers and clients are in the public domain (CC0 license). They can be reused
under any license and changes do not have to be published.

OPC Unified Architecture
------------------------

`OPC UA <http://en.wikipedia.org/wiki/OPC_Unified_Architecture>`_ is a protocol
for industrial communication and has been standardized in the IEC 62541 series.
At its core, OPC UA defines

- an asynchronous :ref:`protocol<protocol>` (built upon TCP, HTTP or SOAP) that
  defines the exchange of messages via sessions, (on top of) secure
  communication channels, (on top of) raw connections,
- a :ref:`type system<types>` for protocol messages with a binary and XML-based
  encoding scheme,
- a meta-model for :ref:`information modeling<information-modelling>`, that
  combines object-orientation with semantic triple-relations, and
- a set of 37 standard :ref:`services<services>` to interact with server-side
  information models. The signature of each service is defined as a request and
  response message in the protocol type system.

The standard itself can be purchased from IEC or downloaded for free on the
website of the OPC Foundation at https://opcfoundation.org/ (you only need to
register with a valid email).

The OPC Foundation drives the continuous improvement of the standard and the
development of companion specifications. Companion specifications translate
established concepts and reusable components from an application domain into OPC
UA. They are created jointly with an established industry council or
standardization body from the application domain. Furthermore, the OPC
Foundation organizes events for the dissemination of the standard and provides
the infrastructure and tools for compliance certification.

open62541 Features
------------------

open62541 implements the OPC UA binary protocol stack as well as a client and
server SDK. It currently supports the Micro Embedded Device Server Profile plus
some additional features. Server binaries can be well under 100kb in size,
depending on the contained information model.

open62541 adheres to the OPC UA specification as closely as possible and the
released features pass the official Conformance Testing Tools (CTT). However,
the library comes without any warranty. If you intend to use OPC UA in a
mission-critical product, please consider talking to a commercial vendor of OPC
UA SDKs and services.

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

As an open source project, we invite new contributors to help improve open62541.
Issue reports, bugfixes and new features are very welcome. Note that there are
ways to begin contributing without deep knowledge of the OPC UA standard:

- `Report bugs <https://github.com/open62541/open62541/issues>`_
- Improve the `documentation <http://open62541.org/doc/current>`_
- Work on issues marked as `easy hacks <https://github.com/open62541/open62541/labels/easy%20hack>`_
