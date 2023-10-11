Tested Environment:
Apollo Lake processor-1.60GHz with Intel i210 Ethernet Controller
OS - Debian/Lubuntu
Kernel - 4.19.37-rt19, 5.4.59-rt36
[Note: The pubsub TSN applications have two functionalities - ETF and XDP.
 ETF will work in all RT kernels above 4.19. XDP is tested only on 5.4.59-rt36 kernel and it may work on kernels above 5.4.
 If user wishes to run these pubsub TSN applications in the kernel below 4.19, there might be minimal changes required in the applications]

PRE-REQUISITES:
We recommend at least two Intel x86-based nodes with 4-cores and Intel i210 Ethernet Controllers connected in peer-peer fashion
RT enabled kernel in nodes (say node1 and node2)
Ensure the nodes are ptp synchronized
PTP SYNCHRONIZATION:
Clone and install linuxptp version 2.0
    git clone https://github.com/richardcochran/linuxptp.git
    cd linuxptp/
    git checkout -f 059269d0cc50f8543b00c3c1f52f33a6c1aa5912
    make
    make install
    cp configs/gPTP.cfg /etc/linuxptp/

Create VLAN with VLAN ID of 8 in peer to peer connected interface, copy paste the following lines in "/etc/network/interfaces" file and setup suitable IP address
    auto <i210 interface>.8
    iface <i210 interface>.8 inet static
        address <suitable IP address>
        netmask 255.255.255.0

To make node as ptp master run the following command (say in node1):
    sudo daemonize -E BUILD_ID=dontKillMe -o /var/log/ptp4l.log -e /var/log/ptp4l.err.log /usr/bin/taskset -c 1 chrt 90 /usr/local/sbin/ptp4l -i <I210 interface> -2 -mq -f /etc/linuxptp/gPTP.cfg --step_threshold=1 --fault_reset_interval=0 --announceReceiptTimeout=10 --transportSpecific=1
To make node as ptp slave run the following command (say in node2):
    sudo daemonize -E BUILD_ID=dontKillMe -o /var/log/ptp4l.log -e /var/log/ptp4l.err.log /usr/bin/taskset -c 1 chrt 90 /usr/local/sbin/ptp4l -i <I210 interface> -2 -mq -s -f /etc/linuxptp/gPTP.cfg --step_threshold=1 --fault_reset_interval=0 --announceReceiptTimeout=10 --transportSpecific=1
To ensure phc2sys synchronization (in both nodes, node1 and node2):
    sudo daemonize -E BUILD_ID=dontKillMe -o /var/log/phc2sys.log -e /var/log/phc2sys.err.log /usr/bin/taskset -c 1 chrt 89 /usr/local/sbin/phc2sys -s <I210 interface> -c CLOCK_REALTIME --step_threshold=1 --transportSpecific=1 -w -m

Check if ptp4l and phc2sys are running
    ptp4l log in /var/log/ptp4l.log
    phc2sys log in /var/log/phc2sys.log

Check if /etc/modules has entry for 8021q
    cat /etc/modules | grep '8021q'

For ETF Transmit: (In both nodes)
    #Configure ETF
    sudo tc qdisc add dev <I210 interface> parent root mqprio num_tc 3 map 2 2 1 0 2 2 2 2 2 2 2 2 2 2 2 2 queues 1@0 1@1 2@2 hw 0
    MQPRIO_NUM=`sudo tc qdisc show | grep mqprio | cut -d ':' -f1 | cut -d ' ' -f3`
    sudo tc qdisc add dev <I210 interface> parent $MQPRIO_NUM:1 etf offload clockid CLOCK_TAI delta 150000
    sudo tc qdisc add dev <I210 interface> parent $MQPRIO_NUM:2 etf offload clockid CLOCK_TAI delta 150000

In both nodes:
    for j in `seq 0 7`;do sudo ip link set <I210 interface>.8 type vlan egress $j:$j ; done
    for j in `seq 0 7`;do sudo ip link set <I210 interface>.8 type vlan ingress $j:$j ; done

TO RUN ETF APPLICATIONS:
To run ETF applications over Ethernet in two nodes connected in peer-to-peer network
    mkdir build
    cd build
    cmake -DUA_BUILD_EXAMPLES=ON -DUA_ENABLE_PUBSUB=ON ..
    make

The generated binaries are generated in build/bin/ folder
    ./bin/examples/pubsub_TSN_publisher -interface <I210 interface> - Run in node 1
    ./bin/examples/pubsub_TSN_loopback -interface <I210 interface>  - Run in node 2
Eg: ./bin/examples/pubsub_TSN_publisher -interface enp2s0
    ./bin/examples/pubsub_TSN_loopback -interface enp2s0

NOTE: It is always recommended to run pubsub_TSN_loopback application first to avoid missing the initial data published by pubsub_TSN_publisher

To know more usage
    ./bin/examples/pubsub_TSN_publisher -help
    ./bin/examples/pubsub_TSN_loopback -help

NOTE: To know more about running the OPC UA PubSub application and to evaluate performance, refer the Quick Start Guide - https://www.kalycito.com/how-to-run-opc-ua-pubsub-tsn/
============================================================================================================
You can also subscribe using XDP (Express Data Path) for faster processing the data, follow the below steps:

Pre-requisties for XDP:
    Minimum kernel version of 5.4 (Tested in RT enabled kernel - 5.4.59-rt36 with Debian OS)
    Install llvm and clang
        apt-get install llvm
        apt-get install clang
    Clone libbpf using the following steps:
        git clone https://github.com/libbpf/libbpf.git
        cd libbpf/
        git checkout ab067ed3710550c6d1b127aac6437f96f8f99447
        cd src/
        OBJDIR=/usr/lib make install

    Check if libbpf.so is available in /usr/lib folder, and bpf/ folder is available in /usr/include folder in both nodes (node1 and node2)

NOTE: Make sure the header file if_xdp.h available in /usr/include/linux/ is the right header file as in kernel version 5.4.59, if not update the if_xdp.h to newer version. (Verify your if_xdp.h file with https://elixir.bootlin.com/linux/v5.4.36/source/include/uapi/linux/if_xdp.h)

As per the sample application, XDP listens to Rx_Queue 2; to direct the incoming ethernet packets to the second Rx_Queue, the follow the given steps
    In both nodes:
    INGRESS POLICY steps:
        #Configure equal weightage to queue 0 and 1
        ethtool -X <I210 interface> equal 2
        #Disable VLAN offload
        ethtool -K <I210 interface> rxvlan off
        ethtool -K <I210 interface> ntuple on
        ethtool --config-ntuple <I210 interface> delete 15
        ethtool --config-ntuple <I210 interface> delete 14
        #Below command forwards the VLAN tagged packets to RX_Queue 2
        ethtool --config-ntuple <I210 interface> flow-type ether proto 0x8100 dst 01:00:5E:7F:00:01 loc 15 action 2
        ethtool --config-ntuple <I210 interface> flow-type ether proto 0x8100 dst 01:00:5E:7F:00:02 loc 14 action 2

To run ETF in Publisher and XDP in Subscriber in two nodes connected in peer-to-peer network
    cmake -DUA_BUILD_EXAMPLES=ON -DUA_ENABLE_PUBSUB=ON ..
    make
    By default XDP will be disabled in the Subscriber, use -enableXdpSubscribe to use XDP in RX.
    ./bin/examples/pubsub_TSN_publisher -interface <I210 interface> -enableXdpSubscribe - Run in node 1
    ./bin/examples/pubsub_TSN_loopback -interface <I210 interface> -enableXdpSubscribe - Run in node 2

Optional steps:
    By default, XDP listens to RX_Queue 2 with SKB and COPY mode.
    To make XDP listen to other RX_Queue, provide -xdpQueue <num> as arguments when executing applications and modify the action value (INGRESS POLICY) to the desired queue number.
    To enable ZEROCOPY mode, provide -xdpBindFlagZeroCopy as an additional argument to the applications
    To enable XDP Driver (DRV) mode, provide -xdpFlagDrvMode as an additional argument to the applications

NOTE: To enable XDP socket, pass its configuration parameters through connectionProperties (KeyValuePair) in connectionConfig as mentioned below. XDP socket has support
for both Publisher and Subscriber connection but it is recommended to use XDP only in Subscriber connection.

UA_KeyValuePair connectionOptions[4];                                                        // KeyValuePair for XDP configuration
connectionOptions[0].key                  = UA_QUALIFIEDNAME(0, "enableXdpSocket");          // Key for enabling the XDP socket
UA_Boolean enableXdp                      = UA_TRUE;                                         // Boolean value for enabling and disabling XDP socket
UA_Variant_setScalar(&connectionOptions[0].value, &enableXdp, &UA_TYPES[UA_TYPES_BOOLEAN]);
connectionOptions[1].key                  = UA_QUALIFIEDNAME(0, "xdpflag");                  // Key for the XDP flags
UA_UInt32 flags                           = xdpFlag;                                         // Value to determine whether XDP works in SKB mode or DRV mode 
UA_Variant_setScalar(&connectionOptions[1].value, &flags, &UA_TYPES[UA_TYPES_UINT32]);
connectionOptions[2].key                  = UA_QUALIFIEDNAME(0, "hwreceivequeue");           // Key for the hardware queue
UA_UInt32 rxqueue                         = xdpQueue;                                        // Value of the hardware queue to receive the packets
UA_Variant_setScalar(&connectionOptions[2].value, &rxqueue, &UA_TYPES[UA_TYPES_UINT32]);
connectionOptions[3].key                  = UA_QUALIFIEDNAME(0, "xdpbindflag");              // Key for the XDP bind flags
UA_UInt32 bindflags                       = xdpBindFlag;                                     // Value to determine whether XDP works in COPY or ZEROCOPY mode 
UA_Variant_setScalar(&connectionOptions[3].value, &bindflags, &UA_TYPES[UA_TYPES_UINT16]);
connectionConfig.connectionProperties     = connectionOptions;                               // Provide all the KeyValuePairs to properties of connectionConfig
connectionConfig.connectionPropertiesSize = 4;

To know more usage
    ./bin/examples/pubsub_TSN_publisher -help
    ./bin/examples/pubsub_TSN_loopback -help
===================================================================================================================================================================
NOTE:
If VLAN tag is used:
    Ensure the MAC address is given along with VLAN ID and PCP
    Eg: "opc.eth://01-00-5E-00-00-01:8.3" where 8 is the VLAN ID and 3 is the PCP
If VLAN tag is not used:
    Ensure the MAC address is not given along with VLAN ID and PCP
    Eg: "opc.eth://01-00-5E-00-00-01"
If MAC address is changed, follow INGRESS POLICY steps with dst address to be the same as SUBSCRIBING_MAC_ADDRESS for both nodes

To increase the payload size, change the REPEATED_NODECOUNTS macro in pubsub_TSN_publisher.c and pubsub_TSN_loopback.c applications (for 1 REPEATED_NODECOUNTS, 9 bytes of data will increase in payload)

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
        cmake -DUA_ENABLE_PUBSUB=ON -DUA_BUILD_UNIT_TESTS=ON ..
        make
        The following binaries are generated in build/bin/tests folder
          ./bin/tests/check_pubsub_connection_ethernet - To check ethernet connection creation
          ./bin/tests/check_pubsub_publish_ethernet    - To check ethernet send functionality

2. To test ethernet connection creation and ethernet publish unit tests(with realtime)
        cd open62541/build
        make clean
        cmake -DUA_ENABLE_PUBSUB=ON -DUA_BUILD_UNIT_TESTS=ON ..
        make
        The following binaries are generated in build/bin/tests folder
          ./bin/tests/check_pubsub_connection_ethernet_etf - To check ethernet connection creation with etf
          ./bin/tests/check_pubsub_publish_ethernet_etf    - To check ethernet send functionality with etf

TO RUN THE UNIT TEST CASES FOR XDP FUNCTIONALITY:

1. To test connection creation using XDP transport layer
        cd open62541/build
        make clean
        cmake -DUA_ENABLE_PUBSUB=ON -DUA_BUILD_UNIT_TESTS=ON ..
        make
        The following binary is generated in build/bin/tests folder
          ./bin/tests/check_pubsub_connection_xdp <I210 interface> - To check connection creation with XDP
