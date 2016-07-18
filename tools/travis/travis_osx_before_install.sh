#!/bin/bash
set -ev

if [ $ANALYZE = "true" ]; then
    echo "Skipping static analysis for OS X"
    exit 0
else
    brew install check
    brew install userspace-rcu
    brew install valgrind

    pip install --user sphinx
    pip install --user breathe
    pip install --user sphinx_rtd_theme
fi
