# open62541 Realtime OPC UA PubSub Publisher

This example is a self-contained PubSub publisher over raw Ethernet. It
showcases the realtime-capabilities of OPC UA PubSub. The core idea is that the
publisher callback can be triggered from a time-triggered system interrupt and
sends out the PubSub message within the interrupt.

The publisher retrieves its configuration and the payload data from the
information model of an OPC UA server. (It could also be run in a standalone
mode without an OPC UA server.) Since the publisher interrupt preempts the
execution of the normal OPC UA server, the information model needs to be
consistent at every time (reentrant). The specific techniques used to make the
OPC UA server reentrant are described in this publication:

```
@inproceedings{pfrommer2018open,
  title={Open source OPC UA PubSub over TSN for realtime industrial communication},
  author={Pfrommer, Julius and Ebner, Andreas and Ravikumar, Siddharth and Karunakaran, Bhagath},
  booktitle={2018 IEEE 23rd International Conference on Emerging Technologies and Factory Automation (ETFA)},
  pages={1087--1090},
  year={2018},
  organization={IEEE}
}
```

Please cite if you use this work.

OPC UA PubSub for open62541 is funded by an industry consortium in the context
of an OSADL project (Open Source Automation Development Lab). Technical
development is conducted by Fraunhofer IOSB and Kalycito Infotech.

https://www.osadl.org/OPC-UA-TSN.opcua-tsn.0.html

## Realtime communication with Time-Sensitive Networking (TSN)

OPC UA PubSub can be used together with TSN for hard-realtime Ethernet-based
communication. Vendor-specific APIs are commonly used for TSN capabilities. This
example only uses the standard Linux API for raw Ethernet. Vendor-specific
examples may be added at a later time.

## Building the RT Publisher

The main open62541 library needs to be built with the following build options
enabled for the realtime PubSub example. Note that some of the other examples
supplied with open62541 will not link against the library with these build
options. For good timings, ensure that the `CMAKE_BUILD_TYPE` is set to
`Release`.

- UA_ENABLE_PUBSUB
- UA_ENABLE_PUBSUB_ETH_UADP
- UA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING
- UA_ENABLE_MALLOC_SINGLETON
- UA_ENABLE_IMMUTABLE_NODES

The publisher contains some hard-coded values that need to be adjusted to
specific systems. Please check the top definitions in
`pubsub_interrupt_publish.c` and `start_rt_publish.sh`.

The publisher code is built and linked against the main open62541 library as follows:

`gcc -O2 ../examples/pubsub_realtime/pubsub_interrupt_publish.c ../examples/pubsub_realtime/bufmalloc.c -I../include -I../plugins/include -Isrc_generated -I../arch/posix -I../arch -I../plugins/networking bin/libopen62541.a -lrt -o rt_publisher`

## Running the RT Publisher

The publisher must be run as root for direct access to the Ethernet interface.

`# ./rt_publisher`

The example contains a script to be used with RT-Preempt Linux kernel. The
following command starts the publisher, locks the process to a specific CPU, and
sets the scheduling policy.

`# start_rt_publish.sh ./rt_publisher`

The measurements are written to a file (publisher_measurement.csv) with the
following fields for every publish callback:

- Counter
- Publication Interval
- Nominal time for the current publish
- Start delay from the nominal time
- Duration of the publish callback
