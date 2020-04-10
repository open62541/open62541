#!/usr/bin/env bash

set -e

echo "== Unit tests (reduced NS0) =="

mkdir -p build && cd build

# Pubsub currently fails in the qemu environment. Error is "Protocol not available" here:
# UA_setsockopt(newChannel->sockfd, IPPROTO_IP, IP_MULTICAST_IF ...
cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DUA_BUILD_EXAMPLES=ON \
    -DUA_BUILD_UNIT_TESTS=ON \
    -DUA_ENABLE_COVERAGE=OFF \
    -DUA_ENABLE_DA=ON \
    -DUA_ENABLE_DISCOVERY=ON \
    -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
    -DUA_ENABLE_ENCRYPTION=ON \
    -DUA_ENABLE_JSON_ENCODING=ON \
    -DUA_ENABLE_PUBSUB=OFF \
    -DUA_ENABLE_PUBSUB_DELTAFRAMES=OFF \
    -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=OFF \
    -DUA_ENABLE_SUBSCRIPTIONS=ON \
    -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
    -DUA_ENABLE_UNIT_TESTS_MEMCHECK=OFF \
    -DUA_NAMESPACE_ZERO=REDUCED ..
make -j && make test ARGS="-V"
