/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

int MPIDI_Progress_test(int flags)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PROGRESS_TEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PROGRESS_TEST);

#ifdef HAVE_SIGNAL
    if (MPIDI_global.sigusr1_count > MPIDI_global.my_sigusr1_count) {
        MPIDI_global.my_sigusr1_count = MPIDI_global.sigusr1_count;
        mpi_errno = MPIDI_check_for_failed_procs();
        MPIR_ERR_CHECK(mpi_errno);
    }
#endif

    if (flags & MPIDI_PROGRESS_HOOKS) {
        int made_progress = 0;
        mpi_errno = MPIR_Progress_hook_exec_all(&made_progress);
        MPIR_ERR_CHECK(mpi_errno);
        if (made_progress) {
            goto fn_exit;
        }
    }
    /* todo: progress unexp_list */

#ifdef MPIDI_CH4_USE_WORK_QUEUES
    mpi_errno = MPIDI_workq_vci_progress();
    MPIR_ERR_CHECK(mpi_errno);
#endif

    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);

    if (flags & MPIDI_PROGRESS_NM) {
        mpi_errno = MPIDI_NM_progress(0, 0);
        if (mpi_errno != MPI_SUCCESS) {
            MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);
            MPIR_ERR_POP(mpi_errno);
        }
    }
#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (flags & MPIDI_PROGRESS_SHM) {
        mpi_errno = MPIDI_SHM_progress(0, 0);
        if (mpi_errno != MPI_SUCCESS) {
            MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);
            MPIR_ERR_POP(mpi_errno);
        }
    }
#endif
    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PROGRESS_TEST);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Progress_test(void)
{
    return MPIDI_Progress_test(MPIDI_PROGRESS_ALL);
}

int MPID_Progress_poke(void)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_POKE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_POKE);

    ret = MPID_Progress_test();

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_POKE);
    return ret;
}

void MPID_Progress_start(MPID_Progress_state * state)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_START);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_START);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_START);
    return;
}

void MPID_Progress_end(MPID_Progress_state * state)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_END);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_END);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_END);
    return;
}

/* when MPID_Progress_test hold the lock too long -- e.g. more that a thousand cycles
 * such as in the case of an empty netmod progress -- it can put the other waiting
 * threads to a sleep, resulting in lock monopoly due to the short window of YIELD
 * being missed. It maybe necessary to add sleep or sched_yield to ensure fairness.
 */
#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL
#define CH4_PROGRESS_YIELD \
    MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX)
#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
#define CH4_PROGRESS_YIELD \
    MPL_thread_yield()
#endif

int MPID_Progress_wait(MPID_Progress_state * state)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_WAIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_WAIT);

    /* we need ensure no other progress has made between testing of request completion
     * (in MPIR_Wait) and loading of progress count. Otherwise, we may be unaware of
     * progress being made and stuck in a empty test loop.
     * FIXME: this is an issue need solve once we remove VCI_GLOBAL mutex.
     */
    int old_count = MPL_atomic_load_int(&MPIDI_global.progress_count);
    MPID_THREAD_CS_EXIT(VCI, MPIR_THREAD_VCI_GLOBAL_MUTEX);
#ifdef MPIDI_CH4_USE_WORK_QUEUES
    mpi_errno = MPID_Progress_test();
    MPIR_ERR_CHECK(mpi_errno);
    CH4_PROGRESS_YIELD;
#else
    while (1) {
        mpi_errno = MPID_Progress_test();
        MPIR_ERR_CHECK(mpi_errno);
        int new_count = MPL_atomic_load_int(&MPIDI_global.progress_count);
        if (old_count != new_count) {
            break;
        }
        CH4_PROGRESS_YIELD;
    }

#endif
    MPID_THREAD_CS_ENTER(VCI, MPIR_THREAD_VCI_GLOBAL_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_WAIT);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
