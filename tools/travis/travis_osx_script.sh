#!/bin/bash
set -e

echo "\n=== Building ==="
export OPENSSL_ROOT_DIR="/usr/local/opt/openssl"
export PATH="/Users/travis/Library/Python/2.7/bin:$PATH"

# OSX may have different hostnames between processes, which causes multicast unit test to fail.
# Hardcode the hostname here
# Note: do not use the `hostname` addon from travis:
# addons:
#  hostname: travis-osx
#
# Travis support says:
# the `hostname` command is actually a super ephemeral way to change it. There’s a different way:
# `sudo scutil --set HostName <new host name>` which is more permanent and potentially more resilient
sudo scutil --set HostName travis-osx

echo "Full Namespace 0 Generation" && echo -en 'travis_fold:start:script.build.ns0\\r'
mkdir -p build
cd build
cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DUA_BUILD_EXAMPLES=ON \
    -DUA_NAMESPACE_ZERO=FULL ..
make -j
if [ $? -ne 0 ] ; then exit 1 ; fi
cd .. && rm -rf build
echo -en 'travis_fold:end:script.build.ns0\\r'

echo "Compile release build for OS X" && echo -en 'travis_fold:start:script.build.osx\\r'
mkdir -p build && cd build
cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=${TRAVIS_BUILD_DIR}/open62541-osx \
    -DUA_BUILD_EXAMPLES=ON \
    -DUA_ENABLE_AMALGAMATION=OFF ..
make -j
if [ $? -ne 0 ] ; then exit 1 ; fi
make install
if [ $? -ne 0 ] ; then exit 1 ; fi
cd ..
tar -pczf open62541-osx.tar.gz LICENSE AUTHORS README.md ${TRAVIS_BUILD_DIR}/open62541-osx/*
rm -rf build
echo -en 'travis_fold:end:script.build.osx\\r'

echo "Compile multithreaded version" && echo -en 'travis_fold:start:script.build.multithread\\r'
mkdir -p build && cd build
cmake \
    -DUA_BUILD_EXAMPLES=ON \
    -DUA_ENABLE_MULTITHREADING=ON ..
make -j
if [ $? -ne 0 ] ; then exit 1 ; fi
cd .. && rm -rf build
echo -en 'travis_fold:end:script.build.multithread\\r'

echo "Debug build and unit tests with valgrind" && echo -en 'travis_fold:start:script.build.unit_test\\r'
mkdir -p build && cd build
cmake \
    -DCHECK_PREFIX=/usr/local/Cellar/check/0.11.0 \
    -DCMAKE_BUILD_TYPE=Debug \
    -DUA_BUILD_EXAMPLES=ON \
    -DUA_BUILD_UNIT_TESTS=ON \
    -DUA_ENABLE_COVERAGE=OFF \
    -DUA_ENABLE_DISCOVERY=ON \
    -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
    -DUA_ENABLE_ENCRYPTION=ON \
    -DUA_ENABLE_UNIT_TESTS_MEMCHECK=OFF ..
make -j && make test ARGS="-V"
if [ $? -ne 0 ] ; then exit 1 ; fi
cd .. && rm -rf build
echo -en 'travis_fold:end:script.build.unit_test\\r'
