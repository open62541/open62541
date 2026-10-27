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
    if [[ "$name" == fuzz_binary_message ]]; then
        cp tests/fuzz/fuzz_binary_message_corpus/generated/* "$corpus/"
    elif [[ "$name" == fuzz_tcp_message ]]; then
        # The binary-message fuzzer processes the full generated corpus.
        # Sample it evenly for TCP, where each seed starts a server.
        index=0
        for seed in tests/fuzz/fuzz_binary_message_corpus/generated/*; do
            if ((index % 16 == 0)); then
                cp "$seed" "$corpus/"
            fi
            index=$((index + 1))
        done
    fi
    echo "Running $name"
    "$fuzzer" "$corpus" \
        -max_total_time="$seconds" \
        -timeout=25 \
        -rss_limit_mb=2560 \
        -artifact_prefix="$build_dir/artifacts/$name-"
done
