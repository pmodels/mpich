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

    ret = MPIDIU_rank_is_local(rank, comm);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_comm_get_gpid(MPIR_Comm * comm_ptr,
                                                    int idx, uint64_t * gpid_ptr, bool is_remote)
{
    int avtid = 0, lpid = 0;
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        MPIDIU_comm_rank_to_pid(comm_ptr, idx, &lpid, &avtid);
    } else if (is_remote) {
        MPIDIU_comm_rank_to_pid(comm_ptr, idx, &lpid, &avtid);
    } else {
        MPIDIU_comm_rank_to_pid_local(comm_ptr, idx, &lpid, &avtid);
    }

    *gpid_ptr = MPIDIU_GPID_CREATE(avtid, lpid);
    return MPI_SUCCESS;

}

#endif /* UCX_PROC_H_INCLUDED */
