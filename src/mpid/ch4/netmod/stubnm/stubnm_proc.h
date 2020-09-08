/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef STUBNM_PROC_H_INCLUDED
#define STUBNM_PROC_H_INCLUDED

#include "stubnm_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rank_is_local(int rank, MPIR_Comm * comm)
{
    int ret;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_RANK_IS_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_RANK_IS_LOCAL);

    MPIR_Assert(0);
    ret = 0;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_RANK_IS_LOCAL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_av_is_local(MPIDI_av_entry_t * av)
{
    int ret;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AV_IS_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AV_IS_LOCAL);

    MPIR_Assert(0);
    ret = 0;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AV_IS_LOCAL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_comm_get_lpid(MPIR_Comm * comm_ptr, int idx, int *lpid_ptr,
                                                    bool is_remote)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#endif /* STUBNM_PROC_H_INCLUDED */
