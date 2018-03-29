Introduction
============

open62541 (http://open62541.org) is an open source and free implementation of
OPC UA (OPC Unified Architecture) written in the common subset of the C99 and
C++98 languages. The library is usable with all major compilers and provides the
necessary tools to implement dedicated OPC UA clients and servers, or to
integrate OPC UA-based communication into existing applications. open62541
library is platform independent. All platform-specific functionality is
implemented via exchangeable plugins. Plugin implementations are provided for
the major operating systems.

open62541 is licensed under the Mozilla Public License v2.0. So the *open62541
library can be used in projects that are not open source*. Only changes to the
open62541 library itself need to published under the same license. The plugins,
as well as the server and client examples are in the public domain (CC0
license). They can be reused under any license and changes do not have to be
published.

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
website of the OPC Foundation at https://opcfoundation.org/ (you need to
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

- Communication Stack

  - OPC UA binary protocol
  - Chunking (splitting of large messages)
  - Exchangeable network layer (plugin) for using custom networking APIs (e.g. on embedded targets)
  - Encrypted communication
  - Asynchronous service requests in the client

- Information model

  - Support for all OPC UA node types (including method nodes)
  - Support for adding and removing nodes and references also at runtime.
  - Support for inheritance and instantiation of object- and variable-types (custom constructor/destructor, instantiation of child nodes)
  - Access control for individual nodes

- Subscriptions

  - Support for subscriptions/monitoreditems for data change notifications
  - Very low resource consumption for each monitored value (event-based server architecture)

- Code-Generation

  - Support for generating data types from standard XML definitions
  - Support for generating server-side information models (nodesets) from standard XML definitions

Features on the roadmap for the 0.3 release series but missing in the initial v0.3 release are:

- Encrypted communication in the client
- Events (notifications emitted by objects, data change notifications are implemented)
- Event-loop (background tasks) in the client

Getting Help
------------

For discussion and help besides this documentation, you can reach the open62541 community via

- the `mailing list <https://groups.google.com/d/forum/open62541>`_
- our `IRC channel <http://webchat.freenode.net/?channels=%23open62541>`_
- the `bugtracker <https://github.com/open62541/open62541/issues>`_

Contributing
------------

As an open source project, we invite new contributors to help improve open62541.
Issue reports, bugfixes and new features are very welcome. The following are
good starting points for new contributors:

- `Report bugs <https://github.com/open62541/open62541/issues>`_
- Improve the `documentation <http://open62541.org/doc/current>`_
- Work on issues marked as `good first issue <https://github.com/open62541/open62541/labels/good%20first%20issue>`_
