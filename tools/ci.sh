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
          -DUA_ENABLE_XML_ENCODING=ON \
          -DUA_ENABLE_NODESETLOADER=ON \
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
          -DUA_ENABLE_XML_ENCODING=ON \
          -DUA_ENABLE_NODESETLOADER=ON \
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
          -DUA_ENABLE_XML_ENCODING=ON \
          -DUA_ENABLE_NODESETLOADER=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_ETH_UADP=ON \
          -DUA_ENABLE_PUBSUB_MQTT=ON \
          -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS=ON \
          -DUA_ENABLE_REDUCED_ITERATIONS_FOR_TESTING=ON \
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
          -DUA_ENABLE_XML_ENCODING=ON \
          -DUA_ENABLE_NODESETLOADER=ON \
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
          -DUA_ENABLE_XML_ENCODING=ON \
          -DUA_ENABLE_NODESETLOADER=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_ETH_UADP=ON \
          -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS=ON \
          -DUA_ENABLE_REDUCED_ITERATIONS_FOR_TESTING=ON \
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
    set_capabilities
    make test ARGS="-V"
}

function unit_tests_alarms {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_DA=ON \
          -DUA_ENABLE_NODESETLOADER=ON \
          -DUA_ENABLE_XML_ENCODING=ON \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
	      -DUA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS=ON \
          -DUA_NAMESPACE_ZERO=FULL \
          ..
    make ${MAKEOPTS}
    set_capabilities
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
    set_capabilities
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
          -DUA_ENABLE_CERT_REJECTED_DIR=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_ENABLE_PUBSUB_MONITORING=ON \
          -DUA_ENABLE_PUBSUB_ENCRYPTION=ON \
          ..
    make ${MAKEOPTS}
    set_capabilities
    make test ARGS="-V"
}

function unit_tests_pubsub_sks {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_NAMESPACE_ZERO=FULL \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_DISCOVERY=ON \
          -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
          -DUA_ENABLE_ENCRYPTION=MBEDTLS \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS=ON \
          -DUA_ENABLE_REDUCED_ITERATIONS_FOR_TESTING=ON \
          -DUA_ENABLE_PUBSUB_MONITORING=ON \
          -DUA_ENABLE_PUBSUB_ENCRYPTION=ON \
          -DUA_ENABLE_PUBSUB_SKS=ON \
          -DUA_ENABLE_UNIT_TESTS_MEMCHECK=ON \
          ..
    make ${MAKEOPTS}
    # set_capabilities not possible with valgrind
    make test ARGS="-V -R sks"
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
          -DUA_ENABLE_XML_ENCODING=ON \
          -DUA_ENABLE_NODESETLOADER=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_ETH_UADP=ON \
          -DUA_ENABLE_PUBSUB_MQTT=ON \
          -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS=ON \
          -DUA_ENABLE_REDUCED_ITERATIONS_FOR_TESTING=ON \
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
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_DISCOVERY=ON \
          -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
          -DUA_ENABLE_ENCRYPTION=$1 \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_ENABLE_HISTORIZING=ON \
          -DUA_ENABLE_JSON_ENCODING=ON \
          -DUA_ENABLE_XML_ENCODING=ON \
          -DUA_ENABLE_NODESETLOADER=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_MQTT=ON \
          -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS=ON \
          -DUA_ENABLE_REDUCED_ITERATIONS_FOR_TESTING=ON \
          -DUA_ENABLE_PUBSUB_MONITORING=ON \
          -DUA_ENABLE_UNIT_TESTS_MEMCHECK=ON \
          ..
    make ${MAKEOPTS}
    # set_capabilities not possible with valgrind
    make test ARGS="-V"
}

########################################
# Build and Run Examples with Valgrind #
########################################

function examples_valgrind {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_ENABLE_DISCOVERY=ON \
          -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
          -DUA_ENABLE_ENCRYPTION=$1 \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS=ON \
          -DUA_ENABLE_HISTORIZING=ON \
          -DUA_ENABLE_JSON_ENCODING=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_ENABLE_PUBSUB_MONITORING=ON \
          -DUA_ENABLE_UNIT_TESTS_MEMCHECK=ON \
          -DUA_ENABLE_PUBSUB_MQTT=ON \
          -DUA_ENABLE_PUBSUB_FILE_CONFIG=ON \
          -DUA_NAMESPACE_ZERO=FULL \
          -DUA_ENABLE_NODESETLOADER=ON \
          -DUA_ENABLE_PUBSUB_ETH_UADP=ON \
          -DUA_ENABLE_PUBSUB_SKS=ON \
          -DUA_ENABLE_PUBSUB_ENCRYPTION=ON \
          ..
    make ${MAKEOPTS}

    # Run each example with valgrind. Wait 10 seconds and send the SIGINT
    # signal. Wait for the process to terminate and collect the exit status.
    # Abort when the exit status is non-null.
    FILES="./bin/examples/*"
    for f in $FILES
    do
	    echo "Processing $f"
	    valgrind --errors-for-leak-kinds=all --leak-check=full --error-exitcode=1 $f &
        pid=$!
	    sleep 10
        # || true to ignore the error if the process is already dead
	    kill -INT $pid || true

        # wait for the process to terminate and check if the process is still running
        sleep 5
        if ps | grep "$pid"; then
            echo $pid is still in the ps output. Must still be running.
            # send the SIGINT signal again       
            kill -INT $pid || true
        fi
        
        # using a 20 second timeout with SIGTERM to kill the process if it is still running
	    timeout 20s bash -c 'wait $pid || kill -TERM $pid' ; EXIT_CODE=$?
	    if [[ $EXIT_CODE -ne 0 ]]; then
		   echo "Processing $f failed with exit code $EXIT_CODE "
		   exit $EXIT_CODE	
	    fi
    done
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
          -DUA_ENABLE_XML_ENCODING=ON \
          -DUA_ENABLE_NODESETLOADER=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_ENABLE_PUBSUB_MONITORING=ON \
          ..
    scan-build-11 --status-bugs make ${MAKEOPTS}
}

###################################################
# Compile alle ua-schema companion specifications #
###################################################

function build_all_companion_specs {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_INFORMATION_MODEL_AUTOLOAD=DI\;ISA95-JOBCONTROL\;OpenSCS\;DEXPI\;AMB\;AutoID\;POWERLINK\;IA\;Machinery\;PackML\;PNEM\;PLCopen\;MachineTool\;PROFINET\;MachineVision\;FDT\;CommercialKitchenEquipment\;Scales\;Weihenstephan\;Pumps\;CAS\;TMC \
          -DUA_NAMESPACE_ZERO=FULL \
          ..
    make ${MAKEOPTS} check_nodeset_compiler_testnodeset
    ./bin/tests/check_nodeset_compiler_testnodeset
}
