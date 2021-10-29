# -- How to cross compile for ARM -- 
# 1) Install the arm compiler
# sudo apt-get install gcc-arm-linux-gnueabihf
# 2) use this toolchain file 
# cmake -DCMAKE_TOOLCHAIN_FILE=../tools/cmake/Toolchain-ARM.cmake -DUA_BUILD_EXAMPLES=ON -DUA_ENABLE_PUBSUB=ON ..
# make
set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_C_COMPILER_AR arm-linux-gnueabihf-gcc-ar)
set(CMAKE_C_COMPILER_RANLIB arm-linux-gnueabihf-gcc-ranlib)
set(CMAKE_C_FLAGS -Wno-error)
set(CMAKE_C_FLAGS_DEBUG -g -Wno-error)
set(CMAKE_C_LINKER arm-linux-gnueabihf-ld)
set(CMAKE_NM arm-linux-gnueabihf-nm)
set(CMAKE_OBJCOPY arm-linux-gnueabihf-objcopy)
set(CMAKE_OBJDUMP arm-linux-gnueabihf-objdump)
set(CMAKE_RANLIB arm-linux-gnueabihf-ranlib)
set(CMAKE_STRIP arm-linux-gnueabihf-strip)
