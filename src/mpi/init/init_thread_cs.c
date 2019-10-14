
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

static int required_is_threaded;

/* called the first thing in init so it can enter critical section immediately */
void MPII_init_thread_and_enter_cs(int thread_required)
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

void MPII_init_thread_and_exit_cs(int thread_provided)
{
    /* need to ensure consistency here */
    int save_is_threaded = MPIR_ThreadInfo.isThreaded;
    MPIR_ThreadInfo.isThreaded = required_is_threaded;
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_ThreadInfo.isThreaded = save_is_threaded;
}

/* called only when encounter failure during during init */
void MPII_init_thread_failed_exit_cs(void)
{
    int err;
    MPID_Thread_mutex_destroy(&MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, &err);
    MPIR_Assert(err == 0);
}

/* similar set of functions for finalize. */
void MPII_finalize_thread_and_enter_cs(void)
{
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
}

void MPII_finalize_thread_and_exit_cs(void)
{
    int err;

    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    MPID_Thread_mutex_destroy(&MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPL_thread_finalize(&err);
    MPIR_Assert(err == 0);
}

void MPII_finalize_thread_failed_exit_cs(void)
{
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
}

#else
/* not MPICH_IS_THREADED, empty stubs */
void MPII_init_thread_and_enter_cs(int thread_required)
{
}

void MPII_init_thread_and_exit_cs(int thread_provided)
{
}

void MPII_init_thread_failed_exit_cs(void)
{
}

void MPII_finalize_thread_and_enter_cs(void)
{
}

void MPII_finalize_thread_and_exit_cs(void)
{
}

void MPII_finalize_thread_failed_exit_cs(void)
{
}

#endif /* MPICH_IS_THREADED */
