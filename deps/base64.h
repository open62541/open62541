#ifndef UA_BASE64_H_
#define UA_BASE64_H_

#include <open62541/config.h>

_UA_BEGIN_DECLS

#include <stddef.h>

/**
 * base64_encode - Base64 encode
 * @src: Data to be encoded
 * @len: Length of the data to be encoded
 * @out_len: Pointer to output length variable
 * Returns: Allocated buffer of out_len bytes of encoded data,
 * or %NULL on failure. The output is NOT Null-terminated. */
unsigned char *
UA_base64(const unsigned char *src, size_t len, size_t *out_len);

/* Requires as input a buffer of length at least 4*((len + 2) / 3).
 * Returns the actual size */
size_t
UA_base64_buf(const unsigned char *src, size_t len, unsigned char *out);

/**
 * base64_decode - Base64 decode
 * @src: Data to be decoded
 * @len: Length of the data to be decoded
 * @out_len: Pointer to output length variable
 * Returns: Allocated buffer of out_len bytes of decoded data,
 * or %NULL on failure. */
unsigned char *
UA_unbase64(const unsigned char *src, size_t len, size_t *out_len);

_UA_END_DECLS

#endif /* UA_BASE64_H_ */
