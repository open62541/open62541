To run the application for long runs, follow the below steps at the node where publisher is running
     mkdir build
     cd open62541/build
     ccmake ..
     Enable the following macros
          UA_BUILD_EXAMPLES
          UA_ENABLE_PUBSUB
          UA_ENABLE_PUBSUB_ETH_UADP
     make pubsub_TSN_publisher

To run the executable
./bin/examples/pubsub_TSN_publisher -interface <IFACE_NAME> -enableLongRunMeasurements

Follow the below steps at the node where loopback is running
     mkdir build
     cd open62541/build
     ccmake ..
     Enable the following macros
          UA_BUILD_EXAMPLES
          UA_ENABLE_PUBSUB
          UA_ENABLE_PUBSUB_ETH_UADP
     make pubsub_TSN_loopback

To run the executable,
./bin/examples/pubsub_TSN_loopback -interface <IFACE_NAME>

For advanced run options, see the help menu by using the below command
./bin/examples/pubsub_TSN_publisher -help
./bin/examples/pubsub_TSN_loopback -help