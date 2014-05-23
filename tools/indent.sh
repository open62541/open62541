#!/bin/bash
if [ $# -eq 0 ]; then
echo "usage: $0 <file1 file2 ...>"
   exit 1
fi
if [ "$(type -t indent)" = "" ]; then
echo "Please install GNU indent first."
   exit 1
fi

# Assume a width of 4 spaces per tab.
# E.g. when indenting to the opening braces of an argument list.

indent \
--line-length120 \
--comment-line-length100 \
--indent-level4 \
--use-tabs \
--tab-size4 \
--case-indentation0 \
--declaration-indentation1 \
--ignore-newlines \
--swallow-optional-blank-lines \
--blank-lines-after-procedures \
--no-blank-lines-after-commas \
--break-after-boolean-operator \
--no-space-after-for \
--no-space-after-if \
--no-space-after-while \
--no-space-after-casts \
--braces-on-if-line \
--braces-on-func-def-line \
--braces-on-struct-decl-line \
--cuddle-do-while \
--cuddle-else \
--dont-break-procedure-type \
--continue-at-parentheses \
--no-space-after-function-call-names \
--no-space-after-parentheses \
--no-comment-delimiters-on-blank-lines \
--comment-indentation0 \
--format-first-column-comments \
--declaration-comment-column0 \
--format-all-comments \
--line-comments-indentation0 \
--space-special-semicolon \
$@


# Remove trailing whitespace
for file in $@; do
	sed -i -e's/[[:space:]]*$//' "$file"
done
