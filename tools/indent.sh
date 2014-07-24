#!/bin/bash
if [ $# -eq 0 ]; then
echo "usage: $0 <file1 file2 ...>"
   exit 1
fi
if [ "$(type -t uncrustify)" = "" ]; then
echo "Please install uncrustify first."
   exit 1
fi

TOOL_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
uncrustify -c $TOOL_DIR/uncrustify.cfg -l C --replace $@

# Remove trailing whitespace
for file in $@; do
	sed -i -e's/[[:space:]]*$//' "$file"
done
