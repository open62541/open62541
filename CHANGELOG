This changelog reports changes to the public API. Internal refactorings and bug
fixes are not reported here.

2018-02-05 pro <profanter at fortiss.org>

 * Also pass client to monitoredItem/Events callback

   The UA_MonitoredItemHandlingFunction and UA_MonitoredEventHandlingFunction
   did not include a reference to the corresponding client used for the call of
   processPublishResponse.
   This now also allows to get the client context within the monitored item
   handling function.

   The API changes are detected by the type-matching in the compiler. So there
   is little risk for bugs due to unaligned implementations.

2017-09-07 jpfr <julius.pfrommer at web.de>

 * Make Client Highlevel Interface consistent

   Two methods in the client highlevel interface were not conformant with the
   API convention that array sizes are given before the array pointer. This
   changes the signature of UA_Client_writeArrayDimensionsAttribute and
   UA_Client_readArrayDimensionsAttribute.

   The API changes are detected by the type-matching in the compiler. So there
   is little risk for bugs due to unaligned implementations.

2017-08-16 jpfr <julius.pfrommer at web.de>

 * Default Attribute Values for Node Attributes

   The AddNodes service takes a structure with the attributes of the new node as
   input. We began checking the consistency of the attributes more closely. You
   can now use constant global definitions with default values as a starting
   point. For example UA_VariableAttributes_default for adding a variable node.

 * All nodes have a context pointer

   All nodes now carry a context pointer. The context pointer is initially set
   to a user-defined memory location. The context pointer is passed to all
   user-visible callbacks involving the node.

 * Global and node-type constructor/destructor

   The constructors are fine-grained mechanisms to control the instantiation and
   deletion of nodes. Constructors receive the node context pointer and can also
   replace it.

2017-08-13 Infinity95 <mark.giraud at student.kit.edu>

 * New/Delete methods for the server configuration

   The default server configuration is no longer a constant global variable.
   Instead, it is now handled with a pair of new/delete methods. This enables
   the use of malloced members of the server configuration. With this change,
   the endpoint descriptions are no longer generated in the server but are
   created as part of the server configuration. See the example server
   implementations on how to use the configuration generation. The change is a
   precursor to the encryption implementation that ties into the generation of
   endpoint descriptions in the server config.

   The generation of the default server configuration is implemented as a
   "plugin". So it comes with CC0 licensing and can be freely adjusted by users.

   The TCP port in the default configuration is 4840. This is the recommended
   port for OPC UA and now used throughout all examples.

2017-07-04 jpfr <julius.pfrommer at web.de>

 * Return partially overlapping ranges

   Test the integrity of the range and compute the max index used for every
   dimension. The standard says in Part 4, Section 7.22:

   When reading a value, the indexes may not speify a range that is within
   the bounds of the array. The Server shall return a partial result if some
   elements exist within the range.

2017-07-03 jpfr <julius.pfrommer at web.de>

 * Implement asynchronous service calls for the client

   All OPC UA services are asynchronous in nature. So several service calls can
   be made without waiting for a response first. Responess may come in a
   different ordering. The client takes a method pointer and a data pointer to
   perform an asynchronous callback on the request response.

   Synchronous service calls are still supported in the client. Asynchronous
   responses are processed in the background until the synchronous response (the
   client is waiting for) returns the control flow back to the user.

2017-06-26 janitza-thbe

 * Enable IPv6 in the networklayer plugin

   The server networklayer listens on several sockets for available networks and
   IP versions. IPv4 connections are still supported.

   The OPC Foundation ANSI C Stack before 2016 does not fully support IPv6. On
   Windows, the 'localhost' target is resolved to IPv6 by default. Old
   applications (e.g. the Conformance Testing Tools) need to connect to
   127.0.0.1 instead of 'localhost' to force IPv4.

2017-06-16 jpfr <julius.pfrommer at web.de>

 * Require the AccessLevel bit UA_ACCESSLEVELMASK_READ for reading

   Set the bit by default when adding nodes for a smooth transition to the new
   API. This will change at a later point with an additional node settings
   argument for the AddNodes methods.

2017-05-03 pro <profanter at fortiss.org>

 * Array dimensions are UInt32 also for the highlevel client read service

2017-04-16 jpfr <julius.pfrommer at web.de>

 * Refactor UA_parseEndpointUrl to work with UA_String

   The output hostname and path now point into the original endpointUrl with an
   appropriate length.

2017-04-14 jpfr <julius.pfrommer at web.de>

 * Auto-instantiate only child nodes marked as mandatory (fixes #1004)
