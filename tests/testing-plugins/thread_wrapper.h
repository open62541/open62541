/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Simple wrapper for unit test threads */

/* Threads */

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

/* Mutex */

/* Windows returns non-zero on success and pthread returns zero,
 * so compare to zero to achieve consistent return values */

#ifndef WIN32
#define MUTEX_HANDLE pthread_mutex_t

/* Will return UA_TRUE when zero */
#define MUTEX_INIT(name) (pthread_mutex_init(&(name), NULL) == 0)
#define MUTEX_LOCK(name) (pthread_mutex_lock(&(name)) == 0)
#define MUTEX_UNLOCK(name) (pthread_mutex_unlock(&(name)) == 0)
#define MUTEX_DESTROY(name) (pthread_mutex_destroy(&(name)) == 0)

#else

#define MUTEX_HANDLE HANDLE

/* Will return UA_FALSE when zero */
#define MUTEX_INIT(name) (CreateMutex(NULL, FALSE, NULL) != 0)
#define MUTEX_LOCK(name) (WaitForSingleObject((name), INFINITE) != 0)
#define MUTEX_UNLOCK(name) (ReleaseMutex((name)) != 0)
#define MUTEX_DESTROY(name) (CloseHandle((name)) != 0)
#endif
