#!/bin/bash
set -e

echo "\n=== Building ==="
export OPENSSL_ROOT_DIR="/usr/local/opt/openssl"
export PATH="/Users/travis/Library/Python/2.7/bin:$PATH"

# echo "Documentation and certificate build" && echo -en 'travis_fold:start:script.build.doc\\r'
# mkdir -p build && cd build
# cmake \
#     -DCMAKE_BUILD_TYPE=Release \
#     -DUA_BUILD_EXAMPLES=ON \
#     -DUA_BUILD_SELFSIGNED_CERTIFICATE=ON ..
# make selfsigned
# ls examples
# cp examples/server_cert.der ../
# cd .. && rm -rf build
# echo -en 'travis_fold:end:script.build.doc\\r'

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
    -DUA_ENABLE_UNIT_TESTS_MEMCHECK=ON ..
make -j && make test ARGS="-V"
if [ $? -ne 0 ] ; then exit 1 ; fi
cd .. && rm -rf build
echo -en 'travis_fold:end:script.build.unit_test\\r'
