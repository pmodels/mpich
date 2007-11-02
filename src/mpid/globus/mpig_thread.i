/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPICH2_MPIG_THREAD_I_INCLUDED) || defined(MPIG_EMIT_INLINE_FUNCS)
#define MPICH2_THREAD_GENQ_I_INCLUDED

#if !defined(MPIG_EMIT_INLINE_FUNCS) /* don't declare types and functions twice */

#if defined(HAVE_GLOBUS_THREADS)
typedef globus_thread_t mpig_thread_t;
typedef globus_mutex_t mpig_mutex_t;
typedef globus_cond_t mpig_cond_t;
#elif defined(HAVE_POSIX_THREADS)
#if defined(HAVE_PTHREAD_H)
#include <pthread.h>
#endif
typedef pthread_t mpig_thread_t;
typedef pthread_mutex_t mpig_mutex_t;
typedef pthread_cond_t mpig_cond_t;
#elif !defined(MPIG_THREADED)
typedef int mpig_thread_t;
typedef int mpig_mutex_t;
typedef int mpig_cond_t;
#else
#error Configure did not report the availability of a threads package!
#endif

#endif /* !defined(MPIG_EMIT_INLINE_FUNCS) */

#if defined(MPIG_INLINE_HDECL)

MPIG_INLINE_HDECL mpig_thread_t mpig_thread_self(void);

MPIG_INLINE_HDECL bool_t mpig_thread_equal(mpig_thread_t t1, mpig_thread_t t2);

MPIG_INLINE_HDECL void mpig_thread_yield(void);

MPIG_INLINE_HDECL int mpig_mutex_construct(mpig_mutex_t * mutex);

MPIG_INLINE_HDECL int mpig_mutex_destruct(mpig_mutex_t * mutex);

MPIG_INLINE_HDECL int mpig_mutex_lock(mpig_mutex_t * mutex);

MPIG_INLINE_HDECL int mpig_mutex_trylock(mpig_mutex_t * mutex, bool_t * acquired_p);

MPIG_INLINE_HDECL int mpig_mutex_unlock(mpig_mutex_t * mutex);

MPIG_INLINE_HDECL int mpig_cond_construct(mpig_cond_t * cond);

MPIG_INLINE_HDECL int mpig_cond_destruct(mpig_cond_t * cond);

MPIG_INLINE_HDECL int mpig_cond_wait(mpig_cond_t * cond, mpig_mutex_t * mutex);

MPIG_INLINE_HDECL int mpig_cond_signal(mpig_cond_t * cond);

MPIG_INLINE_HDECL int mpig_cond_broadcast(mpig_cond_t * cond);

#endif /* defined(MPIG_INLINE_HDECL) */

#if defined(MPIG_INLINE_HDEF)

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_thread_yield
#define FCNAME MPIG_QUOTE(FUNCNAME)
#if defined(HAVE_GLOBUS_THREADS)
MPIG_INLINE_HDEF void mpig_thread_yield(void)
{
    globus_thread_yield();
}
#elif defined(HAVE_PTHREAD_YIELD)
#if defined(HAVE_PTHREAD_H)
#include <pthread.h>
#endif
#if defined(NEEDS_PTHREAD_YIELD_DECL)
void pthread_yield(void);
#endif
MPIG_INLINE_HDEF void mpig_thread_yield(void)
{
    pthread_yield();
}
#elif defined(HAVE_SCHED_YIELD)
#if defined(HAVE_SCHED_H)
#include <sched.h>
#endif
MPIG_INLINE_HDEF void mpig_thread_yield(void)
{
    sched_yield();
}
#elif defined(HAVE_YIELD)
MPIG_INLINE_HDEF void mpig_thread_yield(void)
{
    yield();
}
#elif defined(HAVE_NANOSLEEP)
#if defined(HAVE_TIME_H)
#include <time.h>
#endif
MPIG_INLINE_HDEF void mpig_thread_yield(void)
{
    struct timespec ts;

    ts.tv_seconds = 0;
    ts.tv_nsec = 0;
    nanosleep(&ts);
}
#elif defined(HAVE_USLEEP)
MPIG_INLINE_HDEF void mpig_thread_yield(void)
{
    usleep(0);
}
#elif defined(HAVE_SLEEP)
MPIG_INLINE_HDEF void mpig_thread_yield(void)
{
    sleep(0);
}
#else
#error No mechanism exists that allows the current process to voluntarily relinquishing the processor
#endif /* mpig_thread_yield definitions */

#if !defined(MPIG_THREADED)

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_thread_self
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF mpig_thread_t mpig_thread_self(void)
{
    return 1;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_thread_equal
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF bool_t mpig_thread_equal(mpig_thread_t t1, mpig_thread_t t2)
{
    return TRUE;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_mutex_construct
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_mutex_construct(mpig_mutex_t * const mutex)
{
    *mutex = 0;
    return MPI_SUCCESS;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_mutex_destruct
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_mutex_destruct(mpig_mutex_t * const mutex)
{
    int mpi_errno = MPI_SUCCESS;

    if (*mutex == 0)
    {
        *mutex = -32767;
    }
    else
    {
        mpi_errno = MPI_ERR_OTHER;
    }
    
    return mpi_errno;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_mutex_lock
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_mutex_lock(mpig_mutex_t * const mutex)
{
#   if defined(MPIG_DEBUG)
    {
        int mpi_errno = (*mutex == 0) ? MPI_SUCCESS : MPI_ERR_OTHER;
        *mutex += 1;
        return mpi_errno;
    }
#   else
    {
        MPIG_UNUSED_ARG(mutex);
        return MPI_SUCCESS;
    }
#   endif
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_mutex_trylock
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_mutex_trylock(mpig_mutex_t * const mutex, bool_t * acquired_p)
{
#   if defined(MPIG_DEBUG)
    {
        if (*mutex == 0)
        {
            *mutex += 1;
            *acquired_p = TRUE;
            return MPI_SUCCESS;
        }
        else
        {
            *acquired_p = FALSE;
            return MPI_ERR_OTHER;
        }
    }
#   else
    {
        MPIG_UNUSED_ARG(mutex);
        *acquired_p = TRUE;
        return MPI_SUCCESS;
    }
#   endif
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_mutex_unlock
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_mutex_unlock(mpig_mutex_t * const mutex)
{
#   if defined(MPIG_DEBUG)
    {
        *mutex -= 1;
        return (*mutex == 0) ? MPI_SUCCESS : MPI_ERR_OTHER;
    }
#   else
    {
        MPIG_UNUSED_ARG(mutex);
        return MPI_SUCCESS;
    }
#   endif
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_cond_construct
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_cond_construct(mpig_cond_t * const cond)
{
    *cond = 0;
    return MPI_SUCCESS;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_cond_destruct
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_cond_destruct(mpig_cond_t * const cond)
{
    *cond = -32767;
    return MPI_SUCCESS;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_cond_wait
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_cond_wait(mpig_cond_t * const cond, mpig_mutex_t * const mutex)
{
    mpig_thread_yield();
    
#   if defined(MPIG_DEBUG)
    {
        int mpi_errno = MPI_SUCCESS;
        if (*cond != 0) mpi_errno = MPI_ERR_OTHER;
        if (*mutex != 0) mpi_errno = MPI_ERR_OTHER;
        return mpi_errno;
    }
#   else
    {
        MPIG_UNUSED_ARG(cond);
        MPIG_UNUSED_ARG(mutex);
        return MPI_SUCCESS;
    }
#   endif
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_cond_signal
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_cond_signal(mpig_cond_t * const cond)
{
#   if defined(MPIG_DEBUG)
    {
        return (*cond == 0) ? MPI_SUCCESS : MPI_ERR_OTHER;
    }
#   else
    {
        MPIG_UNUSED_ARG(cond);
        return MPI_SUCCESS;
    }
#   endif
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_cond_broadcast
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_cond_broadcast(mpig_cond_t * const cond)
{
#   if defined(MPIG_DEBUG)
    {
        return (*cond == 0) ? MPI_SUCCESS : MPI_ERR_OTHER;
    }
#   else
    {
        MPIG_UNUSED_ARG(cond);
        return MPI_SUCCESS;
    }
#   endif
}

#elif defined(HAVE_GLOBUS_THREADS)

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_thread_self
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF mpig_thread_t mpig_thread_self(void)
{
    return globus_thread_self();
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_thread_equal
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF bool_t mpig_thread_equal(mpig_thread_t t1, mpig_thread_t t2)
{
    int is_equal;
    is_equal = globus_thread_equal(t1, t2);
    return is_equal ? TRUE : FALSE;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_mutex_construct
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_mutex_construct(mpig_mutex_t * const mutex)
{
    int rc;
    rc = globus_mutex_init(mutex, NULL);
    return (rc == 0) ? MPI_SUCCESS : MPI_ERR_OTHER;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_mutex_destruct
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_mutex_destruct(mpig_mutex_t * const mutex)
{
    int rc;
    rc = globus_mutex_destroy(mutex);
    return (rc == 0) ? MPI_SUCCESS : MPI_ERR_OTHER;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_mutex_lock
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_mutex_lock(mpig_mutex_t * const mutex)
{
    int rc;
    rc = globus_mutex_lock(mutex);
    return (rc == 0) ? MPI_SUCCESS : MPI_ERR_OTHER;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_mutex_trylock
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_mutex_trylock(mpig_mutex_t * const mutex, bool_t * const acquired_p)
{
    int rc;
    int mpi_errno = MPI_SUCCESS;

    rc = globus_mutex_trylock(mutex);
    if (rc == 0)
    {
        *acquired_p = TRUE;
    }
    else if (rc == EBUSY)
    {
        *acquired_p = FALSE;
    }
    else
    {
        *acquired_p = FALSE;
        mpi_errno = MPI_ERR_OTHER;
    }

    return mpi_errno;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_mutex_unlock
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_mutex_unlock(mpig_mutex_t * mutex)
{
    int rc;
    rc = globus_mutex_unlock(mutex);
    return (rc == 0) ? MPI_SUCCESS : MPI_ERR_OTHER;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_cond_construct
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_cond_construct(mpig_cond_t * cond)
{
    int rc;
    rc = globus_cond_init(cond, NULL);
    return (rc == 0) ? MPI_SUCCESS : MPI_ERR_OTHER;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_cond_destruct
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_cond_destruct(mpig_cond_t * cond)
{
    int rc;
    rc = globus_cond_destroy(cond);
    return (rc == 0) ? MPI_SUCCESS : MPI_ERR_OTHER;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_cond_wait
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_cond_wait(mpig_cond_t * cond, mpig_mutex_t * mutex)
{
    int rc;
    rc = globus_cond_wait(cond, mutex);
    return (rc == 0) ? MPI_SUCCESS : MPI_ERR_OTHER;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_cond_signal
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_cond_signal(mpig_cond_t * cond)
{
    int rc;
    rc = globus_cond_signal(cond);
    return (rc == 0) ? MPI_SUCCESS : MPI_ERR_OTHER;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_cond_broadcast
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_cond_broadcast(mpig_cond_t * cond)
{
    int rc;
    rc = globus_cond_broadcast(cond);
    return (rc == 0) ? MPI_SUCCESS : MPI_ERR_OTHER;
}

#elif defined(HAVE_POSIX_THREADS)

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_thread_self
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF mpig_thread_t mpig_thread_self(void)
{
    return pthread_self();
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_thread_equal
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF bool_t mpig_thread_equal(mpig_thread_t t1, mpig_thread_t t2)
{
    int is_equal;
    is_equal = pthread_equal(t1, t2);
    return is_equal ? TRUE : FALSE;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_mutex_construct
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_mutex_construct(mpig_mutex_t * const mutex)
{
    int rc;
    rc = pthread_mutex_init(mutex, NULL);
    return (rc == 0) ? MPI_SUCCESS : MPI_ERR_OTHER;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_mutex_destruct
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_mutex_destruct(mpig_mutex_t * const mutex)
{
    int rc;
    rc = pthread_mutex_destroy(mutex);
    return (rc == 0) ? MPI_SUCCESS : MPI_ERR_OTHER;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_mutex_lock
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_mutex_lock(mpig_mutex_t * const mutex)
{
    int rc;
    rc = pthread_mutex_lock(mutex);
    return (rc == 0) ? MPI_SUCCESS : MPI_ERR_OTHER;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_mutex_trylock
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_mutex_trylock(mpig_mutex_t * const mutex, bool_t * const acquired_p)
{
    int rc;
    int mpi_errno = MPI_SUCCESS;

    rc = pthread_mutex_trylock(mutex);
    if (rc == 0)
    {
        *acquired_p = TRUE;
    }
    else if (errno == EBUSY)
    {
        *acquired_p = FALSE;
    }
    else
    {
        *acquired_p = FALSE;
        mpi_errno = MPI_ERR_OTHER;
    }

    return mpi_errno;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_mutex_unlock
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_mutex_unlock(mpig_mutex_t * mutex)
{
    int rc;
    rc = pthread_mutex_unlock(mutex);
    return (rc == 0) ? MPI_SUCCESS : MPI_ERR_OTHER;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_cond_construct
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_cond_construct(mpig_cond_t * cond)
{
    int rc;
    rc = pthread_cond_init(cond, NULL);
    return (rc == 0) ? MPI_SUCCESS : MPI_ERR_OTHER;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_cond_destruct
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_cond_destruct(mpig_cond_t * cond)
{
    int rc;
    rc = pthread_cond_destroy(cond);
    return (rc == 0) ? MPI_SUCCESS : MPI_ERR_OTHER;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_cond_wait
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_cond_wait(mpig_cond_t * cond, mpig_mutex_t * mutex)
{
    int rc;
    rc = pthread_cond_wait(cond, mutex);
    return (rc == 0) ? MPI_SUCCESS : MPI_ERR_OTHER;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_cond_signal
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_cond_signal(mpig_cond_t * cond)
{
    int rc;
    rc = pthread_cond_signal(cond);
    return (rc == 0) ? MPI_SUCCESS : MPI_ERR_OTHER;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_cond_broadcast
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_cond_broadcast(mpig_cond_t * cond)
{
    int rc;
    rc = pthread_cond_broadcast(cond);
    return (rc == 0) ? MPI_SUCCESS : MPI_ERR_OTHER;
}

#else
#error Configure did not report the availability of a threads package!
#endif

#undef FUNCNAME
#undef FCNAME

#endif /* defined(MPIG_INLINE_HDEF) */

#endif /* !defined(MPICH2_MPIG_THREAD_I_INCLUDED) || defined(MPIG_EMIT_INLINE_FUNCS) */
