#!/bin/bash

#Check whether the user has specified the directory where the latency csv files are generated
if [ -z "$1" ]
  then
    echo "Directory not specified. Specify the directory in which the latency csv files will be generated."
    exit
fi

#Verify whether the PTP and PHC Munin scripts are already copied to Munin plugins directory
if [[ -f "/usr/share/munin/plugins/ptp4l" ]] && [[ -f "/usr/share/munin/plugins/phc2sys" ]]
then
    echo "PTP and PHC2SYS jitter scripts are already copied"
else
    #Copy the Munin scripts to the /usr/share/munin/plugins directory
    #The scripts in the plugins are used by munin to plot the graph for every 5 minutes
    cp ptp4l /usr/share/munin/plugins
    cp phc2sys /usr/share/munin/plugins
fi

#Change the directory of latency generated files location in the open62541latencyScript
#This script is used to find the maximum latency, missed and repeated counters of the generated csv files for every 5 minutes
#It also finds the missed counters and repeated counters of the pubsub application
#The values are stored and updated in /usr/local/src/open62541_pubsub_maxlatency.txt file for every 5 minutes
#From this txt file, Munin scripts will take the latency, missed and repeated counters value to plot in the graph
sed -i "/GENLATENCYDIR=/c GENLATENCYDIR=$1" open62541latencyScript

#Copy the shell script to /usr/local/bin directory
cp open62541latencyScript /usr/local/bin

#Verify whether the Latency Munin scripts are already copied to Munin plugins directory
if [[ -f "/usr/share/munin/plugins/open62541latencyMuninScript" ]] && [[ -f "/usr/share/munin/plugins/open62541latencyInJitterMuninScript" ]] && [[ -f "/usr/share/munin/plugins/open62541latencyErrorMuninScript" ]]
then
    echo "open62541 PubSub latency scripts are already copied"
else
    #Copy the Munin scripts to the /usr/share/munin/plugins directory
    #The scripts in the plugins are used by munin to plot the graph for every 5 minutes
    cp open62541latencyMuninScript /usr/share/munin/plugins
    cp open62541latencyInJitterMuninScript /usr/share/munin/plugins
    cp open62541latencyErrorMuninScript /usr/share/munin/plugins
fi

#Verify whether the PTP and PHC Munin scripts are linked
if [[ -f "/etc/munin/plugins/ptp4l" ]] && [[ -f "/etc/munin/plugins/phc2sys" ]]
then
    echo "PTP and PHC2SYS scripts are already linked"
else
    #Link the munin scripts in /etc/munin/plugins directory to enable these scripts for plotting
    cd /etc/munin/plugins
    sudo ln -s /usr/share/munin/plugins/ptp4l ptp4l
    sudo ln -s /usr/share/munin/plugins/phc2sys phc2sys
fi

#Verify whether the Latency Munin scripts are linked
if [[ -f "/etc/munin/plugins/open62541latencyMuninScript" ]] && [[ -f "/etc/munin/plugins/open62541latencyInJitterMuninScript" ]] && [[ -f "/etc/munin/plugins/open62541latencyErrorMuninScript" ]]
then
    echo "open62541 PubSub latency scripts are already linked"
else
    #Link the munin scripts in /etc/munin/plugins directory to enable these scripts for plotting
    cd /etc/munin/plugins
    sudo ln -s /usr/share/munin/plugins/open62541latencyMuninScript open62541latencyMuninScript
    sudo ln -s /usr/share/munin/plugins/open62541latencyInJitterMuninScript open62541latencyInJitterMuninScript
    sudo ln -s /usr/share/munin/plugins/open62541latencyErrorMuninScript open62541latencyErrorMuninScript
fi

#Restart the munin-node to activate the newly installed scripts
/etc/init.d/munin-node restart

#Verify whether the crontab is already installed.
#If not, install the crontab
#This crontab is called for every minutes and it will execute open62541latencyScript script to calculate the maximum latency, repeated and missed counters from the generated csv files
crontabInstalled=$(crontab -l | grep -c "\*/5 \* \* \* \* /usr/local/bin/open62541latencyScript")
if [[ "$crontabInstalled" -eq "1" ]]
then
    echo "Crontab already installed"
else
    line="*/5 * * * * /usr/local/bin/open62541latencyScript"
    (crontab -l; echo "$line" ) | crontab -
fi

