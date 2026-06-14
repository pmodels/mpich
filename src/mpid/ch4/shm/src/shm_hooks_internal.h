/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHM_HOOKS_INTERNAL_H_INCLUDED
#define SHM_HOOKS_INTERNAL_H_INCLUDED

#include <shm.h>
#include "../posix/shm_inline.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_op_cs_enter_hook(MPIR_Win * win)
{
    int ret;
    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_rma_op_cs_enter_hook(win);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_op_cs_exit_hook(MPIR_Win * win)
{
    int ret;
    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_rma_op_cs_exit_hook(win);

    MPIR_FUNC_EXIT;
    return ret;
}

#endif /* SHM_HOOKS_INTERNAL_H_INCLUDED */
