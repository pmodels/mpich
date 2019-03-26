/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This file is used when configured with (MPICH_THREAD_PACKAGE_NAME ==
 * MPICH_THREAD_PACKAGE_ARGOBOTS) */

#ifndef MPL_THREAD_ARGOBOTS_H_INCLUDED
#define MPL_THREAD_ARGOBOTS_H_INCLUDED

#include "mpl.h"
#include "qt_asserts.h"
#include "qthread/qthread.h"
#include "qthread/tls.h"
#include "qt_atomics.h"

#include <errno.h>
#include <assert.h>
#include <pthread.h>

typedef QTHREAD_FASTLOCK_TYPE MPL_thread_mutex_t;
/* FIXME(nevans): qthreads has a weird way of initializing cond variables */
typedef pthread_cond_t MPL_thread_cond_t;
typedef int MPL_thread_id_t;
typedef qthread_key_t MPL_thread_tls_t;

/* ======================================================================
 *    Creation and misc
 * ======================================================================*/

/* MPL_thread_create() defined in mpiu_thread_argobots.c */
typedef void (*MPL_thread_func_t) (void *data);
void MPL_thread_create(MPL_thread_func_t func, void *data, MPL_thread_id_t * idp, int *errp);

#define MPL_thread_exit()
#define MPL_thread_self(id_)
#define MPL_thread_same(id1_, id2_, same_)

/* ======================================================================
 *    Scheduling
 * ======================================================================*/

#define MPL_thread_yield MPL_sched_yield

/* ======================================================================
 *    Mutexes
 * ======================================================================*/
#define MPL_thread_mutex_create(mutex_ptr_, err_ptr_) QTHREAD_FASTLOCK_INIT_PTR(mutex_ptr_)                     

#define MPL_thread_mutex_destroy(mutex_ptr_, err_ptr_) QTHREAD_FASTLOCK_DESTROY_PTR((mutex_ptr_))

/* TODO(nevans) Need to see if we can do something about priority */

#define MPL_thread_mutex_lock(mutex_ptr_, err_ptr_, prio_) QTHREAD_FASTLOCK_LOCK((mutex_ptr_))                    
#define MPL_thread_mutex_trylock(mutex_ptr_, err_ptr_, cs_acq_ptr) QTHREAD_TRYLOCK_LOCK((mutex_ptr_))             

#define MPL_thread_mutex_unlock(mutex_ptr_, err_ptr_) QTHREAD_FASTLOCK_UNLOCK((mutex_ptr_))                        

/* TODO(nevans): figure out the analog in qthreads */
#define MPL_thread_mutex_unlock_se(mutex_ptr_, err_ptr_)  QTHREAD_FASTLOCK_UNLOCK((mutex_ptr_))                   

/* ======================================================================
 *    Condition Variables
 * ======================================================================*/

#define MPL_thread_cond_create(cond_ptr_, err_ptr_)        QTHREAD_COND_INIT_SOLO_PTR((cond_ptr_))

#define MPL_thread_cond_destroy(cond_ptr_, err_ptr_)  QTHREAD_DESTROYCOND((cond_ptr_))

#define MPL_thread_cond_wait(cond_ptr_, mutex_ptr_, err_ptr_)  QTHREAD_COND_WAIT_DUO(*(cond_ptr_), *(mutex_ptr_))

#define MPL_thread_cond_broadcast(cond_ptr_, err_ptr_) QTHREAD_COND_BCAST(*(cond_ptr_))                          

#define MPL_thread_cond_signal(cond_ptr_, err_ptr_) QTHREAD_COND_SIGNAL((*(cond_ptr_)))

/* ======================================================================
 *    Thread Local Storage
 * ======================================================================*/

#define MPL_thread_tls_create(exit_func_ptr_, tls_ptr_, err_ptr_)         \
    do {                                                                  \
        int err__;                                                        \
        err__ = qthread_key_create((tls_ptr_), (exit_func_ptr_));             \
        if (unlikely(err__))                                              \
        MPL_internal_sys_error_printf("qthread_key_create", err__,            \
                                      "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = 0;                                           \
    } while (0)

#define MPL_thread_tls_destroy(tls_ptr_, err_ptr_)                        \
    do {                                                                  \
        int err__;                                                        \
        err__ = qthread_key_delete(*(tls_ptr_));                                   \
        if (unlikely(err__))                                              \
        MPL_internal_sys_error_printf("qthread_key_free", err__,              \
                                      "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                       \
    } while (0)

#define MPL_thread_tls_set(tls_ptr_, value_, err_ptr_)                    \
    do {                                                                  \
        int err__;                                                        \
        err__ = qthread_setspecific(*(tls_ptr_), *(void **)(value_));                       \
        if (unlikely(err__))                                              \
        MPL_internal_sys_error_printf("qthread_setspecific", err__,               \
                                      "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                       \
    } while (0)

#define MPL_thread_tls_get(tls_ptr_, value_ptr_, err_ptr_)                \
    do {                                                                  \
        int err__;                                                        \
        *(void **)(value_ptr_) = qthread_getspecific(*(tls_ptr_));                   \
        if (unlikely(value_ptr_ == NULL))                                              \
        MPL_internal_sys_error_printf("qthread_getspecific", err__,               \
                                      "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                       \
    } while (0)

#endif /* MPL_THREAD_ARGOBOTS_H_INCLUDED */
