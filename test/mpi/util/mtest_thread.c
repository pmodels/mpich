/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/* This file provides a portability layer for using threads. */

#if THREAD_PACKAGE_NAME == THREAD_PACKAGE_NONE

/* Only empty initialization and finalization functions are supported. */
void MTest_init_thread_pkg(void)
{
}

void MTest_finalize_thread_pkg(void)
{
}

#else /* THREAD_PACKAGE_NAME != THREAD_PACKAGE_NONE */

/*
   Define macro to override gcc strict flags,
   -D_POSIX_C_SOURCE=199506L, -std=c89 and -std=c99,
   that disallow pthread_barrier_t and friends.
*/
#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE < 200112L
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

/* We remember all of the threads we create; this similifies terminating
   (joining) them. */
#ifndef MTEST_MAX_THREADS
#define MTEST_MAX_THREADS 16
#endif

static MTEST_THREAD_HANDLE threads[MTEST_MAX_THREADS];
/* access w/o a lock is broken, but "volatile" should help reduce the amount of
 * speculative loading/storing */
static volatile int nthreads = 0;

/* to-be-defined in the thread package, or default routines will be used */
#undef HAVE_MTEST_THREAD_BARRIER
#undef HAVE_MTEST_INIT_THREAD_PKG

#if !defined(THREAD_PACKAGE_NAME)
#error "thread package (THREAD_PACKAGE_NAME) not defined"

#elif THREAD_PACKAGE_NAME == THREAD_PACKAGE_WIN
#include "mtest_thread_win.h"

#elif THREAD_PACKAGE_NAME == THREAD_PACKAGE_POSIX || THREAD_PACKAGE_NAME == THREAD_PACKAGE_SOLARIS
#include "mtest_thread_pthread.h"

#elif THREAD_PACKAGE_NAME == THREAD_PACKAGE_ARGOBOTS
#include "mtest_thread_abt.h"

#else
#error "thread package (THREAD_PACKAGE_NAME) unknown"

#endif

/* Default routines */

#if !defined(HAVE_MTEST_THREAD_BARRIER)

static MTEST_THREAD_LOCK_TYPE barrierLock;
static volatile int phase = 0;
static volatile int c[2] = { -1, -1 };

int MTest_thread_barrier_init(void)
{
    return MTest_thread_lock_create(&barrierLock);
}

int MTest_thread_barrier_free(void)
{
    return MTest_thread_lock_free(&barrierLock);
}

#define LOCK_ERR_CHECK(err_)                            \
    if (err_) {                                         \
        fprintf(stderr, "Lock failed in barrier!\n");   \
        return err_;                                    \
    }

/* This is a generic barrier implementation.  To ensure that tests don't
   silently fail, this both prints an error message and returns an error
   result on any failure. */
int MTest_thread_barrier(int nt)
{
    volatile int *cntP;
    int err = 0;
    int num_left;

    if (nt < 0)
        nt = nthreads;
    /* Force a write barrier by using lock/unlock */
    err = MTest_thread_lock(&barrierLock);
    LOCK_ERR_CHECK(err);
    cntP = &c[phase];

    /* printf("[%d] cnt = %d, phase = %d\n", pthread_self(), *cntP, phase); */

    /* The first thread to enter will reset the counter */
    if (*cntP < 0)
        *cntP = nt;
    /* printf("phase = %d, cnt = %d\n", phase, *cntP); */
    /* The last thread to enter will force the counter to be negative */
    if (*cntP == 1) {
        /* printf("[%d] changing phase from %d\n", pthread_self(), phase); */
        phase = !phase;
        c[phase] = -1;
        *cntP = 0;
    }
    /* Really need a write barrier here */
    *cntP = *cntP - 1;
    num_left = *cntP;
    err = MTest_thread_unlock(&barrierLock);
    LOCK_ERR_CHECK(err);

    /* wait for other threads */
    while (num_left > 0) {
        err = MTest_thread_lock(&barrierLock);
        LOCK_ERR_CHECK(err);

        /* TODO: This would be improved with atomic ops instead of using
         * a mutex. Integrating MPL for portable atomics would be an
         * improvement. */
        num_left = *cntP;

        err = MTest_thread_unlock(&barrierLock);
        LOCK_ERR_CHECK(err);
    }

    return err;
}
#endif /* Default barrier routine */

#if !defined(HAVE_MTEST_INIT_THREAD_PKG)
void MTest_init_thread_pkg(void)
{
}

void MTest_finalize_thread_pkg(void)
{
}

#endif /* THREAD_PACKAGE_NAME != THREAD_PACKAGE_NONE */

#endif /* Default MTest_init_thread_pkg */
