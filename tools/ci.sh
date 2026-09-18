#!/usr/bin/env bash

# Reusable CI functions. Source this file before invoking a function.

set -e
set -o pipefail
set -o nounset

CI_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CI_REPOSITORY_ROOT="$(cd "${CI_SCRIPT_DIR}/.." && pwd)"

function clang_tidy_all {
    local build_dir="${CLANG_TIDY_BUILD_DIR:-${CI_REPOSITORY_ROOT}/build-clang-tidy}"
    local c_compiler="${CLANG_TIDY_C_COMPILER:-clang-22}"
    local clang_tidy="${CLANG_TIDY_BINARY:-clang-tidy-22}"
    local run_clang_tidy="${RUN_CLANG_TIDY_BINARY:-run-clang-tidy-22}"
    local jobs="${CLANG_TIDY_JOBS:-2}"

    (cd "${CI_REPOSITORY_ROOT}" && "${clang_tidy}" --verify-config)
    cmake -S "${CI_REPOSITORY_ROOT}" -B "${build_dir}" \
        -DCMAKE_C_COMPILER="${c_compiler}" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DUA_BUILD_EXAMPLES=OFF \
        -DUA_BUILD_UNIT_TESTS=OFF
    cmake --build "${build_dir}" \
        --target open62541-code-generation \
        --parallel "${jobs}"

    "${run_clang_tidy}" \
        -p "${build_dir}" \
        -clang-tidy-binary "${clang_tidy}" \
        -j "${jobs}" \
        '(^|/)(arch|deps|plugins|src)/.*\.c$'
}
