#ifndef LIBC_TIME_H_
#define LIBC_TIME_H_

#include <limits.h>
#include <time.h>
int __secs_to_tm(long long t, struct tm *tm);

#endif /* LIBC_TIME_H_ */
