# Find Clang Tools
#
# This module defines
#  CLANG_TIDY_PROGRAM, The  path to the clang tidy binary
#  CLANG_TIDY_FOUND, Whether clang tidy was found
#  CLANG_FORMAT_PROGRAM, The path to the clang format binary
#  CLANG_FORMAT_FOUND, Whether clang format was found

find_program(CLANG_TIDY_PROGRAM
  NAMES clang-tidy-3.9 clang-tidy-3.8 clang-tidy-3.7 clang-tidy-3.6 clang-tidy
  PATHS $ENV{CLANG_TOOLS_PATH} /usr/local/bin /usr/bin
  NO_DEFAULT_PATH)

mark_as_advanced(CLANG_TIDY_PROGRAM)

if("${CLANG_TIDY_PROGRAM}" STREQUAL "CLANG_TIDY_PROGRAM-NOTFOUND")
  set(CLANG_TIDY_FOUND 0)
else()
  set(CLANG_TIDY_FOUND 1)
endif()

find_program(CLANG_FORMAT_PROGRAM
  NAMES clang-format 3.9 clang-format-3.8 clang-format-3.7 clang-format-3.6 clang-format
  PATHS $ENV{CLANG_TOOLS_PATH} /usr/local/bin /usr/bin
  NO_DEFAULT_PATH)

mark_as_advanced(CLANG_FORMAT_PROGRAM)

if("${CLANG_FORMAT_PROGRAM}" STREQUAL "CLANG_FORMAT_PROGRAM-NOTFOUND")
  set(CLANG_FORMAT_FOUND 0)
else()
  set(CLANG_FORMAT_FOUND 1)
endif()
