#!/bin/bash

# This script reduces the manual effort needed to setup environment on adding
# VLAN interface, scheduling of packets using ETF and time synchronization
# between the user & kernel clock
#
# Copyright (C) 2019 Kalycito Infotech Private Limited
# Author: Kalycito Infotech Private Limited
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.


usage() {
        echo "Run the script as mentioned below:"
        echo "./syncTime.sh <i210_interface> <10/11> <master/slave>"
        exit 2
}

if [ "$#" -ne 3 ]; then
        usage
        exit
fi

if [ $USER = "root" ]; then
        echo -e "\n Please run this script without sudo/root privileges\n"
        exit 2
fi

#Local shell variables
username=$USER
interfaceName=$1

ptp4l_master() {
                sudo modprobe 8021q
                sudo pkill ptp4l
                sudo taskset -c 1 chrt 90 ptp4l -i $interfaceName -2 -mq > /home/$username/ptp4l_master.txt 2>&1 &
}

ptp4l_slave() {
                sudo modprobe 8021q
                sudo pkill ptp4l
                sudo taskset -c 1 chrt 90 ptp4l -i $interfaceName -2 -mq -s > /home/$username/ptp4l_slave.txt 2>&1 &
}

phc2sys_master() {
                sudo pkill phc2sys
                sudo taskset -c 1 chrt 89 phc2sys -s $interfaceName -w -mq -O 0 > /home/$username/phc2sys_master.txt 2>&1 &
}

phc2sys_slave() {
                sudo pkill phc2sys
                sudo taskset -c 1 chrt 89 phc2sys -s $interfaceName -w -mq -O 0 > /home/$username/phc2sys_slave.txt 2>&1 &
}

#Set static IP

echo -e "\n======================================="
echo "Setting IP and adding VLAN interface"
echo -e "=======================================\n"
echo "--Setting IP to" $1 "interface--"

sudo ifconfig $1 192.168.10.$2 up

#Add VLAN interface

echo "--Adding VLAN interface" $1".9--"

sudo vconfig add $1 9
sudo ifconfig $1.9 192.168.9.$2 up
sudo vconfig set_egress_map $1.9 0 0

#Schedule packets using ETF

echo -e "\n======================================="
echo "Scheduling packets using ETF"
echo -e "=======================================\n"

sudo tc qdisc replace dev $1 parent root mqprio num_tc 3 map 2 2 1 0 2 2 2 2 2 2 2 2 2 2 2 2 queues 1@0 1@1 2@2 hw 0
sudo tc qdisc add dev $1 parent 8001:1 etf offload clockid CLOCK_TAI delta 150000
sudo tc qdisc add dev $1 parent 8001:2 etf offload clockid CLOCK_TAI delta 150000


#Time synchronization using PTP and PHC2SYS

if [ $3 = "master" ];
then
    ptp4l_master
    phc2sys_master
else
    ptp4l_slave
    phc2sys_slave
fi

echo -e "\nSuccess:\nSystem is time synchronized. Verify the log file available at user's home folder."
echo -e "\n\nYou are ready to run the OPC UA PubSub application.\n\n"
