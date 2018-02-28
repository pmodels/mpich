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

static inline int MPIDI_have_progress_thread(void)
{
    if (MPIDI_CH4_MT_MODEL == MPIDI_CH4_MT_HANDOFF) {
        return OPA_load_int(&MPIDI_CH4_Global.n_active_progress_threads);
    } else {
        return 0;
    }
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Progress_test
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Progress_test(int flags)
{
    int mpi_errno, made_progress, i;
    mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PROGRESS_TEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PROGRESS_TEST);

#ifdef HAVE_SIGNAL
    if (MPIDI_CH4_Global.sigusr1_count > MPIDI_CH4_Global.my_sigusr1_count) {
        MPIDI_CH4_Global.my_sigusr1_count = MPIDI_CH4_Global.sigusr1_count;
        mpi_errno = MPIDI_check_for_failed_procs();
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
#endif

    if (OPA_load_int(&MPIDI_CH4_Global.active_progress_hooks) && (flags & MPIDI_PROGRESS_HOOKS)) {
        MPID_THREAD_CS_ENTER(POBJ, MPIDI_CH4I_THREAD_PROGRESS_MUTEX);
        MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4I_THREAD_PROGRESS_MUTEX);
        for (i = 0; i < MAX_PROGRESS_HOOKS; i++) {
            if (MPIDI_CH4_Global.progress_hooks[i].active == TRUE) {
                MPIR_Assert(MPIDI_CH4_Global.progress_hooks[i].func_ptr != NULL);
                mpi_errno = MPIDI_CH4_Global.progress_hooks[i].func_ptr(&made_progress);
                if (mpi_errno) {
                    MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4I_THREAD_PROGRESS_MUTEX);
                    MPID_THREAD_CS_EXIT(POBJ, MPIDI_CH4I_THREAD_PROGRESS_MUTEX);
                    MPIR_ERR_POP(mpi_errno);
                }
            }
        }
        MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4I_THREAD_PROGRESS_MUTEX);
        MPID_THREAD_CS_EXIT(POBJ, MPIDI_CH4I_THREAD_PROGRESS_MUTEX);
    }
    /* todo: progress unexp_list */

    if (!(flags & MPIDI_PROGRESS_NM))
        goto skip_nm;

    /* Netmod progress */
    for (i = 0; i < MPIDI_CH4_Global.n_netmod_vnis; i++) {
        if (flags & MPIDI_PROGRESS_NM_VNI_LOCK)
            MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_locks[i]);
        mpi_errno = MPIDI_NM_progress(i, 0);
        if (flags & MPIDI_PROGRESS_NM_VNI_LOCK)
            MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_locks[i]);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
    }

  skip_nm:
#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (flags & MPIDI_PROGRESS_SHM) {
        mpi_errno = MPIDI_SHM_progress(0, 0);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
    }
#endif
    MPID_THREAD_CS_YIELD(VNI_GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PROGRESS_TEST);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Progress_test
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Progress_test(void)
{
    int flags = MPIDI_PROGRESS_ALL;

    if (MPIDI_have_progress_thread())
        flags &= ~(MPIDI_PROGRESS_NM);

    return MPIDI_Progress_test(flags);
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

    ret = MPID_Progress_test();

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_WAIT);
    return ret;
}


#undef FUNCNAME
#define FUNCNAME MPID_Progress_register
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Progress_register(int (*progress_fn) (int *), int *id)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_REGISTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_REGISTER);

    MPID_THREAD_CS_ENTER(POBJ, MPIDI_CH4I_THREAD_PROGRESS_HOOK_MUTEX);
    for (i = 0; i < MAX_PROGRESS_HOOKS; i++) {
        if (MPIDI_CH4_Global.progress_hooks[i].func_ptr == NULL) {
            MPIDI_CH4_Global.progress_hooks[i].func_ptr = progress_fn;
            MPIDI_CH4_Global.progress_hooks[i].active = FALSE;
            break;
        }
    }

    if (i >= MAX_PROGRESS_HOOKS)
        goto fn_fail;

    OPA_incr_int(&MPIDI_CH4_Global.active_progress_hooks);

    (*id) = i;

  fn_exit:
    MPID_THREAD_CS_EXIT(POBJ, MPIDI_CH4I_THREAD_PROGRESS_HOOK_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_REGISTER);
    return mpi_errno;
  fn_fail:
    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                     "MPID_Progress_register", __LINE__,
                                     MPI_ERR_INTERN, "**progresshookstoomany", 0);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Progress_deregister
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Progress_deregister(int id)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_DEREGISTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_DEREGISTER);

    MPID_THREAD_CS_ENTER(POBJ, MPIDI_CH4I_THREAD_PROGRESS_HOOK_MUTEX);
    MPIR_Assert(id >= 0);
    MPIR_Assert(id < MAX_PROGRESS_HOOKS);
    MPIR_Assert(MPIDI_CH4_Global.progress_hooks[id].func_ptr != NULL);
    MPIDI_CH4_Global.progress_hooks[id].func_ptr = NULL;
    MPIDI_CH4_Global.progress_hooks[id].active = FALSE;

    OPA_decr_int(&MPIDI_CH4_Global.active_progress_hooks);
    MPID_THREAD_CS_EXIT(POBJ, MPIDI_CH4I_THREAD_PROGRESS_HOOK_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_DEREGISTER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Progress_activate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Progress_activate(int id)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_ACTIVATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_ACTIVATE);

    MPID_THREAD_CS_ENTER(POBJ, MPIDI_CH4I_THREAD_PROGRESS_HOOK_MUTEX);
    MPIR_Assert(id >= 0);
    MPIR_Assert(id < MAX_PROGRESS_HOOKS);
    MPIR_Assert(MPIDI_CH4_Global.progress_hooks[id].active == FALSE);
    MPIR_Assert(MPIDI_CH4_Global.progress_hooks[id].func_ptr != NULL);
    MPIDI_CH4_Global.progress_hooks[id].active = TRUE;

    MPID_THREAD_CS_EXIT(POBJ, MPIDI_CH4I_THREAD_PROGRESS_HOOK_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_ACTIVATE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Progress_deactivate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Progress_deactivate(int id)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROGRESS_DEACTIVATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROGRESS_DEACTIVATE);

    MPID_THREAD_CS_ENTER(POBJ, MPIDI_CH4I_THREAD_PROGRESS_HOOK_MUTEX);
    MPIR_Assert(id >= 0);
    MPIR_Assert(id < MAX_PROGRESS_HOOKS);
    MPIR_Assert(MPIDI_CH4_Global.progress_hooks[id].active == TRUE);
    MPIR_Assert(MPIDI_CH4_Global.progress_hooks[id].func_ptr != NULL);
    MPIDI_CH4_Global.progress_hooks[id].active = FALSE;

    MPID_THREAD_CS_EXIT(POBJ, MPIDI_CH4I_THREAD_PROGRESS_HOOK_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROGRESS_DEACTIVATE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_progress_thread_fn
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void MPIDI_progress_thread_fn(void *data)
{
    int i, vni_idx, n_vnis = *((int *) data);
    const int *vnis = ((int *) data) + 1;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PROGRESS_THREAD_FN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PROGRESS_THREAD_FN);

    /* Inform that I am spawned */
    OPA_incr_int(&MPIDI_CH4_Global.n_active_progress_threads);

    do {
        for (i = 0; i < n_vnis; i++) {
            vni_idx = vnis[i];
            MPIDI_workq_vni_progress(vni_idx);
            MPIDI_NM_progress(vni_idx, 0);
        }
    } while (OPA_load_int(&MPIDI_CH4_Global.progress_thread_exit_signal) == 0);
    OPA_decr_int(&MPIDI_CH4_Global.n_active_progress_threads);

    MPL_free(data);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PROGRESS_THREAD_FN);
}

#endif /* CH4_PROGRESS_H_INCLUDED */
