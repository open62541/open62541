#!/usr/bin/env bash
set -e

# --------------------------------------------------------------------
# Copies all the corpus files, dict and options to the $OUT directory.
# This script is only used on oss-fuzz directly
# --------------------------------------------------------------------

fuzzerFiles=$(find $SRC/open62541/tests/fuzz/ -name "*.cc")

for F in $fuzzerFiles; do
	fuzzerName=$(basename $F .cc)

	if [ -d "$SRC/open62541/tests/fuzz/${fuzzerName}_corpus" ]; then
		zip -jr $OUT/${fuzzerName}_seed_corpus.zip $SRC/open62541/tests/fuzz/${fuzzerName}_corpus/
	fi
done

# fuzz_binary_message and fuzz_tcp_message have the same corpus, just copy the .zip
cp $OUT/fuzz_binary_message_seed_corpus.zip $OUT/fuzz_tcp_message_seed_corpus.zip
zip -j $OUT/fuzz_opn_message_seed_corpus.zip \
	$SRC/open62541/tests/fuzz/fuzz_binary_message_corpus/generated/*_opn.bin
zip -j $OUT/fuzz_msg_message_seed_corpus.zip \
	$SRC/open62541/tests/fuzz/fuzz_binary_message_corpus/generated/*_msg_*.bin

cp $SRC/open62541/tests/fuzz/*.dict $SRC/open62541/tests/fuzz/*.options $OUT/
