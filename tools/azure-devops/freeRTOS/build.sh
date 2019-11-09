#!/usr/bin/env bash
set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
BASE_DIR="$( realpath "$DIR/../../../" )"

echo "== freeRTOS Build =="

# Add xtensa compiler to path
export PATH=$PATH:$IDF_PATH/xtensa-esp32-elf/bin

mkdir -p build_freertos && cd build_freertos

CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Debug \
    -DUA_ENABLE_AMALGAMATION=OFF \
    -DCMAKE_TOOLCHAIN_FILE=${IDF_PATH}/tools/cmake/toolchain-esp32.cmake \
    -DUA_ARCHITECTURE=freertosLWIP \
    -DUA_ARCH_EXTRA_INCLUDES=${IDF_PATH}/components/freertos/include/freertos \
    -DUA_BUILD_EXAMPLES=OFF"


# We first need to call cmake separately to generate the required source code. Then we can call the freeRTOS CMake
mkdir lib && cd lib
cmake \
    ${CMAKE_ARGS} \
    ${BASE_DIR}
if [ $? -ne 0 ] ; then exit 1 ; fi

make -j open62541-code-generation
if [ $? -ne 0 ] ; then exit 1 ; fi

# Now call the freeRTOS CMake with same arguments
cd ..

cmake \
    ${CMAKE_ARGS} \
    ${BASE_DIR}/tools/azure-devops/freeRTOS/
if [ $? -ne 0 ] ; then exit 1 ; fi

# NOTE!!!
# If you came here to see how to build your own version for the ESP32,
# make sure to call `make menuconfig` first and configure the options.

make hello-world.elf
if [ $? -ne 0 ] ; then exit 1 ; fi
