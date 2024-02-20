/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* common header includes */
#include "mpl.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#if defined(MPL_THREAD_PACKAGE_NAME) && (MPL_THREAD_PACKAGE_NAME == MPL_THREAD_PACKAGE_QTHREADS)

typedef long unsigned int (*Qthreads_thread_func_t) (void *data);

typedef struct {
    MPL_thread_func_t func;
    void *data;
} thread_info;

static void thread_start(void *arg)
{
    thread_info *info = (thread_info *) arg;
    MPL_thread_func_t func = info->func;
    void *data = info->data;
    func(data);
}

static int thread_create(MPL_thread_func_t func, void *data, MPL_thread_id_t * idp)
{
    if (qthread_initialize() != QTHREAD_SUCCESS) {
        /* Not initialized yet. */
        return MPL_ERR_THREAD;
    }
    Qthreads_thread_func_t f =  (Qthreads_thread_func_t) func;
    int err = qthread_fork(f, data, idp); 
    return (err == QTHREAD_SUCCESS) ? MPL_SUCCESS : MPL_ERR_THREAD;
}

/*
 * MPL_thread_create()
 */
void MPL_thread_create(MPL_thread_func_t func, void *data, MPL_thread_id_t * idp, int *errp)
{
    int err = thread_create(func, data, idp);

    if (errp != NULL) {
        *errp = err;
    }
}

void MPL_thread_set_affinity(MPL_thread_id_t thread, int *affinity_arr, int affinity_size, int *err)
{
    /* stub implementation */
    if (err)
        *err = MPL_ERR_THREAD;
}

#endif
