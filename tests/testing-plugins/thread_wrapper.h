/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Simple wrapper for unit test threads */


#ifndef WIN32
#include <pthread.h>
#define THREAD_HANDLE pthread_t
#define THREAD_CREATE(handle, callback) pthread_create(&handle, NULL, callback, NULL)
#define THREAD_JOIN(handle) pthread_join(handle, NULL)
#define THREAD_CALLBACK(name) static void * name(void *_)

#else

#include <windows.h>
#define THREAD_HANDLE HANDLE
#define THREAD_CREATE(handle, callback) { handle = CreateThread( NULL, 0, callback, NULL, 0, NULL); }
#define THREAD_JOIN(handle) WaitForSingleObject(handle, INFINITE)


#define THREAD_CALLBACK(name) DWORD WINAPI name( LPVOID lpParam )

#endif
