#!/bin/bash
set -ev

if [ $ANALYZE = "true" ]; then
    echo "\n=== Skipping static analysis on OS X ==="
    exit 0
else
    echo "\n=== Building ==="

    echo "Compile release build for OS X"
    mkdir -p build && cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make -j
    cd .. && rm -rf build
fi
