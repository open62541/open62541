#ifndef MDNS_SDTXT_H_
#define MDNS_SDTXT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "xht.h"

/**
 * returns hashtable of strings from the SD TXT record rdata
 */
xht_t MDNSD_EXPORT *txt2sd(unsigned char *txt, int len);

/**
 * returns a raw block that can be sent with a SD TXT record, sets length
 */
unsigned char MDNSD_EXPORT *sd2txt(xht_t *h, int *len);

#ifdef __cplusplus
} // extern "C"
#endif

#endif	/* MDNS_SDTXT_H_ */
