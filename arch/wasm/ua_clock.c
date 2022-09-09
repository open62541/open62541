#include <open62541/types.h>

#define IMPORT(name) __attribute__((import_module("ua"), import_name(name)))

IMPORT("DateTime_now") UA_DateTime Host_DateTime_now(void);
IMPORT("DateTime_localTimeUtcOffset") UA_Int64 Host_DateTime_localTimeUtcOffset(void);
IMPORT("DateTime_nowMonotonic") UA_DateTime Host_DateTime_nowMonotonic(void);

UA_DateTime UA_DateTime_now(void) {
  return Host_DateTime_now();
}

UA_Int64 UA_DateTime_localTimeUtcOffset(void) {
  return Host_DateTime_localTimeUtcOffset();
}

UA_DateTime UA_DateTime_nowMonotonic(void) {
  return Host_DateTime_nowMonotonic();
}
