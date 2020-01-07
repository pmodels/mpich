/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef SHM_HOOKS_H_INCLUDED
#define SHM_HOOKS_H_INCLUDED

#include <shm.h>
#include "../posix/shm_inline.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_win_cmpl_hook(MPIR_Win * win)
{
    int ret;

    ret = MPIDI_POSIX_rma_win_cmpl_hook(win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_win_local_cmpl_hook(MPIR_Win * win)
{
    int ret;

    ret = MPIDI_POSIX_rma_win_local_cmpl_hook(win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_target_cmpl_hook(int rank, MPIR_Win * win)
{
    int ret;

    ret = MPIDI_POSIX_rma_target_cmpl_hook(rank, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_target_local_cmpl_hook(int rank, MPIR_Win * win)
{
    int ret;

    ret = MPIDI_POSIX_rma_target_local_cmpl_hook(rank, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_op_cs_enter_hook(MPIR_Win * win)
{
    int ret;

    ret = MPIDI_POSIX_rma_op_cs_enter_hook(win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_op_cs_exit_hook(MPIR_Win * win)
{
    int ret;

    ret = MPIDI_POSIX_rma_op_cs_exit_hook(win);

    return ret;
}

#endif /* SHM_HOOKS_H_INCLUDED */
