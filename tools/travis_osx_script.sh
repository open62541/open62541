#!/bin/bash
set -ev

echo "Compile release build for OS X"
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DUA_ENABLE_AMALGAMATION=ON -DUA_BUILD_EXAMPLESERVER=ON -DUA_BUILD_EXAMPLECLIENT=ON -DUA_BUILD_DOCUMENTATION=ON -DUA_GENERATE_SELFSIGNED=ON ..
make
tar -pczf open62541-osx.tar.gz ../doc ../server_cert.der ../LICENSE ../AUTHORS ../README.md server_static server client_static client libopen62541.dylib open62541.h open62541.c
cp open62541-osx.tar.gz ..
cp open62541.h .. #copy single file-release
cp open62541.c .. #copy single file-release
cd .. && rm -rf build

echo "Compile multithreaded version"
mkdir -p build && cd build
cmake -DUA_ENABLE_MULTITHREADING=ON -DUA_BUILD_EXAMPLESERVER=ON ..
make
cd .. && rm -rf build

echo "Debug build and unit tests (64 bit)"
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DUA_BUILD_DEMO_NODESET=ON -DUA_BUILD_UNIT_TESTS=ON -DUA_BUILD_EXAMPLESERVER=ON -DUA_ENABLE_COVERAGE=ON ..
make && make test
echo "Run valgrind to see if the server leaks memory (just starting up and closing..)"
(valgrind --error-exitcode=3 ./server & export pid=$!; sleep 2; kill -INT $pid; wait $pid);
cd .. && rm -rf build
