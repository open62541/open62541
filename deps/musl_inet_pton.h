/* musl as a whole is licensed under the following standard MIT license:
*
 * ----------------------------------------------------------------------
 * Copyright © 2005-2020 Rich Felker, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *----------------------------------------------------------------------
 */

#ifndef MUSL_INET_PTON_H
#define MUSL_INET_PTON_H

#include <open62541/types.h>
#include <open62541/config.h>

#if defined(UA_ARCHITECTURE_WIN32)
#include <winsock2.h>     /* AF_INET */
#elif defined(UA_ARCHITECTURE_LWIP)
#include <lwip/sockets.h> /* AF_INET */
#else
#include <sys/socket.h>   /* AF_INET */
#endif

int musl_inet_pton(int af, const char * UA_RESTRICT s, void * UA_RESTRICT a0);

#endif /* MUSL_INET_PTON_H */
