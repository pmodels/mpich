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
#ifndef OFI_PROC_H_INCLUDED
#define OFI_PROC_H_INCLUDED

#include "ofi_impl.h"

static inline int MPIDI_NM_rank_is_local(int rank, MPIR_Comm * comm)
{
    int ret;




    ret = MPIDIU_rank_is_local(rank, comm);


    return ret;
}

static inline int MPIDI_NM_av_is_local(MPIDI_av_entry_t * av)
{
    int ret = 0;




    ret = MPIDIU_av_is_local(av);


    return ret;
}

static inline int MPIDI_NM_comm_get_lpid(MPIR_Comm * comm_ptr,
                                         int idx, int *lpid_ptr, bool is_remote)
{
    int avtid = 0, lpid = 0;
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM)
        MPIDIU_comm_rank_to_pid(comm_ptr, idx, &lpid, &avtid);
    else if (is_remote)
        MPIDIU_comm_rank_to_pid(comm_ptr, idx, &lpid, &avtid);
    else {
        MPIDIU_comm_rank_to_pid_local(comm_ptr, idx, &lpid, &avtid);
    }

    *lpid_ptr = MPIDIU_LUPID_CREATE(avtid, lpid);
    return MPI_SUCCESS;
}

#endif /* OFI_PROC_H_INCLUDED */
