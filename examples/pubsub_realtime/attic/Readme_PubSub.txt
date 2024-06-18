Tested Environment:
TTTech IP (2.3.0) - Arm architecture
Kernel - 5.4.40 (non-RT)
============================================================================================================
PRE-REQUISITES:
1) We recommend at least two TTTech TSN IP nodes with 2-cores

2) Ensure the nodes are ptp synchronized
    deptp_tool --get-current-dataset

3) Add and aditional clock domain /etc/deptp/ptp_config.xml, reboot the board after applying this change (this is a one time step)
    <Clock>
       <clock_class>187</clock_class>
       <clock_accuracy>1us</clock_accuracy>
       <clock_priority1>247</clock_priority1>
       <clock_priority2>248</clock_priority2>
       <domain>100</domain>
       <time_source>internal oscillator</time_source>
     </Clock>

4) Create VLAN with VLAN ID of 8 in peer to peer connected interface using the below command
    ip link add link <interfaceName> name myvlan type vlan id 8 ingress-qos-map 0:7 1:7 2:7 3:7 4:7 5:7 6:7 7:7 egress-qos-map 0:7 1:7 2:7 3:7 4:7 5:7 6:7 7:7
    ip addr add <VlanIpAddress> dev myvlan
    ip link set dev myvlan up
    bridge vlan add vid 8 dev <portName>
    bridge vlan add vid 8 dev <portName2>            (optional e.g. for monitoring)
    bridge vlan add vid 8 dev <InternalPortName>
    ip route add 224.0.0.0/24 dev myvlan

    NOTE: <portName> refers to the external physical port that has ethernet cable connected to it in each node

5) Create the Qbv configuration i.e setting up the gating configuration:
    1)Create a config file on both the nodes using the below command
            touch qbv.cfg
    2)At node 1 copy the below gating configuration and paste it in the qbv.cfg
            sgs 5000 0x80
            sgs 490000 0x7F
    3)At node 2 copy the below gating configuration and paste it in the qbv.cfg
            sgs 5000 0x80
            sgs 490000 0x7F 	
    4)To schedule the gating configuration run the below command on both the nodes
        tsntool st wrcl <portName> qbv.cfg
        tsntool st rdacl <portName> /dev/stdout
        tsntool st configure 5.0 1/2000 10000 <portName>
        tsntool st show <portName>

6) After these steps the following files shoud exist and have values:
        cat /run/ptp_wc_mon_offset
        cat /sys/class/net/<portName>/ieee8021ST/OperBaseTime
============================================================================================================
Cross Compiler options to compile:
1) To compile the binaries create the folder using the below command:
       mkdir build

2) Traverse to the folder using the command
       cd build

3) Install the arm cross compiler - arm-linux-gnueabihf

4) Cross compilation for ARM architecture
       CMAKE_C_COMPILER /usr/bin/arm-linux-gnueabihf-gcc
       CMAKE_C_COMPILER_AR        /usr/bin/arm-linux-gnueabihf-gcc-ar-7
       CMAKE_C_COMPILER_RANLIB /usr/bin/arm-linux-gnueabihf-gcc-ranlib-7
       CMAKE_C_FLAGS -Wno-error
       CMAKE_C_FLAGS_DEBUG -g -Wno-error
       CMAKE_C_LINKER /usr/bin/arm-linux-gnueabihf-ld
       CMAKE_NM /usr/bin/arm-linux-gnueabihf-nm
       CMAKE_OBJCOPY /usr/bin/arm-linux-gnueabihf-objcopy
       CMAKE_OBJDUMP /usr/bin/arm-linux-gnueabihf-objdump
       CMAKE_RANLIB /usr/bin/arm-linux-gnueabihf-ranlib
       CMAKE_STRIP /usr/bin/arm-linux-gnueabihf-strip

       Note: The above mentioned options are changes for arm compiler.Ignore them if you
       are compiling for x86. The options will be available in tools/cmake/Toolchain-ARM.cmake
       for cross compilation

       CMake options for Publisher application(pubsub_TSN_publisher_multiple_thread):
       cmake -DCMAKE_TOOLCHAIN_FILE=../tools/cmake/Toolchain-ARM.cmake -DUA_BUILD_EXAMPLES=ON -DUA_ENABLE_PUBSUB=ON ..
       make -j4 pubsub_TSN_publisher_multiple_thread

       CMake options for loopback application(pubsub_TSN_loopback_single_thread):
       cmake -DCMAKE_TOOLCHAIN_FILE=../tools/cmake/Toolchain-ARM.cmake -DUA_BUILD_EXAMPLES=ON -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_PUBSUB_BUFMALLOC=ON -DUA_ENABLE_MALLOC_SINGLETON=ON ..
       make -j4 pubsub_TSN_loopback_single_thread

5) Compilation for x86 architecture
       CMake options for Publisher application(pubsub_TSN_publisher_multiple_thread):
       cmake -DUA_BUILD_EXAMPLES=ON -DUA_ENABLE_PUBSUB=ON ..
       make -j4 pubsub_TSN_publisher_multiple_thread

       CMake options for loopback application(pubsub_TSN_loopback_single_thread):
       cmake -DUA_BUILD_EXAMPLES=ON -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_PUBSUB_BUFMALLOC=ON -DUA_ENABLE_MALLOC_SINGLETON=ON ..
       make -j4 pubsub_TSN_loopback_single_thread

============================================================================================================
To RUN the APPLICATIONS:
The generated binaries are generated in build/bin/ folder
============================================================================================================
TWO WAY COMMUNICATION
============================================================================================================

    For Ethernet(Without logs) For long run:
    ./pubsub_TSN_publisher_multiple_thread -interface <interfaceName> -disableSoTxtime -operBaseTime /sys/class/net/<portName>/ieee8021ST/OperBaseTime -monotonicOffset /run/ptp_wc_mon_offset -cycleTimeInMsec 0.5 -subAppPriority 99 -pubAppPriority 98 - Run in node 1

    ./pubsub_TSN_loopback_single_thread -interface <interfaceName> -disableSoTxtime -operBaseTime /sys/class/net/<portName>/ieee8021ST/OperBaseTime -monotonicOffset /run/ptp_wc_mon_offset -cycleTimeInMsec 0.5 -pubSubAppPriority 99  - Run in node 2

    For Ethernet:(With logs - Provide the counterdata and its time which helps to identify the roundtrip time): For Short run
    ./pubsub_TSN_publisher_multiple_thread -interface <interfaceName> -disableSoTxtime -operBaseTime /sys/class/net/<portName>/ieee8021ST/OperBaseTime -monotonicOffset /run/ptp_wc_mon_offset -cycleTimeInMsec 0.5 -enableCsvLog -subAppPriority 99 -pubAppPriority 98 - Run in node 1

    ./pubsub_TSN_loopback_single_thread -interface <interfaceName> -disableSoTxtime -operBaseTime /sys/class/net/<portName>/ieee8021ST/OperBaseTime -monotonicOffset /run/ptp_wc_mon_offset -cycleTimeInMsec 0.5 -enableCsvLog -pubSubAppPriority 99  - Run in node 2

    Note: Application will close after sending 100000 packets and you will get the logs in .csv files. To change the number of packets to capture for short run change the parameter #MAX_MEASUREMENTS define value in pubsub_TSN_publisher_multiple_thread and pubsub_TSN_loopback_single_thread

    For UDP(Without logs) For long run:
    For UDP only one change need to be done before running that is need to provide the interface ip address as an input for -interface option
    ./pubsub_TSN_publisher_multiple_thread -interface <VlanIpAddress> -disableSoTxtime -operBaseTime /sys/class/net/<portName>/ieee8021ST/OperBaseTime -monotonicOffset /run/ptp_wc_mon_offset -cycleTimeInMsec 0.5 -subAppPriority 99 -pubAppPriority 98 - Run in node 1

    ./pubsub_TSN_loopback_single_thread -interface <VlanIpAddress>  -disableSoTxtime -operBaseTime /sys/class/net/<portName>/ieee8021ST/OperBaseTime -monotonicOffset /run/ptp_wc_mon_offset -cycleTimeInMsec 0.5 -pubSubAppPriority 99  - Run in node 2

    For UDP:(With logs - Provide the counterdata and its time which helps to identify the roundtrip time): For Short run
    ./pubsub_TSN_publisher_multiple_thread -interface <VlanIpAddress> -disableSoTxtime -operBaseTime /sys/class/net/<portName>/ieee8021ST/OperBaseTime -monotonicOffset /run/ptp_wc_mon_offset -cycleTimeInMsec 0.5 -enableCsvLog -subAppPriority 99 -pubAppPriority 98 - Run in node 1

    ./pubsub_TSN_loopback_single_thread -interface <VlanIpAddress>  -disableSoTxtime -operBaseTime /sys/class/net/<portName>/ieee8021ST/OperBaseTime -monotonicOffset /run/ptp_wc_mon_offset -cycleTimeInMsec 0.5 -enableCsvLog -pubSubAppPriority 99  - Run in node 2

    NOTE: Application will close after sending 100000 packets and you will get the logs in .csv files. To change the number of packets to capture for short run
    change the parameter #MAX_MEASUREMENTS define value in pubsub_TSN_publisher_multiple_thread and pubsub_TSN_loopback_single_thread
============================================================================================================
ONE WAY COMMUNICATION: For one way communication disable(comment) the macro TWO_WAY_COMMUNICATION in the the pubsub_TSN_publisher_multiple_thread and pubsub_TSN_loopback_single_thread
============================================================================================================

     Run steps for Ethernet:
    ./pubsub_TSN_publisher_multiple_thread -interface <interfaceName> -disableSoTxtime -operBaseTime /sys/class/net/<portName>/ieee8021ST/OperBaseTime -monotonicOffset /run/ptp_wc_mon_offset -cycleTimeInMsec 0.5 -enableCsvLog -pubAppPriority - Run in node 1

    ./pubsub_TSN_loopback_single_thread -interface <interfaceName> -disableSoTxtime -operBaseTime /sys/class/net/<portName>/ieee8021ST/OperBaseTime -monotonicOffset /run/ptp_wc_mon_offset -cycleTimeInMsec 0.5 -enableCsvLog -pubSubAppPriority 99  - Run in node 2

     Run steps for UDP:
    ./pubsub_TSN_publisher_multiple_thread -interface <VlanIpAddress> -disableSoTxtime -operBaseTime /sys/class/net/<portName>/ieee8021ST/OperBaseTime -monotonicOffset /run/ptp_wc_mon_offset -cycleTimeInMsec 0.5 -enableCsvLog -pubAppPriority - Run in node 1

    ./pubsub_TSN_loopback_single_thread -interface <VlanIpAddress> -disableSoTxtime -operBaseTime /sys/class/net/<portName>/ieee8021ST/OperBaseTime -monotonicOffset /run/ptp_wc_mon_offset -cycleTimeInMsec 0.5 -enableCsvLog -pubSubAppPriority 99  - Run in node 2

NOTE: As we have mentioned previously for long run remove the option -enableCsvLog
============================================================================================================
NOTE: It is always recommended to run pubsub_TSN_loopback_single_thread application first to avoid missing the initial data published by pubsub_TSN_publisher_multiple_thread

To know more usage
    ./bin/examples/pubsub_TSN_publisher_multiple_thread -help
    ./bin/examples/pubsub_TSN_loopback_single_thread -help
============================================================================================================
NOTE:
For Ethernet
If VLAN tag is used:
    Ensure the MAC address is given along with VLAN ID and PCP
    Eg: "opc.eth://01-00-5E-00-00-01:8.3" where 8 is the VLAN ID and 3 is the PCP
If VLAN tag is not used:
    Ensure the MAC address is not given along with VLAN ID and PCP
    Eg: "opc.eth://01-00-5E-00-00-01"
If MAC address is changed, follow INGRESS POLICY steps with dst address to be the same as SUBSCRIBING_MAC_ADDRESS for both nodes

To increase the payload size, change the REPEATED_NODECOUNTS macro in pubsub_TSN_publisher_multiple_thread.c and pubsub_TSN_loopback_single_thread.c applications (for 1 REPEATED_NODECOUNTS, 9 bytes of data will increase in payload)

============================================================================================================
Output Logs: If application runs with option enableCsvLog
/**
 *  Trace point setup
 *
 *            +--------------+                        +----------------+
 *         T1 | OPCUA PubSub |  T8                 T5 | OPCUA loopback |  T4
 *         |  |  Application |  ^                  |  |  Application   |  ^
 *         |  +--------------+  |                  |  +----------------+  |
 *   User  |  |              |  |                  |  |                |  |
 *   Space |  |              |  |                  |  |                |  |
 *         |  |              |  |                  |  |                |  |
 *  ----------|--------------|------------------------|----------------|-------
 *         |  |    Node 1    |  |                  |  |     Node 2     |  |
 *   Kernel|  |              |  |                  |  |                |  |
 *   Space |  |              |  |                  |  |                |  |
 *         |  |              |  |                  |  |                |  |
 *         v  +--------------+  |                  v  +----------------+  |
 *         T2 |  TX tcpdump  |  T7<----------------T6 |   RX tcpdump   |  T3
 *         |  +--------------+                        +----------------+  ^
 *         |                                                              |
 *         ----------------------------------------------------------------
 */
For publisher application (pubsub_TSN_publisher_multiple_thread): publisher_T1.csv, subscriber_T8.csv
For loopback application: publisher_T5.csv, subscriber_T4.csv

To Compute the round trip time: Subtract T8-T1 (subtract the time in the .csv files of subscriber_T8.csv - publisher_T1.csv)
============================================================================================================
To close the running application press ctrl+c during the application exit the packet loss count of the entire run will be displayed in the console print.

============================================================================================================

For the TTTech TSN IP version 2.3.0 the following are applicable:
    <interfaceName>       sw0ep
    <portName>            available external physical ports: sw0p2, sw0p3, sw0p5, sw0p3 
    <InternalPortName>    sw0p1
