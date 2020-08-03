#!/bin/bash

build_dir=`pwd`/_build_x86_64_opc_ua

[ -d ${build_dir} ] && { echo "found build folder: ${build_dir}"; } || { mkdir ${build_dir} && { echo "build folder: ${build_dir} has been build"; } || exit 0; }

cd ${build_dir} || { echo "build directory not found"; exit 1; }

#UA_LOGLEVEL
#600: Fatal
#500: Error
#400: Warning
#300: Info
#200: Debug
#100: Trace

#-DUA_ENABLE_IMMUTABLE_NODES=TRUE \
#-DUA_MULTITHREADING=100 \

BUILD_OPT="-DUA_ENABLE_PUBSUB=TRUE \
	-DUA_ENABLE_SUBSCRIPTIONS=TRUE \
	-DUA_ENABLE_SUBSCRIPTIONS_EVENTS=TRUE \
	-DUA_NAMESPACE_ZERO=FULL \
	-DUA_ENABLE_DISCOVERY=TRUE \
	-DUA_ENABLE_DISCOVERY_MULTICAST=TRUE \
    -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=TRUE \
	-DUA_LOGLEVEL=200 \
    -DUA_DEBUG=TRUE" 

DEST_DIR_LOCAL="/usr"

#need to clean-up build folder otherwise cmake does not recognize new bulid configuration
#need to retain install folder
#we are in builddir now
find . -maxdepth 1 -name "*" -type f -exec rm -vf {} +

#build static libraries first
cmake ${BUILD_OPT} -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=${DEST_DIR_LOCAL} ..
cmake --build .
sudo make install

#need to clean-up build folder otherwise cmake does not recognize new bulid configuration
#need to retain install folder
#we are in builddir now
find . -maxdepth 1 -name "*" -type f -exec rm -vf {} +

#then build shared libraries
cmake ${BUILD_OPT} -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=${DEST_DIR_LOCAL} ..
cmake --build .
sudo make install