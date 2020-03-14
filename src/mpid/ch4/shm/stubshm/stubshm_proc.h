/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */
#ifndef STUBSHM_PROC_H_INCLUDED
#define STUBSHM_PROC_H_INCLUDED

#include "stubshm_impl.h"

static inline int MPIDI_STUBSHM_rank_is_local(int rank, MPIR_Comm * comm)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_RANK_IS_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_RANK_IS_LOCAL);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_RANK_IS_LOCAL);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_av_is_local(MPIDI_av_entry_t * av)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_AV_IS_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_AV_IS_LOCAL);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_AV_IS_LOCAL);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_comm_get_lpid(MPIR_Comm * comm_ptr,
                                              int idx, int *lpid_ptr, bool is_remote)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_COMM_GET_LPID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_COMM_GET_LPID);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_COMM_GET_LPID);
    return MPI_SUCCESS;
}

#endif /* STUBSHM_PROC_H_INCLUDED */
