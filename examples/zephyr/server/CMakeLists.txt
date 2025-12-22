cmake_minimum_required(VERSION 3.20.0)

project(server)

# set(CMAKE_OBJCOPY /usr/bin/arm-linux-gnueabihf-objcopy)
set(Python3_EXECUTABLE "/usr/bin/python3" CACHE STRING "" FORCE)
set(EXTRA_ZEPHYR_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

target_sources(app PRIVATE src/main.c)
target_link_libraries(app PUBLIC open62541)
