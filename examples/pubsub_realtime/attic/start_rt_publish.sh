#!/bin/bash

#program path or install dir
IF=enp0s8
CPU_NR=3
PRIO=90
OLDDIR=$(pwd)

reset() {
  #standard on most systems. ondemand -> Dynamic CPU-Freq.
  echo ondemand >/sys/devices/system/cpu/cpu$CPU_NR/cpufreq/scaling_governor
  cd /sys/devices/system/cpu/cpu$CPU_NR/cpuidle
  for i in *
  do
    #~disable sleep state
    echo 0 >$i/disable
  done

  phy=$IF #get interface name
  #get pid's from interface irq
  for i in `ps ax | grep -v grep | grep $phy | sed "s/^ //" | cut -d" " -f1`
  do
    #retrive or set a process's CPU affinity
    taskset -pc 0-$CPU_NR $i >/dev/null
    #manipulate the real-time attributes of a process -p priority -f scheduling policy to SCHED_FIFO
    chrt -pf 50 $i
  done
  #distribute hardware interrupts across processsors on a muliprocessor system
  systemctl start irqbalance
}

trap reset ERR
systemctl stop irqbalance

phy=$IF
for i in `ps ax | grep -v grep | grep $phy | sed "s/^ //" | cut -d" " -f1`
do
  taskset -pc $CPU_NR $i >/dev/null
  chrt -pf $PRIO $i
done

cd /sys/devices/system/cpu/cpu$CPU_NR/cpuidle
for i in `ls -1r`
do
  echo 1 >$i/disable
done

echo performance >/sys/devices/system/cpu/cpu$CPU_NR/cpufreq/scaling_governor

cd $OLDDIR
taskset -c $CPU_NR chrt -f $PRIO $1
