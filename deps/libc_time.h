/* This Source Code Form is subject to the terms of the Mozilla Public 
* License, v. 2.0. If a copy of the MPL was not distributed with this 
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */ 
#ifndef LIBC_TIME_H_
#define LIBC_TIME_H_

#include <limits.h>
#include <time.h>
int __secs_to_tm(long long t, struct tm *tm);

#endif /* LIBC_TIME_H_ */
