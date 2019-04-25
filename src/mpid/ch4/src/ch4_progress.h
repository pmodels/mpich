/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef CH4_PROGRESS_H_INCLUDED
#define CH4_PROGRESS_H_INCLUDED

#include "ch4_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_Progress_test(int flags)
{
    int mpi_errno, made_progress, i;
    mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PROGRESS_TEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PROGRESS_TEST);

#ifdef HAVE_SIGNAL
    if (MPIDI_global.sigusr1_count > MPIDI_global.my_sigusr1_count) {
        MPIDI_global.my_sigusr1_count = MPIDI_global.sigusr1_count;
        mpi_errno = MPIDI_check_for_failed_procs();
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
#endif

    if (flags & MPIDI_PROGRESS_HOOKS) {
        for (i = 0; i < MPIDI_global.registered_progress_hooks; i++) {
            progress_func_ptr_t func_ptr = NULL;
            MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_PROGRESS_HOOK_MUTEX);
            if (MPIDI_global.progress_hooks[i].active == TRUE) {
                MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_PROGRESS_HOOK_MUTEX);
                func_ptr = MPIDI_global.progress_hooks[i].func_ptr;
                MPIR_Assert(func_ptr != NULL);
                mpi_errno = func_ptr(&made_progress);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);

            } else {
                MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_PROGRESS_HOOK_MUTEX);
            }
        }
    }
    /* todo: progress unexp_list */

    mpi_errno = MPIDI_workq_vci_progress();
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);

    if (flags & MPIDI_PROGRESS_NM) {
        mpi_errno = MPIDI_NM_progress(0, 0);
        if (mpi_errno != MPI_SUCCESS) {
            MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
            MPIR_ERR_POP(mpi_errno);
        }
    }
#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (flags & MPIDI_PROGRESS_SHM) {
        mpi_errno = MPIDI_SHM_progress(0, 0);
        if (mpi_errno != MPI_SUCCESS) {
            MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);
            MPIR_ERR_POP(mpi_errno);
        }
    }
#endif
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(MPIDI_VCI_ROOT).lock);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PROGRESS_TEST);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Progress_test(void)
{
    return MPIDI_Progress_test(MPIDI_PROGRESS_ALL);
}

MPL_STATIC_INLINE_PREFIX int MPID_Progress_poke(void)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_POKE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_POKE);

    ret = MPID_Progress_test();

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_POKE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX void MPID_Progress_start(MPID_Progress_state * state)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_START);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_START);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_START);
    return;
}

MPL_STATIC_INLINE_PREFIX void MPID_Progress_end(MPID_Progress_state * state)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_END);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_END);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_END);
    return;
}

MPL_STATIC_INLINE_PREFIX int MPID_Progress_wait(MPID_Progress_state * state)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_WAIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_WAIT);

    if (MPIDI_CH4_MT_MODEL != MPIDI_CH4_MT_DIRECT) {
        ret = MPID_Progress_test();
        if (unlikely(ret))
            MPIR_ERR_POP(ret);
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
        goto fn_exit;
    }

    state->progress_count = OPA_load_int(&MPIDI_global.progress_count);
    do {
        ret = MPID_Progress_test();
        if (unlikely(ret))
            MPIR_ERR_POP(ret);
        if (state->progress_count != OPA_load_int(&MPIDI_global.progress_count))
            break;
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    } while (1);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_WAIT);

  fn_exit:
    return ret;

  fn_fail:
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPID_Progress_register(int (*progress_fn) (int *), int *id)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_REGISTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_REGISTER);

    for (i = 0; i < MAX_PROGRESS_HOOKS; i++) {
        if (MPIDI_global.progress_hooks[i].func_ptr == NULL) {
            MPIDI_global.progress_hooks[i].func_ptr = progress_fn;
            MPIDI_global.progress_hooks[i].active = FALSE;
            break;
        }
    }

    if (i >= MAX_PROGRESS_HOOKS)
        goto fn_fail;

    MPIDI_global.registered_progress_hooks++;

    (*id) = i;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_REGISTER);
    return mpi_errno;
  fn_fail:
    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                     "MPID_Progress_register", __LINE__,
                                     MPI_ERR_INTERN, "**progresshookstoomany", 0);
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Progress_deregister(int id)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_DEREGISTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_DEREGISTER);

    MPIR_Assert(id >= 0);
    MPIR_Assert(id < MAX_PROGRESS_HOOKS);
    MPIR_Assert(MPIDI_global.progress_hooks[id].func_ptr != NULL);
    MPIDI_global.progress_hooks[id].func_ptr = NULL;
    MPIDI_global.progress_hooks[id].active = FALSE;

    MPIDI_global.registered_progress_hooks--;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_DEREGISTER);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPID_Progress_activate(int id)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_ACTIVATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_ACTIVATE);

    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_PROGRESS_HOOK_MUTEX);
    MPIR_Assert(id >= 0);
    MPIR_Assert(id < MAX_PROGRESS_HOOKS);
    /* Asserting that active == FALSE shouldn't be done outside the global lock
     * model. With fine-grained locks, two threads might try to activate the same
     * hook concurrently, in which case one of them will correctly detect that
     * active == TRUE because the other thread set it.*/

    if (MPIDI_global.progress_hooks[id].active == FALSE) {
        MPIR_Assert(MPIDI_global.progress_hooks[id].func_ptr != NULL);
        MPIDI_global.progress_hooks[id].active = TRUE;
    }

    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_PROGRESS_HOOK_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_ACTIVATE);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPID_Progress_deactivate(int id)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_DEACTIVATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_DEACTIVATE);

    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_PROGRESS_HOOK_MUTEX);
    MPIR_Assert(id >= 0);
    MPIR_Assert(id < MAX_PROGRESS_HOOKS);
    /* We shouldn't assert that active == TRUE here for the same reasons
     * as not asserting active == FALSE in Progress_activate */

    if (MPIDI_global.progress_hooks[id].active == TRUE) {
        MPIR_Assert(MPIDI_global.progress_hooks[id].func_ptr != NULL);
        MPIDI_global.progress_hooks[id].active = FALSE;
    }

    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_PROGRESS_HOOK_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_DEACTIVATE);
    return mpi_errno;
}

#endif /* CH4_PROGRESS_H_INCLUDED */
