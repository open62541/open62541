#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e 

# Use the error status of the first failure in a pipeline
set -o pipefail

# Exit if an uninitialized variable is accessed
set -o nounset

# Use all available cores
if which nproc > /dev/null; then
    MAKEOPTS="-j$(nproc)"
else
    MAKEOPTS="-j$(sysctl -n hw.ncpu)"
fi

###########
# cpplint #
###########

function cpplint {
    mkdir -p build; cd build; rm -rf *
    cmake ..
    make ${MAKEOPTS} cpplint
}

#######################
# Build Documentation #
#######################

function build_docs {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DUA_BUILD_EXAMPLES=ON \
          ..
    make doc
}

#########################
# Build Release Version #
#########################

function build_release {
    mkdir -p build; cd build; rm -rf *
    cmake -DBUILD_SHARED_LIBS=ON \
          -DUA_ENABLE_ENCRYPTION=ON \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DCMAKE_BUILD_TYPE=RelWithDebInfo \
          -DUA_BUILD_EXAMPLES=ON \
          ..
    make ${MAKEOPTS}
}

############################
# Build and Run Unit Tests #
############################

function unit_tests {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_DISCOVERY=ON \
          -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_ENABLE_JSON_ENCODING=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=OFF \
          -DUA_ENABLE_PUBSUB_MONITORING=ON \
          ..
    make ${MAKEOPTS}
    make test ARGS="-V"
}

function unit_tests_encryption_mbedtls {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_DISCOVERY=ON \
          -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
          -DUA_ENABLE_ENCRYPTION=ON \
          -DUA_ENABLE_ENCRYPTION_MBEDTLS=ON \
          ..
    make ${MAKEOPTS}
    make test ARGS="-V"
}

function unit_tests_encryption_openssl {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_DISCOVERY=ON \
          -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
          -DUA_ENABLE_ENCRYPTION=ON \
          -DUA_ENABLE_ENCRYPTION_OPENSSL=ON \
          ..
    make ${MAKEOPTS}
    make test ARGS="-V"
}

##########################################
# Build and Run Unit Tests with Valgrind #
##########################################

function unit_tests_valgrind {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_DISCOVERY=ON \
          -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
          -DUA_ENABLE_ENCRYPTION=ON \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_ENABLE_JSON_ENCODING=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=OFF \
          -DUA_ENABLE_PUBSUB_MONITORING=ON \
          -DUA_ENABLE_UNIT_TESTS_MEMCHECK=ON \
          ..
    make ${MAKEOPTS}
    make test ARGS="-V"
}


##############################
# Clang Static Code Analysis #
##############################

function build_clang_analyzer {
    mkdir -p build; cd build; rm -rf *
    scan-build-10 cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_DISCOVERY=ON \
          -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
          -DUA_ENABLE_ENCRYPTION=ON \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_ENABLE_JSON_ENCODING=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=OFF \
          -DUA_ENABLE_PUBSUB_MONITORING=ON \
          ..
    scan-build-10 make ${MAKEOPTS}
}
