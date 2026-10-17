#!/usr/bin/env bash
set -euo pipefail

build_dir="${1:?build directory required}"
seconds="${2:-20}"
mkdir -p "$build_dir/corpus" "$build_dir/artifacts"

shopt -s nullglob
fuzzers=("$build_dir"/bin/fuzz_*)
if ((${#fuzzers[@]} == 0)); then
    echo "No fuzzers were built" >&2
    exit 1
fi

for fuzzer in "${fuzzers[@]}"; do
    name="${fuzzer##*/}"
    corpus="$build_dir/corpus/$name"
    mkdir -p "$corpus"
    if [[ "$name" == fuzz_binary_message || "$name" == fuzz_tcp_message ]]; then
        cp tests/fuzz/fuzz_binary_message_corpus/generated/* "$corpus/"
    fi
    echo "Running $name"
    "$fuzzer" "$corpus" \
        -max_total_time="$seconds" \
        -timeout=25 \
        -rss_limit_mb=2560 \
        -artifact_prefix="$build_dir/artifacts/$name-"
done
