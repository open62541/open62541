This changelog reports changes visible through the public API. Internal
refactorings and bug fixes are not reported here.

# Development

### New Realtime-PubSub model

The new Realtime-PubSub model builds upon two new public APIS: (i) The
possibility to integrate custom state machines to control the state of
PubSub-components and (ii) the generation of offset-tables for the content of
PubSub NetworkMessages. The approach is described in
/examples/pubsub_realtime/README.md with code examples in the same folder.

### JSON encoding changed with the v1.05 specification

The JSON encoding was reworked for the v1.05 version of the OPC UA
specification. The change breaks backwards compatibility. The legacy JSON
encoding is still available throught the UA_ENABLE_JSON_ENCODING_LEGACY build
option. This legacy feature wil get removed at some point in the future.

### PubSub NetworkMessage structure has an explicit DataSetMessageSize

In prior versions of the standard, when the PayloadHeader was missing, the
PubSub NetworkMessage needed to have exactly one DataSetMessage. In the current
standard there can be several DataSetMessages also without a PayloadHeader. To
allow for this the UA_NetworkMessage structure now contains an explicit
DataSetMessageSize field outside of the PayloadHeader.

Note that code could before rely on the default of one DataSetMessage. This code
needs to be revised to set the new DataSetMessageSize field to one.

### PubSub Components are disabled initially

PubSubComponents (PubSubConnections, ReaderGroups, ...) are no longer enabled
automatically after creating them. This makes the behavior uniform for all
PubSubComponents. And the configuration can be finalized prior to enabling. A
method UA_Server_enableAllPubSubComponents simplifies enabling the entire
system of configured components.

### PubSub Configuration freezing

The configuration is now "frozen" automatically when the state machine switches
to an enabled state (PAUSED / OPERATIONAL / PREOPERATIONAL). The freezing
mechanism is no longer exposed in the public API.

### Timer Simplification

For timed callbacks that are not repeated, use the same API
but with the UA_TIMERPOLICY_ONCE TimerPolicy.

### EventLoop Canceling

The `cancel` method of the EventLoop makes the (blocking) EventLoop `run` return
immediately.

### Unify PubSub PublisherId

Use the same definition for the PublisherId everywhere. `UA_PublisherId` is a
tagged union. Before the Subscriber API used a variant -- which is still used in
some standard-defined data types. `UA_PublisherId_fromVariant` and
`UA_PublisherId_toVariant` are provided for the conversion.

# Release 1.4

### Decoding variant with array of structure

When decoding an array of structures the extension objects that each structure
is wrapped into is now unwrapped if the array contains only a single type.
Previously the decoding API returned an array of extension objects. If the array
of extension objects has more than one type the decoding result will be an array
of extension objects.

### KeyValueMap in PubSub configuration

The UA_KeyValueMap structure is used everywhere instead of raw arrays of
UA_KeyValuePair.

### Thread-safe client

A large portion of the client API is now marked UA_THREADSAFE. For those
methods, if multithreading is enabled, an internal mutex protects the client.

### Saving unknown client certificates

'UA_CertificateVerification_CertFolders()' has been extended with an additional
parameter - path to the folder where the rejected certificate should be stored.
This change applies if `UA_ENABLE_CERT_REJECTED_DIR` is set.

### JSON decoding based on JSON5

The JSON decoding can parse the official encoding from the OPC UA specification.
It now further allows the following extensions:

- The strict JSON format is relaxed to also allow the JSON5 extensions
  (https://json5.org/). This allows for more human-readable encoding and adds
  convenience features such as trailing commas in arrays and comments within
  JSON documents.
- If `UA_ENABLE_PARSING` is set, NodeIds and ExpandedNodeIds can be given in the
  string encoding (see `UA_NodeId_parse`). The standard encoding is to express
  NodeIds as JSON objects.

These extensions are not intended to be used for the OPC UA protocol on the
network. They were rather added to allow more convenient configuration file
formats that also include data in the OPC UA type system.

### LocaleId support in the server

The server now supports that descriptions and display names for different
locales are written to a nod e. The server selects a suitable localization
depending on the locale ids that are set for the current session.

Locales are added simply by writing a LocalizedText value with a different
locale. A locale can be removed by writing a LocalizedText value of the
corresponding locale and with an empty text.

### PubSub PublisherId types

The union for the PubSub publisher id 'UA_PublisherId' and the enumeration for
the PubSub publisher type 'UA_PublisherIdType' have been changed to support all
specified publisher id types.

### PubSub state change functions

The following functions in ua_pubsub.h have been extended with an additional
parameter for the cause of the state change. Moreover the parameter order has
been changed.

- UA_DataSetWriter_setPubSubState
- UA_WriterGroup_setPubSubState
- UA_DataSetReader_setPubSubState
- UA_ReaderGroup_setPubSubState

### Server port and hostname configuration

Instead of configuring the port number in the server's "NetworkLayer", the
server configuration now takes a list of ServerURLs like
`opc.tcp://my-server:4840` or `opc.wss://localhost:443`. The URLs are used both
for discovery and to set up the server sockets based on the defined hostnames
(and ports).

The default is to listen on all interfaces with port 4840 for incoming TCP
connections.

### Empty arrays always match the array dimensions

We consider empty arrays (also null-arrays) to have implied array dimensions
[0,0,...]. So they always match the array dimensions defined in the variable.
With this, the initialization of variables without a defined value has been
simplified. We now create a null array if one is required.
