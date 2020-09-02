/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

/* Global progress (polling every vci) is required for correctness. Currently we adopt the
 * simple approach to do global progress every POLL_MASK.
 *
 * TODO: every time we do global progress, there will be a performance lag. We could --
 * * amortize the cost by rotating the global vci to be polled (might be insufficient)
 * * accept user hints (require new user interface)
 */
#define POLL_MASK 0xff

#ifdef MPL_TLS
static MPL_TLS int global_vci_poll_count = 0;
#else
/* We just need ensure global progress happen, so some race condition or even corruption
 * can be tolerated.  */
static int global_vci_poll_count = 0;
#endif

MPL_STATIC_INLINE_PREFIX int do_global_progress(void)
{
    if (MPIDI_global.n_vcis == 1) {
        return 1;
    } else {
        global_vci_poll_count++;
        return ((global_vci_poll_count & POLL_MASK) == 0);
    }
}

/* inside per-vci progress */
MPL_STATIC_INLINE_PREFIX void check_progress_made_idx(MPID_Progress_state * state, int idx)
{
    if (state->progress_counts[idx] != MPIDI_global.progress_counts[state->vci[idx]]) {
        state->progress_counts[idx] = MPIDI_global.progress_counts[state->vci[idx]];
        state->progress_made = 1;
    }
}

/* inside global progress */
MPL_STATIC_INLINE_PREFIX void check_progress_made_vci(MPID_Progress_state * state, int vci)
{
    for (int i = 0; i < state->vci_count; i++) {
        if (vci == state->vci[i]) {
            if (state->progress_counts[i] != MPIDI_global.progress_counts[state->vci[i]]) {
                state->progress_counts[i] = MPIDI_global.progress_counts[state->vci[i]];
                state->progress_made = 1;
            }
            break;
        }
    }
}

static int progress_test(MPID_Progress_state * state, int wait)
{
    int mpi_errno, made_progress;
    mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_PROGRESS_TEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_PROGRESS_TEST);

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

    if (do_global_progress()) {
        for (int vci = 0; vci < MPIDI_global.n_vcis; vci++) {
            MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
            if (state->flag & MPIDI_PROGRESS_NM) {
                mpi_errno = MPIDI_NM_progress(vci, 0);
            }
#ifndef MPIDI_CH4_DIRECT_NETMOD
            if (state->flag & MPIDI_PROGRESS_SHM && mpi_errno == MPI_SUCCESS) {
                mpi_errno = MPIDI_SHM_progress(vci, 0);
            }
#endif
            if (wait) {
                check_progress_made_vci(state, vci);
            }
            MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
            MPIR_ERR_CHECK(mpi_errno);
            if (wait && state->progress_made) {
                break;
            }
        }
    } else {
        for (int i = 0; i < state->vci_count; i++) {
            int vci = state->vci[i];
            MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
            if (state->flag & MPIDI_PROGRESS_NM) {
                mpi_errno = MPIDI_NM_progress(vci, 0);
            }
#ifndef MPIDI_CH4_DIRECT_NETMOD
            if (state->flag & MPIDI_PROGRESS_SHM && mpi_errno == MPI_SUCCESS) {
                mpi_errno = MPIDI_SHM_progress(vci, 0);
            }
#endif
            if (wait) {
                check_progress_made_idx(state, i);
            }
            MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
            MPIR_ERR_CHECK(mpi_errno);
            if (wait && state->progress_made) {
                break;
            }
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_PROGRESS_TEST);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static void progress_state_init(MPID_Progress_state * state, int wait)
{
    state->flag = MPIDI_PROGRESS_ALL;
    state->progress_made = 0;
    if (wait) {
        /* only wait functions need check progress_counts */
        for (int i = 0; i < MPIDI_global.n_vcis; i++) {
            state->progress_counts[i] = MPIDI_global.progress_counts[i];
        }
    }

    /* global progress by default */
    for (int i = 0; i < MPIDI_global.n_vcis; i++) {
        state->vci[i] = i;
    }
    state->vci_count = MPIDI_global.n_vcis;

}

int MPIDI_Progress_test(int flags)
{
    MPID_Progress_state state;
    progress_state_init(&state, 0);
    state.flag = flags;
    return progress_test(&state, 0);
}

void MPID_Progress_start(MPID_Progress_state * state)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_START);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_START);

    progress_state_init(state, 1);

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

int MPID_Progress_test(MPID_Progress_state * state)
{
    if (state == NULL) {
        MPID_Progress_state progress_state;

        progress_state_init(&progress_state, 0);
        return progress_test(&progress_state, 0);
    } else {
        return progress_test(state, 0);
    }
}

int MPID_Progress_poke(void)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_POKE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_POKE);

    ret = MPID_Progress_test(NULL);

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
    state->progress_made = 0;
    while (1) {
        mpi_errno = progress_test(state, 1);
        MPIR_ERR_CHECK(mpi_errno);
        if (state->progress_made) {
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
