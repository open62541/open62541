#!/usr/bin/env bash

# Reusable CI functions. Source this file before invoking a function.

set -e
set -o pipefail
set -o nounset

CI_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CI_REPOSITORY_ROOT="$(cd "${CI_SCRIPT_DIR}/.." && pwd)"

function clang_tidy_prepare {
    local build_dir="${CLANG_TIDY_BUILD_DIR:-${CI_REPOSITORY_ROOT}/build-clang-tidy}"
    local c_compiler="${CLANG_TIDY_C_COMPILER:-clang-18}"
    local clang_tidy="${CLANG_TIDY_BINARY:-clang-tidy-18}"
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
}

function clang_tidy_changed {
    local base_sha="${1:-}"
    local head_sha="${2:-HEAD}"
    local build_dir="${CLANG_TIDY_BUILD_DIR:-${CI_REPOSITORY_ROOT}/build-clang-tidy}"
    local clang_tidy="${CLANG_TIDY_BINARY:-clang-tidy-18}"
    local clang_tidy_diff="${CLANG_TIDY_DIFF_BINARY:-clang-tidy-diff-18.py}"
    local jobs="${CLANG_TIDY_JOBS:-2}"

    if [[ -z "${base_sha}" || "${base_sha}" =~ ^0+$ ]]; then
        base_sha="$(git -C "${CI_REPOSITORY_ROOT}" rev-parse "${head_sha}^")"
    fi

    (
        cd "${CI_REPOSITORY_ROOT}"
        git diff --unified=0 --no-color "${base_sha}" "${head_sha}" -- \
            arch deps include plugins src | \
            "${clang_tidy_diff}" \
                -p1 \
                -path "${build_dir}" \
                -clang-tidy-binary "${clang_tidy}" \
                -regex '^(arch|deps|include|plugins|src)/.*\.(c|h)$' \
                -j "${jobs}" \
                -extra-arg-before=-xc
    )
}
