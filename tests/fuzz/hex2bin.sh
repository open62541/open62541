#!/bin/bash

# Convert the .hex files from tests/dumps to binary files so that they can be fed into the fuzzer as corpus

# Read either the first argument or from stdin (http://stackoverflow.com/questions/6980090/bash-read-from-file-or-stdin)
cat "${1:-/dev/stdin}" | \
# Strip out comments starting with #
sed -E 's/#.*$//' | \
# Strip out comments starting with //
sed -E 's/\/\/.*$//' | \
# Strip out multi-line comments /* ... */
perl -0777 -pe 's{/\*.*?\*/}{}gs' | \
# Strip out all non hexadecimal characters
sed -E 's/[^0-9a-fA-F]*//g' | \
# Convert hex to binary using xxd's reverser in plain hexdump style
xxd -r -ps
