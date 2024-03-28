# open62541 Architecture Support

This folder contains all the architecture-specific code for different operating
systems. The arch-specific functionality covers:

- System clock
- EventLoop (Networking, Timed events, Interrupt handling (optional))

Currently open62541 supports the architectures

- POSIX (Linux, BSD, Mac, etc.)
- Windows

Previously (until v1.3) there was additional support for:

- freeRTOS
- vxWorks
- WEC7
- eCos

We strive to recover support for these going forward and also include their
coverage to the CI.

## Adding new architectures

To port open62541 to a new architecture, follow these steps:

1. Create folder for the target architecture. For example, can be in sub-folder ``arch/my-arch`` of the open62541 repository root folder. Or might be located outside of the open62541 source code tree, for example ``d:/opcua-test/linux``.
2. Copy POSIX-specific source code files from folder ``arch/eventloop_posix`` to this folder to use as a base. Note that file names with architecture-specific code start by ``eventloop_arch_``.
3. Modify files ``eventloop_arch_*.*`` accordingly to support custom target architecture.
   For example, file ``eventloop_arch_clock.c`` implements the following functions defined in open62541/types.h:

   - UA_DateTime UA_DateTime_now(void);
   
   - UA_Int64 UA_DateTime_localTimeUtcOffset(void);
   
   - UA_DateTime UA_DateTime_nowMonotonic(void);

   Adjust these function implementations in this and other files to your target architecture.
4. Note that it is not necessary to port all networking subsystems (and interrupts). The library will detect when a certain protocol (e.g. UDP,
   ETH) is not available. TCP support is however mandatory.

You have to link in your custom clock and EventLoop implementation only for the final binary. To prevent the parallel build of Win32/POSIX versions, set ``UA_ARCHITECTURE=none`` in the cmake setting. To use CMake for the entire build, including a new architecture, do the following:

- Add your architecture to the list in ``/include/open62541/config.h.in``

- Add the architecture to ``/CMakeLists.txt``. If file names in the new architecture folder are the same as in the original ``eventloop_posix`` folder, there will be no need to change list of files in the ``/CMakeLists.txt file``. Name of the folder with custom architecture-specific code should be passed when the project is built with cmake by the build option ``UA_ARCH_FOLDER``, for example:

```
mkdir build-wsl2
cd build-wsl2
cmake -UA_ARCH_FOLDER=/mnt/d/opcua-test/linux ../
```

