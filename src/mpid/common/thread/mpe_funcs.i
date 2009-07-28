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
{							\
    MPIU_Thread_create((func_), (data_), (id_), (err_));	\
}

#define MPID_Thread_exit()			\
{						\
    MPIU_Thread_exit();				\
}

#define MPID_Thread_self(id_)			\
{						\
    MPIU_Thread_self(id_);			\
}

#define MPID_Thread_same(id1_, id2_, same_)	\
{						\
    MPIU_Thread_same((id1_), (id2_), (same_));	\
}

#define MPID_Thread_yield()			\
{						\
    MPIU_Thread_yield();				\
}


/*
 *    Mutexes
 */

#define MPID_Thread_mutex_create(mutex_, err_)	\
{						\
    MPIU_Thread_mutex_create((mutex_), (err_));	\
}

#define MPID_Thread_mutex_destroy(mutex_, err_)	\
{						\
    MPIU_Thread_mutex_destroy((mutex_), (err_));	\
}

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_mutex_lock(mutex_)		\
{						\
    MPIU_Thread_mutex_lock((mutex_), NULL);	\
}
#else
#define MPID_Thread_mutex_lock(mutex_)		\
{						\
    int err_;					\
    MPIU_Thread_mutex_lock((mutex_), &err_);	\
    MPIU_Assert(err_ == MPIU_THREAD_SUCCESS);    \
}
#endif

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_mutex_unlock(mutex_)	\
{						\
    MPIU_Thread_mutex_unlock((mutex_), NULL);	\
}
#else
#define MPID_Thread_mutex_unlock(mutex_)	\
{						\
    int err_;					\
    MPIU_Thread_mutex_unlock((mutex_), &err_);	\
    MPIU_Assert(err_ == MPIU_THREAD_SUCCESS);	\
}
#endif

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_mutex_trylock(mutex_, flag_)	\
{							\
    MPIU_Thread_mutex_trylock((mutex_), (flag_), NULL);	\
}
#else
#define MPID_Thread_mutex_trylock(mutex_, flag_)		\
{								\
    int err_;							\
    MPIU_Thread_mutex_trylock((mutex_), (flag_), &err_);	\
    MPIU_Assert(err_ == MPIU_THREAD_SUCCESS);			\
}
#endif


/*
 * Condition Variables
 */

#define MPID_Thread_cond_create(cond_, err_)	\
{						\
    MPIU_Thread_cond_create((cond_), (err_));	\
}

#define MPID_Thread_cond_destroy(cond_, err_)	\
{						\
    MPIU_Thread_cond_destroy((cond_), (err_));	\
}

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_cond_wait(cond_, mutex_)		\
{							\
    MPIU_Thread_cond_wait((cond_), (mutex_), NULL);	\
}
#else
#define MPID_Thread_cond_wait(cond_, mutex_)		\
{							\
    int err_;						\
    MPIU_Thread_cond_wait((cond_), (mutex_), &err_);	\
    MPIU_Assert(err_ == MPIU_THREAD_SUCCESS);		\
}
#endif

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_cond_broadcast(cond_)	\
{						\
    MPIU_Thread_cond_broadcast(cond_, NULL);	\
}
#else
#define MPID_Thread_cond_broadcast(cond_)	\
{						\
    int err_;					\
    MPIU_Thread_cond_broadcast((cond_), &err_);	\
    MPIU_Assert(err_ == MPIU_THREAD_SUCCESS);	\
}
#endif

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_cond_signal(cond_)		\
{						\
    MPIU_Thread_cond_signal(cond_, NULL);	\
}
#else
#define MPID_Thread_cond_signal(cond_)		\
{						\
    int err_;					\
    MPIU_Thread_cond_signal((cond_), &err_);	\
    MPIU_Assert(err_ == MPIU_THREAD_SUCCESS);	\
}
#endif


/*
 * Thread Local Storage
 */

#define MPID_Thread_tls_create(exit_func_, tls_, err_)		\
{								\
    MPIU_Thread_tls_create((exit_func_), (tls_), (err_));	\
}

#define MPID_Thread_tls_destroy(tls_, err_)	\
{						\
    MPIU_Thread_tls_destroy((tls_), (err_));	\
}

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_tls_set(tls_, value_)	\
{						\
    MPIU_Thread_tls_set((tls_), (value_), NULL);	\
}
#else
#define MPID_Thread_tls_set(tls_, value_)		\
{							\
    int err_;						\
    MPIU_Thread_tls_set((tls_), (value_), &err_);	\
    MPIU_Assert(err_ == MPIU_THREAD_SUCCESS);		\
}
#endif

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_tls_get(tls_, value_)	\
{						\
    MPIU_Thread_tls_get((tls_), (value_), NULL);	\
}
#else
#define MPID_Thread_tls_get(tls_, value_)		\
{							\
    int err_;						\
							\
    MPIU_Thread_tls_get((tls_), (value_), &err_);	\
    MPIU_Assert(err_ == MPIU_THREAD_SUCCESS);		\
}
#endif


