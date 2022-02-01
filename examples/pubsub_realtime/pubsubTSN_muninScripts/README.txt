This repository is for archiving the Munin scripts.
 - Add the scripts that retrieve data for the Munin plugin in the folder 'scripts'
 - Add the custom Munin plugin scripts in the folder 'plugins'
 - Run setup.sh as root user to install scripts and plugins
    1. Enter as root user
        su
    2. Run the below command to install the munin scripts
        chmod +x setup.sh
        ./munin_setup.sh -i <interface_name> -d <directory_name>
        Options:
        -i     i210 interface name
        -d     directory where the latency csv files of PubSub application are generated (Provide absolute path)
    3. Reboot the system
    setup.sh script installs the following munin plugins
        1. Plugin to monitor round trip time latency of open62541 PubSub application
        2. Plugin to monitor errors round trip error of open62541 PubSub application
        3. Plugin to monitor round trip time jitter of open62541 PubSub application
        4. Plugin to monitor ptp4l application
        5. Plugin to monitor phc2sys application
        6. Plugin to monitor packets sent, packet drops, and overlimits of the etf queue in the network interface
        7. Plugin to monitor max latency caused for the PubSub application
        8. Plugin to monitor packets received and dropped packets in all the 4 queues (RX) in the network interface
        9. Plugin to monitor CPU utilization in all cores
        10. Plugin to monitor CPU utilization in core 0
        11. Plugin to monitor CPU utilization in core 1
        12. Plugin to monitor CPU utilization in core 2
        13. Plugin to monitor CPU utilization in core 3
        14. Plugin to monitor cyclictest maximum latency
        15. Plugins to monitor network bandwidth utilisation in the network interface along with the VLAN interfaces
        16. Plugins to monitor latency of the Publisher and UserApplication threads of the pubsub_TSN applications
        17. Plugins to monitor jitter in the wakeup of the Publisher, Subscriber and UserApplication threads of the pubsub_TSN applications
        18. Plugins to monitor packets dropped by the Subscriber of the pubsub_TSN applications
