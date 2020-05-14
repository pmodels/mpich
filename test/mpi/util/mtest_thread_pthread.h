/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/************************************************/
/* Inline file to be included in mtest_thread.c */
/************************************************/

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

int MTest_thread_yield(void)
{
    return 0;
}

/*----------------------------------------------------------------*/
#if defined(HAVE_PTHREAD_BARRIER_INIT)

#define HAVE_MTEST_THREAD_BARRIER 1

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
#endif /* HAVE_PTHREAD_BARRIER_INIT */
