/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

static int progress_made(MPID_Progress_state * state)
{
    if (state->progress_count != state->progress_start) {
        return TRUE;
    }
    state->progress_count = MPL_atomic_load_int(&MPIDI_global.progress_count);
    return (state->progress_count != state->progress_start);
}

static int progress_test(MPID_Progress_state * state)
{
    int mpi_errno, made_progress;
    mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PROGRESS_TEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PROGRESS_TEST);

#ifdef HAVE_SIGNAL
    if (MPIDI_global.sigusr1_count > MPIDI_global.my_sigusr1_count) {
        MPIDI_global.my_sigusr1_count = MPIDI_global.sigusr1_count;
        mpi_errno = MPIDI_check_for_failed_procs();
        MPIR_ERR_CHECK(mpi_errno);
    }
#endif

    if (state->flag & MPIDI_PROGRESS_HOOKS) {
        mpi_errno = MPIR_Progress_hook_exec_all(&made_progress);
        MPIR_ERR_CHECK(mpi_errno);
    }
    /* todo: progress unexp_list */

#ifdef MPIDI_CH4_USE_WORK_QUEUES
    mpi_errno = MPIDI_workq_vci_progress();
    MPIR_ERR_CHECK(mpi_errno);
#endif

    for (int i = 0; i < MPIDI_global.n_vcis; i++) {
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(i).lock);
        if (!progress_made(state)) {
            if (state->flag & MPIDI_PROGRESS_NM) {
                mpi_errno = MPIDI_NM_progress(i, 0);
            }
        }
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(i).lock);
    }

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (state->flag & MPIDI_PROGRESS_SHM && mpi_errno == MPI_SUCCESS) {
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
        mpi_errno = MPIDI_SHM_progress(0, 0);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PROGRESS_TEST);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_Progress_test(int flags)
{
    MPID_Progress_state state;
    MPID_Progress_start(&state);
    state.flag = flags;
    return progress_test(&state);
}

void MPID_Progress_start(MPID_Progress_state * state)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_START);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_START);

    state->flag = MPIDI_PROGRESS_ALL;
    state->progress_count = MPL_atomic_load_int(&MPIDI_global.progress_count);
    state->progress_start = state->progress_count;

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

int MPID_Progress_test(void)
{
    MPID_Progress_state state;
    MPID_Progress_start(&state);
    return progress_test(&state);
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

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL
#define MPIDI_PROGRESS_YIELD() MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX)
#else
#define MPIDI_PROGRESS_YIELD() MPL_thread_yield()
#endif

int MPID_Progress_wait(MPID_Progress_state * state)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_WAIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_WAIT);

#ifdef MPIDI_CH4_USE_WORK_QUEUES
    mpi_errno = MPID_Progress_test(state);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_PROGRESS_YIELD();

#else
    /* track progress from last time left off */
    state->progress_start = state->progress_count;
    while (1) {
        mpi_errno = progress_test(state);
        MPIR_ERR_CHECK(mpi_errno);
        if (progress_made(state)) {
            break;
        }
        MPIDI_PROGRESS_YIELD();
    }

#endif
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_WAIT);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
