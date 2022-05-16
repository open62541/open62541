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

if [ -z ${TRAVIS+x} ]; then
	export CC=clang
	export CXX=clang++
else
	# Travis needs a specific
	export CC=clang-6.0
	export CXX=clang++-6.0
fi

# First build and run the unit tests without any specific fuzz settings
echo -en "\r\n=== Building Fuzzing for Corpus ===\r\n"
cmake -DUA_BUILD_FUZZING_CORPUS=ON -DUA_BUILD_UNIT_TESTS=ON ..
make -j && make test
if [ $? -ne 0 ] ; then exit 1 ; fi

# Run our special generator
echo -en "\r\n=== Generate Corpus ===\r\n"
$BUILD_DIR_CORPUS/bin/corpus_generator
if [ $? -ne 0 ] ; then exit 1 ; fi

# Now build the fuzzer executables
echo -en "\r\n=== Build Fuzzer Executables ===\r\n"
cd $BUILD_DIR_FUZZ_MODE
cmake -DUA_BUILD_FUZZING=ON ..
make -j
if [ $? -ne 0 ] ; then exit 1 ; fi

merge_corpus() {
    local fuzzer="$1"
    local corpus_existing="$2"
    local corpus_new="$3"

    if [ -d "$corpus_existing" ]; then
        echo "Merging ${corpus_new} into ${corpus_existing}"
        "$fuzzer" -merge=1 "$corpus_existing" "${corpus_new}"
    else
        echo "Copying ${corpus_new} into ${corpus_existing}"
        cp -r ${corpus_new} ${corpus_existing}
    fi
}


# Iterate over all files and combine single message files to a full interaction, i.e.,
# After running the corpus generator, the output directory contains single files for each
# message (HEL, OPN, MSG..., CLO). Fuzzer needs these files to be combined into one single file





CORPUS_SINGLE=$BUILD_DIR_CORPUS/corpus
CORPUS_COMBINED=$BUILD_DIR_CORPUS/corpus_combined

if [ -d  $CORPUS_COMBINED ]; then
	rm -r $CORPUS_COMBINED
fi
mkdir $CORPUS_COMBINED

# iterate over all the subdirectories
subDirs=$(find $CORPUS_SINGLE -maxdepth 1 -mindepth 1 -type d)
for dirPath in $subDirs; do
	# if empty, skip
	if ! [ -n "$(ls -A $dirPath)" ]; then
		#echo "Skipping empty $dirPath"
		continue
	fi

	dir=$(basename $dirPath)
	dirPathTmp=$CORPUS_COMBINED/${dir}
	if [ -d  $dirPathTmp ]; then
		rm -r $dirPathTmp
	fi
	mkdir $dirPathTmp

	# The files are ordered by interaction. So we start with the first file
	# and combine all of them until we get the CLO file.
	# Then we start a new file and combine them again.

	currCount=1

	for binFile in `ls $dirPath/*.bin | sort -V`; do

		#echo "Combining $binFile to $dirPathTmp/msg_${currCount}.bin"
		cat $binFile >> $dirPathTmp/${dir}_msg_${currCount}.bin

		# if it is a close message, start new message
		if [[ "$binFile" == *clo.bin ]]; then
			currCount=$((currCount+1))
		fi
	done
done

echo -en "\r\n=== Merge Corpus ===\r\n"

merge_corpus $BUILD_DIR_FUZZ_MODE/bin/fuzz_binary_message $BASE_DIR/tests/fuzz/fuzz_binary_message_corpus/generated $CORPUS_COMBINED
if [ $? -ne 0 ] ; then exit 1 ; fi
