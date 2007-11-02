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
    MPE_Thread_create((func_), (data_), (id_), (err_));	\
}

#define MPID_Thread_exit()			\
{						\
    MPE_Thread_exit();				\
}

#define MPID_Thread_self(id_)			\
{						\
    MPE_Thread_self(id_);			\
}

#define MPID_Thread_same(id1_, id2_, same_)	\
{						\
    MPE_Thread_same((id1_), (id2_), (same_));	\
}

#define MPID_Thread_yield()			\
{						\
    MPE_Thread_yield();				\
}


/*
 *    Mutexes
 */

#define MPID_Thread_mutex_create(mutex_, err_)	\
{						\
    MPE_Thread_mutex_create((mutex_), (err_));	\
}

#define MPID_Thread_mutex_destroy(mutex_, err_)	\
{						\
    MPE_Thread_mutex_destroy((mutex_), (err_));	\
}

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_mutex_lock(mutex_)		\
{						\
    MPE_Thread_mutex_lock((mutex_), NULL);	\
}
#else
#define MPID_Thread_mutex_lock(mutex_)		\
{						\
    int err_;					\
    MPE_Thread_mutex_lock((mutex_), &err_);	\
    MPIU_Assert(err_ == MPE_THREAD_SUCCESS);    \
}
#endif

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_mutex_unlock(mutex_)	\
{						\
    MPE_Thread_mutex_unlock((mutex_), NULL);	\
}
#else
#define MPID_Thread_mutex_unlock(mutex_)	\
{						\
    int err_;					\
    MPE_Thread_mutex_unlock((mutex_), &err_);	\
    MPIU_Assert(err_ == MPE_THREAD_SUCCESS);	\
}
#endif

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_mutex_trylock(mutex_, flag_)	\
{							\
    MPE_Thread_mutex_trylock((mutex_), (flag_), NULL);	\
}
#else
#define MPID_Thread_mutex_trylock(mutex_, flag_)		\
{								\
    int err_;							\
    MPE_Thread_mutex_trylock((mutex_), (flag_), &err_);	\
    MPIU_Assert(err_ == MPE_THREAD_SUCCESS);			\
}
#endif


/*
 * Condition Variables
 */

#define MPID_Thread_cond_create(cond_, err_)	\
{						\
    MPE_Thread_cond_create((cond_), (err_));	\
}

#define MPID_Thread_cond_destroy(cond_, err_)	\
{						\
    MPE_Thread_cond_destroy((cond_), (err_));	\
}

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_cond_wait(cond_, mutex_)		\
{							\
    MPE_Thread_cond_wait((cond_), (mutex_), NULL);	\
}
#else
#define MPID_Thread_cond_wait(cond_, mutex_)		\
{							\
    int err_;						\
    MPE_Thread_cond_wait((cond_), (mutex_), &err_);	\
    MPIU_Assert(err_ == MPE_THREAD_SUCCESS);		\
}
#endif

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_cond_broadcast(cond_)	\
{						\
    MPE_Thread_cond_broadcast(cond_, NULL);	\
}
#else
#define MPID_Thread_cond_broadcast(cond_)	\
{						\
    int err_;					\
    MPE_Thread_cond_broadcast((cond_), &err_);	\
    MPIU_Assert(err_ == MPE_THREAD_SUCCESS);	\
}
#endif

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_cond_signal(cond_)		\
{						\
    MPE_Thread_cond_signal(cond_, NULL);	\
}
#else
#define MPID_Thread_cond_signal(cond_)		\
{						\
    int err_;					\
    MPE_Thread_cond_signal((cond_), &err_);	\
    MPIU_Assert(err_ == MPE_THREAD_SUCCESS);	\
}
#endif


/*
 * Thread Local Storage
 */

#define MPID_Thread_tls_create(exit_func_, tls_, err_)		\
{								\
    MPE_Thread_tls_create((exit_func_), (tls_), (err_));	\
}

#define MPID_Thread_tls_destroy(tls_, err_)	\
{						\
    MPE_Thread_tls_destroy((tls_), (err_));	\
}

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_tls_set(tls_, value_)	\
{						\
    MPE_Thread_tls_set((tls_), (value_), NULL);	\
}
#else
#define MPID_Thread_tls_set(tls_, value_)		\
{							\
    int err_;						\
    MPE_Thread_tls_set((tls_), (value_), &err_);	\
    MPIU_Assert(err_ == MPE_THREAD_SUCCESS);		\
}
#endif

#if !defined(MPID_THREAD_DEBUG)
#define MPID_Thread_tls_get(tls_, value_)	\
{						\
    MPE_Thread_tls_get((tls_), (value_), NULL);	\
}
#else
#define MPID_Thread_tls_get(tls_, value_)		\
{							\
    int err_;						\
							\
    MPE_Thread_tls_get((tls_), (value_), &err_);	\
    MPIU_Assert(err_ == MPE_THREAD_SUCCESS);		\
}
#endif


