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
#ifndef STUBNM_PROC_H_INCLUDED
#define STUBNM_PROC_H_INCLUDED

#include "stubnm_impl.h"

static inline int MPIDI_NM_rank_is_local(int rank, MPIR_Comm * comm)
{
    int ret;



    MPIR_Assert(0);
    ret = 0;


    return ret;
}

static inline int MPIDI_NM_av_is_local(MPIDI_av_entry_t * av)
{
    int ret;



    MPIR_Assert(0);
    ret = 0;


    return ret;
}

static inline int MPIDI_NM_comm_get_lpid(MPIR_Comm * comm_ptr, int idx, int *lpid_ptr,
                                         bool is_remote)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#endif /* STUBNM_PROC_H_INCLUDED */
