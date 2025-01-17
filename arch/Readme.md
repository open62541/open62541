# open62541 Architecture Support

This folder contains all the architecture-specific code for different operating systems.
The arch-specific code covers:

- System clock
- EventLoop (Networking, Interrupt handling (optional))

Currently open62541 supports

- Windows
- POSIX (Linux, BSD, Mac, QNX, etc.)

Previously (until v1.3) there was additional support for:

- freeRTOS
- vxWorks
- WEC7
- eCos

We strive to recover support for these.
The main effort of porting is forking the EventLoop implementation.

## Adding new architectures

To port to a new architecture you should follow these steps:

1. Create a ua_clock.c file that implements the following functions defined in open62541/types.h:

   * UA_DateTime UA_DateTime_now(void);
   
   * UA_Int64 UA_DateTime_localTimeUtcOffset(void);
   
   * UA_DateTime UA_DateTime_nowMonotonic(void);

2. Fork the EventLoop code (the default is POSIX) and adjust to your architecture

3. Add your architecture to the list in /include/open62541/config.h.in

4. Add the architecture and the new files to /CMakeLists.txt

You can also use a custom architecture implementation outside of the library build.
For that, set UA_ARCHITECTURE=none in the cmake setting.
Then no architecture-specific code will be included in the open62541 library itself.
You have to link in your custom clock and EventLoop implementation only for the final binary.
