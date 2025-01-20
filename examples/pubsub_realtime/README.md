# open62541 Realtime-PubSub Support

Two features of open625451 enable realtime Pubsub: The custom state machine for
PubSub-Components and the generation of offset tables for the PubSub
NetworkMessage. Both features are showcased in the respective examples for
Publishers and Subscribers.

## Custom State Machine

The custom state machine mechanism lets user-defined code control the state of
the PubSub-Components (PubSubConnection, ReaderGroup, Reader, ...). Hence the
user-defined code is also responsible to register time-based events, network
connections, and so on.

This allows the integration with non-POSIX networking APIs for publishers and
subscribers. Note that the provided examples are still POSIX based, but handle
all sockets in userland.

## NetworkMessage Offset Table

Typically realtime PubSub is used together with the periodic-fixed header layout
where the position of the content in the NetworkMessage does not change between
publish cycles. Then the NetworkMessage offset tables can be generated to speed
up the updating and parsing of NetworkMessages. For the relevant fields, the
well-known offsets are computed ahead of time. Each offset furthermore has a
known source (the NodeId of the respective PubSub-Component in the information
model).

The NetworkMessage offset table is usually generated when the
ReaderGroup/WriterGroup changes its state to OPERATIONAL. At this time, from the
offset table, each field of the NetworkMessage is resolved to point to a backend
information source with fast access. Then the updating/parsing of the
NetworkMessage in the publish interval is simply a matter of looping over the
offset table and performing mostly memcpy steps.
