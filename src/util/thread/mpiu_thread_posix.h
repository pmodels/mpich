/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * Threads
 */
#ifndef MPIU_THREAD_POSIX_H_INCLUDED
#define MPIU_THREAD_POSIX_H_INCLUDED

#include "mpiu_process_wrappers.h"      /* for MPIU_PW_Sched_yield */

#include <errno.h>
#include <pthread.h>
#include "opa_primitives.h"

typedef struct {
    pthread_mutex_t mutex;
    OPA_int_t num_queued_threads;
} MPIU_Thread_mutex_t;
typedef pthread_cond_t MPIU_Thread_cond_t;
typedef pthread_t MPIU_Thread_id_t;
typedef pthread_key_t MPIU_Thread_tls_t;

#if defined(NEEDS_PTHREAD_MUTEXATTR_SETTYPE_DECL)
int pthread_mutexattr_settype(pthread_mutexattr_t * attr, int kind);
#endif /* NEEDS_PTHREAD_MUTEXATTR_SETTYPE_DECL */

typedef void (*MPIU_Thread_func_t) (void *data);
void MPIU_Thread_create(MPIU_Thread_func_t func, void *data, MPIU_Thread_id_t * id, int *err);

#define MPIU_Thread_exit()			\
    do {                                        \
        pthread_exit(NULL);                     \
    } while (0)

#define MPIU_Thread_self(id_)			\
    do {                                        \
        *(id_) = pthread_self();                \
    } while (0)

#define MPIU_Thread_same(id1_, id2_, same_)                             \
    do {                                                                \
        *(same_) = pthread_equal(*(id1_), *(id2_)) ? TRUE : FALSE;	\
    } while (0)

#define MPIU_Thread_yield(mutex_ptr_)                                   \
    do {                                                                \
        int err;                                                        \
        MPIU_DBG_MSG(THREAD,VERBOSE,"enter MPIU_Thread_yield");         \
        if (OPA_load_int(&(mutex_ptr_)->num_queued_threads) == 0)       \
            break;                                                      \
        MPIU_Thread_mutex_unlock(mutex_ptr_, &err);                     \
        MPIU_PW_Sched_yield();                                          \
        MPIU_Thread_mutex_lock(mutex_ptr_, &err);                       \
        MPIU_DBG_MSG(THREAD,VERBOSE,"exit MPIU_Thread_yield");          \
    } while (0)


/*
 *    Mutexes
 */

/* FIXME: mutex creation and destruction should be implemented as routines
   because there is no reason to use macros (these are not on the performance
   critical path).  Making these macros requires that any code that might use
   these must load all of the pthread.h (or other thread library) support.
 */

/* FIXME: using constant initializer if available */

/* FIXME: convert errors to an MPIU_THREAD_ERR value */

#if !defined(MPICH_PTHREAD_MUTEX_ERRORCHECK_VALUE)

#define MPIU_Thread_mutex_create(mutex_ptr_, err_ptr_)                  \
    do {                                                                \
        int err__;                                                      \
                                                                        \
        OPA_store_int(&(mutex_ptr_)->num_queued_threads, 0);            \
        err__ = pthread_mutex_init(&(mutex_ptr_)->mutex, NULL);         \
        if (unlikely(err__))                                            \
            MPL_internal_sys_error_printf("pthread_mutex_init", err__,  \
                                          "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                     \
        MPIU_DBG_MSG_P(THREAD,TYPICAL,"Created MPIU_Thread_mutex %p", (mutex_ptr_)); \
    } while (0)

#else /* defined(MPICH_PTHREAD_MUTEX_ERRORCHECK_VALUE) */

#define MPIU_Thread_mutex_create(mutex_ptr_, err_ptr_)                  \
    do {                                                                \
        int err__;                                                      \
        pthread_mutexattr_t attr__;                                     \
                                                                        \
        OPA_store_int(&(mutex_ptr_)->num_queued_threads, 0);            \
        pthread_mutexattr_init(&attr__);                                \
        pthread_mutexattr_settype(&attr__, MPICH_PTHREAD_MUTEX_ERRORCHECK_VALUE); \
        err__ = pthread_mutex_init(&(mutex_ptr_)->mutex, &attr__);      \
        if (unlikely(err__))                                            \
            MPL_internal_sys_error_printf("pthread_mutex_init", err__,  \
                                          "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                     \
        MPIU_DBG_MSG_P(THREAD,TYPICAL,"Created MPIU_Thread_mutex %p", (mutex_ptr_)); \
    } while (0)

#endif /* defined(MPICH_PTHREAD_MUTEX_ERRORCHECK_VALUE) */

#define MPIU_Thread_mutex_destroy(mutex_ptr_, err_ptr_)                 \
    do {                                                                \
        int err__;							\
                                                                        \
        MPIU_DBG_MSG_P(THREAD,TYPICAL,"About to destroy MPIU_Thread_mutex %p", (mutex_ptr_)); \
        err__ = pthread_mutex_destroy(&(mutex_ptr_)->mutex);            \
        if (unlikely(err__))                                            \
            MPL_internal_sys_error_printf("pthread_mutex_destroy", err__, \
                                          "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                     \
    } while (0)


#define MPIU_Thread_mutex_lock(mutex_ptr_, err_ptr_)                    \
    do {                                                                \
        int err__;                                                      \
        MPIU_DBG_MSG_P(THREAD,VERBOSE,"enter MPIU_Thread_mutex_lock %p", (mutex_ptr_)); \
        OPA_incr_int(&(mutex_ptr_)->num_queued_threads);                \
        err__ = pthread_mutex_lock(&(mutex_ptr_)->mutex);               \
        OPA_decr_int(&(mutex_ptr_)->num_queued_threads);                \
        if (unlikely(err__)) {                                          \
            MPIU_DBG_MSG_S(THREAD,TERSE,"  mutex lock error: %s", MPIU_Strerror(err__)); \
            MPL_internal_sys_error_printf("pthread_mutex_lock", err__,  \
                                          "    %s:%d\n", __FILE__, __LINE__); \
        }                                                               \
        *(int *)(err_ptr_) = err__;                                     \
        MPIU_DBG_MSG_P(THREAD,VERBOSE,"exit MPIU_Thread_mutex_lock %p", (mutex_ptr_)); \
    } while (0)


#define MPIU_Thread_mutex_unlock(mutex_ptr_, err_ptr_)                  \
    do {                                                                \
        int err__;                                                      \
                                                                        \
        MPIU_DBG_MSG_P(THREAD,VERBOSE,"MPIU_Thread_mutex_unlock %p", (mutex_ptr_)); \
        err__ = pthread_mutex_unlock(&(mutex_ptr_)->mutex);             \
        if (unlikely(err__)) {                                          \
            MPIU_DBG_MSG_S(THREAD,TERSE,"  mutex unlock error: %s", MPIU_Strerror(err__)); \
            MPL_internal_sys_error_printf("pthread_mutex_unlock", err__, \
                                          "    %s:%d\n", __FILE__, __LINE__); \
        }                                                               \
        *(int *)(err_ptr_) = err__;                                     \
    } while (0)


/*
 * Condition Variables
 */

#define MPIU_Thread_cond_create(cond_ptr_, err_ptr_)                    \
    do {                                                                \
        int err__;							\
                                                                        \
        err__ = pthread_cond_init((cond_ptr_), NULL);                   \
        if (unlikely(err__))                                            \
            MPL_internal_sys_error_printf("pthread_cond_init", err__,   \
                                          "    %s:%d\n", __FILE__, __LINE__); \
        MPIU_DBG_MSG_P(THREAD,TYPICAL,"Created MPIU_Thread_cond %p", (cond_ptr_)); \
        *(int *)(err_ptr_) = err__;                                     \
    } while (0)

#define MPIU_Thread_cond_destroy(cond_ptr_, err_ptr_)                   \
    do {                                                                \
        int err__;							\
                                                                        \
        MPIU_DBG_MSG_P(THREAD,TYPICAL,"About to destroy MPIU_Thread_cond %p", (cond_ptr_)); \
        err__ = pthread_cond_destroy(cond_ptr_);                        \
        if (unlikely(err__))                                            \
            MPL_internal_sys_error_printf("pthread_cond_destroy", err__, \
                                          "    %s:%d\n", __FILE__, __LINE__); \
        *(int *)(err_ptr_) = err__;                                     \
    } while (0)

#define MPIU_Thread_cond_wait(cond_ptr_, mutex_ptr_, err_ptr_)		\
    do {                                                                \
        int err__;                                                      \
    									\
        /* The latest pthread specification says that cond_wait         \
         * routines aren't allowed to return EINTR, but some of the     \
         * older implementations still do. */                           \
        MPIU_DBG_MSG_FMT(THREAD,TYPICAL,(MPIU_DBG_FDEST,"Enter cond_wait on cond=%p mutex=%p",(cond_ptr_),(mutex_ptr_))) \
            do {                                                        \
                OPA_incr_int(&(mutex_ptr_)->num_queued_threads);        \
                err__ = pthread_cond_wait((cond_ptr_), &(mutex_ptr_)->mutex); \
                OPA_decr_int(&(mutex_ptr_)->num_queued_threads);        \
            } while (err__ == EINTR);                                   \
        if (unlikely(err__))                                            \
            MPL_internal_sys_error_printf("pthread_cond_wait", err__,   \
                                          "    %s:%d\n", __FILE__, __LINE__); \
									\
        *(int *)(err_ptr_) = err__;                                     \
        MPIU_Assert_fmt_msg(err__ == 0,                                 \
                            ("cond_wait failed, err=%d (%s)",err__,strerror(err__))); \
        MPIU_DBG_MSG_FMT(THREAD,TYPICAL,(MPIU_DBG_FDEST,"Exit cond_wait on cond=%p mutex=%p",(cond_ptr_),(mutex_ptr_))); \
    } while (0)

#define MPIU_Thread_cond_broadcast(cond_ptr_, err_ptr_)                 \
    do {                                                                \
        int err__;							\
                                                                        \
        MPIU_DBG_MSG_P(THREAD,TYPICAL,"About to cond_broadcast on MPIU_Thread_cond %p", (cond_ptr_)); \
        err__ = pthread_cond_broadcast(cond_ptr_);			\
        if (unlikely(err__))                                            \
            MPL_internal_sys_error_printf("pthread_cond_broadcast", err__, \
                                          "    %s:%d\n", __FILE__, __LINE__); \
                                                                        \
        *(int *)(err_ptr_) = err__;                                     \
        MPIU_Assert_fmt_msg(err__ == 0,                                 \
                            ("cond_broadcast failed, err__=%d (%s)",err__,strerror(err__))); \
    } while (0)

#define MPIU_Thread_cond_signal(cond_ptr_, err_ptr_)                    \
    do {                                                                \
        int err__;							\
                                                                        \
        MPIU_DBG_MSG_P(THREAD,TYPICAL,"About to cond_signal on MPIU_Thread_cond %p", (cond_ptr_)); \
        err__ = pthread_cond_signal(cond_ptr_);                         \
        if (unlikely(err__))                                            \
            MPL_internal_sys_error_printf("pthread_cond_signal", err__, \
                                          "    %s:%d\n", __FILE__, __LINE__); \
                                                                        \
        *(int *)(err_ptr_) = err__;                                     \
        MPIU_Assert_fmt_msg(err__ == 0,                                 \
                            ("cond_signal failed, err__=%d (%s)",err__,strerror(err__))); \
    } while (0)


/*
 * Thread Local Storage
 */

#define MPIU_Thread_tls_create(exit_func_ptr_, tls_ptr_, err_ptr_)	\
    do {                                                                \
        int err__;                                                      \
    									\
        err__ = pthread_key_create((tls_ptr_), (exit_func_ptr_));       \
        if (unlikely(err__))                                            \
            MPL_internal_sys_error_printf("pthread_key_create", err__,  \
                                          "    %s:%d\n", __FILE__, __LINE__); \
									\
        *(int *)(err_ptr_) = err__;                                     \
    } while (0)

#define MPIU_Thread_tls_destroy(tls_ptr_, err_ptr_)     \
    do {                                                \
        int err__;                                      \
                                                        \
        err__ = pthread_key_delete(*(tls_ptr_));        \
        if (unlikely(err__))                                            \
            MPL_internal_sys_error_printf("pthread_key_delete", err__,  \
                                          "    %s:%d\n", __FILE__, __LINE__); \
                                                        \
        *(int *)(err_ptr_) = err__;                     \
    } while (0)

#define MPIU_Thread_tls_set(tls_ptr_, value_, err_ptr_)                 \
    do {                                                                \
        int err__;							\
                                                                        \
        err__ = pthread_setspecific(*(tls_ptr_), (value_));		\
        if (unlikely(err__))                                            \
            MPL_internal_sys_error_printf("pthread_setspecific", err__, \
                                          "    %s:%d\n", __FILE__, __LINE__); \
                                                                        \
        *(int *)(err_ptr_) = err__;                                     \
        MPIU_Assert_fmt_msg(err__ == 0,                                 \
                            ("tls_set failed, err__=%d (%s)",err__,strerror(err__))); \
    } while (0)

#define MPIU_Thread_tls_get(tls_ptr_, value_ptr_, err_ptr_)	\
    do {                                                        \
        *(value_ptr_) = pthread_getspecific(*(tls_ptr_));       \
								\
        *(int *)(err_ptr_) = MPIU_THREAD_SUCCESS;               \
    } while (0)

#endif /* MPIU_THREAD_POSIX_H_INCLUDED */
