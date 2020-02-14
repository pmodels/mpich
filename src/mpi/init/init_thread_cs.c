
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

/* called the first thing in init so it can enter critical section immediately */
void MPII_init_thread_and_enter_cs(void)
{
    int thread_err;
    MPID_Thread_init(&thread_err);
    MPIR_Assert(thread_err == 0);

    /* creating the allfunc mutex that MPIR_Init_thread will use */
    int err;
    MPID_Thread_mutex_create(&MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, &err);
    MPIR_Assert(err == 0);

    /* Setting isThreaded to 0 to ensure no mutexes are used during Init. */
    MPIR_ThreadInfo.isThreaded = 0;
}

void MPII_init_thread_and_exit_cs(void)
{
    MPIR_ThreadInfo.isThreaded = (MPIR_ThreadInfo.thread_provided == MPI_THREAD_MULTIPLE);
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
    /* Setting isThreaded to 0 to ensure no mutexes are used during Finalize. */
    MPIR_ThreadInfo.isThreaded = 0;
}

void MPII_finalize_thread_and_exit_cs(void)
{
    int err;

    MPID_Thread_mutex_destroy(&MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_finalize(&err);
    MPIR_Assert(err == 0);
}

void MPII_finalize_thread_failed_exit_cs(void)
{
}

#else
/* not MPICH_IS_THREADED, empty stubs */
void MPII_init_thread_and_enter_cs(void)
{
}

void MPII_init_thread_and_exit_cs(void)
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
