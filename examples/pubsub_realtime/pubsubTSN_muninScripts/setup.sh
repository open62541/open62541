#!/bin/bash
#------------------------------------------------------------------------------
#Copyright (C) 2021 Kalycito Infotech Private Limited
#Author: Kalycito Infotech Private Limited
#
#This program is free software; you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation; either version 2 of the License, or
#(at your option) any later version.
#
#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#------------------------------------------------------------------------------

###############################################################################
#Global Variable - constant                                                   #
###############################################################################
IFACE=0
DIRECTORY_NAME=0

###############################################################################
#Help - This displays various help options of this script                     #
###############################################################################
Help()
{
    #Display Help
    echo ""
    echo "This script installs the following munin plugins"
    echo "1. Plugin to monitor round trip time latency of open62541 PubSub application"
    echo "2. Plugin to monitor errors round trip error of open62541 PubSub application"
    echo "3. Plugin to monitor round trip time jitter of open62541 PubSub application"
    echo "4. Plugin to monitor ptp4l application"
    echo "5. Plugin to monitor phc2sys application"
    echo "6. Plugin to monitor packets sent, packet drops, and overlimits of the etf queue in the network interface"
    echo "7. Plugin to monitor max latency caused for the PubSub application"
    echo "8. Plugins to monitor packets received and dropped packets in queue that receive PubSub application in the network interface"
    echo "9. Plugin to monitor CPU utilization in all cores"
    echo "10. Plugin to monitor CPU utilization in core 0"
    echo "11. Plugin to monitor CPU utilization in core 1"
    echo "12. Plugin to monitor CPU utilization in core 2"
    echo "13. Plugin to monitor CPU utilization in core 3"
    echo "14. Plugin to monitor cyclictest maximum latency"
    echo "15. Plugins to monitor network bandwidth utilisation in the network interface along with the VLAN interfaces"
    echo "16. Plugins to monitor latency of the Publisher and UserApplication threads of the pubsub_TSN applications"
    echo "17. Plugins to monitor jitter in the wakeup of the Publisher, Subscriber and UserApplication threads of the pubsub_TSN applications"
    echo "18. Plugins to monitor packets dropped by the Subscriber and Subscriber processing time of the pubsub_TSN applications"
    echo "Usage: $0 -i <interface_name> -d <directory_name>"
    echo Options:
    echo "-i     i210 interface name"
    echo "-d     directory where the latency csv files of PubSub application \
are generated (Provide absolute path)"
}

###############################################################################
#crontab_install - Function to verify whether crontab is already installed    #
#If not, install the crontab to generate data for the corresponding           #
#munin plugins                                                                #
###############################################################################
crontab_install() {
    crontabInstalled=$(crontab -l | grep -c "\*/5 \* \* \* \* $1")
    if [[ "$crontabInstalled" -eq "1" ]]
    then
        echo Crontab already installed for $1
    else
        line="*/5 * * * * $1"
        (crontab -l; echo "$line" ) | crontab -
    fi
}

###############################################################################
#Main program                                                                 #
###############################################################################
main()
{

    #Prints help function if no interface parameter is provided by the user
    if test "$IFACE" == "0"; then
        echo Interface name not specified
        Help
        exit
    fi

    #Prints help function if the user has not specified the directory
    #where the latency csv files are generated
    if test "$DIRECTORY_NAME" == "0"; then
        echo Directory not specified. Specify the directory in which the \
            latency csv files will be generated.
        Help
        exit
    fi

    PLUGIN_DIR=/usr/share/munin/plugins
    SYMBOLIC_LINK_DIR=/etc/munin/plugins
    SCRIPT_FOR_CRON_DIR=/usr/local/bin

    #Dependency packages for monitoring CPU and network bandwidth
    sudo apt-get install sysstat -y

    #Change the directory of latency generated files location in the open62541_latency_script, open62541_loopback_latency_script and
    #open62541_countermiss_at_subscriber_compute files
    #open62541_latency_script, open62541_loopback_latency_script - find the maximum latency, missed and repeated counters of the generated csv files for every 5 minutes
    #It also finds the missed counters and repeated counters of the pubsub application along with the latency and jitter values for
    #Publisher thread, User Application thread and Subscriber thread in the pubsub applications
    #open62541_countermiss_at_subscriber_compute - find the number of packets missed at the user layer of subscriber threads in pubsub applications
    #The values are stored and updated in txt files for every 5 minutes
    #From this txt file, Munin scripts will take values to plot in the graph
    sed -i "/GENLATENCYDIR=/c GENLATENCYDIR=$DIRECTORY_NAME" \
        scripts/open62541_latency_script
    sed -i "/GENLATENCYDIR=/c GENLATENCYDIR=$DIRECTORY_NAME" \
        scripts/open62541_loopback_latency_script
    sed -i "/GENLATENCYDIR=/c GENLATENCYDIR=$DIRECTORY_NAME" \
        scripts/open62541_countermiss_at_subscriber_compute

    #Copy the shell script to /usr/local/bin directory
    sudo cp scripts/* $SCRIPT_FOR_CRON_DIR

    #Remove the previously existing symbolic links and create new links in
    # /usr/local/bin directory
    sudo rm -rf $SCRIPT_FOR_CRON_DIR/tc_qdisc_statistics_compute_$IFACE
    sudo ln -s $SCRIPT_FOR_CRON_DIR/tc_qdisc_statistics_compute_ \
        $SCRIPT_FOR_CRON_DIR/tc_qdisc_statistics_compute_$IFACE

    sudo rm -rf $SCRIPT_FOR_CRON_DIR/max_latency_victim_compute_pubsub
    sudo ln -s $SCRIPT_FOR_CRON_DIR/max_latency_victim_compute_ \
        $SCRIPT_FOR_CRON_DIR/max_latency_victim_compute_pubsub

    sudo rm -rf $SCRIPT_FOR_CRON_DIR/ethtool_rx_queue_statistics_compute_$IFACE
    sudo ln -s $SCRIPT_FOR_CRON_DIR/ethtool_rx_queue_statistics_compute_ \
        $SCRIPT_FOR_CRON_DIR/ethtool_rx_queue_statistics_compute_$IFACE

    #Remove all the previously existing symbolic links in
    # /etc/munin/plugins directory
    sudo rm -rf $SYMBOLIC_LINK_DIR/open62541*
    sudo rm -rf $SYMBOLIC_LINK_DIR/ptp4l*
    sudo rm -rf $SYMBOLIC_LINK_DIR/phc2sys*
    sudo rm -rf $SYMBOLIC_LINK_DIR/tc_qdisc_statistics_*
    sudo rm -rf $SYMBOLIC_LINK_DIR/max_latency_victim_*
    sudo rm -rf $SYMBOLIC_LINK_DIR/ethtool_rx_queue_*
    sudo rm -rf $SYMBOLIC_LINK_DIR/cpu_utilization_*
    sudo rm -rf $SYMBOLIC_LINK_DIR/cyclictest_max_latency
    sudo rm -rf $SYMBOLIC_LINK_DIR/publisherThread*
    sudo rm -rf $SYMBOLIC_LINK_DIR/subscriberThread*
    sudo rm -rf $SYMBOLIC_LINK_DIR/userThread*
    sudo rm -rf $SYMBOLIC_LINK_DIR/countermiss_at_subscriber
    sudo rm -rf $SYMBOLIC_LINK_DIR/cstates
    sudo rm -rf $SYMBOLIC_LINK_DIR/irqrtprio
    sudo rm -rf $SYMBOLIC_LINK_DIR/if_$IFACE.*

    #Copy all the plugins into the /usr/share/munin/plugins directory
    sudo cp plugins/* $PLUGIN_DIR

    #Create symbolic link for the munin plugins in
    # /etc/munin/plugins directory to enable these plugins for plotting
    sudo ln -s $PLUGIN_DIR/open62541latencyMuninScript \
        $SYMBOLIC_LINK_DIR/open62541latencyMuninScript
    sudo ln -s $PLUGIN_DIR/open62541latencyInJitterMuninScript \
        $SYMBOLIC_LINK_DIR/open62541latencyInJitterMuninScript
    sudo ln -s $PLUGIN_DIR/open62541latencyErrorMuninScript \
        $SYMBOLIC_LINK_DIR/open62541latencyErrorMuninScript
    sudo ln -s $PLUGIN_DIR/ptp4l $SYMBOLIC_LINK_DIR/ptp4l
    sudo ln -s $PLUGIN_DIR/phc2sys $SYMBOLIC_LINK_DIR/phc2sys
    sudo ln -s $PLUGIN_DIR/tc_qdisc_statistics_ $SYMBOLIC_LINK_DIR/tc_qdisc_statistics_$IFACE
    sudo ln -s $PLUGIN_DIR/max_latency_victim_ $SYMBOLIC_LINK_DIR/max_latency_victim_pubsub
    sudo ln -s $PLUGIN_DIR/ethtool_rx_queue_1_statistics_ $SYMBOLIC_LINK_DIR/ethtool_rx_queue_1_statistics_$IFACE
    sudo ln -s $PLUGIN_DIR/cpu_utilization_statistics $SYMBOLIC_LINK_DIR/cpu_utilization_statistics
    sudo ln -s $PLUGIN_DIR/cpu_utilization_core0_statistics $SYMBOLIC_LINK_DIR/cpu_utilization_core0_statistics
    sudo ln -s $PLUGIN_DIR/cpu_utilization_core1_statistics $SYMBOLIC_LINK_DIR/cpu_utilization_core1_statistics
    sudo ln -s $PLUGIN_DIR/cpu_utilization_core2_statistics $SYMBOLIC_LINK_DIR/cpu_utilization_core2_statistics
    sudo ln -s $PLUGIN_DIR/cpu_utilization_core3_statistics $SYMBOLIC_LINK_DIR/cpu_utilization_core3_statistics
    sudo ln -s $PLUGIN_DIR/cyclictest_max_latency $SYMBOLIC_LINK_DIR/cyclictest_max_latency
    sudo ln -s $PLUGIN_DIR/if_ $SYMBOLIC_LINK_DIR/if_$IFACE.1
    sudo ln -s $PLUGIN_DIR/if_ $SYMBOLIC_LINK_DIR/if_$IFACE.2
    sudo ln -s $PLUGIN_DIR/if_ $SYMBOLIC_LINK_DIR/if_$IFACE.3
    sudo ln -s $PLUGIN_DIR/if_ $SYMBOLIC_LINK_DIR/if_$IFACE.4
    sudo ln -s $PLUGIN_DIR/if_ $SYMBOLIC_LINK_DIR/if_$IFACE.5
    sudo ln -s $PLUGIN_DIR/if_ $SYMBOLIC_LINK_DIR/if_$IFACE.6
    sudo ln -s $PLUGIN_DIR/if_ $SYMBOLIC_LINK_DIR/if_$IFACE.7
    sudo ln -s $PLUGIN_DIR/if_ $SYMBOLIC_LINK_DIR/if_$IFACE.8
    sudo ln -s $PLUGIN_DIR/publisherThreadJitterStatistics $SYMBOLIC_LINK_DIR/publisherThreadJitterStatistics
    sudo ln -s $PLUGIN_DIR/publisherThreadLatencyStatistics $SYMBOLIC_LINK_DIR/publisherThreadLatencyStatistics
    sudo ln -s $PLUGIN_DIR/subscriberThreadJitterStatistics $SYMBOLIC_LINK_DIR/subscriberThreadJitterStatistics
    sudo ln -s $PLUGIN_DIR/subscriberThreadDataProcessingStatistics $SYMBOLIC_LINK_DIR/subscriberThreadDataProcessingStatistics
    sudo ln -s $PLUGIN_DIR/userThreadJitterStatistics $SYMBOLIC_LINK_DIR/userThreadJitterStatistics
    sudo ln -s $PLUGIN_DIR/userThreadLatencyStatistics $SYMBOLIC_LINK_DIR/userThreadLatencyStatistics
    sudo ln -s $PLUGIN_DIR/countermiss_at_subscriber $SYMBOLIC_LINK_DIR/countermiss_at_subscriber
    sudo ln -s $PLUGIN_DIR/cstates $SYMBOLIC_LINK_DIR/cstates
    sudo ln -s $PLUGIN_DIR/irqrtprio $SYMBOLIC_LINK_DIR/irqrtprio

    #Restart the munin-node to activate the newly installed scripts
    sudo /etc/init.d/munin-node restart

    #Verify and install crontabs to generate data
    crontab_install /usr/local/bin/open62541_loopback_latency_script

    crontab_install /usr/local/bin/open62541_latency_script

    crontab_install /usr/local/bin/tc_qdisc_statistics_compute_$IFACE

    crontab_install /usr/local/bin/max_latency_victim_compute_pubsub

    crontab_install /usr/local/bin/histogram_reset_enable

    crontab_install /usr/local/bin/max_latency_generation

    crontab_install /usr/local/bin/ethtool_rx_queue_statistics_compute_$IFACE

    crontab_install /usr/local/bin/cpu_utilization

    crontab_install /usr/local/bin/open62541_countermiss_at_subscriber_compute

}

###############################################################################
#Process the input options. Add options as needed.                            #
###############################################################################
#Get the options
while getopts "d:i:h" flag
do
    case ${flag} in
        h)
            Help
            exit;;
        i)
            IFACE=${OPTARG};;
        d)
            DIRECTORY_NAME=${OPTARG};;
        \?)
            echo Error: Invalid option
            Help
            exit;;
    esac
done

main

