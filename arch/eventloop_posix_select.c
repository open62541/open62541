/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "eventloop_posix.h"

#if !defined(UA_HAVE_EPOLL)

UA_StatusCode
UA_EventLoopPOSIX_registerFD(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd) {
    UA_LOCK(&el->elMutex);

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Registering fd: %u", (unsigned)rfd->fd);

    /* Realloc */
    UA_RegisteredFD **fds_tmp = (UA_RegisteredFD**)
        UA_realloc(el->fds, sizeof(UA_RegisteredFD*) * (el->fdsSize + 1));
    if(!fds_tmp) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    el->fds = fds_tmp;

    /* Add to the last entry */
    el->fds[el->fdsSize] = rfd;
    el->fdsSize++;

    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_EventLoopPOSIX_modifyFD(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd) {
    /* Do nothing, it is enough if the data was changed in the rfd */
    return UA_STATUSCODE_GOOD;
}

void
UA_EventLoopPOSIX_deregisterFD(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd) {
    UA_LOCK(&el->elMutex);

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Unregistering fd: %u", (unsigned)rfd->fd);

    /* Find the entry */
    size_t i = 0;
    for(; i < el->fdsSize; i++) {
        if(el->fds[i] == rfd)
            break;
    }

    /* Not found? */
    if(i == el->fdsSize) {
        UA_UNLOCK(&el->elMutex);
        return;
    }

    if(el->fdsSize > 1) {
        /* Move the last entry in the ith slot and realloc. */
        el->fdsSize--;
        el->fds[i] = el->fds[el->fdsSize];
        UA_RegisteredFD **fds_tmp = (UA_RegisteredFD**)
            UA_realloc(el->fds, sizeof(UA_RegisteredFD*) * el->fdsSize);
        /* if realloc fails the fds are still in a correct state with
         * possibly lost memory, so failing silently here is ok */
        if(fds_tmp)
            el->fds = fds_tmp;
    } else {
        /* Remove the last entry */
        UA_free(el->fds);
        el->fds = NULL;
        el->fdsSize = 0;
    }

    UA_UNLOCK(&el->elMutex);
}

static UA_FD
setFDSets(UA_EventLoopPOSIX *el, fd_set *readset, fd_set *writeset, fd_set *errset) {
    FD_ZERO(readset);
    FD_ZERO(writeset);
    FD_ZERO(errset);
    UA_FD highestfd = UA_INVALID_FD;
    for(size_t i = 0; i < el->fdsSize; i++) {

        UA_FD currentFD = el->fds[i]->fd;
        /* Add to the fd_sets */
        if(el->fds[i]->listenEvents & UA_FDEVENT_IN)
            UA_fd_set(currentFD, readset);
        if(el->fds[i]->listenEvents & UA_FDEVENT_OUT)
            UA_fd_set(currentFD, writeset);

        /* Always return errors */
        UA_fd_set(currentFD, errset);

        /* Highest fd? */
        if(currentFD > highestfd || highestfd == UA_INVALID_FD)
            highestfd = currentFD;
    }
    return highestfd;
}

UA_StatusCode
UA_EventLoopPOSIX_pollFDs(UA_EventLoopPOSIX *el, UA_DateTime listenTimeout) {
    UA_assert(listenTimeout >= 0);

    fd_set readset, writeset, errset;
    UA_FD highestfd = setFDSets(el, &readset, &writeset, &errset);

    /* Nothing to do? */
    if(highestfd == UA_INVALID_FD) {
        UA_LOG_TRACE(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                     "No valid FDs for processing");
        return UA_STATUSCODE_GOOD;
    }

    struct timeval tmptv = {
#ifndef _WIN32
        (time_t)(listenTimeout / UA_DATETIME_SEC),
        (suseconds_t)((listenTimeout % UA_DATETIME_SEC) / UA_DATETIME_USEC)
#else
        (long)(listenTimeout / UA_DATETIME_SEC),
        (long)((listenTimeout % UA_DATETIME_SEC) / UA_DATETIME_USEC)
#endif
    };

    int selectStatus = UA_select(highestfd+1, &readset, &writeset, &errset, &tmptv);
    if(selectStatus < 0) {
        /* We will retry, only log the error */
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                           "Error during select: %s", errno_str));
        return UA_STATUSCODE_GOOD;
    }

    /* Loop over all registered FD to see if an event arrived. Yes, this is why
      * select is slow for many open sockets. */
    for(size_t i = 0; i < el->fdsSize; i++) {
        UA_RegisteredFD *rfd = el->fds[i];
        UA_FD fd = rfd->fd;

        /* Error Event */
        short event = 0;
        if(UA_fd_isset(fd, &readset)) {
            event = UA_FDEVENT_IN;
        } else if(UA_fd_isset(fd, &writeset)) {
            event = UA_FDEVENT_OUT;
        } else if(UA_fd_isset(fd, &errset)) {
            event = UA_FDEVENT_ERR;
        } else {
            continue;
        }

        UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                     "Processing event %u on fd %u", (unsigned)event, (unsigned)fd);

        UA_UNLOCK(&el->elMutex);
        rfd->callback(rfd->es, rfd, event);
        UA_LOCK(&el->elMutex);

        /* The fd has removed itself */
        if(i == el->fdsSize || rfd != el->fds[i])
            i--;
    }
    return UA_STATUSCODE_GOOD;
}

#endif /* !defined(UA_HAVE_EPOLL) */
