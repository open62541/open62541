set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-m33)

set(CMAKE_C_COMPILER "C:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10/bin/arm-none-eabi-gcc.exe")

set(CMAKE_ASM_COMPILER "C:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10/bin/arm-none-eabi-gcc.exe")

set(CMAKE_C_COMPILER_AR "C:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10/bin/arm-none-eabi-gcc-ar.exe")
set(CMAKE_C_COMPILER_RANLIB "C:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10/bin/arm-none-eabi-gcc-ranlib.exe")
set(CMAKE_C_FLAGS -Wno-error)
set(CMAKE_C_FLAGS_DEBUG -g -Wno-error)
set(CMAKE_C_LINKER "C:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10/bin/arm-none-eabi-ld.exe")
set(CMAKE_NM "C:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10/bin/arm-none-eabi-nm.exe")
set(CMAKE_OBJCOPY "C:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10/bin/arm-none-eabi-objcopy.exe")
set(CMAKE_OBJDUMP "C:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10/bin/arm-none-eabi-objdump.exe")
set(CMAKE_RANLIB "C:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10/bin/arm-none-eabi-ranlib.exe")
set(CMAKE_STRIP "C:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10/bin/arm-none-eabi-strip.exe")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(FREERTOS_HEAP "4" CACHE STRING "" FORCE)
set(FREERTOS_PORT "GCC_ARM_CM33_NTZ_NONSECURE" CACHE STRING "" FORCE)

set(CMAKE_EXE_LINKER_FLAGS "-mcpu=cortex-m33 -mthumb -nostdlib")
set(CMAKE_ASM_FLAGS "-mcpu=cortex-m33 -mthumb")
set(CMAKE_C_FLAGS "-mcpu=cortex-m33 -mthumb -mfloat-abi=softfp -mfpu=fpv5-sp-d16 -std=c99 -Wall -Wextra -lc -specs=nosys.specs -ffunction-sections -fdata-sections -g -fno-common -fmessage-length=0")
