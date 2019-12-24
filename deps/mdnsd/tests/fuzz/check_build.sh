#!/usr/bin/env bash

set -e

git clone https://github.com/Pro/oss-fuzz -bmdnsd $HOME/oss-fuzz
python $HOME/oss-fuzz/infra/helper.py build_fuzzers --sanitizer address mdnsd $TRAVIS_BUILD_DIR && python $HOME/oss-fuzz/infra/helper.py check_build --sanitizer address mdnsd
