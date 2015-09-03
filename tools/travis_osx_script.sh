#!/bin/bash
set -ev
echo "Documentation and certificate build"
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_DOCUMENTATION=ON -DGENERATE_SELFSIGNED=ON ..
make doc
make selfsigned
cp -r doc ..
cp server_cert.der ..
echo "Testing builds"
cd .. && rm -rf build && mkdir -p build && cd build
echo "Compile release build for OS X"
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_AMALGAMATION=ON -DBUILD_EXAMPLESERVER=ON -DBUILD_EXAMPLECLIENT=ON ..
make
tar -pczf open62541-osx.tar.gz ../doc ../server_cert.der ../LICENSE ../AUTHORS ../README.md server_static server client_static client libopen62541.dylib open62541.h open62541.c
cp open62541-osx.tar.gz ..
cp open62541.h .. #copy single file-release
cp open62541.c .. #copy single file-release
cd .. && rm -rf build && mkdir -p build && cd build
echo "Compile multithreaded version"
cmake -DENABLE_MULTITHREADING=ON -DBUILD_EXAMPLESERVER=ON ..
make
cd .. && rm -rf build && mkdir -p build && cd build
echo "Debug build and unit tests (64 bit)"
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_DEMO_NODESET=ON -DBUILD_UNIT_TESTS=ON -DBUILD_EXAMPLESERVER=ON -DENABLE_COVERAGE=ON ..
make && make test
echo "Run valgrind to see if the server leaks memory (just starting up and closing..)"
(valgrind --error-exitcode=3 ./server & export pid=$!; sleep 2; kill -INT $pid; wait $pid);
cd ..
