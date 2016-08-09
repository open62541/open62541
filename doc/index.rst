Welcome to open62541's documentation!
=====================================

`OPC UA <http://en.wikipedia.org/wiki/OPC_Unified_Architecture>`_ (short for OPC
Unified Architecture) is a protocol for industrial communication and has been
standardized in the IEC62541. At its core, OPC UA defines a set of services to
interact with a server-side object-oriented information model. Besides the
service-calls initiated by the client, push-notification of remote events (such
as data changes) can be negotiated with the server. The client/server
interaction is mapped either to a binary encoding and TCP-based transmission or
to SOAP-based webservices. As of late, OPC UA is marketed as the one standard
for non-realtime industrial communication.

We believe that it is best to understand OPC UA *from the inside out*, building
upon conceptually simple first principles. After establishing a first
understanding, we go on explaining how these principles are realized in detail.
Examples are given based on the *open62541* implementation of the
standard.

.. toctree::
   :maxdepth: 3

   in_a_nutshell
   building
   tutorials
   types
   constants
   server
   client
   internal
