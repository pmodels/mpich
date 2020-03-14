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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_RANK_IS_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_RANK_IS_LOCAL);

    ret = MPIDIU_rank_is_local(rank, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_RANK_IS_LOCAL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_av_is_local(MPIDI_av_entry_t * av)
{
    int ret;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AV_IS_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AV_IS_LOCAL);

    ret = MPIDIU_av_is_local(av);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AV_IS_LOCAL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_comm_get_lpid(MPIR_Comm * comm_ptr,
                                                    int idx, int *lpid_ptr, bool is_remote)
{
    int avtid = 0, lpid = 0;
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        MPIDIU_comm_rank_to_pid(comm_ptr, idx, &lpid, &avtid);
    } else if (is_remote) {
        MPIDIU_comm_rank_to_pid(comm_ptr, idx, &lpid, &avtid);
    } else {
        MPIDIU_comm_rank_to_pid_local(comm_ptr, idx, &lpid, &avtid);
    }

    *lpid_ptr = MPIDIU_LUPID_CREATE(avtid, lpid);
    return MPI_SUCCESS;

}

#endif /* UCX_PROC_H_INCLUDED */
