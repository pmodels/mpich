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
#ifndef SHM_STUBSHM_INIT_H_INCLUDED
#define SHM_STUBSHM_INIT_H_INCLUDED

#include "stubshm_impl.h"

static inline int MPIDI_SHM_mpi_init_hook(int rank, int size)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_mpi_finalize_hook(void)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline void *MPIDI_SHM_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    MPIR_Assert(0);
    return NULL;
}

static inline int MPIDI_SHM_mpi_free_mem(void *ptr)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_comm_get_lpid(MPIR_Comm * comm_ptr,
                                          int idx, int *lpid_ptr, MPL_bool is_remote)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_gpid_get(MPIR_Comm * comm_ptr, int rank, MPIR_Gpid * gpid)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_get_node_id(MPIR_Comm * comm, int rank, MPID_Node_id_t * id_p)
{
    *id_p = (MPID_Node_id_t) 0;
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_get_max_node_id(MPIR_Comm * comm, MPID_Node_id_t * max_id_p)
{
    *max_id_p = (MPID_Node_id_t) 1;
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_getallincomm(MPIR_Comm * comm_ptr,
                                         int local_size, MPIR_Gpid local_gpids[], int *singleAVT)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_gpid_tolpidarray(int size, MPIR_Gpid gpid[], int lpid[])
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr,
                                                        int size, const int lpids[])
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#endif /* SHM_STUBSHM_INIT_H_INCLUDED */
