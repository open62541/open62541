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

# Allow to reuse TIME-WAIT sockets for new connections
sudo sysctl -w net.ipv4.tcp_tw_reuse=1

##################################
# Install lwip and setup TUN/TAP #
##################################

function install_lwip {
    # Store the current directory and go to ${HOME}
    CURRDIR="$PWD"
    cd "${HOME}"

    # Get lwip
    git clone https://github.com/lwip-tcpip/lwip.git
    cd ./lwip
    git checkout tags/STABLE-2_2_1_RELEASE

    # Apply the modification to lwipopts.h
    sed -i '30a\
    #define MEMP_NUM_TCP_PCB 128\n\
    #define MEMP_NUM_UDP_PCB 128\n\
    #define MEMP_NUM_REASSDATA 128\n\
    #define MEMP_NUM_TCP_PCB_LISTEN 128\n\
    #define MEMP_NUM_TCP_SEG 128\n\
    #define MEMP_NUM_PBUF 128\n\
    #define MEMP_NUM_RAW_PCB 128\n\
    #define MEMP_NUM_SELECT_CB 128\n\
    #define MEMP_NUM_NETBUF 128\n\
    #define MEMP_NUM_NETCONN 128\n\
    #define PBUF_POOL_SIZE 128\n\
    #define MEM_SIZE 64000\n\
    #define LWIP_SOCKET 1\n\
    #define LWIP_DNS 0' ./contrib/ports/unix/posixlib/lwipopts.h

    # Make and install
    cd ./contrib/ports/unix/posixlib
    mkdir build
    cd ./build
    cmake ..
    make -j$(nproc)
    sudo make install

    cd "${CURRDIR}" # Reset directory

    # Setup the tun/tap device
    sudo ip tuntap add dev tap0 mode tap
    sudo ip link set dev tap0 up
    sudo ip addr add 192.168.0.1/24 dev tap0
    sudo sysctl -w net.ipv4.ip_forward=1

    # Set the environment variable of the tap0 device to use
    export PRECONFIGURED_TAPIF=tap0
    echo 'export PRECONFIGURED_TAPIF=tap0' >> ~/.bashrc
}

#######################
# Install tpm2-pkcs11 #
#######################

function install_tpm2_pkcs11 {
    # Store the current directory and go to ${HOME}
    CURRDIR="$PWD"
    cd "${HOME}"

    # Get tpm2-tss
    git clone https://github.com/tpm2-software/tpm2-tss.git
    cd ./tpm2-tss
    git checkout $1

    # Build and install
    ./bootstrap
    ./configure --with-udevrulesdir=/etc/udev/rules.d \
                --with-udevrulesprefix=70- \
                --with-systemdsystemunitdir=no
    make -j$(nproc)
    sudo make install
    sudo ldconfig
    sudo udevadm control --reload-rules
    sudo udevadm trigger

    cd "${HOME}" # Go back to $HOME

    # Get tpm2-pkcs11 code
    git clone https://github.com/tpm2-software/tpm2-pkcs11.git
    cd ./tpm2-pkcs11
    git checkout $2

    # Build and install
    ./bootstrap
    ./configure
    make -j$(nproc)
    sudo make install
    sudo ldconfig
    sudo cp ./src/pkcs11.h /usr/include

    # Build and install python package
    cd ./tools/
    pip3 install --break-system-packages .

    cd "${CURRDIR}" # Reset directory
}

#####################################
# Build Documentation including PDF #
#####################################

function build_docs_pdf {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_FORCE_WERROR=ON \
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
          -DUA_FORCE_WERROR=ON \
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
          -DCMAKE_BUILD_TYPE=RelWithDebInfo \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_FORCE_WERROR=ON \
          ..
    make ${MAKEOPTS}
}

function build_release_amalgamation {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=None \
          -DUA_ENABLE_AMALGAMATION=ON \
          -DUA_NAMESPACE_ZERO=FULL \
          -DUA_ENABLE_DATATYPES_ALL=ON \
          -DUA_ENABLE_ENCRYPTION=MBEDTLS \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_ENCRYPTION=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          ..
    make open62541-amalgamation ${MAKEOPTS}
    gcc -Wall -Werror -c open62541.c
}

######################
# Build Amalgamation #
######################

function build_amalgamation {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_ENABLE_AMALGAMATION=ON \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_ENABLE_JSON_ENCODING=ON \
          -DUA_ENABLE_XML_ENCODING=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          ..
    make open62541-amalgamation ${MAKEOPTS}
    gcc -Wall -Werror -c open62541.c
}

function build_amalgamation_mt {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_ENABLE_AMALGAMATION=ON \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_ENABLE_JSON_ENCODING=ON \
          -DUA_ENABLE_XML_ENCODING=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_MULTITHREADING=100 \
          ..
    make open62541-amalgamation ${MAKEOPTS}
    gcc -Wall -Werror -c open62541.c
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
    if [ "${CC:-x}" = "tcc" ]; then
        # tcc does not allow multi-threading up to version 0.9.27.
        # because it supports atomic intrinsics only after.
        MULTITHREADING=0
        tcc_recent=$(tcc -v | awk 'match($0, /tcc version [0-9]+\.[0-9]+\.[0-9]+/) {split(substr($0, 13, RLENGTH-12), ver, "."); print(ver[1] > 0 || ver[2] > 9 || ver[3] > 27);}')
        if [ $tcc_recent = 1 ]; then
            MULTITHREADING=100
        fi
    else
        MULTITHREADING=100
    fi
    # Only build coverage for gcc. Clang fails because coverage build passes
    # invalid --coverage flag and clang complains because of
    # -Werror,-Wunused-command-line-argument. tcc doesn't seem to support
    # coverage at all.
    if [[ "${CC:-gcc}" =~ gcc* ]]; then
        COVERAGE=ON
    else
        COVERAGE=OFF
    fi
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_COVERAGE=${COVERAGE} \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_ENABLE_JSON_ENCODING=ON \
          -DUA_ENABLE_XML_ENCODING=ON \
          -DUA_ENABLE_NODESETLOADER=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_MQTT=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_FORCE_WERROR=ON \
          -DUA_MULTITHREADING=${MULTITHREADING} \
          ..
    make ${MAKEOPTS}
    set_capabilities
    make test ARGS="-V"
    if [ "$COVERAGE" = "ON" ]; then
        make gcov
    fi
}

function unit_tests_lwip {
    mkdir -p build; cd build; rm -rf *
    cmake -DUA_ARCHITECTURE="posix-lwip" \
          -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_COVERAGE=ON \
          -DUA_ENABLE_PUBSUB=OFF \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=OFF \
          -DUA_FORCE_WERROR=ON \
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
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_ENABLE_JSON_ENCODING=ON \
          -DUA_ENABLE_XML_ENCODING=ON \
          -DUA_ENABLE_NODESETLOADER=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_FORCE_32BIT=ON \
          -DUA_FORCE_WERROR=ON \
          ..
    make ${MAKEOPTS}
    set_capabilities
    make test ARGS="-V"
}

function unit_tests_nosub {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_COVERAGE=ON \
          -DUA_ENABLE_HISTORIZING=OFF \
          -DUA_ENABLE_SUBSCRIPTIONS=OFF \
          -DUA_FORCE_WERROR=ON \
          ..
    make ${MAKEOPTS}
    set_capabilities
    make test ARGS="-V"
    make gcov
}

function unit_tests_diag {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_COVERAGE=ON \
          -DUA_ENABLE_DIAGNOSTICS=ON \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_ENABLE_JSON_ENCODING=ON \
          -DUA_ENABLE_XML_ENCODING=ON \
          -DUA_ENABLE_NODESETLOADER=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_FORCE_WERROR=ON \
          ..
    make ${MAKEOPTS}
    set_capabilities
    make test ARGS="-V"
    make gcov
}

function unit_tests_mt {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_MULTITHREADING=200 \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_COVERAGE=ON \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_FORCE_WERROR=ON \
          ..
    make ${MAKEOPTS}
    set_capabilities
    make test ARGS="-V"
    make gcov
}

function unit_tests_alarms {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_COVERAGE=ON \
          -DUA_ENABLE_DA=ON \
          -DUA_ENABLE_NODESETLOADER=ON \
          -DUA_ENABLE_XML_ENCODING=ON \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
              -DUA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS=ON \
          -DUA_FORCE_WERROR=ON \
          -DUA_NAMESPACE_ZERO=FULL \
          ..
    make ${MAKEOPTS}
    set_capabilities
    make test ARGS="-V"
    make gcov
}

function unit_tests_encryption {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_ENABLE_GDS_PUSHMANAGEMENT=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_COVERAGE=ON \
          -DUA_ENABLE_ENCRYPTION=$1 \
          -DUA_FORCE_WERROR=ON \
          -DUA_NAMESPACE_ZERO=FULL \
          ..
    make ${MAKEOPTS}
    set_capabilities
    make test ARGS="-V"
    make gcov
}

function unit_tests_encryption_mbedtls_pubsub {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_COVERAGE=ON \
          -DUA_ENABLE_ENCRYPTION=MBEDTLS \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_FORCE_WERROR=ON \
          ..
    make ${MAKEOPTS}
    set_capabilities
    make test ARGS="-V"
    make gcov
}

function unit_tests_pubsub_sks {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_NAMESPACE_ZERO=FULL \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_COVERAGE=ON \
          -DUA_ENABLE_ENCRYPTION=MBEDTLS \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_ENABLE_PUBSUB_SKS=ON \
          -DUA_ENABLE_UNIT_TESTS_MEMCHECK=ON \
          -DUA_FORCE_WERROR=ON \
          ..
    make ${MAKEOPTS}
    sudo -E bash -c "make test ARGS=\"-V -R sks\""
    make gcov
}

##########################################
# Build and Run Unit Tests with Valgrind #
##########################################

function unit_tests_valgrind {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_ARCHITECTURE=$2 \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_ENCRYPTION=$1 \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_ENABLE_JSON_ENCODING=ON \
          -DUA_ENABLE_XML_ENCODING=ON \
          -DUA_ENABLE_NODESETLOADER=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_MQTT=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_ENABLE_UNIT_TESTS_MEMCHECK=ON \
          -DUA_FORCE_WERROR=ON \
          ..
    make ${MAKEOPTS}
    # set_capabilities not possible with valgrind
    sudo -E bash -c "make test ARGS=\"-V\""
}

##########################
# Build and Run Examples #
##########################

function run_examples {
    mkdir -p build; cd build; rm -rf *

    # create certificates for the examples
    python3 ../tools/certs/create_self-signed.py -c server
    python3 ../tools/certs/create_self-signed.py -c client

    # copy json server config
    cp ../examples/json_config/server_json_config.json5 server_json_config.json5

    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_ENABLE_ENCRYPTION=$1 \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS=ON \
          -DUA_ENABLE_JSON_ENCODING=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_ENABLE_UNIT_TESTS_MEMCHECK=ON \
          -DUA_ENABLE_MQTT=ON \
          -DUA_ENABLE_PUBSUB_FILE_CONFIG=ON \
          -DUA_NAMESPACE_ZERO=FULL \
          -DUA_ENABLE_NODESETLOADER=ON \
          -DUA_ENABLE_PUBSUB_SKS=ON \
          -DUA_ENABLE_DISCOVERY=ON \
          -DUA_ENABLE_DISCOVERY_MULTICAST=$2 \
          -DUA_FORCE_WERROR=ON \
          ..
    make ${MAKEOPTS}

    # Run each example. Wait 10 seconds and send the SIGINT
    # signal. Wait for the process to terminate and collect the exit status.
    # Abort when the exit status is non-null.
    sudo -E bash -c "python3 ../tools/ci/linux/examples_with_valgrind.py --no-valgrind"
    EXIT_CODE=$?
    if [[ $EXIT_CODE -ne 0 ]]; then
        echo "Processing failed with exit code $EXIT_CODE"
        exit $EXIT_CODE
    fi
}

########################################
# Build and Run Examples with Valgrind #
########################################

function examples_valgrind {
    mkdir -p build; cd build; rm -rf *

    # create certificates for the examples
    python3 ../tools/certs/create_self-signed.py -c server
    python3 ../tools/certs/create_self-signed.py -c client

    # copy json server config
    cp ../examples/json_config/server_json_config.json5 server_json_config.json5

    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_ARCHITECTURE=$3 \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_ENABLE_ENCRYPTION=$1 \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS=ON \
          -DUA_ENABLE_JSON_ENCODING=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_ENABLE_UNIT_TESTS_MEMCHECK=ON \
          -DUA_ENABLE_MQTT=ON \
          -DUA_ENABLE_PUBSUB_FILE_CONFIG=ON \
          -DUA_NAMESPACE_ZERO=FULL \
          -DUA_ENABLE_NODESETLOADER=ON \
          -DUA_ENABLE_PUBSUB_SKS=ON \
          -DUA_ENABLE_DISCOVERY=ON \
          -DUA_ENABLE_DISCOVERY_MULTICAST=$2 \
          -DUA_FORCE_WERROR=ON \
          ..
    make ${MAKEOPTS}

    # Run each example with valgrind. Wait 10 seconds and send the SIGINT
    # signal. Wait for the process to terminate and collect the exit status.
    # Abort when the exit status is non-null.
    # set_capabilities not possible with valgrind
    sudo -E bash -c "python3 ../tools/ci/linux/examples_with_valgrind.py"
    EXIT_CODE=$?
    if [[ $EXIT_CODE -ne 0 ]]; then
        echo "Processing failed with exit code $EXIT_CODE"
        exit $EXIT_CODE
    fi
}

##############################
# Clang Static Code Analysis #
##############################

function build_clang_analyzer {
    local version=$1
    mkdir -p build; cd build; rm -rf *
    scan-build-$version cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_ENABLE_ENCRYPTION=MBEDTLS \
          -DUA_ENABLE_GDS_PUSHMANAGEMENT=ON \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
          -DUA_ENABLE_JSON_ENCODING=ON \
          -DUA_ENABLE_XML_ENCODING=ON \
          -DUA_ENABLE_NODESETLOADER=ON \
          -DUA_ENABLE_PUBSUB=ON \
          -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
          -DUA_FORCE_WERROR=ON \
          -DUA_NAMESPACE_ZERO=FULL \
          ..
    scan-build-$version \
          --status-bugs \
          --exclude ../src/util \
          --exclude ../tests \
          make ${MAKEOPTS}
}

########################################
# Compile all Companion Specifications #
########################################

function build_all_companion_specs {
    mkdir -p build; cd build; rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DUA_BUILD_EXAMPLES=ON \
          -DUA_BUILD_UNIT_TESTS=ON \
          -DUA_FORCE_WERROR=ON \
          -DUA_INFORMATION_MODEL_AUTOLOAD=DI\;IA\;ISA95-JOBCONTROL\;OpenSCS\;CNC\;\
AMB\;AutoID\;POWERLINK\;Machinery\;LADS\;PackML\;PNEM\;PLCopen\;MachineTool\;\
PROFINET\;MachineVision\;FDT\;CommercialKitchenEquipment\;Scales\;Weihenstephan\;Pumps\;CAS\;TMC \
          -DUA_NAMESPACE_ZERO=FULL \
          ..
    make ${MAKEOPTS} check_nodeset_compiler_testnodeset
    ./bin/tests/check_nodeset_compiler_testnodeset
}
