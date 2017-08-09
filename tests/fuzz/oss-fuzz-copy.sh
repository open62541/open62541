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

		# first combine any files in subfolders to one single binary file
		subDirs=$(find $SRC/open62541/tests/fuzz/${fuzzerName}_corpus -maxdepth 1 -mindepth 1 -type d)
		for dirPath in $subDirs; do
			dir=$(basename $dirPath)
			outPath=$SRC/open62541/tests/fuzz/${fuzzerName}_corpus/$dir.bin
			if [ -f $outPath ]; then
				rm $outPath;
			fi
			echo "Combining content of $dir into $outPath"
			cat $dirPath/*.bin > $outPath
		done

		zip -j $OUT/${fuzzerName}_seed_corpus.zip $SRC/open62541/tests/fuzz/${fuzzerName}_corpus/*
	fi
done

cp $SRC/open62541/tests/fuzz/*.dict $SRC/open62541/tests/fuzz/*.options $OUT/