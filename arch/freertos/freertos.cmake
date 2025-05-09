# Select the heap port
set( FREERTOS_HEAP "3" CACHE STRING "" FORCE)

# Select the native compile PORT
set( FREERTOS_PORT "GCC_POSIX" CACHE STRING "" FORCE)

# Create the freertos_config library
add_library(freertos_config INTERFACE)
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/AbortDelay.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/BlockQ.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/blocktim.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/countsem.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/death.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/dynamic.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/EventGroupsDemo.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/flop.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/GenQTest.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/integer.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/IntSemTest.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/MessageBufferAMP.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/MessageBufferDemo.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/PollQ.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/QPeek.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/QueueOverwrite.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/QueueSet.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/QueueSetPolling.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/recmutex.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/semtest.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/StaticAllocation.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/StreamBufferDemo.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/StreamBufferInterrupt.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/TaskNotify.c
            # ${FREERTOS_PORT_PATH}/../Common/Minimal/TimerDemo.c)

target_include_directories(freertos_config INTERFACE ${FREERTOS_PORT_PATH}
                                           "${FREERTOS_KERNEL_PATH}/include")
target_compile_definitions(freertos_config INTERFACE projCOVERAGE_TEST=0)
target_compile_definitions(freertos_config INTERFACE projENABLE_TRACING=0)

# Adding the FreeRTOS-Kernel subdirectory
add_subdirectory(${FREERTOS_KERNEL_PATH} ${CMAKE_CURRENT_BINARY_DIR}/FreeRTOS-Kernel )
