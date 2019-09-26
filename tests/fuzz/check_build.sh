#!/usr/bin/env bash

set -e

git clone https://github.com/google/oss-fuzz $HOME/oss-fuzz
python $HOME/oss-fuzz/infra/helper.py build_fuzzers --sanitizer address open62541 $TRAVIS_BUILD_DIR && python $HOME/oss-fuzz/infra/helper.py check_build --sanitizer address open62541
