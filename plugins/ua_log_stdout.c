/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2016-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Thomas Stalder, Blue Time Concept SA
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/types.h>

#include <stdio.h>

#ifdef _WIN32
# include <windows.h>
# include <VersionHelpers.h>
#endif

#include "mp_printf.h"

/* Run functions before the user calls main()
 * https://stackoverflow.com/questions/1113409/attribute-constructor-equivalent-in-vc */

#ifdef __cplusplus
# define UA_RUN_BEFORE_MAIN(f) \
    namespace { \
        struct f##_t { f##_t() { f(); } }; \
        static f##_t f##_inst; \
    }
#elif defined(_MSC_VER)
# pragma section(".CRT$XCU", read)
# define UA_RUN_BEFORE_MAIN_2_(f,prefix) \
    __declspec(allocate(".CRT$XCU")) void (*f##_ptr)(void) = f; \
    __pragma(comment(linker, "/include:" prefix #f "_ptr"))
# ifdef _WIN64
#  define UA_RUN_BEFORE_MAIN(f) UA_RUN_BEFORE_MAIN_2_(f, "")
# else
#  define UA_RUN_BEFORE_MAIN(f) UA_RUN_BEFORE_MAIN_2_(f, "_")
# endif
#else
# define UA_RUN_BEFORE_MAIN(f) \
    static void f##_wrapper(void) __attribute__((constructor)); \
    static void f##_wrapper(void) { f(); }
#endif

/* ANSI escape sequences for color output taken from here:
 * https://stackoverflow.com/questions/3219393/stdlib-and-colored-output-in-c*/

static const char *ansiColorRed     = "";
static const char *ansiColorGreen   = "";
static const char *ansiColorYellow  = "";
static const char *ansiColorMagenta = "";
static const char *ansiColorReset   = "";

static void
enableAnsiColorSequences(void) {
    ansiColorRed     = "\x1b[31m";
    ansiColorGreen   = "\x1b[32m";
    ansiColorYellow  = "\x1b[33m";
    ansiColorMagenta = "\x1b[35m";
    ansiColorReset   = "\x1b[0m";
}

#if defined(UA_ARCHITECTURE_POSIX) || defined(UA_FORCE_ANSI_COLORS)
static void
initializeTerminalColors(void) {
    enableAnsiColorSequences();
}
UA_RUN_BEFORE_MAIN(initializeTerminalColors)
#elif defined(_WIN32)
static bool
tryEnableVirtualTerminalProcessing(HANDLE consoleHandle) {
    DWORD mode = 0;
    if(consoleHandle == INVALID_HANDLE_VALUE || consoleHandle == NULL)
        return false;
    if(!GetConsoleMode(consoleHandle, &mode))
        return false;
    if(mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING)
        return true;
    if(!SetConsoleMode(consoleHandle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
        return false;
    return true;
}

static void
initializeTerminalColors(void) {
    if(!IsWindows10OrGreater())
        return;

    if(tryEnableVirtualTerminalProcessing(GetStdHandle(STD_OUTPUT_HANDLE)) ||
       tryEnableVirtualTerminalProcessing(GetStdHandle(STD_ERROR_HANDLE))) {
        enableAnsiColorSequences();
    }
}
UA_RUN_BEFORE_MAIN(initializeTerminalColors)
#endif

static const char *
getLogLevelColor(int slot) {
    switch(slot) {
    case 0: return "";
    case 1: return "";
    case 2: return ansiColorGreen;
    case 3: return ansiColorYellow;
    case 4: return ansiColorRed;
    default: return ansiColorMagenta;
    }
}

static const char *
getLogLevelName(int slot) {
    switch(slot) {
    case 0: return "trace";
    case 1: return "debug";
    case 2: return "info";
    case 3: return "warn";
    case 4: return "error";
    default: return "fatal";
    }
}

static const char *
logCategoryNames[UA_LOGCATEGORIES] =
    {"network", "channel", "session", "server", "client",
     "application", "security", "eventloop", "pubsub", "discovery"};

/* Protect crosstalk during logging via global lock. Use a spinlock as we cannot
 * statically initialize a global lock across all platforms. */
#if UA_MULTITHREADING >= 100
static UA_atomic(void *) logSpinLock = NULL;
static UA_INLINE void spinLock(void) {
    void *expected;
    do {
        expected = NULL;
        UA_atomic_cmpxchg(&logSpinLock, &expected, (void*)0x1);
    } while(expected != NULL);
}
static UA_INLINE void spinUnLock(void) {
    UA_atomic_store(&logSpinLock, NULL);
}
#endif

#ifdef __clang__
__attribute__((__format__(__printf__, 4 , 0)))
#endif
static void
UA_Log_Stdout_log(void *context, UA_LogLevel level, UA_LogCategory category,
                  const char *msg, va_list args) {
    /* MinLevel encoded in the context pointer */
    UA_LogLevel minLevel = (UA_LogLevel)(uintptr_t)context;
    if(minLevel > level)
        return;

    UA_Int64 tOffset = UA_DateTime_localTimeUtcOffset();
    UA_DateTimeStruct dts = UA_DateTime_toStruct(UA_DateTime_now() + tOffset);

    int logLevelSlot = ((int)level / 100) - 1;
    if(logLevelSlot < 0 || logLevelSlot > 5)
        logLevelSlot = 5; /* Set to fatal if the level is outside the range */

    /* Lock */
#if UA_MULTITHREADING >= 100
    spinLock();
#endif

#define STDOUT_LOGBUFSIZE 512
    char logbuf[STDOUT_LOGBUFSIZE];

    /* Log */
    printf("[%04u-%02u-%02u %02u:%02u:%02u.%03u (UTC%+05d)] %s%s/%s%s\t",
           dts.year, dts.month, dts.day, dts.hour, dts.min, dts.sec, dts.milliSec,
           (int)(tOffset / UA_DATETIME_SEC / 36),
           getLogLevelColor(logLevelSlot),
           getLogLevelName(logLevelSlot),
           logCategoryNames[category],
           ansiColorReset);
    mp_vsnprintf(logbuf, STDOUT_LOGBUFSIZE, msg, args);
    printf("%s\n", logbuf);
    fflush(stdout);

    /* Unlock */
#if UA_MULTITHREADING >= 100
    spinUnLock();
#endif
}

static void
UA_Log_Stdout_clear(UA_Logger *logger) {
    UA_free(logger);
}

const UA_Logger UA_Log_Stdout_ = {UA_Log_Stdout_log, NULL, NULL};
const UA_Logger *UA_Log_Stdout = &UA_Log_Stdout_;

UA_Logger
UA_Log_Stdout_withLevel(UA_LogLevel minlevel) {
    UA_Logger logger =
        {UA_Log_Stdout_log, (void*)(uintptr_t)minlevel, NULL};
    return logger;
}

UA_Logger *
UA_Log_Stdout_new(UA_LogLevel minlevel) {
    UA_Logger *logger = (UA_Logger*)UA_malloc(sizeof(UA_Logger));
    if(!logger)
        return NULL;
    *logger = UA_Log_Stdout_withLevel(minlevel);
    logger->clear = UA_Log_Stdout_clear;
    return logger;
}
