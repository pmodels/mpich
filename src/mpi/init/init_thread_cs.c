
/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpi_init.h"

#if defined(MPICH_IS_THREADED)

/* MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX is always initialized regardless of
 * MPICH_THREAD_GRANULARITY (if MPICH_IS_THREADED).
 */

MPID_Thread_mutex_t MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX;

static int required_is_threaded;

/* called the first thing in init so it can enter critical section immediately */
void init_thread_and_enter_cs(int thread_required)
{
    int thread_err;
    MPL_thread_init(&thread_err);
    MPIR_Assert(thread_err == 0);

    /* creating the allfunc mutex that MPIR_Init_thread will use */
    int err;
    MPID_Thread_mutex_create(&MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, &err);
    MPIR_Assert(err == 0);

    /* We don't know requested or provided thread level yet, set isThreaded
     * to TRUE so we'll protect the init with critical section anyway.
     */
    /* To ensure consistency, we save the required isThreaded */
    required_is_threaded = (thread_required == MPI_THREAD_MULTIPLE);
    MPIR_ThreadInfo.isThreaded = required_is_threaded;
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
}

void init_thread_and_exit_cs(int thread_provided)
{
    /* create the rest of the mutexes */
    MPIR_Thread_CS_Init();

    /* need to ensure consistency here */
    int save_is_threaded = MPIR_ThreadInfo.isThreaded;
    MPIR_ThreadInfo.isThreaded = required_is_threaded;
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_ThreadInfo.isThreaded = save_is_threaded;
}

/* called only when encounter failure during during init */
void init_thread_failed_exit_cs(void)
{
    int err;
    MPID_Thread_mutex_destroy(&MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, &err);
    MPIR_Assert(err == 0);
}

/* called in MPI_Finalize after exiting critical section */
void finalize_thread_cs(void)
{
    /* destroy (all) mutex locks */
    MPIR_Thread_CS_Finalize();

    int thread_err;
    MPL_thread_finalize(&thread_err);
    MPIR_Assert(thread_err == 0);
}

/* All the other mutexes that depend on the configured thread granularity */
/* FIXME: we currently have  various mutexes scattered throughout the code
 * and their logic interweaved between thread model and objects/features.
 * We should use a register mechanism to manage creation and destroying of
 * all mutexes
 */

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
void MPIR_Thread_CS_Init(void)
{
    int err;

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL
    /* Use the single MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX for all MPI calls */

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__POBJ
    /* MPICH_THREAD_GRANULARITY__POBJ: Multiple locks */
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
}

void MPIR_Thread_CS_Finalize(void)
{
    int err;

    MPL_DBG_MSG(MPIR_DBG_INIT, TYPICAL, "Freeing global mutex and private storage");
    /* Always need destroy MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX */
    MPID_Thread_mutex_destroy(&MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, &err);
    MPIR_Assert(err == 0);

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL
    /* A single, global lock, held for the duration of an MPI call */

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__POBJ
    /* MPICH_THREAD_GRANULARITY__POBJ: There are multiple locks,
     * one for each logical class (e.g., each type of object) */
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
}

#else
/* not MPICH_IS_THREADED, empty stubs */
void pre_init_thread_cs(void)
{
}

void finalize_thread_cs(void)
{
}

void destroy_allfunc_mutex(void)
{
}

void MPIR_Thread_CS_Init(void)
{
}

void MPIR_Thread_CS_Finalize(void)
{
}

#endif /* MPICH_IS_THREADED */
