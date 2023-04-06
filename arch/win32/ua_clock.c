/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2016-2017 (c) Julius Pfrommer, Fraunhofer IOSB
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Thomas Stalder
 */

#ifdef UA_ARCHITECTURE_WIN32

#ifndef _BSD_SOURCE
# define _BSD_SOURCE
#endif

#include <open62541/types.h>

#include <time.h>
/* Backup definition of SLIST_ENTRY on mingw winnt.h */
# ifdef SLIST_ENTRY
#  pragma push_macro("SLIST_ENTRY")
#  undef SLIST_ENTRY
#  define POP_SLIST_ENTRY
# endif
# include <windows.h>
/* restore definition */
# ifdef POP_SLIST_ENTRY
#  undef SLIST_ENTRY
#  undef POP_SLIST_ENTRY
#  pragma pop_macro("SLIST_ENTRY")
# endif

UA_DateTime UA_DateTime_now(void) {
    /* Windows filetime has the same definition as UA_DateTime */
    FILETIME ft;
    SYSTEMTIME st;
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ft);
    ULARGE_INTEGER ul;
    ul.LowPart = ft.dwLowDateTime;
    ul.HighPart = ft.dwHighDateTime;
    return (UA_DateTime)ul.QuadPart;
}

/* Credit to https://stackoverflow.com/questions/13804095/get-the-time-zone-gmt-offset-in-c */
UA_Int64 UA_DateTime_localTimeUtcOffset(void) {
    time_t gmt, rawtime = time(NULL);

    struct tm ptm;
#ifdef __CODEGEARC__
    gmtime_s(&rawtime, &ptm);
#else
    gmtime_s(&ptm, &rawtime);
#endif
    // Request that mktime() looksup dst in timezone database
    ptm.tm_isdst = -1;
    gmt = mktime(&ptm);

    return (UA_Int64) (difftime(rawtime, gmt) * UA_DATETIME_SEC);
}

UA_DateTime UA_DateTime_nowMonotonic(void) {
    LARGE_INTEGER freq, ticks;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&ticks);
    UA_Double ticks2dt = UA_DATETIME_SEC / (UA_Double)freq.QuadPart;
    return (UA_DateTime)(ticks.QuadPart * ticks2dt);
}

#endif /* UA_ARCHITECTURE_WIN32 */
