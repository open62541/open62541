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
			dirPathTmp=$SRC/open62541/tests/fuzz/${fuzzerName}_corpus/${dir}_tmp
			if [ -d  $dirPathTmp ]; then
				rm -r $dirPathTmp
			fi
			mkdir $dirPathTmp
			# copy the files to get unique names
			binFiles=$(find $dirPath -name "*.bin")
			for binFile in $binFiles; do
				binFileName=$(basename $binFile .bin)
				cp $binFile $dirPathTmp/${dir}_${binFileName}.bin
			done

			outPath=$dirPathTmp/$dir.bin
			cat $dirPath/*.bin > $outPath

			zip -j $OUT/${fuzzerName}_seed_corpus.zip $dirPathTmp/*

			rm -r $dirPathTmp

		done

		# zip -j $OUT/${fuzzerName}_seed_corpus.zip $SRC/open62541/tests/fuzz/${fuzzerName}_corpus/*.bin
	fi
done

cp $SRC/open62541/tests/fuzz/*.dict $SRC/open62541/tests/fuzz/*.options $OUT/