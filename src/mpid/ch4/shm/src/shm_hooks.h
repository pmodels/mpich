/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SHM_HOOKS_H_INCLUDED
#define SHM_HOOKS_H_INCLUDED

#include <shm.h>
#include "../posix/shm_inline.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_win_cmpl_hook(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_rma_win_cmpl_hook(win);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_win_local_cmpl_hook(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_rma_win_local_cmpl_hook(win);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_target_cmpl_hook(int rank, MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_rma_target_cmpl_hook(rank, win);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_target_local_cmpl_hook(int rank, MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_rma_target_local_cmpl_hook(rank, win);

    MPIR_FUNC_EXIT;
    return ret;
}

#endif /* SHM_HOOKS_H_INCLUDED */
