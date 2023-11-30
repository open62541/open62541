#!/bin/bash
set -ev

if [ $ANALYZE = "true" ]; then
    echo "Skipping static analysis for OS X"
    exit 0
else
    brew update
    brew install check
    brew install userspace-rcu
    brew install valgrind
fi
