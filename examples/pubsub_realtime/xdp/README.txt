NOTE:
1. To change cycle time of application, change CYCLE_TIME macro in
       open62541/plugins/ua_pubsub_realtime.c to 100*1000
       open62541/examples/pubsub_realtime/xdp/TSN_XDP_mps_loopback.c to 100*1000
       open62541/examples/pubsub_realtime/xdp/TSN_XDP_mps_publisher.c to 100*1000
2. To view the data written in csv files, uncomment the following #define in TSN_XDP_mps_loopback.c and TSN_XDP_mps_publisher.c
       #define             UPDATE_MEASUREMENTS

Pre-requisites for XDP:
    1. Clone bpf-next-4.19 from github in the location /usr/src/
        cd /usr/src/
        git clone https://github.com/xdp-project/bpf-next.git
        cd bpf-next/
        git checkout 84df9525b0c27f3ebc2ebb1864fa62a97fdedb7d
    2. Install llvm and clang
    3. Steps to generate libbpf.a in bpf-next-4.19
        make defconfig
        make headers_install
        cd samples/bpf
        make

Pre-requisites for TSN_XDP_mps_publisher.c and TSN_XDP_mps_loopback.c($INTERFACE is the interface to which the second node is connected to):
In node using TSN_XDP_mps_publisher.c:
    1. Configure ETF:
        NOTE: Make sure iproute2 is installed in the node
        sudo tc qdisc add dev $INTERFACE parent root mqprio num_tc 3 map 2 2 1 0 2 2 2 2 2 2 2 2 2 2 2 2 queues 1@0 1@1 2@2 hw 0
        MQPRIO_NUM=`sudo tc qdisc show | grep mqprio | cut -d ':' -f1 | cut -d ' ' -f3`
        sudo tc qdisc add dev $INTERFACE parent $MQPRIO_NUM:1 etf offload clockid CLOCK_TAI delta 150000
        sudo tc qdisc add dev $INTERFACE parent $MQPRIO_NUM:2 etf offload clockid CLOCK_TAI delta 150000
    2. Configure Multicast MAC
        ip link set $INTERFACE promisc off
        /* Enable filtering of multicast MAC at the interface using ntuple */
        ethtool -K $INTERFACE ntuple on
        ethtool --config-ntuple  $INTERFACE flow-type ether dst 01:00:5E:00:00:01 loc 15
    3. Configure ingress queue
        /* Add ingress rule to redirect packet with given destination MAC to second ingress queue */
        tc qdisc delete dev $INTERFACE ingress
        tc qdisc add dev $INTERFACE ingress
        tc filter add dev $INTERFACE parent ffff: proto 0xb62c flower dst_mac 01:00:5E:00:00:01 hw_tc 2 skip_sw
    4. Set VLAN id 8 for interface with ingress and egress mapping
        sudo vconfig set_ingress_map $IFACE.8 0 0
        sudo vconfig set_ingress_map $IFACE.8 1 1
        sudo vconfig set_ingress_map $IFACE.8 2 2
        sudo vconfig set_ingress_map $IFACE.8 3 3
        sudo vconfig set_ingress_map $IFACE.8 4 4
        sudo vconfig set_ingress_map $IFACE.8 5 5
        sudo vconfig set_ingress_map $IFACE.8 6 6
        sudo vconfig set_ingress_map $IFACE.8 7 7

        sudo vconfig set_egress_map $IFACE.8 0 0
        sudo vconfig set_egress_map $IFACE.8 1 1
        sudo vconfig set_egress_map $IFACE.8 2 2
        sudo vconfig set_egress_map $IFACE.8 3 3
        sudo vconfig set_egress_map $IFACE.8 4 4
        sudo vconfig set_egress_map $IFACE.8 5 5
        sudo vconfig set_egress_map $IFACE.8 6 6
        sudo vconfig set_egress_map $IFACE.8 7 7
    5. ethtool -X $IFACE equal 2
    6. ethtool -K $IFACE rxvlan off


In node using TSN_XDP_mps_loopback.c:
    1. Configure ETF:
        NOTE: Make sure iproute2 is installed in the node
        sudo tc qdisc add dev $INTERFACE parent root mqprio num_tc 3 map 2 2 1 0 2 2 2 2 2 2 2 2 2 2 2 2 queues 1@0 1@1 2@2 hw 0
        MQPRIO_NUM=`sudo tc qdisc show | grep mqprio | cut -d ':' -f1 | cut -d ' ' -f3`
        sudo tc qdisc add dev $INTERFACE parent $MQPRIO_NUM:1 etf offload clockid CLOCK_TAI delta 150000
        sudo tc qdisc add dev $INTERFACE parent $MQPRIO_NUM:2 etf offload clockid CLOCK_TAI delta 150000
    2. Configure Multicast MAC
        ip link set $INTERFACE promisc off
        /* Enable filtering of multicast MAC at the interface using ntuple */
        ethtool -K $INTERFACE ntuple on
        ethtool --config-ntuple  $INTERFACE flow-type ether dst 01:00:5E:7F:00:01 loc 15
    3. Configure ingress queue
        /* Add ingress rule to redirect packet with given destination MAC to second ingress queue */
        tc qdisc delete dev $INTERFACE ingress
        tc qdisc add dev $INTERFACE ingress
        tc filter add dev $INTERFACE parent ffff: proto 0xb62c flower dst_mac 01:00:5E:7F:00:01 hw_tc 2 skip_sw
    4. Set VLAN id 8 for interface with ingress and egress mapping
        sudo vconfig set_ingress_map $IFACE.8 0 0
        sudo vconfig set_ingress_map $IFACE.8 1 1
        sudo vconfig set_ingress_map $IFACE.8 2 2
        sudo vconfig set_ingress_map $IFACE.8 3 3
        sudo vconfig set_ingress_map $IFACE.8 4 4
        sudo vconfig set_ingress_map $IFACE.8 5 5
        sudo vconfig set_ingress_map $IFACE.8 6 6
        sudo vconfig set_ingress_map $IFACE.8 7 7

        sudo vconfig set_egress_map $IFACE.8 0 0
        sudo vconfig set_egress_map $IFACE.8 1 1
        sudo vconfig set_egress_map $IFACE.8 2 2
        sudo vconfig set_egress_map $IFACE.8 3 3
        sudo vconfig set_egress_map $IFACE.8 4 4
        sudo vconfig set_egress_map $IFACE.8 5 5
        sudo vconfig set_egress_map $IFACE.8 6 6
        sudo vconfig set_egress_map $IFACE.8 7 7
    5. ethtool -X $IFACE equal 2
    6. ethtool -K $IFACE rxvlan off

Steps to build open62541 with XDP integrated examples:
    1. cd open62541
    2. mkdir build && cd build
    3. cmake -DUA_BUILD_EXAMPLES=ON -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING=ON -DUA_ENABLE_PUBSUB_ETH_UADP=ON -DUA_ENABLE_PUBSUB_ETH_UADP_XDP_RECV=ON ..
    4. make

Steps to run TSN_XDP_mps_publisher.c and TSN_XDP_mps_loopback.c
    1. ./TSN_XDP_mps_publisher from /build/bin/examples folder
    2. ./TSN_XDP_mps_loopback from /build/bin/examples folder
    NOTE: Make sure the interface given in TSN_XDP_mps_publisher.c and TSN_XDP_mps_loopback.c are right
          #define             PUBLISHING_VLAN_INTERFACE
          #define             SUBSCRIBING_INTERFACE

Peer-Peer communication can be done with these steps, in order to use the code in a 4 node network,
make sure that the PublisherId for each publisher is unique.
Ingress queue(Q2) should be configured correctly to receive the published packets in all nodes.
