/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "logobject_internal.h"

#ifdef UA_ENABLE_DRIVER_LOGOBJECT

#include <string.h>

/* OPC UA Part 26 LogObjects as a driver. The driver uses only the public API of
 * the server. Its state is protected by the EventLoop lock, which the server
 * lock takes first. */

static UA_StatusCode LogObjectDriver_start(UA_Driver *drv);

UA_Boolean
isLogObjectDriver(const UA_Driver *driver) {
    return driver && driver->start == LogObjectDriver_start;
}

/*************/
/* Lifecycle */
/*************/

static UA_StatusCode
LogObjectDriver_start(UA_Driver *drv) {
    if(drv->state != UA_LIFECYCLESTATE_STOPPED)
        return UA_STATUSCODE_BADINVALIDSTATE;
    if(!drv->server)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_EventLoop *el = UA_Server_getConfig(drv->server)->eventLoop;
    el->lock(el);
    drv->state = UA_LIFECYCLESTATE_STARTED;
    el->unlock(el);
    return UA_STATUSCODE_GOOD;
}

/* Also called for a driver whose start failed and by the application before
 * UA_Server_removeDriver */
static void
LogObjectDriver_stop(UA_Driver *drv) {
    if(drv->state == UA_LIFECYCLESTATE_STOPPED || !drv->server)
        return;
    UA_EventLoop *el = UA_Server_getConfig(drv->server)->eventLoop;
    el->lock(el);
    drv->state = UA_LIFECYCLESTATE_STOPPED;
    el->unlock(el);
}

/* The server frees its drivers after the admin-session is cleared. So the
 * address space is not touched here, stop has undone all changes. */
static UA_StatusCode
LogObjectDriver_free(UA_Driver *drv) {
    if(drv->state != UA_LIFECYCLESTATE_STOPPED)
        return UA_STATUSCODE_BADINVALIDSTATE;
    LogObjectDriver *lod = (LogObjectDriver*)drv;
    if(lod->backend.clear)
        lod->backend.clear(&lod->backend);
    UA_KeyValueMap_clear(&drv->params);
    UA_free(lod);
    return UA_STATUSCODE_GOOD;
}

/****************/
/* Construction */
/****************/

static UA_Boolean
backendComplete(const UA_LogObjectBackend *b) {
    return b->registerLogObject && b->unregisterLogObject &&
        b->addRecord && b->getRecords;
}

UA_StatusCode
UA_LogObjectDriver_new(const UA_LogObjectBackend *backend, UA_Driver **outDriver) {
    if(!outDriver)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    *outDriver = NULL;
    if(backend && !backendComplete(backend))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    LogObjectDriver *lod = (LogObjectDriver*)UA_calloc(1, sizeof(LogObjectDriver));
    if(!lod)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    if(backend) {
        lod->backend = *backend;
    } else {
        UA_StatusCode res = UA_LogObjectBackend_memory(&lod->backend);
        if(res != UA_STATUSCODE_GOOD) {
            UA_free(lod);
            return res;
        }
    }

    UA_Driver *base = &lod->driver;
    base->driverType = UA_DRIVERTYPE_LOGOBJECT;
    base->name = UA_STRING(UA_DRIVER_LOGOBJECT_NAME);
    base->start = LogObjectDriver_start;
    base->stop = LogObjectDriver_stop;
    base->free = LogObjectDriver_free;
    *outDriver = base;
    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_DRIVER_LOGOBJECT */
