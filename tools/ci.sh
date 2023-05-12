#!/bin/bash

# Exit immediately if a command exits with a non-zero status
#set -e

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
          -DUA_ENABLE_MQTT=ON \
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
          -DUA_ENABLE_MQTT=ON \
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
          -DUA_ENABLE_MQTT=ON \
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

    declare -A example_args
    example_args=(["client_encryption"]="opc.tcp://localhost:4840 client_cert.der client_key.der server_cert.der"
        ["access_control_client_encrypt"]="opc.tcp://localhost:4840 client_cert.der client_key.der server_cert.der"
        ["client_event_filter"]="opc.tcp://localhost:4840"
        ["client_connect"]="-username user1 -password password -cert client_cert.der -key client_key.der opc.tcp://localhost:4840"
        ["pubsub_nodeset_rt_publisher"]="-i lo"
        ["pubsub_nodeset_rt_subscriber"]="-i lo"
        ["pubsub_TSN_loopback"]="-i lo"
        ["pubsub_TSN_loopback_single_thread"]="-i lo"
        ["pubsub_TSN_publisher"]="-i lo"
        ["pubsub_TSN_publisher_multiple_thread"]="-i lo"
        ["server_encryption"]="server_cert.der server_key.der client_cert.der"
        ["server_loglevel"]="--loglevel=1"
        ["ci_server"]="4840 server_cert.der server_key.der client_cert.der"
    )

    ciServer="ci_server 4840 server_cert.der server_key.der client_cert.der"

    declare -A server_needed_examples
    server_needed_examples=(["access_control_client"]=$ciServer
        ["access_control_client_encrypt"]=$ciServer
        ["client"]=$ciServer
        ["client_async"]=$ciServer
        ["client_connect"]=$ciServer
        ["client_connect_loop"]=$ciServer
        ["client_encryption"]="server_encryption server_cert.der server_key.der client_cert.der"
        ["client_event_filter"]=$ciServer
        ["client_historical"]="tutorial_server_historicaldata"
        ["client_method_async"]=$ciServer
        ["client_subscription_loop"]=$ciServer
        ["custom_datatype_client"]="custom_datatype_server"
        ["discovery_client_find_servers"]="discovery_server_lds"
        ["discovery_server_register"]="discovery_server_lds"
        ["pubsub_publish_encrypted"]="pubsub_subscribe_encrypted"
        ["pubsub_publish_encrypted_sks"]="server_pubsub_central_sks server_cert.der server_key.der --enableUnencrypted --enableAnonymous"
        ["pubsub_subscribe_encrypted"]="pubsub_publish_encrypted"
        ["pubsub_subscribe_encrypted_sks"]="server_pubsub_central_sks server_cert.der server_key.der --enableUnencrypted --enableAnonymous"
        ["pubsub_subscribe_standalone_dataset"]="tutorial_pubsub_publish"
        ["server_pubsub_publish_rt_level"]="server_pubsub_subscribe_rt_level"
        ["server_pubsub_publish_rt_level_raw"]="server_pubsub_subscribe_rt_level"
        ["server_pubsub_rt_information_model"]="server_pubsub_subscribe_rt_level"
        ["server_pubsub_subscribe_custom_monitoring"]="tutorial_pubsub_publish_raw"
        ["server_pubsub_subscribe_rt_level"]="server_pubsub_publish_rt_level"
        ["tutorial_client_events"]="tutorial_server_events"
        ["tutorial_client_firststeps"]="tutorial_server_firststeps"
        ["tutorial_pubsub_connection"]="tutorial_pubsub_subscribe_raw"
        ["tutorial_pubsub_mqtt_subscrib"]="tutorial_pubsub_mqtt_publish"
        ["tutorial_pubsub_publish"]="tutorial_pubsub_subscribe"
        ["tutorial_pubsub_publish_raw"]="tutorial_pubsub_subscribe_raw"
        ["tutorial_pubsub_subscribe"]="tutorial_pubsub_publish"
        ["tutorial_pubsub_subscribe_raw"]="tutorial_pubsub_publish_raw"
        ["tutorial_server_reverseconnect"]="ci_server 4841 client_cert.der client_key.der server_cert.der")

    declare -A client_needed_examples
    client_needed_examples=(["ci_server"]="client"
        ["server_ctt"]="client"
        ["server_inheritance"]="client"
        ["server_instantiation"]="client"
        ["server_loglevel"]="client"
        ["server_mainloop"]="client"
        ["server_nodeset"]="client"
        ["server_nodeset_loader"]="client"
        ["server_nodeset_plcopen"]="client"
        ["server_nodeset_powerlink"]="client"
        ["server_settimestamp"]="client"
        ["server_testnodeset"]="client"
        ["tutorial_server_monitoreditems"]="client"
        ["tutorial_server_variabletype"]="client"
        ["tutorial_server_object"]="client"
        ["tutorial_server_variable"]="client"
        ["access_control_server"]="access_control_client"
        ["custom_datatype_server"]="custom_datatype_client"
        ["discovery_server_lds"]="discovery_client_find_servers"
        ["server_encryption"]="client_encryption opc.tcp://localhost:4840 client_cert.der client_key.der server_cert.der"
        ["server_events_random"]="tutorial_client_events opc.tcp://localhost:4840"
        ["server_pubsub_central_sks"]="pubsub_publish_encrypted_sks"
        ["tutorial_server_alarms_conditions"]="tutorial_client_events opc.tcp://localhost:4840"
        ["tutorial_server_datasource"]="tutorial_client_events opc.tcp://localhost:4840"
        ["tutorial_server_firststeps"]="tutorial_client_firststeps"
        ["tutorial_server_historicaldata"]="client_historical"
        ["tutorial_server_historicaldata_circular"]="client_historical"
        ["tutorial_server_method"]="client_method_async"
        ["tutorial_server_method_async"]="client_method_async")

    # a blacklist for examples taht cause issues. Mostly because of EXIT_FAILURE...
    declare -A blacklist
    blacklist=(
        ["pubsub_TSN_loopback"]=1
        ["pubsub_TSN_loopback_single_thread"]=1
        ["pubsub_TSN_publisher"]=1
        ["pubsub_TSN_publisher_multiple_thread"]=1
        ["pubsub_nodeset_rt_publisher"]=1
        ["pubsub_nodeset_rt_subscriber"]=1
        ["access_control_client_encrypt"]=1
        ["client_encryption"]=1
        ["client_historical"]=1
    )
    mkdir -p build; cd build; rm -rf *

    python3 ../tools/certs/create_self-signed.py -c server 
    python3 ../tools/certs/create_self-signed.py -c client 

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
          -DUA_ENABLE_MQTT=ON \
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
        example_name=$(basename $f)
        args=${example_args[$example_name]:-}
        
        #check if examples is in blacklist. If yes, skip it
        if [[ ${blacklist[$example_name]:-} ]]; then
            echo "Skipping $example_name"
            continue
        fi

        # check if a server/client is needed for the example
        if [[ ${server_needed_examples[$example_name]:-} ]]; then
            # Start the server in the background
            server=${server_needed_examples[$example_name]:-}
            echo "Starting $server for $example_name"
            # silence all output of the server
            
            eval "./bin/examples/$server" >/dev/null 2>&1 &
            # Save the PID of the server process
            server_pid=$!
            # Give the server some time to start up
            sleep 10
        fi
 
        eval "valgrind --errors-for-leak-kinds=all --leak-check=full --error-exitcode=1 $f $args" &
        pid=$!

        # Give the example some time to start up
        sleep 5
        # check if a client is needed for the example
        if [[ ${client_needed_examples[$example_name]:-} ]]; then
            # Start the client in the background
            client=${client_needed_examples[$example_name]:-}
            echo "Starting $client for $example_name"
            # silence all output of the client
            eval "./bin/examples/$client" >/dev/null 2>&1 &
            # Save the PID of the client process
            client_pid=$!
        fi

        sleep 10
        # || true to ignore the error if the process is already dead
        kill -INT $pid || true

        sleep 5

        # Check if the process is still running.
        tries=0
        while ps | grep "$pid" >/dev/null; do
            tries=$((tries+1))
            if [[ $tries -gt 5 ]]; then
                echo "Process $pid is still running after 5 tries. Terminating process."
                kill -9 $pid || true
            fi
            echo "$pid is still in the ps output. Must still be running. Try: $tries"
            kill -INT $pid || true
            sleep 10
        done

        wait $pid
        EXIT_CODE=$?
        if [[ $EXIT_CODE -ne 0 ]]; then
            echo "Processing $f failed with exit code $EXIT_CODE "
            exit $EXIT_CODE
        fi

        if [[ ${server_needed_examples[$example_name]:-} ]]; then
            # Once the client has finished, kill the server
            echo "Stopping server_encryption with pid $server_pid for $example_name"
            kill -9 $server_pid || true
            wait $server_pid
        fi

        if [[ ${client_needed_examples[$example_name]:-} ]]; then
            # Once the client has finished, kill the client
            echo "Stopping client with pid $client_pid for $example_name"
            kill -9 $client_pid || true
            wait $client_pid
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
