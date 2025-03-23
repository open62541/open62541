/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "eventloop_posix.h"

#if defined(UA_HAVE_EPOLL)

UA_StatusCode
UA_EventLoopPOSIX_registerFD(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd) {
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));
    event.data.ptr = rfd;
    event.events = 0;
    if(rfd->listenEvents & UA_FDEVENT_IN)
        event.events |= EPOLLIN;
    if(rfd->listenEvents & UA_FDEVENT_OUT)
        event.events |= EPOLLOUT;

    int err = epoll_ctl(el->epollfd, EPOLL_CTL_ADD, rfd->fd, &event);
    if(err != 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "TCP %u\t| Could not register for epoll (%s)",
                          rfd->fd, errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_EventLoopPOSIX_modifyFD(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd) {
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));
    event.data.ptr = rfd;
    event.events = 0;
    if(rfd->listenEvents & UA_FDEVENT_IN)
        event.events |= EPOLLIN;
    if(rfd->listenEvents & UA_FDEVENT_OUT)
        event.events |= EPOLLOUT;

    int err = epoll_ctl(el->epollfd, EPOLL_CTL_MOD, rfd->fd, &event);
    if(err != 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "TCP %u\t| Could not modify for epoll (%s)",
                          rfd->fd, errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

void
UA_EventLoopPOSIX_deregisterFD(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd) {
    int res = epoll_ctl(el->epollfd, EPOLL_CTL_DEL, rfd->fd, NULL);
    if(res != 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "TCP %u\t| Could not deregister from epoll (%s)",
                          rfd->fd, errno_str));
    }
}

UA_StatusCode
UA_EventLoopPOSIX_pollFDs(UA_EventLoopPOSIX *el, UA_DateTime listenTimeout) {
    UA_assert(listenTimeout >= 0);

    /* If there is a positive timeout, wait at least one millisecond, the
     * minimum for blocking epoll_wait. This prevents a busy-loop, as the
     * open62541 library allows even smaller timeouts, which can result in a
     * zero timeout due to rounding to an integer here. */
    int timeout = (int)(listenTimeout / UA_DATETIME_MSEC);
    if(timeout == 0 && listenTimeout > 0)
        timeout = 1;

    /* Poll the registered sockets */
    struct epoll_event epoll_events[64];
    UA_UNLOCK(&el->elMutex);
    int events = epoll_wait(el->epollfd, epoll_events, 64, timeout);
    UA_LOCK(&el->elMutex);

    /* TODO: Replace with pwait2 for higher-precision timeouts once this is
     * available in the standard library.
     *
     * struct timespec precisionTimeout = {
     *  (long)(listenTimeout / UA_DATETIME_SEC),
     *   (long)((listenTimeout % UA_DATETIME_SEC) * 100)
     * };
     * int events = epoll_pwait2(epollfd, epoll_events, 64,
     *                        precisionTimeout, NULL); */

    /* Handle error conditions */
    if(events == -1) {
        if(errno == EINTR) {
            /* We will retry, only log the error */
            UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                           "Timeout during poll");
            return UA_STATUSCODE_GOOD;
        }
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "TCP\t| Error %s, closing the server socket",
                          errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Process all received events */
    for(int i = 0; i < events; i++) {
        UA_RegisteredFD *rfd = (UA_RegisteredFD*)epoll_events[i].data.ptr;

        /* The rfd is already registered for removal. Don't process incoming
         * events any longer. */
        if(rfd->dc.callback)
            continue;

        /* Get the event */
        short revent = 0;
        if((epoll_events[i].events & EPOLLIN) == EPOLLIN) {
            revent = UA_FDEVENT_IN;
        } else if((epoll_events[i].events & EPOLLOUT) == EPOLLOUT) {
            revent = UA_FDEVENT_OUT;
        } else {
            revent = UA_FDEVENT_ERR;
        }

        /* Call the EventSource callback */
        rfd->eventSourceCB(rfd->es, rfd, revent);
    }
    return UA_STATUSCODE_GOOD;
}

#endif /* defined(UA_HAVE_EPOLL) */
