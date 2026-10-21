/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef TESTING_DESCRIPTORS_H_
#define TESTING_DESCRIPTORS_H_

#include <open62541/config.h>

#ifdef UA_ARCHITECTURE_POSIX

#include <fcntl.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <unistd.h>

/* Open descriptors until all numbers below FD_SETSIZE are in use. The next
 * socket gets a descriptor that select() cannot handle. Returns false if the
 * process is not allowed to open that many descriptors. */

#define TESTING_MAX_FILLERS (FD_SETSIZE + 8)
static int testingFillers[TESTING_MAX_FILLERS];
static size_t testingFillersSize = 0;

static UA_INLINE UA_Boolean
testing_fillDescriptors(void) {
    struct rlimit rl;
    if(getrlimit(RLIMIT_NOFILE, &rl) != 0)
        return false;
    rlim_t needed = FD_SETSIZE + 64;
    if(rl.rlim_cur < needed) {
        if(rl.rlim_max < needed)
            return false;
        rl.rlim_cur = needed;
        if(setrlimit(RLIMIT_NOFILE, &rl) != 0)
            return false;
    }

    while(testingFillersSize < TESTING_MAX_FILLERS) {
        int fd = open("/dev/null", O_RDONLY);
        if(fd < 0)
            break;
        testingFillers[testingFillersSize++] = fd;
        if(fd >= FD_SETSIZE)
            return true;
    }
    return false;
}

static UA_INLINE void
testing_releaseDescriptors(void) {
    for(size_t i = 0; i < testingFillersSize; i++)
        close(testingFillers[i]);
    testingFillersSize = 0;
}

#endif /* UA_ARCHITECTURE_POSIX */

#endif /* TESTING_DESCRIPTORS_H_ */
