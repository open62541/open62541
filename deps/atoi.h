#ifndef ATOI_H
#define ATOI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <open62541/types.h>

size_t atoiUnsigned(const char *pos, size_t size, UA_UInt64 *result);
size_t atoiSigned(const char *pos, size_t size, UA_Int64 *result);
    
#ifdef __cplusplus
}
#endif

#endif /* ATOI_H */

