#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Use the error status of the first failure in a pipeline
set -o pipefail

# Exit if an uninitialized variable is accessed
set -o nounset

# Clang build options
if [ "$CC_SHORTNAME" = "clang-mingw" ]; then
    export CC="clang --target=x86_64-w64-mingw32"
    export CXX="clang++ --target=x86_64-w64-mingw32"
fi

mkdir build
cd build

echo "Build MinGW Unit Tests"
cmake -G "Ninja" -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_BUILD_TYPE=Debug -DUA_BUILD_EXAMPLES=OFF -DUA_BUILD_UNIT_TESTS=ON -DUA_ENABLE_DA=ON -DUA_ENABLE_DISCOVERY=ON -DUA_ENABLE_ENCRYPTION:STRING=MBEDTLS -DUA_ENABLE_JSON_ENCODING:BOOL=ON -DUA_ENABLE_PUBSUB:BOOL=ON -DUA_ENABLE_PUBSUB_INFORMATIONMODEL:BOOL=ON -DUA_FORCE_WERROR:BOOL=OFF ..
cmake --build .
echo "Run MinGW Unit Tests"
cmake --build . --target test-verbose -j 1
rm -r *

echo "Build MinGW Examples"
cmake -G "Ninja" -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_BUILD_TYPE=Debug -DUA_BUILD_EXAMPLES:BOOL=ON -DUA_FORCE_WERROR=ON ..
cmake --build .
rm -r *

echo "Build MingW with NS0"
cmake -G "Ninja" -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_BUILD_TYPE=Debug -DUA_ENABLE_DA:BOOL=ON -DUA_ENABLE_JSON_ENCODING:BOOL=ON -DUA_ENABLE_PUBSUB:BOOL=ON -DUA_ENABLE_PUBSUB_INFORMATIONMODEL:BOOL=ON -DUA_ENABLE_SUBSCRIPTIONS_EVENTS:BOOL=ON -DUA_FORCE_WERROR=ON -DUA_NAMESPACE_ZERO:STRING=FULL ..
cmake --build .
rm -r *

echo "Build MinGW with .dll"
cmake -G "Ninja" -DBUILD_SHARED_LIBS:BOOL=ON -DCMAKE_BUILD_TYPE=Debug -DUA_FORCE_WERROR=ON ..
cmake --build .
rm -r *
