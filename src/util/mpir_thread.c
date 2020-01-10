/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>

#ifdef MPICH_IS_THREADED
/* use MPIR_Add_mutex to register mutex, they will be created/destroyed together
 * at init/finalize */

#define MAX_MUTEX_COUNT 128
static MPID_Thread_mutex_t *mutex_list[MAX_MUTEX_COUNT];
static int mutex_count = 0;

void MPIR_Add_mutex(MPID_Thread_mutex_t * p_mutex)
{
    MPIR_Assert(mutex_count < MAX_MUTEX_COUNT);
    mutex_list[mutex_count++] = p_mutex;
}

/* All the other mutexes that depend on the configured thread granularity */
/* FIXME: we currently have  various mutexes scattered throughout the code
 * and their logic interweaved between thread model and objects/features.
 * We should use a register mechanism to manage creation and destroying of
 * all mutexes
 */

/* These routine handle any thread initialization that may be required */
void MPIR_Thread_CS_Init(void)
{
#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL
    /* Use the single MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX for all MPI calls */

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__POBJ
    /* MPICH_THREAD_GRANULARITY__POBJ: Multiple locks */
    MPIR_Add_mutex(&MPIR_THREAD_POBJ_HANDLE_MUTEX);
    MPIR_Add_mutex(&MPIR_THREAD_POBJ_MSGQ_MUTEX);
    MPIR_Add_mutex(&MPIR_THREAD_POBJ_COMPLETION_MUTEX);
    MPIR_Add_mutex(&MPIR_THREAD_POBJ_CTX_MUTEX);
    MPIR_Add_mutex(&MPIR_THREAD_POBJ_PMI_MUTEX);

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
    MPIR_Add_mutex(&MPIR_THREAD_VCI_GLOBAL_MUTEX);
    MPIR_Add_mutex(&MPIR_THREAD_VCI_HANDLE_MUTEX);
    for (int i = 0; i < HANDLE_NUM_POOLS; i++) {
        MPIR_Add_mutex(&MPIR_THREAD_VCI_HANDLE_POOL_MUTEXES[i].lock);
    }

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__LOCKFREE
/* Updates to shared data and access to shared services is handled without
   locks where ever possible. */
#error lock-free not yet implemented

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__SINGLE
/* No thread support, make all operations a no-op */

#else
#error Unrecognized thread granularity
#endif

    int err;
    for (int i = 0; i < mutex_count; i++) {
        MPID_Thread_mutex_create(mutex_list[i], &err);
        MPIR_Assert(err == 0);
    }

    MPID_THREADPRIV_KEY_CREATE;

    MPL_DBG_MSG(MPIR_DBG_INIT, TYPICAL, "Created global mutex and private storage");
}

void MPIR_Thread_CS_Finalize(void)
{
    MPL_DBG_MSG(MPIR_DBG_INIT, TYPICAL, "Freeing global mutex and private storage");

    int err;
    for (int i = 0; i < mutex_count; i++) {
        MPID_Thread_mutex_destroy(mutex_list[i], &err);
        MPIR_Assert(err == 0);
    }

    MPID_THREADPRIV_KEY_DESTROY;
}

#else
/* not MPICH_IS_THREADED, empty stubs */
void MPIR_Add_mutex(MPID_Thread_mutex_t * p_mutex)
{
}

void MPIR_Thread_CS_Init(void)
{
}

void MPIR_Thread_CS_Finalize(void)
{
}

#endif /* MPICH_IS_THREADED */
