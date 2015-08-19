/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIU_THREAD_SOLARIS_H_INCLUDED
#define MPIU_THREAD_SOLARIS_H_INCLUDED

#include <thread.h>
#include <synch.h>

typedef mutex_t MPIU_Thread_mutex_t;
typedef cond_t MPIU_Thread_cond_t;
typedef thread_t MPIU_Thread_id_t;

typedef void (*MPIU_Thread_func_t) (void *data);
void MPIU_Thread_create(MPIU_Thread_func_t func, void *data, MPIU_Thread_id_t * id, int *err);

/*
 * Threads
 */

#define MPIU_Thread_exit()			\
    do {                                        \
        thr_exit(NULL);				\
    } while (0)

#define MPIU_Thread_self(id_ptr_)		\
    do {                                        \
        *(id_ptr_) = thr_self();                \
    } while (0)

#define MPIU_Thread_same(id1_ptr_, id2_ptr_, same_ptr_)                 \
    do {                                                                \
        *(same_ptr_) = (*(id1_ptr_) == *(id2_ptr_)) ? TRUE : FALSE;     \
    } while (0)

#define MPIU_Thread_yield(mutex_ptr_)                   \
    do {                                                \
        int err;                                        \
        MPIU_Thread_mutex_unlock(mutex_ptr_, &err);     \
        thr_yield();                                    \
        MPIU_Thread_mutex_lock(mutex_ptr_, &err);       \
    } while (0)


/*
 *    Mutexes
 */

#define MPIU_Thread_mutex_create(mutex_ptr_, err_ptr_)	\
    do {                                                \
        *(mutex_ptr_) = DEFAULTMUTEX;			\
        if ((err_ptr_) == NULL) {                       \
            *(err_ptr_) = MPIU_THREAD_SUCCESS;		\
        }                                               \
    } while (0)

#define MPIU_Thread_mutex_destroy(mutex_ptr_, err_ptr_)                 \
    do {                                                                \
        if ((err_ptr_) == NULL) {                                       \
            mutex_destroy(mutex_ptr_);                                  \
        }								\
        else {                                                          \
            *(err_ptr_) = mutex_destroy(mutex_ptr_);                    \
            /* FIXME: convert error to an MPIU_THREAD_ERR value */	\
        }								\
    } while (0)

#define MPIU_Thread_mutex_lock(mutex_ptr_, err_ptr_)                    \
    do {                                                                \
        if ((err_ptr_) == NULL) {                                       \
            mutex_lock(mutex_ptr_);					\
        }								\
        else {                                                          \
            *(err_ptr_) = mutex_lock(mutex_ptr_);			\
            /* FIXME: convert error to an MPIU_THREAD_ERR value */	\
        }								\
    } while (0)

#define MPIU_Thread_mutex_unlock(mutex_ptr_, err_ptr_)                  \
    do {                                                                \
        if ((err_ptr_) == NULL) {                                       \
            mutex_unlock(mutex_ptr_);                                   \
        }								\
        else {                                                          \
            *(err_ptr_) = mutex_unlock(mutex_ptr_);			\
            /* FIXME: convert error to an MPIU_THREAD_ERR value */	\
        }								\
    } while (0)


/*
 * Condition Variables
 */

#define MPIU_Thread_cond_create(cond_ptr_, err_ptr_)	\
    do {                                                \
        *(cond_ptr_) = DEFAULTCV;                       \
        if ((err_ptr_) == NULL) {                       \
            *(err_ptr_) = MPIU_THREAD_SUCCESS;		\
        }                                               \
    } while (0)

#define MPIU_Thread_cond_destroy(cond_ptr_, err_ptr_)                   \
    do {                                                                \
        if ((err_ptr_) == NULL) {                                       \
            cond_destroy(cond_ptr_);                                    \
        }								\
        else {                                                          \
            *(err_ptr_) = cond_destroy(cond_ptr_);			\
            /* FIXME: convert error to a MPIU_THREAD_ERR value */	\
        }								\
    } while (0)

#define MPIU_Thread_cond_wait(cond_ptr_, mutex_ptr_, err_ptr_)          \
    do {                                                                \
        if ((err_ptr_) == NULL) {                                       \
            cond_wait((cond_ptr_), (mutex_ptr_));			\
        }								\
        else {                                                          \
            *(err_ptr_) = cond_wait((cond_ptr_), (mutex_ptr_));         \
            /* FIXME: convert error to a MPIU_THREAD_ERR value */	\
        }								\
    } while (0)

#define MPIU_Thread_cond_broadcast(cond_ptr_, err_ptr_)                 \
    do {                                                                \
        if ((err_ptr_) == NULL) {                                       \
            cond_broadcast(cond_ptr_);                                  \
        }								\
        else {                                                          \
            *(err_ptr_) = cond_broadcast(cond_ptr_);                    \
            /* FIXME: convert error to a MPIU_THREAD_ERR value */	\
        }								\
    } while (0)

#define MPIU_Thread_cond_signal(cond_ptr_, err_ptr_)                    \
    do {                                                                \
        if ((err_ptr_) == NULL) {                                       \
            cond_signal(cond_ptr_);					\
        }								\
        else {                                                          \
            *(err_ptr_) = cond_signal(cond_ptr_);			\
            /* FIXME: convert error to a MPIU_THREAD_ERR value */	\
        }								\
    } while (0)


/*
 * Thread Local Storage
 */
#define MPIU_Thread_tls_create(exit_func_ptr_, tls_ptr_, err_ptr_)	\
    do {                                                                \
        if ((err_ptr_) == NULL) {                                       \
            thr_keycreate((tls_ptr), (exit_func_ptr));			\
        }                                                               \
        else {								\
            *(err_ptr_) = thr_keycreate((tls_ptr), (exit_func_ptr));	\
            /* FIXME: convert error to a MPIU_THREAD_ERR value */       \
        }                                                               \
    } while (0)

#define MPIU_Thread_tls_destroy(tls_ptr_, err_ptr_)                     \
    do {                                                                \
        /*                                                              \
         * FIXME: Solaris threads does not have a key destroy.  We      \
         * need to create equivalent functionality to prevent a         \
         * callback from occuring when a thread exits after the TLS is  \
         * destroyed.  This is the only way to prevent subsystems that  \
         * have shutdown from continuing to receive callbacks.          \
         */                                                             \
        if ((err_ptr_) != NULL) {                                       \
            *(err_ptr) = MPIU_THREAD_SUCCESS;                           \
        }                                                               \
    } while (0)

#define MPIU_Thread_tls_set(tls_ptr, value_)                            \
    do {                                                                \
        if ((err_ptr_) == NULL) {                                       \
            thr_setspecific(*(tls_ptr), (value_));			\
        }								\
        else {                                                          \
            *(err_ptr_) = thr_setspecific(*(tls_ptr), (value_));	\
            /* FIXME: convert error to a MPIU_THREAD_ERR value */	\
        }								\
    } while (0)

#define MPIU_Thread_tls_get(tls_ptr, value_ptr_)                        \
    do {                                                                \
        if ((err_ptr_) == NULL) {                                       \
            thr_setspecific(*(tls_ptr), (value_ptr_));			\
        }                                                               \
        else {								\
            *(err_ptr_) = thr_setspecific(*(tls_ptr), (value_ptr_));	\
            /* FIXME: convert error to a MPIU_THREAD_ERR value */       \
        }                                                               \
    } while (0)

#endif /* MPIU_THREAD_SOLARIS_H_INCLUDED */
