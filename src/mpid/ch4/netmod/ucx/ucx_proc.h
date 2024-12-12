/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UCX_PROC_H_INCLUDED
#define UCX_PROC_H_INCLUDED

#include "ucx_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rank_is_local(int rank, MPIR_Comm * comm)
{
    int ret;
    MPIR_FUNC_ENTER;

    ret = MPIDIU_av_is_local(MPIDIU_comm_rank_to_av(comm, rank));

    MPIR_FUNC_EXIT;
    return ret;
}

#endif /* UCX_PROC_H_INCLUDED */
