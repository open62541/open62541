#!/bin/sh

set -eu

echo -e "\r\n== Unit tests (full NS0) =="
mkdir -p build && cd build

# Valgrind cannot handle the full NS0 because the generated file is too big. Thus run NS0 full without valgrind
cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/$PYTHON \
    -DUA_BUILD_EXAMPLES=ON \
    -DUA_BUILD_UNIT_TESTS=ON \
    -DUA_ENABLE_COVERAGE=OFF \
    -DUA_ENABLE_DA=ON \
    -DUA_ENABLE_DISCOVERY=ON \
    -DUA_ENABLE_DISCOVERY_MULTICAST=ON \
    -DUA_ENABLE_ENCRYPTION=ON \
    -DUA_ENABLE_JSON_ENCODING=ON \
    -DUA_ENABLE_PUBSUB=ON \
    -DUA_ENABLE_PUBSUB_DELTAFRAMES=ON \
    -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON \
    -DUA_ENABLE_SUBSCRIPTIONS=ON \
    -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
    -DUA_ENABLE_UNIT_TESTS_MEMCHECK=OFF \
    -DUA_NAMESPACE_ZERO=FULL ..
make -j
