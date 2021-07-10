To run the application for long runs, follow the below steps at the node where publisher is running
     mkdir build
     cd open62541/build
     ccmake ..
     Enable the following macros
          UA_BUILD_EXAMPLES
          UA_ENABLE_PUBSUB
          UA_ENABLE_PUBSUB_ETH_UADP
     make -j4

To run the executable
./bin/examples/pubsub_TSN_publisher -interface <IFACE_NAME> -enableBlockingSocket -enableLongRunMeasurements -pubMacAddress opc.eth://<MAC_ofnoderunning_pubsub_TSN_loopback>:8.3 -subMacAddress opc.eth://<Self_MAC>:8.3 > application_log.txt 2>&1 &

Example: ./bin/examples/pubsub_TSN_publisher -interface enp2s0 -enableBlockingSocket -enableLongRunMeasurements -pubMacAddress opc.eth://00-d0-93-46-b2-fc:8.3 -subMacAddress opc.eth://00-d0-93-46-b2-fc:8.3 > application_log.txt 2>&1 &

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
./bin/examples/pubsub_TSN_loopback -interface <IFACE_NAME> -enableBlockingSocket -enableLongRunMeasurements -pubMacAddress opc.eth://<MAC_ofnoderunning_pubsub_TSN_publisher>:8.3 -subMacAddress opc.eth://<Self_MAC>:8.3 > application_log.txt 2>&1 &

Example: ./bin/examples/pubsub_TSN_loopback -interface enp2s0 -enableBlockingSocket -enableLongRunMeasurements -pubMacAddress opc.eth://00-d0-93-46-b2-ed:8.3 -subMacAddress opc.eth://00-d0-93-46-b2-ed:8.3 > application_log.txt 2>&1 &

For advanced run options, see the help menu by using the below command
./bin/examples/pubsub_TSN_publisher -help
./bin/examples/pubsub_TSN_loopback -help