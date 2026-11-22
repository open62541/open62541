cmake_minimum_required(VERSION 3.20.0)
list(APPEND EXTRA_ZEPHYR_MODULES "${CMAKE_CURRENT_SOURCE_DIR}/../../../arch")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(eventloop_wakeup)
# open62541 also supports Zephyr releases with the legacy <version.h> path.
zephyr_include_directories("${PROJECT_BINARY_DIR}/zephyr/include/generated/zephyr")
target_sources(app PRIVATE main.c)
target_link_libraries(app PRIVATE open62541)
