#!/usr/bin/env bash

# NodesetLoader `AddNode()` integration test use-case for the
# ordering of the nodes within nodeset XML file. Two different
# versions of DI companion standard are tested:
#      - Official version
#      - Version where the ordering is invalid:
#          * Parent node (ns=1;i=15001) is defined after child nodes:
#              + ns=1;i=15002
#              + ns=1;i=15003
#              + ns=1;i=15004
#              + ns=1;i=15005
#              + ns=1;i=15006
#              + ns=1;i=15007
#              + ns=1;i=15008
#              + ns=1;i=15031
#              + ns=1;i=15032
#              + ns=1;i=15033
#

IFS=

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" && pwd )

argv=("$@")
argc=$#
CLIENT_BINARY_PATH="${argv[0]}"
SERVER_BINARY_PATH="${argv[1]}"
SERVER_PID=
CustomNsStartNum=
declare -a NodeIdFiles=()

function start_server() {
    echo "Start server: "$@
    $SERVER_BINARY_PATH $@ & disown
    SERVER_PID=$!
}

function prepare_nodeids() {
    CustomNsStartNum=2
    for nodeset in "$@"
    do
        nodesetIdPath=$(basename $nodeset)
        nodesetIdPath=${nodesetIdPath%.*}

        grep --line-buffered -R '<UAObjectType\|<UAObject\|<UAVariable\|<UAVariableType\|<UAMethod\|<UADataType\|<UAReferenceType\|<UAView' \
                                $nodeset | awk '{ sub(/.*\ NodeId="/, ""); sub(/".*/, ""); print $1}' >> $nodesetIdPath
        sed -i "s/ns=1;/ns=$CustomNsStartNum;/gI" $nodesetIdPath

        NodeIdFiles+=("$nodesetIdPath")
        CustomNsStartNum=$((CustomNsStartNum+1))
    done
}

function run_client() {
    echo "Start client: "$@
    $CLIENT_BINARY_PATH $@
    clientResult=$?
    if [ $clientResult -ne 0 ] ; then
        stop_server
        rm ${NodeIdFiles[@]}
        exit 1
    fi
}

function stop_server() {
    echo "Stop server:"
    kill -9 $SERVER_PID
    if [ $? -ne 0 ] ; then
        echo "Error: stopping the server failed"
        echo "PID = " $SERVER_PID
        rm ${NodeIdFiles[@]}
        exit 1
    fi
}

function add_node_integration_test() {
    nodesetName=$1
    shift 1
    prepare_nodeids $@
    start_server $@
    sleep 3
    run_client $nodesetName ${NodeIdFiles[@]}
    stop_server
    rm ${NodeIdFiles[@]}
    NodeIdFiles=()
}

add_node_integration_test "DI" \
    $SCRIPT_DIR/../../deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml

add_node_integration_test "DI-Invalid-Ordering" \
    $SCRIPT_DIR/../../deps/nodesetLoader/nodesets/open62541/Opc.Ua.Di.NodeSet2_invalid_ordering.xml
