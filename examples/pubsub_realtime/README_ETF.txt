PRE-REQUISITES:
RT enabled kernel in nodes (say node1 and node2)

Ensure the nodes are ptp synchronized
PTP SYNCHRONIZATION:
Clone and install linuxptp version 2.0
    git clone https://github.com/richardcochran/linuxptp.git
    cd linuxptp/
    git checkout -f 059269d0cc50f8543b00c3c1f52f33a6c1aa5912
Install application
    make
    make install
    cp configs/gPTP.cfg /etc/linuxptp/
    cd ../ 

To make node as ptp master run the following command (say in node1):
    sudo daemonize -E BUILD_ID=dontKillMe -o /var/log/ptp4l.log -e /var/log/ptp4l.err.log /usr/bin/taskset -c 1 chrt 90 /usr/local/sbin/ptp4l -i $IFACE -2 -mq -f /etc/linuxptp/gPTP.cfg --step_threshold=1 --fault_reset_interval=0 --announceReceiptTimeout=10
To make node as ptp slave run the following command (say in node2):
    sudo daemonize -E BUILD_ID=dontKillMe -o /var/log/ptp4l.log -e /var/log/ptp4l.err.log /usr/bin/taskset -c 1 chrt 90 /usr/local/sbin/ptp4l -i $IFACE -2 -mq -s -f /etc/linuxptp/gPTP.cfg --step_threshold=1 --fault_reset_interval=0 --announceReceiptTimeout=10
To ensure phc2sys synchronization (in both nodes, node1 and node2):
    sudo daemonize -E BUILD_ID=dontKillMe -o /var/log/phc2sys.log -e /var/log/phc2sys.err.log /usr/bin/taskset -c 1 chrt 89 /usr/local/sbin/phc2sys -s $IFACE -c CLOCK_REALTIME --step_threshold=1 --transportSpecific=1 -w -m 

Check if /etc/modules has entry for 8021q
    cat /etc/modules | grep '8021q'

TO RUN ETF APPLICATIONS:
To run ETF applications over Ethernet in two nodes connected in peer-to-peer network -
    cd open62541/build
    cmake -DUA_BUILD_EXAMPLES=OFF -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING=ON -DUA_ENABLE_PUBSUB_ETH_UADP=ON -DUA_ENABLE_PUBSUB_ETH_UADP_ETF=ON ..
    make

Execute the following gcc command to build publisher and subscriber applications
gcc -O2 ../examples/pubsub_realtime/TSN_ETF_publisher.c -I../include -I../plugins/include -Isrc_generated -I../arch/posix -I../arch -I../plugins/networking -I../deps/ -I../src/server -I../src -I../src/pubsub bin/libopen62541.a -lrt -lpthread -D_GNU_SOURCE -o ./bin/TSN_ETF_publisher
gcc -O2 ../examples/pubsub_realtime/TSN_ETF_loopback.c -I../include -I../plugins/include -Isrc_generated -I../arch/posix -I../arch -I../plugins/networking -I../deps/ -I../src/server -I../src -I../src/pubsub bin/libopen62541.a -lrt -lpthread -D_GNU_SOURCE -o ./bin/TSN_ETF_loopback

The generated binaries are generated in build/bin/ folder
    ./bin/TSN_ETF_publisher <I210 interface> - Run in node 1
    ./bin/TSN_ETF_loopback <I210 interface>  - Run in node 2
Eg: ./bin/TSN_ETF_publisher enp2s0
    ./bin/TSN_ETF_loopback enp2s0

NOTE: It is always recommended to run TSN_XDP_mps_loopback application first to avoid missing the initial data published by TSN_XDP_mps_publisher
====================================================================================================================================================================
NOTE:
To change CYCLE_TIME for applications, change CYCLE_TIME macro value in the following files:
    examples/pubsub_realtime/TSN_ETF_loopback.c
    examples/pubsub_realtime/TSN_ETF_publisher.c

Eg: To run applications in 100us cycletime, modify CYCLE_TIME value as 0.10 in TSN_ETF_publisher.c and TSN_ETF_loopback.c applications

To modify the existing multicast MAC address with physical MAC address, modify
    PUBLISHING_MAC_ADDRESS to the MAC address of the other node
    SUBSCRIBING_MAC_ADDRESS to the MAC address of the same node

Eg: For TSN_ETF_publisher.c is run in node 1, PUBLISHING_MAC_ADDRESS is the MAC of node 2 and SUBSCRIBING_MAC_ADDRESS is the MAC of node 1
    For TSN_ETF_loopback.c is run in node 2, PUBLISHING_MAC_ADDRESS is the MAC of node 1 and SUBSCRIBING_MAC_ADDRESS is the MAC of node 2

To write the published counter data along with timestamp in csv uncomment #define UPDATE_MEASUREMENTS in TSN_ETF_publisher.c and TSN_ETF_loopback.c applications

NOTE: The CSV files will be generated in the build directory containing the timestamp and counter values for benchmarking only after terminating both the applications using “Ctrl + C”

To increase the payload size, change the REPEATED_NODECOUNTS macro in TSN_ETF_publisher.c and TSN_ETF_loopback.c applications (for 1 REPEATED_NODECOUNTS, 8 bytes of data will increase in payload)
Eg: To increase the payload to 64 bytes, change REPEATED_NODECOUNTS to 4
=====================================================================================================================================================================

TO RUN THE UNIT TEST CASES FOR ETHERNET FUNCTIONALITY:

Change the ethernet interface in #define ETHERNET_INTERFACE macro in
    check_pubsub_connection_ethernet.c
    check_pubsub_publish_ethernet.c
    check_pubsub_connection_ethernet_etf.c
    check_pubsub_publish_ethernet_etf.c

1. To test ethernet connection creation and ethernet publish unit tests(without realtime)
        cd open62541/build
        make clean
        cmake -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_PUBSUB_ETH_UADP=ON -DUA_BUILD_UNIT_TESTS=ON ..
        make
        The following binaries are generated in build/bin/tests folder
          ./bin/tests/check_pubsub_connection_ethernet - To check ethernet connection creation
          ./bin/tests/check_pubsub_publish_ethernet    - To check ethernet send functionality

2. To test ethernet connection creation and ethernet publish unit tests(with realtime)
        cd open62541/build
        make clean
        cmake -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING=OFF -DUA_ENABLE_PUBSUB_ETH_UADP=ON -DUA_ENABLE_PUBSUB_ETH_UADP_ETF=ON -DUA_BUILD_UNIT_TESTS=ON ..
        make
        The following binaries are generated in build/bin/tests folder
          ./bin/tests/check_pubsub_connection_ethernet_etf - To check ethernet connection creation with etf
          ./bin/tests/check_pubsub_publish_ethernet_etf - To check ethernet send functionality with etf


