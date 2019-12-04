/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
   Define macro to override gcc strict flags,
   -D_POSIX_C_SOURCE=199506L, -std=c89 and -std=c99,
   that disallow pthread_barrier_t and friends.
*/
#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE < 200112L
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include "mpithreadtest.h"

/* This file provides a portability layer for using threads.  Currently,
   it supports POSIX threads (pthreads) and Windows threads.  Testing has
   been performed for pthreads.
 */

/* We remember all of the threads we create; this similifies terminating
   (joining) them. */
#ifndef MTEST_MAX_THREADS
#define MTEST_MAX_THREADS 16
#endif

static MTEST_THREAD_HANDLE threads[MTEST_MAX_THREADS];
/* access w/o a lock is broken, but "volatile" should help reduce the amount of
 * speculative loading/storing */
static volatile int nthreads = 0;

#if defined(THREAD_PACKAGE_NAME) && (THREAD_PACKAGE_NAME == THREAD_PACKAGE_WIN)
int MTest_Start_thread(MTEST_THREAD_RETURN_TYPE(*fn) (void *p), void *arg)
{
    int errs = 0;
    if (nthreads >= MTEST_MAX_THREADS) {
        fprintf(stderr, "Too many threads already created: max is %d\n", MTEST_MAX_THREADS);
        return 1;
    }
    threads[nthreads] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) fn, (LPVOID) arg, 0, NULL);
    if (threads[nthreads] == NULL) {
        return GetLastError();
    } else {
        nthreads++;
    }
    return MTestReturnValue(errs);
}

int MTest_Join_threads(void)
{
    int i, err = 0;
    for (i = 0; i < nthreads; i++) {
        if (threads[i] != INVALID_HANDLE_VALUE) {
            if (WaitForSingleObject(threads[i], INFINITE) == WAIT_FAILED) {
                err = GetLastError();
                fprintf(stderr, "Error WaitForSingleObject(), err = %d\n", err);
            } else {
                CloseHandle(threads[i]);
            }
        }
    }
    nthreads = 0;
    return err;
}

int MTest_thread_lock_create(MTEST_THREAD_LOCK_TYPE * lock)
{
    int errs = 0;
    if (lock == NULL)
        return -1;

    /* Create an unnamed uninheritable mutex */
    *lock = CreateMutex(NULL, FALSE, NULL);
    if (*lock == NULL)
        return -1;

    return MTestReturnValue(errs);
}

int MTest_thread_lock(MTEST_THREAD_LOCK_TYPE * lock)
{
    int errs = 0;
    if (lock == NULL)
        return -1;

    /* Wait infinitely for the mutex */
    if (WaitForSingleObject(*lock, INFINITE) != WAIT_OBJECT_0) {
        return -1;
    }
    return MTestReturnValue(errs);
}

int MTest_thread_unlock(MTEST_THREAD_LOCK_TYPE * lock)
{
    int errs = 0;
    if (lock == NULL)
        return -1;
    if (ReleaseMutex(*lock) == 0) {
        return -1;
    }
    return MTestReturnValue(errs);
}

int MTest_thread_lock_free(MTEST_THREAD_LOCK_TYPE * lock)
{
    int errs = 0;
    if (lock != NULL) {
        if (CloseHandle(*lock) == 0) {
            return -1;
        }
    }
    return MTestReturnValue(errs);
}

/* FIXME: We currently assume Solaris threads and Pthreads are interoperable.
 * We need to use Solaris threads explicitly to avoid potential interoperability issues.*/
#elif defined(THREAD_PACKAGE_NAME) && (THREAD_PACKAGE_NAME == THREAD_PACKAGE_POSIX ||   \
                                       THREAD_PACKAGE_NAME == THREAD_PACKAGE_SOLARIS)
int MTest_Start_thread(MTEST_THREAD_RETURN_TYPE(*fn) (void *p), void *arg)
{
    int err;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    if (nthreads >= MTEST_MAX_THREADS) {
        fprintf(stderr, "Too many threads already created: max is %d\n", MTEST_MAX_THREADS);
        return 1;
    }
    err = pthread_create(threads + nthreads, &attr, fn, arg);
    if (!err) {
        nthreads++;
    }
    pthread_attr_destroy(&attr);
    return err;
}

int MTest_Join_threads(void)
{
    int i, rc, err = 0;
    for (i = 0; i < nthreads; i++) {
        rc = pthread_join(threads[i], 0);
        if (rc)
            err = rc;
    }
    nthreads = 0;
    return err;
}

int MTest_thread_lock_create(MTEST_THREAD_LOCK_TYPE * lock)
{
    int err;
    err = pthread_mutex_init(lock, NULL);
    if (err) {
        perror("Failed to initialize lock:");
    }
    return err;
}

int MTest_thread_lock(MTEST_THREAD_LOCK_TYPE * lock)
{
    int err;
    err = pthread_mutex_lock(lock);
    if (err) {
        perror("Failed to acquire lock:");
    }
    return err;
}

int MTest_thread_unlock(MTEST_THREAD_LOCK_TYPE * lock)
{
    int err;
    err = pthread_mutex_unlock(lock);
    if (err) {
        perror("Failed to release lock:");
    }
    return err;
}

int MTest_thread_lock_free(MTEST_THREAD_LOCK_TYPE * lock)
{
    int err;
    err = pthread_mutex_destroy(lock);
    if (err) {
        perror("Failed to free lock:");
    }
    return err;
}

#elif defined(THREAD_PACKAGE_NAME) && (THREAD_PACKAGE_NAME == THREAD_PACKAGE_ARGOBOTS)

extern ABT_pool pools[MTEST_NUM_XSTREAMS];

int MTest_Start_thread(MTEST_THREAD_RETURN_TYPE(*fn) (void *p), void *arg)
{
    int ret;

    if (nthreads >= MTEST_MAX_THREADS) {
        fprintf(stderr, "Too many threads already created: max is %d\n", MTEST_MAX_THREADS);
        return 1;
    }
    /* We push threads to pools[0] and let the random work-stealing
     * scheduler balance things out. */
    ret = ABT_thread_create(pools[0], fn, arg, ABT_THREAD_ATTR_NULL, &threads[nthreads]);
    MTEST_ABT_ERROR(ret, "ABT_thread_create");

    nthreads++;

    return 0;
}

int MTest_Join_threads(void)
{
    int i, ret, err = 0;
    for (i = 0; i < nthreads; i++) {
        ret = ABT_thread_free(&threads[i]);
        MTEST_ABT_ERROR(ret, "ABT_thread_free");
    }
    nthreads = 0;
    return 0;
}

int MTest_thread_lock_create(MTEST_THREAD_LOCK_TYPE * lock)
{
    int ret;
    ret = ABT_mutex_create(lock);
    MTEST_ABT_ERROR(ret, "ABT_mutex_create");
    return 0;
}

int MTest_thread_lock(MTEST_THREAD_LOCK_TYPE * lock)
{
    int ret;
    ret = ABT_mutex_lock(*(lock));
    MTEST_ABT_ERROR(ret, "ABT_mutex_lock");
    return 0;
}

int MTest_thread_unlock(MTEST_THREAD_LOCK_TYPE * lock)
{
    int ret;
    ret = ABT_mutex_unlock(*(lock));
    MTEST_ABT_ERROR(ret, "ABT_mutex_unlock");
    return 0;
}

int MTest_thread_lock_free(MTEST_THREAD_LOCK_TYPE * lock)
{
    int ret;
    ret = ABT_mutex_free(lock);
    MTEST_ABT_ERROR(ret, "ABT_mutex_free");
    return 0;
}
#else
#error "thread package (THREAD_PACKAGE_NAME) not defined or unknown"
#endif

#if defined(THREAD_PACKAGE_NAME) && (THREAD_PACKAGE_NAME == THREAD_PACKAGE_POSIX ||     \
                                     THREAD_PACKAGE_NAME == THREAD_PACKAGE_SOLARIS) &&  \
                                     defined(HAVE_PTHREAD_BARRIER_INIT)
static MTEST_THREAD_LOCK_TYPE barrierLock;
static pthread_barrier_t barrier;
static int bcount = -1;
int MTest_thread_barrier_init(void)
{
    bcount = -1;        /* must reset to force barrier re-creation */
    return MTest_thread_lock_create(&barrierLock);
}

int MTest_thread_barrier_free(void)
{
    MTest_thread_lock_free(&barrierLock);
    return pthread_barrier_destroy(&barrier);
}

/* FIXME this barrier interface should be changed to more closely match the
 * pthread interface.  Specifically, nt should not be a barrier-time
 * parameter but an init-time parameter.  The double-checked locking below
 * isn't valid according to pthreads, and it isn't guaranteed to be robust
 * in the presence of aggressive CPU/compiler optimization. */
int MTest_thread_barrier(int nt)
{
    int err;
    if (nt < 0)
        nt = nthreads;
    if (bcount != nt) {
        /* One thread needs to initialize the barrier */
        MTest_thread_lock(&barrierLock);
        /* Test again in case another thread already fixed the problem */
        if (bcount != nt) {
            if (bcount > 0) {
                err = pthread_barrier_destroy(&barrier);
                if (err)
                    return err;
            }
            err = pthread_barrier_init(&barrier, NULL, nt);
            if (err)
                return err;
            bcount = nt;
        }
        err = MTest_thread_unlock(&barrierLock);
        if (err)
            return err;
    }
    return pthread_barrier_wait(&barrier);
}
#elif defined(THREAD_PACKAGE_NAME) && (THREAD_PACKAGE_NAME == THREAD_PACKAGE_ARGOBOTS)
static MTEST_THREAD_LOCK_TYPE barrierLock;
static ABT_barrier barrier;
static int bcount = -1;
int MTest_thread_barrier_init(void)
{
    bcount = -1;        /* must reset to force barrier re-creation */
    return MTest_thread_lock_create(&barrierLock);
}

int MTest_thread_barrier_free(void)
{
    int ret;
    MTest_thread_lock_free(&barrierLock);
    ret = ABT_barrier_free(&barrier);
    MTEST_ABT_ERROR(ret, "ABT_barrier_free");
    return 0;
}

/* FIXME this barrier interface should be changed to more closely match the
 * pthread interface.  Specifically, nt should not be a barrier-time
 * parameter but an init-time parameter.  The double-checked locking below
 * isn't valid according to pthreads, and it isn't guaranteed to be robust
 * in the presence of aggressive CPU/compiler optimization. */
int MTest_thread_barrier(int nt)
{
    int ret;
    if (nt < 0)
        nt = nthreads;
    if (bcount != nt) {
        /* One thread needs to initialize the barrier */
        ret = MTest_thread_lock(&barrierLock);
        /* Test again in case another thread already fixed the problem */
        if (bcount != nt) {
            if (bcount > 0) {
                ret = ABT_barrier_free(&barrier);
                MTEST_ABT_ERROR(ret, "ABT_barrier_free");
            }
            ret = ABT_barrier_create(nt, &barrier);
            MTEST_ABT_ERROR(ret, "ABT_barrier_create");
            bcount = nt;
        }
        ret = MTest_thread_unlock(&barrierLock);
    }
    ret = ABT_barrier_wait(barrier);
    MTEST_ABT_ERROR(ret, "ABT_barrier_wait");
    return 0;
}
#else
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

/* This is a generic barrier implementation.  To ensure that tests don't
   silently fail, this both prints an error message and returns an error
   result on any failure. */
int MTest_thread_barrier(int nt)
{
    volatile int *cntP;
    int err = 0;

    if (nt < 0)
        nt = nthreads;
    /* Force a write barrier by using lock/unlock */
    err = MTest_thread_lock(&barrierLock);
    if (err) {
        fprintf(stderr, "Lock failed in barrier!\n");
        return err;
    }
    cntP = &c[phase];
    err = MTest_thread_unlock(&barrierLock);
    if (err) {
        fprintf(stderr, "Unlock failed in barrier!\n");
        return err;
    }

    /* printf("[%d] cnt = %d, phase = %d\n", pthread_self(), *cntP, phase); */
    err = MTest_thread_lock(&barrierLock);
    if (err) {
        fprintf(stderr, "Lock failed in barrier!\n");
        return err;
    }
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
    err = MTest_thread_unlock(&barrierLock);
    if (err) {
        fprintf(stderr, "Unlock failed in barrier!\n");
        return err;
    }
    while (*cntP > 0);

    return err;
}
#endif /* Default barrier routine */


/* MTest_init_thread_pkg()
 * Initialize the threading package if needed. Argobots requires this
 * but Pthreads doesn't, for example. */

#if defined(THREAD_PACKAGE_NAME) && (THREAD_PACKAGE_NAME == THREAD_PACKAGE_ARGOBOTS)
static ABT_xstream xstreams[MTEST_NUM_XSTREAMS];
static ABT_sched scheds[MTEST_NUM_XSTREAMS];
ABT_pool pools[MTEST_NUM_XSTREAMS];

void MTest_init_thread_pkg(int argc, char **argv)
{
    int i, k, ret;
    int num_xstreams;

    ABT_init(argc, argv);

    /* Create pools */
    for (i = 0; i < MTEST_NUM_XSTREAMS; i++) {
        ret = ABT_pool_create_basic(ABT_POOL_FIFO, ABT_POOL_ACCESS_MPMC, ABT_TRUE, &pools[i]);
        MTEST_ABT_ERROR(ret, "ABT_pool_create_basic");
    }

    /* Create schedulers */
    ABT_pool my_pools[MTEST_NUM_XSTREAMS];
    num_xstreams = MTEST_NUM_XSTREAMS;
    for (i = 0; i < num_xstreams; i++) {
        for (k = 0; k < num_xstreams; k++) {
            my_pools[k] = pools[(i + k) % num_xstreams];
        }

        ret = ABT_sched_create_basic(ABT_SCHED_RANDWS, num_xstreams, my_pools,
                                     ABT_SCHED_CONFIG_NULL, &scheds[i]);
        MTEST_ABT_ERROR(ret, "ABT_sched_create_basic");
    }

    /* Create Execution Streams */
    ret = ABT_xstream_self(&xstreams[0]);
    MTEST_ABT_ERROR(ret, "ABT_xstream_self");
    ret = ABT_xstream_set_main_sched(xstreams[0], scheds[0]);
    MTEST_ABT_ERROR(ret, "ABT_xstream_set_main_sched");
    for (i = 1; i < num_xstreams; i++) {
        ret = ABT_xstream_create(scheds[i], &xstreams[i]);
        MTEST_ABT_ERROR(ret, "ABT_xstream_create");
    }
}

void MTest_finalize_thread_pkg()
{
    int i, ret;
    int num_xstreams = MTEST_NUM_XSTREAMS;

    for (i = 1; i < num_xstreams; i++) {
        ret = ABT_xstream_join(xstreams[i]);
        MTEST_ABT_ERROR(ret, "ABT_xstream_join");
        ret = ABT_xstream_free(&xstreams[i]);
        MTEST_ABT_ERROR(ret, "ABT_xstream_free");
    }

    ret = ABT_finalize();
    MTEST_ABT_ERROR(ret, "ABT_finalize");
}

#else
void MTest_init_thread_pkg(int argc, char **argv)
{
}

void MTest_finalize_thread_pkg()
{
}

#endif
