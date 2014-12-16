#!/bin/bash

#
# This script checks whether all patches from /ports dir are applicable
#

PATCHES=ports/*

for p in $PATCHES
do
	git apply --check $p
	if [[ "$?" == 0 ]] 
	then
		echo $p is applicable
	else
		echo failed to apply $p
		exit 1
	fi
	
done
