/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#include "ua_server.h"
#include "networklayer_tcp.h"
#include "logger_stdout.h"

/* #define MANUFACTURER_NAME "open62541" */
/* #define PRODUCT_NAME "open62541 OPC UA Server" */
/* #define STRINGIFY(x) #x //some magic */
/* #define TOSTRING(x) STRINGIFY(x) //some magic */
/* #define SOFTWARE_VERSION TOSTRING(VERSION) */
/* #define BUILD_NUMBER "0" */

/*     static struct tm ct; */
/*     ct.tm_year = (__DATE__[7] - '0') * 1000 + (__DATE__[8] - '0') * 100 + */
/*         (__DATE__[9] - '0') * 10 + (__DATE__[10] - '0')- 1900; */

/*     if(__DATE__[0]=='J' && __DATE__[1]=='a' && __DATE__[2]=='n') ct.tm_mon = 1-1; */
/*     else if(__DATE__[0]=='F' && __DATE__[1]=='e' && __DATE__[2]=='b') ct.tm_mon = 2-1; */
/*     else if(__DATE__[0]=='M' && __DATE__[1]=='a' && __DATE__[2]=='r') ct.tm_mon = 3-1; */
/*     else if(__DATE__[0]=='A' && __DATE__[1]=='p' && __DATE__[2]=='r') ct.tm_mon = 4-1; */
/*     else if(__DATE__[0]=='M' && __DATE__[1]=='a' && __DATE__[2]=='y') ct.tm_mon = 5-1; */
/*     else if(__DATE__[0]=='J' && __DATE__[1]=='u' && __DATE__[2]=='n') ct.tm_mon = 6-1; */
/*     else if(__DATE__[0]=='J' && __DATE__[1]=='u' && __DATE__[2]=='l') ct.tm_mon = 7-1; */
/*     else if(__DATE__[0]=='A' && __DATE__[1]=='u' && __DATE__[2]=='g') ct.tm_mon = 8-1; */
/*     else if(__DATE__[0]=='S' && __DATE__[1]=='e' && __DATE__[2]=='p') ct.tm_mon = 9-1; */
/*     else if(__DATE__[0]=='O' && __DATE__[1]=='c' && __DATE__[2]=='t') ct.tm_mon = 10-1; */
/*     else if(__DATE__[0]=='N' && __DATE__[1]=='o' && __DATE__[2]=='v') ct.tm_mon = 11-1; */
/*     else if(__DATE__[0]=='D' && __DATE__[1]=='e' && __DATE__[2]=='c') ct.tm_mon = 12-1; */

/*     // special case to handle __DATE__ not inserting leading zero on day of month */
/*     // if Day of month is less than 10 - it inserts a blank character */
/*     // this results in a negative number for tm_mday */

/*     if(__DATE__[4] == ' ') */
/*         ct.tm_mday = __DATE__[5]-'0'; */
/*     else */
/*         ct.tm_mday = (__DATE__[4]-'0')*10 + (__DATE__[5]-'0'); */
/*     ct.tm_hour = ((__TIME__[0] - '0') * 10 + __TIME__[1] - '0'); */
/*     ct.tm_min = ((__TIME__[3] - '0') * 10 + __TIME__[4] - '0'); */
/*     ct.tm_sec = ((__TIME__[6] - '0') * 10 + __TIME__[7] - '0'); */
/*     ct.tm_isdst = -1; // information is not available. */
/*     server->buildDate = (mktime(&ct) * UA_SEC_TO_DATETIME) + UA_DATETIME_UNIX_EPOCH; */

#define UA_STRING_STATIC(s) (UA_String){sizeof(s)-1, (UA_Byte*)s}
#define UA_STRING_STATIC_NULL {0, NULL}

const UA_ServerConfig UA_ServerConfig_standard = {
    .nThreads = 1,
    .logger = Logger_Stdout,

    .buildInfo = {
        .productUri = UA_STRING_STATIC("urn:unconfigured:open62541"),
        .manufacturerName = UA_STRING_STATIC_NULL,
        .productName = UA_STRING_STATIC("urn:unconfigured:open62541:open62541Server"),
        .softwareVersion = UA_STRING_STATIC("0"),
        .buildNumber = UA_STRING_STATIC("0"),
        .buildDate = 0},
    .applicationDescription = {
        .applicationUri = UA_STRING_STATIC("urn:unconfigured:application"),
        .productUri = UA_STRING_STATIC("urn:unconfigured:product"),
        .applicationName = { UA_STRING_STATIC(""), UA_STRING_STATIC("open62541Server") },
        .applicationType = UA_APPLICATIONTYPE_SERVER,
        .gatewayServerUri = UA_STRING_STATIC_NULL,
        .discoveryProfileUri = UA_STRING_STATIC_NULL,
        .discoveryUrlsSize = 0,
        .discoveryUrls = NULL
    },
    .serverCertificate = UA_STRING_STATIC_NULL,

    .networkLayersSize = 0, .networkLayers = NULL,

    .enableAnonymousLogin = UA_TRUE,
    .enableUsernamePasswordLogin = UA_TRUE,
    .usernamePasswordLogins =
    { { UA_STRING_STATIC("user1"), UA_STRING_STATIC("password") },
      { UA_STRING_STATIC("uset2"), UA_STRING_STATIC("password1") } },
    .usernamePasswordLoginsSize = 2
};
