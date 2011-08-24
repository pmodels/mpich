/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * Threads
 */

#include "mpiu_process_wrappers.h" /* for MPIU_PW_Sched_yield */

/* 
   One of PTHREAD_MUTEX_RECURSIVE_NP and PTHREAD_MUTEX_RECURSIVE seem to be 
   present in different versions.  For example, Mac OS X 10.4 had 
   PTHREAD_MUTEX_RECURSIVE_NP but Mac OS X 10.5 does not; instead it has
   PTHREAD_MUTEX_RECURSIVE 
*/
#if defined(HAVE_PTHREAD_MUTEX_RECURSIVE_NP)
#define PTHREAD_MUTEX_RECURSIVE_VALUE PTHREAD_MUTEX_RECURSIVE_NP
#elif defined(HAVE_PTHREAD_MUTEX_RECURSIVE)
#define PTHREAD_MUTEX_RECURSIVE_VALUE PTHREAD_MUTEX_RECURSIVE
#else
#error 'Unable to determine pthrad mutex recursive value'
#endif /* pthread mutex recursive value */

/* MPIU_Thread_create() defined in mpiu_thread.c */

#define MPIU_Thread_exit()			\
do {                                               \
    pthread_exit(NULL);				\
} while (0)

#define MPIU_Thread_self(id_)			\
do {                                               \
    *(id_) = pthread_self();			\
} while (0)

#define MPIU_Thread_same(id1_, id2_, same_)			\
do {                                                               \
    *(same_) = pthread_equal(*(id1_), *(id2_)) ? TRUE : FALSE;	\
} while (0)

#define MPIU_Thread_yield()						\
do {                                                                       \
    /* FIXME: need to check for different types of yield */	\
    MPIU_DBG_MSG(THREAD,VERBOSE,"enter MPIU_Thread_yield");    \
    MPIU_PW_Sched_yield();                                     \
    MPIU_DBG_MSG(THREAD,VERBOSE,"exit MPIU_Thread_yield");     \
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
#if !defined(MPICH_DEBUG_MUTEX) || !defined(PTHREAD_MUTEX_ERRORCHECK_VALUE)
#define MPIU_Thread_mutex_create(mutex_ptr_, err_ptr_)                   \
do {                                                                       \
    int err__;                                                          \
                                                                        \
    err__ = pthread_mutex_init((mutex_ptr_), NULL);			\
    if ((err_ptr_) != NULL)                                             \
    {                                                                   \
	/* FIXME: convert error to an MPIU_THREAD_ERR value */           \
	*(int *)(err_ptr_) = err__;                                     \
    }                                                                   \
    MPIU_DBG_MSG_P(THREAD,TYPICAL,"Created MPIU_Thread_mutex %p", (mutex_ptr_));    \
} while (0)
#else /* MPICH_DEBUG_MUTEX */
#define MPIU_Thread_mutex_create(mutex_ptr_, err_ptr_)                   \
do {                                                                       \
    int err__;                                                          \
    pthread_mutexattr_t attr__;                                         \
                                                                        \
    /* FIXME this used to be PTHREAD_MUTEX_ERRORCHECK_NP, but we had to change
       it for the thread granularity work when we needed recursive mutexes.  We
       should go through this code and see if there's any good way to implement
       error checked versions with the recursive mutexes. */ \
    pthread_mutexattr_init(&attr__);                                    \
    pthread_mutexattr_settype(&attr__, PTHREAD_MUTEX_ERRORCHECK_VALUE); \
    err__ = pthread_mutex_init((mutex_ptr_), &attr__);                  \
    if (err__)                                                          \
        MPIU_Internal_sys_error_printf("pthread_mutex_init", err__,     \
                                       "    %s:%d\n", __FILE__, __LINE__);\
    if ((err_ptr_) != NULL)                                              \
    {                                                                    \
	/* FIXME: convert error to an MPIU_THREAD_ERR value */            \
	*(int *)(err_ptr_) = err__;                                      \
    }                                                                    \
    MPIU_DBG_MSG_P(THREAD,TYPICAL,"Created MPIU_Thread_mutex %p", (mutex_ptr_));    \
} while (0)
#endif

#define MPIU_Thread_mutex_destroy(mutex_ptr_, err_ptr_)		\
do {                                                               \
    int err__;							\
								\
    MPIU_DBG_MSG_P(THREAD,TYPICAL,"About to destroy MPIU_Thread_mutex %p", (mutex_ptr_));    \
    err__ = pthread_mutex_destroy(mutex_ptr_);			\
    if ((err_ptr_) != NULL)					\
    {								\
	/* FIXME: convert error to an MPIU_THREAD_ERR value */	\
	*(int *)(err_ptr_) = err__;				\
    }								\
} while (0)

#ifndef MPICH_DEBUG_MUTEX
#define MPIU_Thread_mutex_lock(mutex_ptr_, err_ptr_)             \
do {                                                               \
    int err__;                                                  \
    MPIU_DBG_MSG_P(THREAD,VERBOSE,"enter MPIU_Thread_mutex_lock %p", (mutex_ptr_));      \
    err__ = pthread_mutex_lock(mutex_ptr_);                     \
    if ((err_ptr_) != NULL)                                     \
    {                                                           \
	/* FIXME: convert error to an MPIU_THREAD_ERR value */   \
	*(int *)(err_ptr_) = err__;                             \
    }                                                           \
    MPIU_DBG_MSG_P(THREAD,VERBOSE,"exit MPIU_Thread_mutex_lock %p", (mutex_ptr_));      \
} while (0)
#else /* MPICH_DEBUG_MUTEX */
#define MPIU_Thread_mutex_lock(mutex_ptr_, err_ptr_)             \
do {                                                               \
    int err__;                                                  \
    MPIU_DBG_MSG_P(THREAD,VERBOSE,"enter MPIU_Thread_mutex_lock %p", (mutex_ptr_));      \
    err__ = pthread_mutex_lock(mutex_ptr_);                     \
    if (err__)                                                  \
    {                                                           \
        MPIU_DBG_MSG_S(THREAD,TERSE,"  mutex lock error: %s", MPIU_Strerror(err__));       \
        MPIU_Internal_sys_error_printf("pthread_mutex_lock", err__,\
                                       "    %s:%d\n", __FILE__, __LINE__);\
    }                                                          \
    if ((err_ptr_) != NULL)                                    \
    {                                                          \
	/* FIXME: convert error to an MPIU_THREAD_ERR value */  \
	*(int *)(err_ptr_) = err__;                            \
    }                                                          \
    MPIU_DBG_MSG_P(THREAD,VERBOSE,"exit MPIU_Thread_mutex_lock %p", (mutex_ptr_));      \
} while (0)
#endif

#ifndef MPICH_DEBUG_MUTEX
#define MPIU_Thread_mutex_unlock(mutex_ptr_, err_ptr_)           \
do {                                                               \
    int err__;                                                  \
                                                                \
    MPIU_DBG_MSG_P(THREAD,TYPICAL,"MPIU_Thread_mutex_unlock %p", (mutex_ptr_));    \
    err__ = pthread_mutex_unlock(mutex_ptr_);                   \
    if ((err_ptr_) != NULL)                                     \
    {                                                           \
	/* FIXME: convert error to an MPIU_THREAD_ERR value */   \
	*(int *)(err_ptr_) = err__;                             \
    }                                                           \
} while (0)
#else /* MPICH_DEBUG_MUTEX */
#define MPIU_Thread_mutex_unlock(mutex_ptr_, err_ptr_)           \
do {                                                               \
    int err__;                                                  \
                                                                \
    MPIU_DBG_MSG_P(THREAD,VERBOSE,"MPIU_Thread_mutex_unlock %p", (mutex_ptr_));    \
    err__ = pthread_mutex_unlock(mutex_ptr_);                   \
    if (err__)                                                  \
    {                                                           \
        MPIU_DBG_MSG_S(THREAD,TERSE,"  mutex unlock error: %s", MPIU_Strerror(err__));     \
        MPIU_Internal_sys_error_printf("pthread_mutex_unlock", err__,         \
                                       "    %s:%d\n", __FILE__, __LINE__);    \
    }                                                           \
    if ((err_ptr_) != NULL)                                     \
    {                                                           \
	/* FIXME: convert error to an MPIU_THREAD_ERR value */   \
	*(int *)(err_ptr_) = err__;                             \
    }                                                           \
} while (0)
#endif

#ifndef MPICH_DEBUG_MUTEX
#define MPIU_Thread_mutex_trylock(mutex_ptr_, flag_ptr_, err_ptr_)    \
do {                                                                    \
    int err__;                                                       \
                                                                     \
    err__ = pthread_mutex_trylock(mutex_ptr_);                       \
    *(flag_ptr_) = (err__ == 0) ? TRUE : FALSE;                      \
    MPIU_DBG_MSG_FMT(THREAD,VERBOSE,(MPIU_DBG_FDEST, "MPIU_Thread_mutex_trylock mutex=%p result=%s", (mutex_ptr_), (*(flag_ptr_) ? "success" : "failure")));    \
    if ((err_ptr_) != NULL)                                          \
    {                                                                \
	*(int *)(err_ptr_) = (err__ == EBUSY) ? MPIU_THREAD_SUCCESS : err__;\
	/* FIXME: convert error to an MPIU_THREAD_ERR value */        \
    }                                                                \
} while (0)
#else /* MPICH_DEBUG_MUTEX */
#define MPIU_Thread_mutex_trylock(mutex_ptr_, flag_ptr_, err_ptr_)    \
do {                                                                    \
    int err__;                                                       \
                                                                     \
    err__ = pthread_mutex_trylock(mutex_ptr_);                       \
    if (err__ && err__ != EBUSY)                                     \
    {                                                                \
        MPIU_DBG_MSG_S(THREAD,TERSE,"  mutex trylock error: %s", MPIU_Strerror(err__));    \
        MPIU_Internal_sys_error_printf("pthread_mutex_trylock", err__,\
                                       "    %s:%d\n", __FILE__, __LINE__);\
    }                                                                \
    *(flag_ptr_) = (err__ == 0) ? TRUE : FALSE;                      \
    MPIU_DBG_MSG_FMT(THREAD,VERBOSE,(MPIU_DBG_FDEST, "MPIU_Thread_mutex_trylock mutex=%p result=%s", (mutex_ptr_), (*(flag_ptr_) ? "success" : "failure")));    \
    if ((err_ptr_) != NULL)                                          \
    {                                                                \
	*(int *)(err_ptr_) = (err__ == EBUSY) ? MPIU_THREAD_SUCCESS : err__;    \
	/* FIXME: convert error to an MPIU_THREAD_ERR value */        \
    }                                                                \
} while (0)
#endif

/*
 * Condition Variables
 */

#define MPIU_Thread_cond_create(cond_ptr_, err_ptr_)		\
do {                                                               \
    int err__;							\
    								\
    err__ = pthread_cond_init((cond_ptr_), NULL);		\
    MPIU_DBG_MSG_P(THREAD,TYPICAL,"Created MPIU_Thread_cond %p", (cond_ptr_));    \
    if ((err_ptr_) != NULL)					\
    {								\
	/* FIXME: convert error to an MPIU_THREAD_ERR value */	\
	*(int *)(err_ptr_) = err__;				\
    }								\
} while (0)

#define MPIU_Thread_cond_destroy(cond_ptr_, err_ptr_)		\
do {                                                               \
    int err__;							\
    								\
    MPIU_DBG_MSG_P(THREAD,TYPICAL,"About to destroy MPIU_Thread_cond %p", (cond_ptr_));    \
    err__ = pthread_cond_destroy(cond_ptr_);			\
    if ((err_ptr_) != NULL)					\
    {								\
	/* FIXME: convert error to an MPIU_THREAD_ERR value */	\
	*(int *)(err_ptr_) = err__;				\
    }								\
} while (0)

#define MPIU_Thread_cond_wait(cond_ptr_, mutex_ptr_, err_ptr_)		\
do {                                                                       \
    int err__;								\
    									\
    /* The latest pthread specification says that cond_wait routines    \
       aren't allowed to return EINTR,	                                \
       but some of the older implementations still do. */		\
    MPIU_DBG_MSG_FMT(THREAD,TYPICAL,(MPIU_DBG_FDEST,"Enter cond_wait on cond=%p mutex=%p",(cond_ptr_),(mutex_ptr_))) \
    do									\
    {									\
	err__ = pthread_cond_wait((cond_ptr_), (mutex_ptr_));		\
    }									\
    while (err__ == EINTR);						\
									\
    if ((err_ptr_) != NULL)						\
    {									\
	/* FIXME: convert error to an MPIU_THREAD_ERR value */		\
	*(int *)(err_ptr_) = err__;					\
        if (err__) {                                                    \
            MPIU_DBG_MSG_FMT(THREAD,TYPICAL,(MPIU_DBG_FDEST,"error in cond_wait on cond=%p mutex=%p err__=%d",(cond_ptr_),(mutex_ptr_), err__)) \
        }                                                               \
    }									\
    MPIU_DBG_MSG_FMT(THREAD,TYPICAL,(MPIU_DBG_FDEST,"Exit cond_wait on cond=%p mutex=%p",(cond_ptr_),(mutex_ptr_))) \
} while (0)

#define MPIU_Thread_cond_broadcast(cond_ptr_, err_ptr_)		\
do {                                                               \
    int err__;							\
    								\
    MPIU_DBG_MSG_P(THREAD,TYPICAL,"About to cond_broadcast on MPIU_Thread_cond %p", (cond_ptr_));    \
    err__ = pthread_cond_broadcast(cond_ptr_);			\
								\
    if ((err_ptr_) != NULL)					\
    {								\
	/* FIXME: convert error to an MPIU_THREAD_ERR value */	\
	*(int *)(err_ptr_) = err__;				\
    }								\
} while (0)

#define MPIU_Thread_cond_signal(cond_ptr_, err_ptr_)		\
do {                                                               \
    int err__;							\
								\
    MPIU_DBG_MSG_P(THREAD,TYPICAL,"About to cond_signal on MPIU_Thread_cond %p", (cond_ptr_));    \
    err__ = pthread_cond_signal(cond_ptr_);			\
								\
    if ((err_ptr_) != NULL)					\
    {								\
	/* FIXME: convert error to an MPIU_THREAD_ERR value */	\
	*(int *)(err_ptr_) = err__;				\
    }								\
} while (0)


/*
 * Thread Local Storage
 */

#define MPIU_Thread_tls_create(exit_func_ptr_, tls_ptr_, err_ptr_)	\
do {                                                                       \
    int err__;								\
    									\
    err__ = pthread_key_create((tls_ptr_), (exit_func_ptr_));		\
									\
    if ((err_ptr_) != NULL)						\
    {									\
	/* FIXME: convert error to an MPIU_THREAD_ERR value */		\
	*(int *)(err_ptr_) = err__;					\
    }									\
} while (0)

#define MPIU_Thread_tls_destroy(tls_ptr_, err_ptr_)		\
do {                                                               \
    int err__;							\
    								\
    err__ = pthread_key_delete(*(tls_ptr_));			\
								\
    if ((err_ptr_) != NULL)					\
    {								\
	/* FIXME: convert error to an MPIU_THREAD_ERR value */	\
	*(int *)(err_ptr_) = err__;				\
    }								\
} while (0)

#define MPIU_Thread_tls_set(tls_ptr_, value_, err_ptr_)		\
do {                                                               \
    int err__;							\
								\
    err__ = pthread_setspecific(*(tls_ptr_), (value_));		\
								\
    if ((err_ptr_) != NULL)					\
    {								\
	/* FIXME: convert error to an MPIU_THREAD_ERR value */	\
	*(int *)(err_ptr_) = err__;				\
    }								\
} while (0)

#define MPIU_Thread_tls_get(tls_ptr_, value_ptr_, err_ptr_)	\
do {                                                               \
    *(value_ptr_) = pthread_getspecific(*(tls_ptr_));		\
								\
    if ((err_ptr_) != NULL)					\
    {								\
	/* FIXME: convert error to an MPIU_THREAD_ERR value */	\
	*(int *)(err_ptr_) = MPIU_THREAD_SUCCESS;		\
    }								\
} while (0)
