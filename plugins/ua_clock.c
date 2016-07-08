/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include "ua_types.h"

#include <time.h>
#ifdef _WIN32
# ifdef SLIST_ENTRY
#  undef SLIST_ENTRY /* Fix redefinition of SLIST_ENTRY on mingw winnt.h */
# endif
# include <windows.h>
#else
# include <sys/time.h>
#endif

#if defined(__APPLE__) || defined(__MACH__)
# include <mach/clock.h>
# include <mach/mach.h>
#endif

UA_DateTime UA_DateTime_now(void) {
#if defined(_WIN32)
    /* Windows filetime has the same definition as UA_DateTime */
    FILETIME ft;
    SYSTEMTIME st;
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ft);
    ULARGE_INTEGER ul;
    ul.LowPart = ft.dwLowDateTime;
    ul.HighPart = ft.dwHighDateTime;
    return (UA_DateTime)ul.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * UA_SEC_TO_DATETIME) + (tv.tv_usec * UA_USEC_TO_DATETIME) + UA_DATETIME_UNIX_EPOCH;
#endif
}

UA_DateTime UA_DateTime_nowMonotonic(void) {
#if defined(_WIN32)
    LARGE_INTEGER freq, ticks;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&ticks);
    UA_Double ticks2dt = UA_SEC_TO_DATETIME / (UA_Double)freq.QuadPart;
    return (UA_DateTime)(ticks.QuadPart * ticks2dt);
#elif defined(__APPLE__) || defined(__MACH__)
    /* OS X does not have clock_gettime, use clock_get_time */
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    return (mts.tv_sec * UA_SEC_TO_DATETIME) + (mts.tv_nsec / 100);
#elif !defined(CLOCK_MONOTONIC_RAW)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * UA_SEC_TO_DATETIME) + (ts.tv_nsec / 100);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (ts.tv_sec * UA_SEC_TO_DATETIME) + (ts.tv_nsec / 100);
#endif
}
