#ifndef NANOLIB_MD5_H
#define NANOLIB_MD5_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER) || defined(_WIN32)
#else
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define READ_DATA_SIZE	1024
#define MD5_SIZE		16
#define MD5_STR_LEN		(MD5_SIZE * 2)
#define MD5_LEN			32

typedef struct
{
	unsigned int count[2];
	unsigned int state[4];
	unsigned char buffer[64];   
} MD5_CTX;


#define F(x,y,z) ((x & y) | (~x & z))
#define G(x,y,z) ((x & z) | (y & ~z))
#define H(x,y,z) (x^y^z)
#define I(x,y,z) (y ^ (x | ~z))
#define ROTATE_LEFT(x,n) ((x << n) | (x >> (32-n)))

#define FF(a,b,c,d,x,s,ac) \
{ \
	a += F(b,c,d) + x + ac; \
	a = ROTATE_LEFT(a,s); \
	a += b; \
}
#define GG(a,b,c,d,x,s,ac) \
{ \
	a += G(b,c,d) + x + ac; \
	a = ROTATE_LEFT(a,s); \
	a += b; \
}
#define HH(a,b,c,d,x,s,ac) \
{ \
	a += H(b,c,d) + x + ac; \
	a = ROTATE_LEFT(a,s); \
	a += b; \
}
#define II(a,b,c,d,x,s,ac) \
{ \
	a += I(b,c,d) + x + ac; \
	a = ROTATE_LEFT(a,s); \
	a += b; \
}                                            

int ComputeStringMD5(unsigned char *dest_str, unsigned int dest_len, char *md5_str);
int ComputeFileMD5(const char *file_path, char *md5_str);

void MD5Init(MD5_CTX *context);
void MD5Update(MD5_CTX *context, unsigned char *input, unsigned int inputlen);
void MD5Final(MD5_CTX *context, unsigned char digest[16]);
void MD5Transform(unsigned int state[4], unsigned char block[64]);
void MD5Encode(unsigned char *output, unsigned int *input, unsigned int len);
void MD5Decode(unsigned int *output, unsigned char *input, unsigned int len);

#ifdef __cplusplus
}
#endif

#endif
