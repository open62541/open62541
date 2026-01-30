# Architecture Support

The open62541 core library (in the /src folder) is operating system-independent.
The individual integration lives in the /arch folder and in specific plugin
implementations in /plugins (e.g. for filesystem access of cryptographic PKI
functionality).

open62541 supports all 32/64bit CPU instruction sets. Instruction sets that are
known to be little-endian and/or with IEEE 754 floating point representations
can use speed-ups for the encoding/decoding of network messages. The generic
implementation is slower, but works on all instruction sets.

## Operating Systems

open62541 supports many operating systems (also embedded). See the table below
for an overview. The operating system support is classified into three tiers:

- Tier 1 operating systems have the best support. All code contributions need to
  pass our continuous integration (CI) infrastructure for these operating
  systems. For Tier 1 support, the open62541 maintainers (help to) fix bugs in
  the current "stable" and "oldstable" release families.

- Tier 2 operating systems are "known to work", but there is no coverage in the
  continuous integration (CI) infrastructure. The open62541 maintainers have to
  manually test against these operating systems. This leads to intermittent
  breakage and to more undetected bugs in released versions. For tier 2 the
  maintainers fix known bugs in the current "stable" release family. But they
  cannot go bug hunting in older release families.

- Tier 3 operating systems have no no automated continuous integration (CI)
  environment and the open62541 maintainers have no permanent access for manual
  testing. This can lead to releases of open62541 with undetected issues for
  those OS. There is limited support / code review by the open62541 maintainers
  to integrate contributions into the master branch.

| Operating System | Support Tier |
|------------------|--------------|
| **Tier 1**       |              |
| Linux            | Tier 1       |
| Windows          | Tier 1       |
| MacOS            | Tier 1       |
| Zephyr           | Tier 1       |
| FreeRTOS         | Tier 1       |
| **Tier 2**       |              |
| BSD              | Tier 2       |
| **Tier 3**       |              |
| QNX              | Tier 3       |
| vxWorks          | Tier 3       |

The **o6 Automation GmbH** (https://www.o6-automation.com) is the legal entity
launched by the maintainers of open62541. Contact o6 if you need **commercial
support for open62541**. This includes porting or integration of open62541 for
your specific needs. The commercial support also allows for **long-term
support** for old release families (besides stable/oldstable). This enables
commercial products developed with a specific version of open62541.

## Adding New Operating Systems

The OS-specific functionality covers access to the system clock and the
EventLoop implementation (Networking, Timed events, Interrupt handling
(optional)).

To port open62541 to a new architecture, follow these steps:

1. Modify the clock.c file that implements the following functions defined in
   open62541/types.h:

   - UA_DateTime UA_DateTime_now(void);
   - UA_Int64 UA_DateTime_localTimeUtcOffset(void);
   - UA_DateTime UA_DateTime_nowMonotonic(void);

2. Fork the EventLoop code (the default is POSIX) and adjust it to your
   architecture. Note that it is not necessary to port all networking subsystems
   (and interrupts). The library will detect when a certain protocol (e.g. UDP,
   ETH) is not available. TCP support is however mandatory.

3. The code for the initial configuration of client/server applications lives in
   /plugins/ua_config_default.c. Fork/update this code to instantiate your
   EventLoop implementation in the client/server configuration.

To prevent the parallel build of Win32/POSIX versions, set UA_ARCHITECTURE=none
in the cmake setting and link the final binary against your new code files.
Otherwise modify /include/open62541/config.h.in and /CMakelists.txt to add
permanent support for your operating system.
