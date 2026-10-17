#!/bin/bash -eu

# Build the PR checkout copied into the image, using the OSS-Fuzz toolchain.
cmake -S "$SRC/open62541" -B "$WORK/open62541" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_SHARED_LIBS=OFF \
    -DUA_ENABLE_AMALGAMATION=OFF \
    -DUA_BUILD_EXAMPLES=OFF \
    -DUA_LOGLEVEL=600 \
    -DUA_ENABLE_ENCRYPTION=OPENSSL \
    -DOPENSSL_USE_STATIC_LIBS=TRUE \
    -DUA_BUILD_OSS_FUZZ=ON
cmake --build "$WORK/open62541" --parallel "$(nproc)"

bash "$SRC/open62541/tests/fuzz/oss-fuzz-copy.sh"
if [[ -f "$SRC/open62541/deps/mdnsd/tests/fuzz/oss-fuzz-copy.sh" ]]; then
    SRC="$SRC/open62541/deps" bash \
        "$SRC/open62541/deps/mdnsd/tests/fuzz/oss-fuzz-copy.sh"
fi
