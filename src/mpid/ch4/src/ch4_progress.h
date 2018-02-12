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

#define MPIDI_PROGRESS_HOOKS  (1)
#define MPIDI_PROGRESS_NM     (1<<1)
#define MPIDI_PROGRESS_SHM    (1<<2)

#define MPIDI_PROGRESS_ALL (MPIDI_PROGRESS_HOOKS|MPIDI_PROGRESS_NM|MPIDI_PROGRESS_SHM)

/* Flags argument allows to control execution of different parts of progress function,
 * for aims of prioritization of different transports and reentrant-safety of progress call.
 *
 * MPIDI_PROGRESS_HOOKS - enables progress on progress hooks. Hooks may invoke upper-level logic internaly,
 *      that's why MPIDI_Progress_test call with MPIDI_PROGRESS_HOOKS set isn't reentrant safe, and shouldn't be called from netmod's fallback logic.
 * MPIDI_PROGRESS_NM and MPIDI_PROGRESS_SHM enables progress on transports only, and guarantee reentrant-safety.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_Progress_test
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Progress_test(int flags)
{
    int mpi_errno, made_progress, i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PROGRESS_TEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PROGRESS_TEST);

    if (OPA_load_int(&MPIDI_CH4_Global.active_progress_hooks) && (flags & MPIDI_PROGRESS_HOOKS)) {
        MPID_THREAD_CS_ENTER(POBJ, MPIDI_CH4I_THREAD_PROGRESS_MUTEX);
        for (i = 0; i < MAX_PROGRESS_HOOKS; i++) {
            if (MPIDI_CH4_Global.progress_hooks[i].active == TRUE) {
                MPIR_Assert(MPIDI_CH4_Global.progress_hooks[i].func_ptr != NULL);
                mpi_errno = MPIDI_CH4_Global.progress_hooks[i].func_ptr(&made_progress);
                if (mpi_errno) {
                    MPID_THREAD_CS_EXIT(POBJ, MPIDI_CH4I_THREAD_PROGRESS_MUTEX);
                    MPIR_ERR_POP(mpi_errno);
                }
            }
        }
        MPID_THREAD_CS_EXIT(POBJ, MPIDI_CH4I_THREAD_PROGRESS_MUTEX);
    }
    /* todo: progress unexp_list */

    if (flags & MPIDI_PROGRESS_NM) {
        mpi_errno = MPIDI_NM_progress(0, 0);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
    }
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    if (flags & MPIDI_PROGRESS_SHM) {
        mpi_errno = MPIDI_SHM_progress(0, 0);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
    }
#endif
    MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PROGRESS_TEST);
    return mpi_errno;
  fn_fail:
    goto fn_exit;;
}

#undef FUNCNAME
#define FUNCNAME MPID_Progress_test
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

/* Device level wait/test implementations */
#define MPID_Test MPIR_Test_impl
#define MPID_Testall MPIR_Testall_impl
#define MPID_Testany MPIR_Testany_impl
#define MPID_Testsome MPIR_Testsome_impl
#define MPID_Wait MPIR_Wait_impl
#define MPID_Waitall MPIR_Waitall_impl
#define MPID_Waitany MPIR_Waitany_impl
#define MPID_Waitsome MPIR_Waitsome_impl

#endif /* CH4_PROGRESS_H_INCLUDED */
