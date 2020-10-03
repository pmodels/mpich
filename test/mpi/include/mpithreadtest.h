/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPITHREADTEST_H_INCLUDED
#define MPITHREADTEST_H_INCLUDED

#include "mpitestconf.h"

/*
   Define routines to start a thread for different thread packages.
   The routine that is started is expected to return data when it
   exits; the type of this data is

   MTEST_THREAD_RETURN_TYPE
*/

#define THREAD_PACKAGE_INVALID 0
#define THREAD_PACKAGE_NONE    1
#define THREAD_PACKAGE_POSIX   2
#define THREAD_PACKAGE_SOLARIS 3
#define THREAD_PACKAGE_WIN     4
#define THREAD_PACKAGE_UTI     5
#define THREAD_PACKAGE_ARGOBOTS 6

#if !defined(THREAD_PACKAGE_NAME)
#error "thread package (THREAD_PACKAGE_NAME) not defined"

#elif THREAD_PACKAGE_NAME == THREAD_PACKAGE_NONE
/* Empty. No threaded tests should run. */

#elif THREAD_PACKAGE_NAME == THREAD_PACKAGE_WIN
#include <windows.h>
#define MTEST_THREAD_RETURN_TYPE DWORD
#define MTEST_THREAD_HANDLE HANDLE
#define MTEST_THREAD_LOCK_TYPE HANDLE
#define MTEST_THREAD_RETVAL_IGN 0

#elif THREAD_PACKAGE_NAME == THREAD_PACKAGE_POSIX || THREAD_PACKAGE_NAME == THREAD_PACKAGE_SOLARIS
#include <pthread.h>
#define MTEST_THREAD_RETURN_TYPE void *
#define MTEST_THREAD_HANDLE pthread_t
#define MTEST_THREAD_LOCK_TYPE pthread_mutex_t
#define MTEST_THREAD_RETVAL_IGN NULL

#elif THREAD_PACKAGE_NAME == THREAD_PACKAGE_ARGOBOTS
#include <abt.h>

#define MTEST_THREAD_RETURN_TYPE void
#define MTEST_THREAD_HANDLE ABT_thread
#define MTEST_THREAD_LOCK_TYPE ABT_mutex
#define MTEST_THREAD_RETVAL_IGN

#else
#error "thread package (THREAD_PACKAGE_NAME) unknown"

#endif

#if THREAD_PACKAGE_NAME != THREAD_PACKAGE_NONE
int MTest_Start_thread(MTEST_THREAD_RETURN_TYPE(*fn) (void *p), void *arg);
int MTest_Join_threads(void);
int MTest_thread_lock_create(MTEST_THREAD_LOCK_TYPE *);
int MTest_thread_lock(MTEST_THREAD_LOCK_TYPE *);
int MTest_thread_unlock(MTEST_THREAD_LOCK_TYPE *);
int MTest_thread_lock_free(MTEST_THREAD_LOCK_TYPE *);
int MTest_thread_yield(void);
int MTest_thread_barrier_init(void);
int MTest_thread_barrier(int);
int MTest_thread_barrier_free(void);
#endif

void MTest_init_thread_pkg(void);
void MTest_finalize_thread_pkg(void);

#endif /* MPITHREADTEST_H_INCLUDED */
