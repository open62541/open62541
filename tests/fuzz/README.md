I Can haz fuzz
==============

open62541 is continuously tested with the awesome oss-fuzz project from Google:
https://github.com/google/oss-fuzz

Currently tested is processing of binary messages and encoding/decoding of
binary encoded data.

# Status

* [Build status](https://oss-fuzz-build-logs.storage.googleapis.com/index.html)
* [Open issues](https://bugs.chromium.org/p/oss-fuzz/issues/list?q=label:Proj-open62541)

# Update the corpus

To update the current corpus used for fuzzing you need to follow these steps.
It will execute all the unit tests, dump the received data packages to a directory
and then update and merge the corpus.

1. The script will create two directories: `open62541/build_fuzz` and `open62541/build_corpus`.
   Make sure that these directories are not existing or do not contain any important data.

2. Run the generate script:

   `open62541/tests/fuzz/generate_corpus.sh`
   
   This script will build all the unit tests, dump the packages and then merge the current 
   corpus with the new packages. 
    
3. If there is new coverage with the generated data there will be new files in the directory:

   `open62541/fuzz/fuzz_binary_message_corpus/generated`
   
   Commit the new files and then you can delete the build directories created in step 1.