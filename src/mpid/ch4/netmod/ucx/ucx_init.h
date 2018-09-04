/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#ifndef UCX_INIT_H_INCLUDED
#define UCX_INIT_H_INCLUDED

#include "ucx_impl.h"

static inline int MPIDI_NM_comm_get_lpid(MPIR_Comm * comm_ptr,
                                         int idx, int *lpid_ptr, MPL_bool is_remote)
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

#endif /* UCX_INIT_H_INCLUDED */
