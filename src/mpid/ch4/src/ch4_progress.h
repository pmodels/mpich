/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_PROGRESS_H_INCLUDED
#define CH4_PROGRESS_H_INCLUDED

#include "ch4_impl.h"
#include "stream_workq.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_GLOBAL_PROGRESS
      category    : CH4
      type        : boolean
      default     : 1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If on, poll global progress every once a while. With per-vci configuration, turning global progress off may improve the threading performance.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* Global progress (polling every vci) is required for correctness. Currently we adopt the
 * simple approach to do global progress every MPIDI_CH4_PROG_POLL_MASK.
 */
#define MPIDI_CH4_PROG_POLL_MASK 0xff

extern MPL_TLS int global_vci_poll_count;

MPL_STATIC_INLINE_PREFIX int MPIDI_do_global_progress(void)
{
    if (MPIDI_global.n_total_vcis == 1 || !MPIDI_global.is_initialized ||
        !MPIR_CVAR_CH4_GLOBAL_PROGRESS) {
        return 0;
    } else {
        global_vci_poll_count++;
        return ((global_vci_poll_count & MPIDI_CH4_PROG_POLL_MASK) == 0);
    }
}

#define MPIDI_THREAD_CS_ENTER_VCI_OPTIONAL(vci)         \
    if (!MPIDI_VCI_IS_EXPLICIT(vci) && !(state->flag & MPIDI_PROGRESS_NM_LOCKLESS)) {	\
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock); \
    }

#define MPIDI_THREAD_CS_EXIT_VCI_OPTIONAL(vci)          \
    if (!MPIDI_VCI_IS_EXPLICIT(vci) && !(state->flag & MPIDI_PROGRESS_NM_LOCKLESS)) {  \
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);	\
    } while (0)


/* define MPIDI_PROGRESS to make the code more readable (to avoid nested '#ifdef's) */
#ifdef MPIDI_CH4_DIRECT_NETMOD
#define MPIDI_PROGRESS(vci) \
    do {                                              \
        if (state->flag & MPIDI_PROGRESS_NM && !made_progress) {	      \
            MPIDI_THREAD_CS_ENTER_VCI_OPTIONAL(vci);  \
            mpi_errno = MPIDI_NM_progress(vci, &made_progress); \
            MPIDI_THREAD_CS_EXIT_VCI_OPTIONAL(vci);   \
            MPIR_ERR_CHECK(mpi_errno); \
        }                                             \
    } while (0)

#else
#define MPIDI_PROGRESS(vci)			\
    do {                                                \
        if (state->flag & MPIDI_PROGRESS_SHM && !made_progress) { \
            MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);             \
            mpi_errno = MPIDI_SHM_progress(vci, &made_progress); \
            MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);              \
            MPIR_ERR_CHECK(mpi_errno); \
        }                                                               \
        if (state->flag & MPIDI_PROGRESS_NM && !made_progress) { \
            MPIDI_THREAD_CS_ENTER_VCI_OPTIONAL(vci);            \
            mpi_errno = MPIDI_NM_progress(vci, &made_progress); \
            MPIDI_THREAD_CS_EXIT_VCI_OPTIONAL(vci);                     \
            MPIR_ERR_CHECK(mpi_errno); \
        }                                                               \
  } while (0)
#endif

MPL_STATIC_INLINE_PREFIX int MPIDI_progress_test(MPID_Progress_state * state)
{
    int mpi_errno = MPI_SUCCESS;
    int made_progress = 0;

    MPIR_FUNC_ENTER;

#ifdef HAVE_SIGNAL
    if (MPIDI_global.sigusr1_count > MPIDI_global.my_sigusr1_count) {
        MPIDI_global.my_sigusr1_count = MPIDI_global.sigusr1_count;
        mpi_errno = MPIDI_check_for_failed_procs();
        MPIR_ERR_CHECK(mpi_errno);
    }
#endif

#ifdef ENABLE_THREADCOMM
    MPIR_Threadcomm_progress(&made_progress);
    if (made_progress) {
        goto fn_exit;
    }
#endif

    if (state->flag & MPIDI_PROGRESS_HOOKS) {
        mpi_errno = MPIR_Progress_hook_exec_all(&made_progress);
        MPIR_ERR_CHECK(mpi_errno);
        if (made_progress) {
            goto fn_exit;
        }
    }
    /* todo: progress unexp_list */

#if MPIDI_CH4_MAX_VCIS == 1
    /* fast path for single vci */
    MPIDI_PROGRESS(0);
#else
    /* multiple vci */
    bool is_explicit_vci = (state->vci_count == 1 && MPIDI_VCI_IS_EXPLICIT(state->vci[0]));
    if (!is_explicit_vci && MPIDI_do_global_progress()) {
        for (int vci = 0; vci < MPIDI_global.n_vcis; vci++) {
            MPIDI_PROGRESS(vci);
        }
    } else {
        for (int i = 0; i < state->vci_count; i++) {
            int vci = state->vci[i];
            if (vci >= MPIDI_global.n_total_vcis) {
                continue;
            }
            MPIDI_PROGRESS(vci);
        }
    }
#endif

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Init with all VCIs. Performance critical path should always pass in explicit
 * state, thus avoid poking all progresses */
MPL_STATIC_INLINE_PREFIX void MPIDI_progress_state_init(MPID_Progress_state * state)
{
    state->flag = MPIDI_PROGRESS_ALL;
    /* For lockless, no VCI lock is needed during NM progress */
    if (MPIDI_CH4_MT_MODEL == MPIDI_CH4_MT_LOCKLESS) {
        state->flag |= MPIDI_PROGRESS_NM_LOCKLESS;
    }

    if (!MPIDI_global.is_initialized) {
        state->vci[0] = 0;
        state->vci_count = 1;
    } else {
        /* global progress by default */
        for (int i = 0; i < MPIDI_global.n_total_vcis; i++) {
            state->vci[i] = i;
        }
        state->vci_count = MPIDI_global.n_total_vcis;
    }
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Progress_test(int flags)
{
    MPID_Progress_state state;
    MPIDI_progress_state_init(&state);
    state.flag = flags;
    return MPIDI_progress_test(&state);
}

/* provide an internal direct progress function. This is used in e.g. RMA, where
 * we need poke internal progress from inside a per-vci lock.
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_progress_test_vci(int vci)
{
    int mpi_errno = MPI_SUCCESS;

    if (!MPIDI_VCI_IS_EXPLICIT(vci) && MPIDI_do_global_progress()) {
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
        mpi_errno = MPID_Progress_test(NULL);
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    } else {
        int made_progress = 0;
        mpi_errno = MPIDI_NM_progress(vci, &made_progress);
        MPIR_ERR_CHECK(mpi_errno);
#ifndef MPIDI_CH4_DIRECT_NETMOD
        mpi_errno = MPIDI_SHM_progress(vci, &made_progress);
        MPIR_ERR_CHECK(mpi_errno);
#endif
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX void MPID_Progress_start(MPID_Progress_state * state)
{
    MPIR_FUNC_ENTER;

    MPIDI_progress_state_init(state);

    MPIR_FUNC_EXIT;
    return;
}

MPL_STATIC_INLINE_PREFIX void MPID_Progress_end(MPID_Progress_state * state)
{
    MPIR_FUNC_ENTER;

    MPIR_FUNC_EXIT;
    return;
}

MPL_STATIC_INLINE_PREFIX int MPID_Progress_test(MPID_Progress_state * state)
{
    if (state == NULL) {
        MPID_Progress_state progress_state;

        MPIDI_progress_state_init(&progress_state);
        return MPIDI_progress_test(&progress_state);
    } else {
        return MPIDI_progress_test(state);
    }
}

MPL_STATIC_INLINE_PREFIX int MPID_Progress_poke(void)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPID_Progress_test(NULL);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Stream_progress(MPIR_Stream * stream_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (stream_ptr == NULL) {
        MPID_Progress_test(NULL);
    } else {
        if (stream_ptr->type == MPIR_STREAM_GPU) {
            MPIDU_stream_workq_progress_ops(stream_ptr->vci);
        }
        MPID_Progress_state state;
        state.flag = MPIDI_PROGRESS_ALL;
        /* For lockless, no VCI lock is needed during NM progress */
        if (MPIDI_CH4_MT_MODEL == MPIDI_CH4_MT_LOCKLESS) {
            state.flag |= MPIDI_PROGRESS_NM_LOCKLESS;
        }

        state.vci[0] = stream_ptr->vci;
        state.vci_count = 1;
        MPID_Progress_test(&state);

        if (stream_ptr->type == MPIR_STREAM_GPU) {
            MPIDU_stream_workq_progress_wait_list(stream_ptr->vci);
        }
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL
#define MPIDI_PROGRESS_YIELD() MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX)
#else
#define MPIDI_PROGRESS_YIELD() MPID_Thread_yield()
#endif

MPL_STATIC_INLINE_PREFIX int MPID_Progress_wait(MPID_Progress_state * state)
{
    int mpi_errno = MPID_Progress_test(state);
    MPIDI_PROGRESS_YIELD();
    return mpi_errno;
}

#endif /* CH4_PROGRESS_H_INCLUDED */
