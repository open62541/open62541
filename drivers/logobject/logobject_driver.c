/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "logobject_internal.h"

#ifdef UA_ENABLE_DRIVER_LOGOBJECT

#include <stdio.h>
#include <string.h>

/* OPC UA Part 26 LogObjects as a driver. The driver uses only the public API of
 * the server. Its state is protected by the EventLoop lock, which the server
 * lock takes first. */

static UA_StatusCode LogObjectDriver_start(UA_Driver *drv);

UA_Boolean
isLogObjectDriver(const UA_Driver *driver) {
    return driver && driver->start == LogObjectDriver_start;
}

LogObjectDriver *
findLogObjectDriver(UA_Server *server) {
    for(UA_Driver *drv = UA_Server_getDrivers(server); drv; drv = drv->next) {
        if(isLogObjectDriver(drv))
            return (LogObjectDriver*)drv;
    }
    return NULL;
}

UA_DateTime
logObjectNow(LogObjectDriver *lod) {
    UA_EventLoop *el = UA_Server_getConfig(lod->driver.server)->eventLoop;
    return el->dateTime_now(el);
}

LogObjectEntry *
findLogObjectEntry(LogObjectDriver *lod, const UA_NodeId *logObjectId) {
    LogObjectEntry *entry;
    LIST_FOREACH(entry, &lod->logObjects, pointers) {
        if(UA_NodeId_equal(&entry->nodeId, logObjectId))
            return entry;
    }
    return NULL;
}

/***********/
/* Records */
/***********/

UA_StatusCode
addLogObjectRecord(LogObjectDriver *lod, LogObjectEntry *entry,
                   const UA_LogRecord *record) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;

    /* The MinimumSeverity is checked when the record is generated (Part 26,
     * 5.2). Records below the limit are dropped silently. */
    if(record->severity >= entry->settings.minimumSeverity) {
        UA_Boolean overflow = false;
        res = lod->backend.addRecord(lod->driver.server, lod->backend.context,
                                     &entry->nodeId, record, logObjectNow(lod),
                                     &overflow);
        if(overflow)
            scheduleOverflowEvent(lod, entry);
    }

    /* The ServerLog contains the records of all LogObjects (Part 26, 7.2) */
    if(res == UA_STATUSCODE_GOOD && !entry->isServerLog && lod->serverLogRegistered)
        res = addLogObjectRecord(lod, &lod->serverLog, record);
    return res;
}

UA_StatusCode
UA_LogObjectDriver_addRecord(UA_Driver *driver, const UA_NodeId logObjectId,
                             const UA_LogRecord *record) {
    if(!isLogObjectDriver(driver) || !record)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(record->severity < 1 || record->severity > 1000)
        return UA_STATUSCODE_BADOUTOFRANGE;
    if(!driver->server)
        return UA_STATUSCODE_BADINVALIDSTATE;

    LogObjectDriver *lod = (LogObjectDriver*)driver;
    UA_EventLoop *el = UA_Server_getConfig(driver->server)->eventLoop;
    el->lock(el);
    UA_StatusCode res = UA_STATUSCODE_BADINVALIDSTATE;
    if(driver->state == UA_LIFECYCLESTATE_STARTED) {
        LogObjectEntry *entry = findLogObjectEntry(lod, &logObjectId);
        res = UA_STATUSCODE_BADNODEIDUNKNOWN;
        if(entry) {
            /* Shallow copy to fill in the time. The backend deep-copies. */
            UA_LogRecord r = *record;
            if(r.time == 0)
                r.time = logObjectNow(lod);
            res = addLogObjectRecord(lod, entry, &r);
        }
    }
    el->unlock(el);
    return res;
}

/************/
/* Settings */
/************/

/* (UA_Double)UA_INT64_MAX rounds up to 2^63, which is not an Int64. So the
 * upper bound is exclusive. */
UA_Boolean
logObjectValidDuration(UA_Double ms) {
    return ms * UA_DATETIME_MSEC >= 1.0 && ms < (UA_Double)UA_INT64_MAX / UA_DATETIME_MSEC;
}

/* A MaxStorageDuration of zero disables the limit */
static UA_Boolean
validSettings(const UA_LogObjectSettings *s) {
    return s->maxRecords > 0 && s->minimumSeverity >= 1 && s->minimumSeverity <= 1000 &&
        (s->maxStorageDuration == 0.0 || logObjectValidDuration(s->maxStorageDuration));
}

/* The change does not affect the records that are already stored (Part 26,
 * 5.2). The capture reads the MinimumSeverity of the ServerLog without lock. */
void
setMinimumSeverity(LogObjectDriver *lod, LogObjectEntry *entry, UA_UInt16 severity) {
    entry->settings.minimumSeverity = severity;
    if(entry->isServerLog)
        UA_atomic_store(&lod->serverLogMinimumSeverity, (uintptr_t)severity);
}

UA_StatusCode
UA_LogObjectDriver_setSettings(UA_Driver *driver, const UA_NodeId logObjectId,
                               const UA_LogObjectSettings *settings) {
    if(!isLogObjectDriver(driver) || !settings || !validSettings(settings))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(!driver->server)
        return UA_STATUSCODE_BADINVALIDSTATE;
    LogObjectDriver *lod = (LogObjectDriver*)driver;
    UA_EventLoop *el = UA_Server_getConfig(driver->server)->eventLoop;
    el->lock(el);
    UA_StatusCode res = UA_STATUSCODE_BADINVALIDSTATE;
    LogObjectEntry *entry = NULL;
    if(driver->state == UA_LIFECYCLESTATE_STARTED) {
        entry = findLogObjectEntry(lod, &logObjectId);
        res = (entry) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    /* Zero is not a valid value of the MaxStorageDuration Property */
    if(res == UA_STATUSCODE_GOOD && settings->maxStorageDuration == 0.0 &&
       hasMaxStorageDurationValue(lod, entry))
        res = UA_STATUSCODE_BADINVALIDARGUMENT;

    /* The limits of the storage */
    if(res == UA_STATUSCODE_GOOD &&
       (settings->maxRecords != entry->settings.maxRecords ||
        settings->maxStorageDuration != entry->settings.maxStorageDuration)) {
        if(!lod->backend.updateLogObject)
            res = UA_STATUSCODE_BADNOTSUPPORTED;
        else
            res = lod->backend.updateLogObject(driver->server, lod->backend.context,
                                               &entry->nodeId, settings);
    }

    if(res == UA_STATUSCODE_GOOD) {
        entry->settings.maxRecords = settings->maxRecords;
        entry->settings.maxStorageDuration = settings->maxStorageDuration;
        setMinimumSeverity(lod, entry, settings->minimumSeverity);
        mirrorLogObjectProperties(lod, entry);
    }
    el->unlock(el);
    return res;
}

UA_StatusCode
UA_LogObjectDriver_getSettings(UA_Driver *driver, const UA_NodeId logObjectId,
                               UA_LogObjectSettings *settings) {
    if(!isLogObjectDriver(driver) || !settings)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(!driver->server)
        return UA_STATUSCODE_BADINVALIDSTATE;
    LogObjectDriver *lod = (LogObjectDriver*)driver;
    UA_EventLoop *el = UA_Server_getConfig(driver->server)->eventLoop;
    el->lock(el);
    UA_StatusCode res = UA_STATUSCODE_BADINVALIDSTATE;
    if(driver->state == UA_LIFECYCLESTATE_STARTED) {
        LogObjectEntry *entry = findLogObjectEntry(lod, &logObjectId);
        res = UA_STATUSCODE_BADNODEIDUNKNOWN;
        if(entry) {
            *settings = entry->settings;
            res = UA_STATUSCODE_GOOD;
        }
    }
    el->unlock(el);
    return res;
}

/*****************/
/* Notifications */
/*****************/

/* A closed Session releases its continuation points */
static void
LogObjectDriver_notification(UA_Driver *drv, UA_ApplicationNotificationType type,
                             const UA_KeyValueMap payload) {
    if(type != UA_APPLICATIONNOTIFICATIONTYPE_SESSION_CLOSED)
        return;
    const UA_NodeId *sessionId = (const UA_NodeId*)
        UA_KeyValueMap_getScalar(&payload, UA_QUALIFIEDNAME(0, "session-id"),
                                 &UA_TYPES[UA_TYPES_NODEID]);
    if(sessionId)
        releaseContinuationPoints((LogObjectDriver*)drv, sessionId, NULL);
}

/**************/
/* Parameters */
/**************/

/* A known key with another type than documented is a configuration error,
 * not an absent value */
static const void *
driverParam(LogObjectDriver *lod, const char *key, const UA_DataType *type,
            UA_StatusCode *res) {
    const UA_Variant *value =
        UA_KeyValueMap_get(&lod->driver.params, UA_QUALIFIEDNAME(0, (char*)(uintptr_t)key));
    if(!value)
        return NULL;
    /* Also subtypes of the numeric types, e.g. a Duration for a Double */
    if(UA_Variant_isScalar(value) && value->type->typeKind == type->typeKind &&
       type->typeKind <= UA_DATATYPEKIND_DOUBLE)
        return value->data;
    UA_LOG_ERROR(UA_Server_getConfig(lod->driver.server)->logging, UA_LOGCATEGORY_SERVER,
                 "LogObject: The parameter %s has the wrong type", key);
    *res = UA_STATUSCODE_BADINVALIDARGUMENT;
    return NULL;
}

static void
readSettingsParams(LogObjectDriver *lod, const char *prefix, UA_LogObjectSettings *s,
                   UA_StatusCode *res) {
    char key[64];
    snprintf(key, sizeof(key), "%s-max-records", prefix);
    const UA_UInt32 *maxRecords = (const UA_UInt32*)
        driverParam(lod, key, &UA_TYPES[UA_TYPES_UINT32], res);
    if(maxRecords)
        s->maxRecords = *maxRecords;
    snprintf(key, sizeof(key), "%s-max-storage-duration", prefix);
    const UA_Double *duration = (const UA_Double*)
        driverParam(lod, key, &UA_TYPES[UA_TYPES_DURATION], res);
    if(duration)
        s->maxStorageDuration = *duration;
    snprintf(key, sizeof(key), "%s-minimum-severity", prefix);
    const UA_UInt16 *severity = (const UA_UInt16*)
        driverParam(lod, key, &UA_TYPES[UA_TYPES_UINT16], res);
    if(severity)
        s->minimumSeverity = *severity;
}

static UA_StatusCode
configureDriver(LogObjectDriver *lod) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;

    lod->serverLogEnabled = true;
    const UA_Boolean *serverLog = (const UA_Boolean*)
        driverParam(lod, "server-log", &UA_TYPES[UA_TYPES_BOOLEAN], &res);
    if(serverLog)
        lod->serverLogEnabled = *serverLog;

    /* Information and above (Part 26, Table 9) */
    UA_LogObjectSettings defaultServerLog = {1000, 0.0, 51};
    lod->serverLogSettings = defaultServerLog;
    readSettingsParams(lod, "server-log", &lod->serverLogSettings, &res);

    /* The LogObjects in the information model without the Properties */
    UA_LogObjectSettings defaultInstance = {1000, 0.0, 1};
    lod->defaultSettings = defaultInstance;
    readSettingsParams(lod, "default", &lod->defaultSettings, &res);

    lod->maxRecordsPerCall = 1000;
    const UA_UInt32 *perCall = (const UA_UInt32*)
        driverParam(lod, "max-records-per-call", &UA_TYPES[UA_TYPES_UINT32], &res);
    if(perCall)
        lod->maxRecordsPerCall = *perCall;

    lod->maxContinuationPoints = 32;
    const UA_UInt16 *cps = (const UA_UInt16*)
        driverParam(lod, "max-continuation-points", &UA_TYPES[UA_TYPES_UINT16], &res);
    if(cps)
        lod->maxContinuationPoints = *cps;

    lod->overflowEventInterval = 60000.0; /* At most one Event per minute */
    const UA_Double *interval = (const UA_Double*)
        driverParam(lod, "overflow-event-interval", &UA_TYPES[UA_TYPES_DURATION], &res);
    if(interval)
        lod->overflowEventInterval = *interval;

    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(!validSettings(&lod->serverLogSettings) || !validSettings(&lod->defaultSettings) ||
       !logObjectValidDuration(lod->overflowEventInterval)) {
        UA_LOG_ERROR(UA_Server_getConfig(lod->driver.server)->logging,
                     UA_LOGCATEGORY_SERVER,
                     "LogObject: Invalid parameters");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    return UA_STATUSCODE_GOOD;
}

/*************/
/* Lifecycle */
/*************/

/* Remember the closed LogObjects to find the orphaned records at the next
 * start */
static void
rememberClosedLogObject(LogObjectDriver *lod, const UA_NodeId *id) {
    UA_NodeId *ids = (UA_NodeId*)
        UA_realloc(lod->closedIds, (lod->closedIdsSize + 1) * sizeof(UA_NodeId));
    if(!ids)
        return;
    lod->closedIds = ids;
    if(UA_NodeId_copy(id, &lod->closedIds[lod->closedIdsSize]) == UA_STATUSCODE_GOOD)
        lod->closedIdsSize++;
}

/* Delete the records of LogObjects that were deleted while the driver was
 * stopped. Their destructor did not run. */
static void
deleteOrphanedRecords(LogObjectDriver *lod) {
    for(size_t i = 0; i < lod->closedIdsSize; i++) {
        if(!findLogObjectEntry(lod, &lod->closedIds[i]))
            lod->backend.unregisterLogObject(lod->driver.server, lod->backend.context,
                                             &lod->closedIds[i], true);
    }
    UA_Array_delete(lod->closedIds, lod->closedIdsSize, &UA_TYPES[UA_TYPES_NODEID]);
    lod->closedIds = NULL;
    lod->closedIdsSize = 0;
}

/* Undo everything start did. Idempotent, also for a partial start. The type
 * lifecycles are restored before the ServerLog is deleted, so that the
 * deletion does not drop its records. */
static void
stopDriver(LogObjectDriver *lod) {
    removeLogCapture(lod);
    cancelOverflowEvents(lod);
    restoreLogObjectTypeLifecycles(lod);
    unbindLogObjectMethods(lod);
    LogObjectEntry *entry, *tmp;
    LIST_FOREACH_SAFE(entry, &lod->logObjects, pointers, tmp) {
        unregisterLogObjectNode(lod, entry, false);
        rememberClosedLogObject(lod, &entry->nodeId);
        if(entry != &lod->serverLog) {
            UA_NodeId_clear(&entry->nodeId);
            UA_free(entry);
        }
    }
    lod->serverLogRegistered = false;
    releaseContinuationPoints(lod, NULL, NULL);
    deleteServerLogNodes(lod);
}

static UA_StatusCode
startDriver(LogObjectDriver *lod) {
    UA_StatusCode res = configureDriver(lod);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = createServerLogNodes(lod);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Register the ServerLog with the settings of the parameters */
    if(lod->serverLogEnabled) {
        memset(&lod->serverLog, 0, sizeof(LogObjectEntry));
        lod->serverLog.nodeId = UA_NS0ID(SERVERLOG);
        lod->serverLog.settings = lod->serverLogSettings;
        lod->serverLog.isServerLog = true;
        res = registerLogObjectNode(lod, &lod->serverLog);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        lod->serverLogRegistered = true;
    }

    /* Serve all other LogObjects of the information model */
    res = serveLogObjectInstances(lod);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    deleteOrphanedRecords(lod);

    /* Capture the server logger. This is done last so that no record is
     * generated before the ServerLog is ready. */
    if(lod->serverLogRegistered)
        installLogCapture(lod);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_LogObjectDriver_refresh(UA_Driver *driver) {
    if(!isLogObjectDriver(driver) || !driver->server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_EventLoop *el = UA_Server_getConfig(driver->server)->eventLoop;
    el->lock(el);
    UA_StatusCode res = UA_STATUSCODE_BADINVALIDSTATE;
    if(driver->state == UA_LIFECYCLESTATE_STARTED)
        res = serveLogObjectInstances((LogObjectDriver*)driver);
    el->unlock(el);
    return res;
}

static UA_StatusCode
LogObjectDriver_start(UA_Driver *drv) {
    if(drv->state != UA_LIFECYCLESTATE_STOPPED)
        return UA_STATUSCODE_BADINVALIDSTATE;
    if(!drv->server)
        return UA_STATUSCODE_BADINTERNALERROR;
    LogObjectDriver *lod = (LogObjectDriver*)drv;
    UA_EventLoop *el = UA_Server_getConfig(drv->server)->eventLoop;
    el->lock(el);
    UA_StatusCode res = startDriver(lod);
    if(res == UA_STATUSCODE_GOOD) {
        drv->state = UA_LIFECYCLESTATE_STARTED;
    } else {
        stopDriver(lod);
        UA_LOG_ERROR(UA_Server_getConfig(drv->server)->logging, UA_LOGCATEGORY_SERVER,
                     "LogObject: Starting the driver failed with %s",
                     UA_StatusCode_name(res));
    }
    el->unlock(el);
    return res;
}

/* Also called for a driver whose start failed and by the application before
 * UA_Server_removeDriver */
static void
LogObjectDriver_stop(UA_Driver *drv) {
    if(drv->state == UA_LIFECYCLESTATE_STOPPED || !drv->server)
        return;
    UA_EventLoop *el = UA_Server_getConfig(drv->server)->eventLoop;
    el->lock(el);
    stopDriver((LogObjectDriver*)drv);
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
    UA_Array_delete(lod->closedIds, lod->closedIdsSize, &UA_TYPES[UA_TYPES_NODEID]);
    clearLogObjectTypeLifecycles(lod);
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
    LIST_INIT(&lod->logObjects);
    LIST_INIT(&lod->continuationPoints);

    UA_Driver *base = &lod->driver;
    base->driverType = UA_DRIVERTYPE_LOGOBJECT;
    base->name = UA_STRING(UA_DRIVER_LOGOBJECT_NAME);
    base->notificationCallback = LogObjectDriver_notification;
    base->notificationFilter = UA_APPLICATIONNOTIFICATIONTYPE_SESSION;
    base->start = LogObjectDriver_start;
    base->stop = LogObjectDriver_stop;
    base->free = LogObjectDriver_free;
    *outDriver = base;
    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_DRIVER_LOGOBJECT */
