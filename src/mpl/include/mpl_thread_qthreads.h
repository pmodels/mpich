/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* This file is used when configured with (MPICH_THREAD_PACKAGE_NAME ==
 * MPICH_THREAD_PACKAGE_QTHREADS) */

#ifndef MPL_THREAD_QTHREADS_H_INCLUDED
#define MPL_THREAD_QTHREADS_H_INCLUDED

#include "mpl.h"
#include "qthread.h"
#include "qthread/tls.h"

#include <errno.h>
#include <assert.h>

typedef qthread_spinlock_t MPL_thread_mutex_t;
typedef qthread_key_t MPL_thread_tls_key_t;
typedef aligned_t qthread_t;
typedef qthread_t MPL_thread_id_t;

/* ======================================================================
 *    Conditional type
 * ======================================================================*/

typedef struct qthread_cond_waiter_t {
    int m_signaled;
    struct qthread_cond_waiter_t *m_prev;
} qthread_cond_waiter_t;

typedef struct {
    MPL_thread_mutex_t m_lock;
    qthread_cond_waiter_t *m_waiter_head;
    qthread_cond_waiter_t *m_waiter_tail;
} qthread_cond_t;

typedef qthread_cond_t MPL_thread_cond_t;

/* ======================================================================
 *    Creation and misc
 * ======================================================================*/

/* MPL_thread_init()/MPL_thread_finalize() can be called in a nested manner
 * (e.g., MPI_T_init_thread() and MPI_Init_thread()), but that is ok. */
#define MPL_thread_init(err_ptr_)                                             \
    do {                                                                      \
        int err__;                                                            \
        err__ = qthread_initialize();                                         \
        if (unlikely(err__))                                                  \
            MPL_internal_sys_error_printf("qthread_initialize", err__,        \
                                          "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                           \
    } while (0)

#define MPL_thread_finalize(err_ptr_)                                         \
    do {                                                                      \
        int err__;                                                            \
        qthread_finalize(); /*void function*/                                 \
        err__ = QTHREAD_SUCCESS;                                              \
        if (unlikely(err__))                                                  \
            MPL_internal_sys_error_printf("qthread_finalize", err__,          \
                                          "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                           \
    } while (0)

/* MPL_thread_create() defined in mpl_thread_qthreads.c */
//typedef long unsigned int (*MPL_thread_func_t) (void *data);
typedef void (*MPL_thread_func_t) (void *data);
void MPL_thread_create(MPL_thread_func_t func, void *data, MPL_thread_id_t * idp, int *errp);

#define MPL_thread_exit()
#define MPL_thread_self(idp_)                                                 \
    do {                                                                      \
        qthread_t self_thread_tmp_ = QTHREAD_NON_TASK_ID;                     \
        self_thread_tmp_ = (qthread_t) qthread_retloc();                      \
        uintptr_t id_tmp_;                                                    \
        if (self_thread_tmp_ == QTHREAD_NON_TASK_ID) {                        \
            /* It seems that an external thread calls this function. */       \
            /* Use Pthreads ID instead. */                                    \
            id_tmp_ = (uintptr_t)pthread_self();                              \
            /* Set a bit to the last bit.                                     \
             * Note that the following shifts bits first because pthread_t    \
             * might use the last bit if, for example, pthread_t saves an ID  \
             * starting from zero; overwriting the last bit can cause a       \
             * conflict.  The last bit that is shifted out is less likely to  \
             * be significant. */                                             \
            id_tmp_ = (id_tmp_ << 1) | (uintptr_t)0x1;                        \
        } else {                                                              \
            id_tmp_ = (uintptr_t)self_thread_tmp_;                            \
            /* If ID is that of Qthreads, the last bit is not set because     \
             * qthread_t points to an aligned memory region.  Since           \
             * qthread_t is not modified, this ID can be directly used by     \
             * MPL_thread_join().  Let's check it. */                         \
            assert(!(id_tmp_ & (uintptr_t)0x1));                              \
        }                                                                     \
        *(idp_) = id_tmp_;                                                    \
    } while (0)
#define MPL_thread_join(id_)
#define MPL_thread_same(idp1_, idp2_, same_)                                  \
    do {                                                                      \
        /*                                                                    \
         * TODO: strictly speaking, Pthread-Pthread and Pthread-Qthreads IDs  \
         * are not arithmetically comparable, while it is okay on most        \
         * platforms.  This should be fixed.  Note that Qthreads-Qthreads ID  \
         * comparison is okay.                                                \
         */                                                                   \
        *(same_) = (*(idp1_) == *(idp2_)) ? TRUE : FALSE;                     \
    } while (0)

/* See mpl_thread_posix.h for interface description. */
void MPL_thread_set_affinity(MPL_thread_id_t thread, int *affinity_arr, int affinity_size,
                             int *err);


/* ======================================================================
 *    Scheduling
 * ======================================================================*/

#define MPL_thread_yield qthread_yield

/* ======================================================================
 *    Mutexes
 * ======================================================================*/
#define MPL_thread_mutex_create(mutex_ptr_, err_ptr_)                         \
    do {                                                                      \
        int err__;                                                            \
        err__ = qthread_spinlock_init(mutex_ptr_, false);                     \
        if (unlikely(err__))                                                  \
            MPL_internal_sys_error_printf("qthread_spinlock_init", err__,     \
                                          "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                           \
    } while (0)

#define MPL_thread_mutex_destroy(mutex_ptr_, err_ptr_)                        \
    do {                                                                      \
        int err__;                                                            \
        err__ = qthread_spinlock_destroy(mutex_ptr_);                         \
        if (unlikely(err__))                                                  \
            MPL_internal_sys_error_printf("qthread_spinlock_destroy", err__,  \
                                          "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                           \
    } while (0)

#define MPL_thread_mutex_lock(mutex_ptr_, err_ptr_, prio_)                    \
    do {                                                                      \
        int err__;                                                            \
        err__ = qthread_spinlock_lock(mutex_ptr_);                            \
        if (unlikely(err__))                                                  \
            MPL_internal_sys_error_printf("qthread_spinlock_lock", err__,     \
                                          "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                           \
    } while (0)

#define MPL_thread_mutex_unlock(mutex_ptr_, err_ptr_)                         \
    do {                                                                      \
        int err__;                                                            \
        err__ = qthread_spinlock_unlock(mutex_ptr_);                          \
        if (unlikely(err__))                                                  \
            MPL_internal_sys_error_printf("qthread_spinlock_unlock", err__,   \
                                          "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                           \
    } while (0)

#define MPL_thread_mutex_unlock_se(mutex_ptr_, err_ptr_)                      \
    MPL_thread_mutex_unlock(mutex_ptr_, err_ptr_)


/* ======================================================================
 *    Implementation of conditionals
 * ======================================================================*/

static inline int qthread_cond_init(MPL_thread_cond_t *p_cond)
{
    qthread_spinlock_init(&p_cond->m_lock, false /* is_recursive */);
    p_cond->m_waiter_head = NULL;
    p_cond->m_waiter_tail = NULL;
    return QTHREAD_SUCCESS;
}

static inline void qthread_cond_wait(MPL_thread_cond_t *p_cond,
                                                  MPL_thread_mutex_t *p_mutex)
{
    /* This thread is taking "lock", so only this thread can access this
     * condition variable.  */
    qthread_spinlock_lock(&p_cond->m_lock);
    qthread_cond_waiter_t waiter = {0, NULL};
    if (NULL == p_cond->m_waiter_head) {
        p_cond->m_waiter_tail = &waiter;
    } else {
        p_cond->m_waiter_head->m_prev = &waiter;
    }
    p_cond->m_waiter_head = &waiter;
    qthread_spinlock_unlock(&p_cond->m_lock);
    while (1) {
        qthread_spinlock_unlock(p_mutex);
        qthread_yield();
        qthread_spinlock_lock(p_mutex);
        /* Check if someone woke me up. */
        qthread_spinlock_lock(&p_cond->m_lock);
        int signaled = waiter.m_signaled;
        qthread_spinlock_unlock(&p_cond->m_lock);
        if (1 == signaled) {
            break;
        }
        /* Unlock the lock again. */
    }
}

static inline void qthread_cond_broadcast(MPL_thread_cond_t *p_cond)
{
    qthread_spinlock_lock(&p_cond->m_lock);
    while (NULL != p_cond->m_waiter_tail) {
        qthread_cond_waiter_t *p_cur_tail = p_cond->m_waiter_tail;
        p_cond->m_waiter_tail = p_cur_tail->m_prev;
        /* Awaken one of threads in a FIFO manner. */
        p_cur_tail->m_signaled = 1;
    }
    /* No waiters. */
    p_cond->m_waiter_head = NULL;
    qthread_spinlock_unlock(&p_cond->m_lock);
}

static inline void qthread_cond_signal(MPL_thread_cond_t *p_cond)
{
    qthread_spinlock_lock(&p_cond->m_lock);
    if (NULL != p_cond->m_waiter_tail) {
        qthread_cond_waiter_t *p_cur_tail = p_cond->m_waiter_tail;
        p_cond->m_waiter_tail = p_cur_tail->m_prev;
        /* Awaken one of threads. */
        p_cur_tail->m_signaled = 1;
        if (NULL == p_cond->m_waiter_tail) {
            p_cond->m_waiter_head = NULL;
        }
    }
    qthread_spinlock_unlock(&p_cond->m_lock);
}

static inline void qthread_cond_destroy(MPL_thread_cond_t *p_cond)
{
    /* No destructor is needed. */
}


/* ======================================================================
 *    Condition Variables
 * ======================================================================*/

#define MPL_thread_cond_create(cond_ptr_, err_ptr_)                           \
    do {                                                                      \
        int err__;                                                            \
        err__ = qthread_cond_init((cond_ptr_));                               \
        if (unlikely(err__))                                                  \
            MPL_internal_sys_error_printf("qthread_cond_init", err__,         \
                                          "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                           \
    } while (0)

#define MPL_thread_cond_destroy(cond_ptr_, err_ptr_)                          \
    do {                                                                      \
        int err__;                                                            \
        err__ = qthread_cond_destroy(cond_ptr_);                              \
        if (unlikely(err__))                                                  \
            MPL_internal_sys_error_printf("qthread_cond_destroy", err__,      \
                                          "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                           \
    } while (0)

#define MPL_thread_cond_wait(cond_ptr_, mutex_ptr_, err_ptr_)                   \
    do {                                                                        \
        int err__;                                                              \
        MPL_DBG_MSG_FMT(THREAD,TYPICAL,                                         \
                        (MPL_DBG_FDEST,                                         \
                         "Enter cond_wait on cond=%p mutex=%p",                 \
                         (cond_ptr_),(mutex_ptr_)));                            \
        do {                                                                    \
            err__ = qthread_cond_wait((*cond_ptr_), *mutex_ptr_);               \
        } while (err__ == EINTR);                                               \
        *(int *)(err_ptr_) = err__;                                             \
        if (unlikely(err__))                                                    \
            MPL_internal_sys_error_printf("qthread_cond_wait", err__,           \
                   "    %s:%d error in cond_wait on cond=%p mutex=%p err__=%d", \
                   __FILE__, __LINE__);       \
        MPL_DBG_MSG_FMT(THREAD,TYPICAL,(MPL_DBG_FDEST,                          \
                                        "Exit cond_wait on cond=%p mutex=%p",   \
                                        (cond_ptr_),(mutex_ptr_)));             \
    } while (0)

#define MPL_thread_cond_broadcast(cond_ptr_, err_ptr_)                        \
    do {                                                                      \
        int err__;                                                            \
        MPL_DBG_MSG_P(THREAD,TYPICAL,                                         \
                      "About to cond_broadcast on MPL_thread_cond %p",        \
                      (cond_ptr_));                                           \
        err__ = qthread_cond_broadcast((*cond_ptr_));                         \
        if (unlikely(err__))                                                  \
            MPL_internal_sys_error_printf("qthread_cond_broadcast", err__,    \
                                          "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                           \
    } while (0)

#define MPL_thread_cond_signal(cond_ptr_, err_ptr_)                           \
    do {                                                                      \
        int err__;                                                            \
        MPL_DBG_MSG_P(THREAD,TYPICAL,                                         \
                      "About to cond_signal on MPL_thread_cond %p",           \
                      (cond_ptr_));                                           \
        err__ = qthread_cond_signal((*cond_ptr_));                            \
        if (unlikely(err__))                                                  \
            MPL_internal_sys_error_printf("qthread_cond_signal", err__,       \
                                          "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                           \
    } while (0)

/* ======================================================================
 *    Thread Local Storage
 * ======================================================================*/

#define MPL_NO_COMPILER_TLS     /* Cannot use compiler tls with qthreads */

#define MPL_thread_tls_create(exit_func_ptr_, tls_ptr_, err_ptr_)         \
    do {                                                                  \
        int err__;                                                        \
        err__ = qthread_key_create((exit_func_ptr_), (tls_ptr_));         \
        if (unlikely(err__))                                              \
        MPL_internal_sys_error_printf("qthread_key_create", err__,        \
                                      "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = 0;                                           \
    } while (0)

#define MPL_thread_tls_destroy(tls_ptr_, err_ptr_)                        \
    do {                                                                  \
        int err__;                                                        \
        err__ = qthread_key_delete(tls_ptr_);                             \
        if (unlikely(err__))                                              \
        MPL_internal_sys_error_printf("qthread_key_delete", err__,        \
                                      "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                       \
    } while (0)

#define MPL_thread_tls_set(tls_ptr_, value_, err_ptr_)                    \
    do {                                                                  \
        int err__;                                                        \
        err__ = qthread_setspecific(*(tls_ptr_), (value_));               \
        if (unlikely(err__))                                              \
        MPL_internal_sys_error_printf("qthread_setspecific", err__,       \
                                      "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                       \
    } while (0)

#define MPL_thread_tls_get(tls_ptr_, value_ptr_, err_ptr_)                \
    do {                                                                  \
        int err__;                                                        \
        err__ = qthread_getspecific(*(tls_ptr_), (value_ptr_));           \
        if (unlikely(err__))                                              \
        MPL_internal_sys_error_printf("qthread_getspecific", err__,       \
                                      "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                       \
    } while (0)

#endif /* MPL_THREAD_QTHREADS_H_INCLUDED */
