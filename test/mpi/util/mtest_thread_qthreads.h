/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/************************************************/
/* Inline file to be included in mtest_thread.c */
/************************************************/

/* Error handling */

#include <qthread/barrier.h>

typedef long unsigned int (*Qthreads_thread_func_t) (void *data);

static void MTest_qthreads_error(int err, const char *msg, const char *file, int line)
{
    if (err == QTHREAD_SUCCESS)
        return;
    fprintf(stderr, "%s (%d) (%s:%d)\n", msg, err, file, line);
    exit(EXIT_FAILURE);
}

#define MTEST_QTHR_ERROR(e,m) MTest_qthreads_error(e,m,__FILE__,__LINE__)

/* -------------------------------------------- */

int MTest_Start_thread(MTEST_THREAD_RETURN_TYPE(*fn) (void *p), void *arg)
{
    if (nthreads >= MTEST_MAX_THREADS) {
        fprintf(stderr, "Too many threads already created: max is %d\n", MTEST_MAX_THREADS);
        return 1;
    }

    Qthreads_thread_func_t f = (Qthreads_thread_func_t) fn;
    int ret = qthread_fork(f, arg, &threads[nthreads]);
    MTEST_QTHR_ERROR(ret, "qthread_fork");
    nthreads++;

    return 0;
}

int MTest_Join_threads(void)
{
    int i, ret, err = 0;
    for (i = 0; i < nthreads; i++) {
        ret = qthread_readFE(&threads[i], NULL);
        MTEST_QTHR_ERROR(ret, "qthread_readFE");
    }
    
    nthreads = 0;
    return 0;
}

int MTest_thread_lock_create(MTEST_THREAD_LOCK_TYPE * lock)
{
    return 0;
}

int MTest_thread_lock_free(MTEST_THREAD_LOCK_TYPE * lock)
{
    return 0;
}

int MTest_thread_lock(MTEST_THREAD_LOCK_TYPE * lock)
{
    int ret;
    ret  =  qthread_lock(lock);
    MTEST_QTHR_ERROR(ret, "qthread_lock");
    return 0;
}

int MTest_thread_unlock(MTEST_THREAD_LOCK_TYPE * lock)
{
    int ret;
    ret  =  qthread_unlock(lock);
    MTEST_QTHR_ERROR(ret, "qthread_unlock");
    return 0;
}

int MTest_thread_yield(void)
{
    qthread_yield();
    return 0;
}

/* -------------------------------------------- */
#define HAVE_MTEST_THREAD_BARRIER 1

qt_barrier_t * qt_barrier;

int MTest_thread_barrier_init(void)
{
    qt_barrier = qt_barrier_create(nthreads, REGION_BARRIER);
    return 0;
}

int MTest_thread_barrier_free(void)
{
    qt_barrier_destroy(qt_barrier);
    return 0;
}

int MTest_thread_barrier(int nt)
{
    qt_barrier_enter(qt_barrier);
    return 0;
}

#define HAVE_MTEST_INIT_THREAD_PKG

void MTest_init_thread_pkg(void)
{
    int ret;
    ret = qthread_initialize();
    MTEST_QTHR_ERROR(ret, "qthread_initialize");
}

void MTest_finalize_thread_pkg(void)
{
    qthread_finalize();
}
