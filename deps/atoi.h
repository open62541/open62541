#ifndef ATOI_H
#define ATOI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_types.h"
UA_StatusCode atoiUnsigned(const char *s, size_t size, UA_UInt64 *result);
UA_StatusCode atoiSigned(const char *s, size_t size, UA_Int64 *result);
    
#ifdef __cplusplus
}
#endif

#endif /* ATOI_H */

