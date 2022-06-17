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

#####################################
# Build Documentation including PDF #
#####################################

function build_docs_pdf {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DUA_BUILD_EXAMPLES=ON \
          ..
    make doc doc_pdf
}

#######################
# Build TPM tool #
#######################

function build_tpm_tool {
    mkdir -p build; cd build; rm -rf *
    cmake -DUA_BUILD_TOOLS=ON \
          -DUA_ENABLE_ENCRYPTION=MBEDTLS \
          -DUA_ENABLE_ENCRYPTION_TPM2=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_ETH_UADP=ON \
          -DUA_ENABLE_PUBSUB_ENCRYPTION=ON \
          ..
    make ${MAKEOPTS}
}

#########################
# Build Release Version #
#########################

function build_release {
    mkdir -p build; cd build; rm -rf *
    cmake -DBUILD_SHARED_LIBS=ON \
          -DUA_ENABLE_ENCRYPTION=MBEDTLS \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_ENABLE_HISTORIZING=ON \
          -DCMAKE_BUILD_TYPE=RelWithDebInfo \
          -DUA_BUILD_EXAMPLES=ON \
          ..
    make ${MAKEOPTS}
}

######################
# Build Amalgamation #
######################

function build_amalgamation {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_ENABLE_AMALGAMATION=ON \
          -DUA_ENABLE_DISCOVERY=ON \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_ENABLE_JSON_ENCODING=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_ETH_UADP=ON \
          -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_ENABLE_PUBSUB_MONITORING=ON \
          ..
    make ${MAKEOPTS}
}

function build_amalgamation_mt {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_ENABLE_AMALGAMATION=ON \
          -DUA_ENABLE_DISCOVERY=ON \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_ENABLE_JSON_ENCODING=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_ETH_UADP=ON \
          -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_ENABLE_PUBSUB_MONITORING=ON \
          -DUA_MULTITHREADING=100 \
          ..
    make ${MAKEOPTS}
}

############################
# Build and Run Unit Tests #
############################

function set_capabilities {
    for filename in bin/tests/*; do
        sudo setcap cap_sys_ptrace,cap_net_raw,cap_net_admin=eip $filename
    done
} 

function unit_tests {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_DISCOVERY=ON \
          -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_ENABLE_HISTORIZING=ON \
          -DUA_ENABLE_JSON_ENCODING=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_ETH_UADP=ON \
          -DUA_ENABLE_PUBSUB_MQTT=ON \
          -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS=ON \
          -DUA_ENABLE_PUBSUB_MONITORING=ON \
          -DUA_ENABLE_PUBSUB_MQTT=ON \
          ..
    make ${MAKEOPTS}
    set_capabilities
    make test ARGS="-V"
}

function unit_tests_32 {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_DISCOVERY=ON \
          -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_ENABLE_HISTORIZING=ON \
          -DUA_ENABLE_JSON_ENCODING=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_ENABLE_PUBSUB_MONITORING=ON \
          -DUA_FORCE_32BIT=ON \
          ..
          #-DUA_ENABLE_PUBSUB_ETH_UADP=ON \ # TODO: Enable this
    make ${MAKEOPTS}
    set_capabilities
    make test ARGS="-V"
}

function unit_tests_nosub {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_SUBSCRIPTIONS=OFF \
          ..
    make ${MAKEOPTS}
    set_capabilities
    make test ARGS="-V"
}

function unit_tests_diag {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_DIAGNOSTICS=ON \
          -DUA_ENABLE_DISCOVERY=ON \
          -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_ENABLE_HISTORIZING=ON \
          -DUA_ENABLE_JSON_ENCODING=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_ETH_UADP=ON \
          -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS=ON \
          -DUA_ENABLE_PUBSUB_MONITORING=ON \
          ..
    make ${MAKEOPTS}
    set_capabilities
    make test ARGS="-V"
}

function unit_tests_mt {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_MULTITHREADING=200 \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_ENABLE_HISTORIZING=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_DISCOVERY=ON \
          -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          ..
    make ${MAKEOPTS}
    make test ARGS="-V"
}

function unit_tests_alarms {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_DA=ON \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
	      -DUA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS=ON \
          -DUA_NAMESPACE_ZERO=FULL \
          ..
    make ${MAKEOPTS}
    make test ARGS="-V"
}

function unit_tests_encryption {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_DISCOVERY=ON \
          -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
          -DUA_ENABLE_ENCRYPTION=$1 \
          ..
    make ${MAKEOPTS}
    make test ARGS="-V"
}

function unit_tests_encryption_mbedtls_pubsub {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_DISCOVERY=ON \
          -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
          -DUA_ENABLE_ENCRYPTION=MBEDTLS \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_ENABLE_PUBSUB_MONITORING=ON \
          -DUA_ENABLE_PUBSUB_ENCRYPTION=ON \
          ..
    make ${MAKEOPTS}
    make test ARGS="-V"
}

##########################################
# Build and Run Unit Tests with Coverage #
##########################################

function unit_tests_with_coverage {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_COVERAGE=ON \
          -DUA_ENABLE_DISCOVERY=ON \
          -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_ENABLE_HISTORIZING=ON \
          -DUA_ENABLE_JSON_ENCODING=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_ETH_UADP=ON \
          -DUA_ENABLE_PUBSUB_MQTT=ON \
          -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS=ON \
          -DUA_ENABLE_PUBSUB_MONITORING=ON \
          -DUA_ENABLE_ENCRYPTION=MBEDTLS \
          ..
    make ${MAKEOPTS}
    set_capabilities
    make test ARGS="-V"
    make gcov
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
          -DUA_ENABLE_ENCRYPTION=$1 \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_ENABLE_HISTORIZING=ON \
          -DUA_ENABLE_JSON_ENCODING=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_MQTT=ON \
          -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS=ON \
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
    scan-build-11 cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_DISCOVERY=ON \
          -DUA_ENABLE_ENCRYPTION=MBEDTLS \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_ENABLE_HISTORIZING=ON \
          -DUA_ENABLE_JSON_ENCODING=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_ENABLE_PUBSUB_MONITORING=ON \
          ..
    scan-build-11 --status-bugs make ${MAKEOPTS}
}
