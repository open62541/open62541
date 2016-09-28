# -- How to cross compile for Raspberry Pi (on a 64bit host) -- 
# 1) get the toolchain
# cd ~
# git clone https://github.com/raspberrypi/tools
# 2) export path to one of the compilers
# export PATH=$PATH:~/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/
# 3) use this toolchain file 
# cmake -DCMAKE_TOOLCHAIN_FILE=../tools/cmake/Toolchain-rpi64.cmake -DEXAMPLESERVER=ON ..
# make
set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)
set(CMAKE_STRIP arm-linux-gnueabihf-strip)
