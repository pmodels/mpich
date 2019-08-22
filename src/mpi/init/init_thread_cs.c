
/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpi_init.h"

#if defined(MPICH_IS_THREADED)

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL || \
    MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__POBJ   || \
    MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
MPID_Thread_mutex_t MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX;
#endif

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__POBJ
MPID_Thread_mutex_t MPIR_THREAD_POBJ_HANDLE_MUTEX;
MPID_Thread_mutex_t MPIR_THREAD_POBJ_MSGQ_MUTEX;
MPID_Thread_mutex_t MPIR_THREAD_POBJ_COMPLETION_MUTEX;
MPID_Thread_mutex_t MPIR_THREAD_POBJ_CTX_MUTEX;
MPID_Thread_mutex_t MPIR_THREAD_POBJ_PMI_MUTEX;
#endif

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
MPID_Thread_mutex_t MPIR_THREAD_POBJ_HANDLE_MUTEX;
#endif

/* These routine handle any thread initialization that my be required */
int MPIR_Thread_CS_Init(void)
{
    int err;

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL
/* There is a single, global lock, held for the duration of an MPI call */
    MPID_Thread_mutex_create(&MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, &err);
    MPIR_Assert(err == 0);

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__POBJ
    /* MPICH_THREAD_GRANULARITY__POBJ: Multiple locks */
    MPID_Thread_mutex_create(&MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, &err);
    MPIR_Assert(err == 0);
    MPID_Thread_mutex_create(&MPIR_THREAD_POBJ_HANDLE_MUTEX, &err);
    MPIR_Assert(err == 0);
    MPID_Thread_mutex_create(&MPIR_THREAD_POBJ_MSGQ_MUTEX, &err);
    MPIR_Assert(err == 0);
    MPID_Thread_mutex_create(&MPIR_THREAD_POBJ_COMPLETION_MUTEX, &err);
    MPIR_Assert(err == 0);
    MPID_Thread_mutex_create(&MPIR_THREAD_POBJ_CTX_MUTEX, &err);
    MPIR_Assert(err == 0);
    MPID_Thread_mutex_create(&MPIR_THREAD_POBJ_PMI_MUTEX, &err);
    MPIR_Assert(err == 0);

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
    MPID_Thread_mutex_create(&MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, &err);
    MPIR_Assert(err == 0);
    MPID_Thread_mutex_create(&MPIR_THREAD_POBJ_HANDLE_MUTEX, &err);
    MPIR_Assert(err == 0);

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__LOCKFREE
/* Updates to shared data and access to shared services is handled without
   locks where ever possible. */
#error lock-free not yet implemented

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__SINGLE
/* No thread support, make all operations a no-op */

#else
#error Unrecognized thread granularity
#endif

    MPID_THREADPRIV_KEY_CREATE;

    {
        /*
         * Hack to workaround an Intel compiler bug on macOS. Touching
         * MPIR_Per_thread in this file forces the compiler to allocate it as TLS.
         * See https://github.com/pmodels/mpich/issues/3437.
         */
        MPIR_Per_thread_t *per_thread = NULL;

        MPID_THREADPRIV_KEY_GET_ADDR(MPIR_ThreadInfo.isThreaded, MPIR_Per_thread_key,
                                     MPIR_Per_thread, per_thread, &err);
        MPIR_Assert(err == 0);
        memset(per_thread, 0, sizeof(MPIR_Per_thread_t));
    }

    MPL_DBG_MSG(MPIR_DBG_INIT, TYPICAL, "Created global mutex and private storage");
    return MPI_SUCCESS;
}

int MPIR_Thread_CS_Finalize(void)
{
    int err;

    MPL_DBG_MSG(MPIR_DBG_INIT, TYPICAL, "Freeing global mutex and private storage");
#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL
/* There is a single, global lock, held for the duration of an MPI call */
    MPID_Thread_mutex_destroy(&MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, &err);
    MPIR_Assert(err == 0);

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__POBJ
    /* MPICH_THREAD_GRANULARITY__POBJ: There are multiple locks,
     * one for each logical class (e.g., each type of object) */
    MPID_Thread_mutex_destroy(&MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, &err);
    MPIR_Assert(err == 0);
    MPID_Thread_mutex_destroy(&MPIR_THREAD_POBJ_HANDLE_MUTEX, &err);
    MPIR_Assert(err == 0);
    MPID_Thread_mutex_destroy(&MPIR_THREAD_POBJ_MSGQ_MUTEX, &err);
    MPIR_Assert(err == 0);
    MPID_Thread_mutex_destroy(&MPIR_THREAD_POBJ_COMPLETION_MUTEX, &err);
    MPIR_Assert(err == 0);
    MPID_Thread_mutex_destroy(&MPIR_THREAD_POBJ_CTX_MUTEX, &err);
    MPIR_Assert(err == 0);
    MPID_Thread_mutex_destroy(&MPIR_THREAD_POBJ_PMI_MUTEX, &err);
    MPIR_Assert(err == 0);

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
    MPID_Thread_mutex_destroy(&MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, &err);
    MPIR_Assert(err == 0);
    MPID_Thread_mutex_destroy(&MPIR_THREAD_POBJ_HANDLE_MUTEX, &err);
    MPIR_Assert(err == 0);

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__LOCKFREE
/* Updates to shared data and access to shared services is handled without
   locks where ever possible. */
#error lock-free not yet implemented

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__SINGLE
/* No thread support, make all operations a no-op */

#else
#error Unrecognized thread granularity
#endif

    MPID_CS_finalize();

    MPID_THREADPRIV_KEY_DESTROY;

    return MPI_SUCCESS;
}

#else /* MPICH_IS_THREADED */

int MPIR_Thread_CS_Init(void)
{
    return MPI_SUCCESS;
}

int MPIR_Thread_CS_Finalize(void)
{
    return MPI_SUCCESS;
}

#endif /* MPICH_IS_THREADED */
