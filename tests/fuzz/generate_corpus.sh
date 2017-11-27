#!/usr/bin/env bash

set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

BASE_DIR="$( cd "$DIR/../../" && pwd )"

BUILD_DIR_FUZZ_MODE="$DIR/../../build_fuzz"
if [ ! -d "$BUILD_DIR_FUZZ_MODE" ]; then
    mkdir $BUILD_DIR_FUZZ_MODE
fi
BUILD_DIR_FUZZ_MODE="$( cd "$DIR/../../build_fuzz" && pwd )"

BUILD_DIR_CORPUS="$DIR/../../build_corpus"
if [ ! -d "$BUILD_DIR_CORPUS" ]; then
    mkdir $BUILD_DIR_CORPUS
fi
BUILD_DIR_CORPUS="$( cd "$DIR/../../build_corpus" && pwd )"

cd $BUILD_DIR_CORPUS
if [ -d "$BUILD_DIR_CORPUS/corpus" ]; then
    rm -rf "$BUILD_DIR_CORPUS/corpus"
fi

if [ $TRAVIS = true ]; then
	export CC=clang-3.9
	export CXX=clang++-3.9
else
	export CC=clang-5.0
	export CXX=clang++-5.0
fi
# First build and run the unit tests without any specific fuzz settings
cmake -DUA_BUILD_FUZZING_CORPUS=ON -DUA_BUILD_UNIT_TESTS=ON ..
make -j && make test ARGS="-V"
if [ $? -ne 0 ] ; then exit 1 ; fi
# Run our special generator
$BUILD_DIR_CORPUS/bin/corpus_generator
if [ $? -ne 0 ] ; then exit 1 ; fi

# Now build the fuzzer executables
cd $BUILD_DIR_FUZZ_MODE
cmake -DUA_BUILD_FUZZING=ON ..
make -j
if [ $? -ne 0 ] ; then exit 1 ; fi

merge_corpus() {
    local fuzzer="$1"
    local corpus_existing="$2"
    local corpus_new="$3"


    if [ -d "$corpus_existing" ]; then
        echo "Merging ${corpus_existing} into ${corpus_new}"
        "$fuzzer" -merge=1 "$corpus_existing" "${corpus_new}"
    else
        echo "Copying ${corpus_new} into ${corpus_existing}"
        cp -r ${corpus_new} ${corpus_existing}
    fi
}


merge_corpus $BUILD_DIR_FUZZ_MODE/bin/fuzz_binary_message $BASE_DIR/tests/fuzz/fuzz_binary_message_corpus/generated $BUILD_DIR_CORPUS/corpus
if [ $? -ne 0 ] ; then exit 1 ; fi