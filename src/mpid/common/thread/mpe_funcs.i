/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * Threads
 */

#define MPID_Thread_create(func_, data_, id_, err_)	\
do {                                                       \
    MPIU_Thread_create((func_), (data_), (id_), (err_));	\
} while (0)

#define MPID_Thread_exit()			\
do {                                               \
    MPIU_Thread_exit();				\
} while (0)

#define MPID_Thread_self(id_)			\
do {                                               \
    MPIU_Thread_self(id_);			\
} while (0)

#define MPID_Thread_same(id1_, id2_, same_)	\
do {                                               \
    MPIU_Thread_same((id1_), (id2_), (same_));	\
} while (0)

#define MPID_Thread_yield()			\
do {                                               \
    MPIU_Thread_yield();				\
} while (0)


/*
 *    Mutexes
 */

#define MPID_Thread_mutex_create(mutex_, err_)	\
do {                                               \
    MPIU_Thread_mutex_create((mutex_), (err_));	\
} while (0)

#define MPID_Thread_mutex_destroy(mutex_, err_)	\
do {                                               \
    MPIU_Thread_mutex_destroy((mutex_), (err_));	\
} while (0)

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_mutex_lock(mutex_)		\
do {                                               \
    MPIU_Thread_mutex_lock((mutex_), NULL);	\
} while (0)
#else
#define MPID_Thread_mutex_lock(mutex_)		\
do {                                               \
    int err_;					\
    MPIU_Thread_mutex_lock((mutex_), &err_);	\
    MPIU_Assert_fmt_msg(err_ == MPIU_THREAD_SUCCESS,                                   \
                        ("mutex_lock failed, err_=%d (%s)",err_,MPIU_Strerror(err_))); \
} while (0)
#endif

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_mutex_unlock(mutex_)	\
do {                                               \
    MPIU_Thread_mutex_unlock((mutex_), NULL);	\
} while (0)
#else
#define MPID_Thread_mutex_unlock(mutex_)	\
do {                                               \
    int err_;					\
    MPIU_Thread_mutex_unlock((mutex_), &err_);	\
    MPIU_Assert_fmt_msg(err_ == MPIU_THREAD_SUCCESS,                                     \
                        ("mutex_unlock failed, err_=%d (%s)",err_,MPIU_Strerror(err_))); \
} while (0)
#endif

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_mutex_trylock(mutex_, flag_)	\
do {                                                       \
    MPIU_Thread_mutex_trylock((mutex_), (flag_), NULL);	\
} while (0)
#else
#define MPID_Thread_mutex_trylock(mutex_, flag_)		\
do {                                                               \
    int err_;							\
    MPIU_Thread_mutex_trylock((mutex_), (flag_), &err_);	\
    MPIU_Assert_fmt_msg(err_ == MPIU_THREAD_SUCCESS,                                      \
                        ("mutex_trylock failed, err_=%d (%s)",err_,MPIU_Strerror(err_))); \
} while (0)
#endif


/*
 * Condition Variables
 */

#define MPID_Thread_cond_create(cond_, err_)	\
do {                                               \
    MPIU_Thread_cond_create((cond_), (err_));	\
} while (0)

#define MPID_Thread_cond_destroy(cond_, err_)	\
do {                                               \
    MPIU_Thread_cond_destroy((cond_), (err_));	\
} while (0)

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_cond_wait(cond_, mutex_)		\
do {                                                       \
    MPIU_Thread_cond_wait((cond_), (mutex_), NULL);	\
} while (0)
#else
#define MPID_Thread_cond_wait(cond_, mutex_)		\
do {                                                       \
    int err_;						\
    MPIU_Thread_cond_wait((cond_), (mutex_), &err_);	\
    MPIU_Assert_fmt_msg(err_ == MPIU_THREAD_SUCCESS,                                  \
                        ("cond_wait failed, err_=%d (%s)",err_,MPIU_Strerror(err_))); \
} while (0)
#endif

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_cond_broadcast(cond_)	\
do {                                               \
    MPIU_Thread_cond_broadcast(cond_, NULL);	\
} while (0)
#else
#define MPID_Thread_cond_broadcast(cond_)	\
do {                                               \
    int err_;					\
    MPIU_Thread_cond_broadcast((cond_), &err_);	\
    MPIU_Assert_fmt_msg(err_ == MPIU_THREAD_SUCCESS,                                       \
                        ("cond_broadcast failed, err_=%d (%s)",err_,MPIU_Strerror(err_))); \
} while (0)
#endif

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_cond_signal(cond_)		\
do {                                               \
    MPIU_Thread_cond_signal(cond_, NULL);	\
} while (0)
#else
#define MPID_Thread_cond_signal(cond_)		\
do {                                               \
    int err_;					\
    MPIU_Thread_cond_signal((cond_), &err_);	\
    MPIU_Assert_fmt_msg(err_ == MPIU_THREAD_SUCCESS,                                    \
                        ("cond_signal failed, err_=%d (%s)",err_,MPIU_Strerror(err_))); \
} while (0)
#endif


/*
 * Thread Local Storage
 */

#define MPID_Thread_tls_create(exit_func_, tls_, err_)		\
do {                                                               \
    MPIU_Thread_tls_create((exit_func_), (tls_), (err_));	\
} while (0)

#define MPID_Thread_tls_destroy(tls_, err_)	\
do {                                               \
    MPIU_Thread_tls_destroy((tls_), (err_));	\
} while (0)

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_tls_set(tls_, value_)	\
do {                                               \
    MPIU_Thread_tls_set((tls_), (value_), NULL);	\
} while (0)
#else
#define MPID_Thread_tls_set(tls_, value_)		\
do {                                                       \
    int err_;						\
    MPIU_Thread_tls_set((tls_), (value_), &err_);	\
    MPIU_Assert_fmt_msg(err_ == MPIU_THREAD_SUCCESS,                                \
                        ("tls_set failed, err_=%d (%s)",err_,MPIU_Strerror(err_))); \
} while (0)
#endif

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_tls_get(tls_, value_)	\
do {                                               \
    MPIU_Thread_tls_get((tls_), (value_), NULL);	\
} while (0)
#else
#define MPID_Thread_tls_get(tls_, value_)		\
do {                                                       \
    int err_;						\
							\
    MPIU_Thread_tls_get((tls_), (value_), &err_);	\
    /* can't strerror here, possible endless recursion in strerror */ \
    MPIU_Assert_fmt_msg(err_ == MPIU_THREAD_SUCCESS,("tls_get failed, err_=%d",err_)); \
} while (0)
#endif


