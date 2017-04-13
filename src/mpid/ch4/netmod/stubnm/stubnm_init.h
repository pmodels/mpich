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
#ifndef STUBNM_INIT_H_INCLUDED
#define STUBNM_INIT_H_INCLUDED

#include "stubnm_impl.h"

static inline int MPIDI_NM_mpi_init_hook(int rank,
                                         int size,
                                         int appnum,
                                         int *tag_ub,
                                         MPIR_Comm * comm_world,
                                         MPIR_Comm * comm_self,
                                         int spawned)
{
    int mpi_errno = MPI_SUCCESS;


    MPIR_Assert(0);
    return mpi_errno;
}

static inline int MPIDI_NM_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(0);
    return mpi_errno;
}


static inline int MPIDI_NM_comm_get_lpid(MPIR_Comm * comm_ptr,
                                         int idx, int *lpid_ptr, MPL_bool is_remote)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_get_local_upids(MPIR_Comm * comm, size_t ** local_upid_size,
                                           char **local_upids)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_upids_to_lupids(int size,
                                           size_t * remote_upid_size,
                                           char *remote_upids, int **remote_lupids)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr,
                                                       int size, const int lpids[])
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_mpi_free_mem(void *ptr)
{
    return MPIDI_CH4U_mpi_free_mem(ptr);
}

static inline void *MPIDI_NM_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    return MPIDI_CH4U_mpi_alloc_mem(size, info_ptr);
}


#endif /* STUBNM_INIT_H_INCLUDED */
