/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/************************************************/
/* Inline file to be included in mtest_thread.c */
/************************************************/

/* Error handling */

static void MTest_abt_error(int err, const char *msg, const char *file, int line)
{
    char *err_str;
    size_t len;
    int ret;

    if (err == ABT_SUCCESS)
        return;

    ret = ABT_error_get_str(err, NULL, &len);
    if (ret != ABT_SUCCESS) {
        fprintf(stderr, "Error %d at ABT_error_get_str\n", ret);
        exit(EXIT_FAILURE);
    }
    err_str = (char *) malloc(sizeof(char) * len + 1);
    if (ret != ABT_SUCCESS) {
        fprintf(stderr, "Error %d at malloc\n", ret);
        exit(EXIT_FAILURE);
    }
    ret = ABT_error_get_str(err, err_str, NULL);
    if (ret != ABT_SUCCESS) {
        fprintf(stderr, "Error %d at ABT_error_get_str\n", ret);
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "%s (%d): %s (%s:%d)\n", err_str, err, msg, file, line);

    free(err_str);

    exit(EXIT_FAILURE);
}

#define MTEST_ABT_ERROR(e,m) MTest_abt_error(e,m,__FILE__,__LINE__)

#define MTEST_NUM_XSTREAMS 2

/* -------------------------------------------- */
extern ABT_pool pools[MTEST_NUM_XSTREAMS];

int MTest_Start_thread(MTEST_THREAD_RETURN_TYPE(*fn) (void *p), void *arg)
{
    int ret;
    ABT_thread_attr thread_attr;

    if (nthreads >= MTEST_MAX_THREADS) {
        fprintf(stderr, "Too many threads already created: max is %d\n", MTEST_MAX_THREADS);
        return 1;
    }

    /* Make stack size large. */
    ret = ABT_thread_attr_create(&thread_attr);
    MTEST_ABT_ERROR(ret, "ABT_thread_attr_create");
    ret = ABT_thread_attr_set_stacksize(thread_attr, 2 * 1024 * 1024);
    MTEST_ABT_ERROR(ret, "ABT_thread_attr_set_stacksize");

    /* We push threads to pools[0] and let the random work-stealing
     * scheduler balance things out. */
    ret = ABT_thread_create(pools[0], fn, arg, thread_attr, &threads[nthreads]);
    MTEST_ABT_ERROR(ret, "ABT_thread_create");

    ret = ABT_thread_attr_free(&thread_attr);
    MTEST_ABT_ERROR(ret, "ABT_thread_attr_free");

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

int MTest_thread_yield(void)
{
    int ret;
    ret = ABT_thread_yield();
    MTEST_ABT_ERROR(ret, "ABT_thread_yield");
    return 0;
}

/* -------------------------------------------- */
#define HAVE_MTEST_THREAD_BARRIER 1

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

/* -------------------------------------------- */
#define HAVE_MTEST_INIT_THREAD_PKG

static ABT_xstream xstreams[MTEST_NUM_XSTREAMS];
static ABT_sched scheds[MTEST_NUM_XSTREAMS];
ABT_pool pools[MTEST_NUM_XSTREAMS];

void MTest_init_thread_pkg(void)
{
    int i, k, ret;
    int num_xstreams;

    ABT_init(0, NULL);

    /* Get the main pool of the primary execution stream.
     *
     * When MPI_Init_thread() is called before MTest_init_thread_pkg() e.g.
     * threads/spawn/th_taskmanager, so some threads (e.g., asynchronous
     * progress threads) can be associated with the current main pool of the
     * primary execution stream.  Since we cannot update the scheduler of the
     * primary execution stream in that case, the default scheduler and its pool
     * are used for the primary execution stream. */
    ret = ABT_xstream_self(&xstreams[0]);
    MTEST_ABT_ERROR(ret, "ABT_xstream_self");
    ret = ABT_xstream_get_main_pools(xstreams[0], 1, &pools[0]);
    MTEST_ABT_ERROR(ret, "ABT_xstream_get_main_pools");

    /* Create pools */
    for (i = 1; i < MTEST_NUM_XSTREAMS; i++) {
        ret = ABT_pool_create_basic(ABT_POOL_FIFO, ABT_POOL_ACCESS_MPMC, ABT_TRUE, &pools[i]);
        MTEST_ABT_ERROR(ret, "ABT_pool_create_basic");
    }

    /* Create schedulers */
    ABT_pool my_pools[MTEST_NUM_XSTREAMS];
    num_xstreams = MTEST_NUM_XSTREAMS;
    scheds[0] = ABT_SCHED_NULL;
    for (i = 1; i < num_xstreams; i++) {
        for (k = 0; k < num_xstreams; k++) {
            my_pools[k] = pools[(i + k) % num_xstreams];
        }

        ret = ABT_sched_create_basic(ABT_SCHED_RANDWS, num_xstreams, my_pools,
                                     ABT_SCHED_CONFIG_NULL, &scheds[i]);
        MTEST_ABT_ERROR(ret, "ABT_sched_create_basic");
    }

    /* Create Execution Streams */
    for (i = 1; i < num_xstreams; i++) {
        ret = ABT_xstream_create(scheds[i], &xstreams[i]);
        MTEST_ABT_ERROR(ret, "ABT_xstream_create");
    }
}

void MTest_finalize_thread_pkg(void)
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
