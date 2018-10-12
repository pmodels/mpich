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

#include "mpidimpl.h"
#include "stubnm_impl.h"

int MPIDI_STUBNM_mpi_comm_create_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(0);
    return mpi_errno;
}

int MPIDI_STUBNM_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(0);
    return mpi_errno;
}

int MPIDI_NM_mpi_comm_set_info_mutable(MPIR_Comm * comm, MPIR_Info * info)
{
    return MPIDI_CH4R_mpi_comm_set_info_mutable(comm, info);
}

int MPIDI_NM_mpi_comm_set_info_immutable(MPIR_Comm * comm, MPIR_Info * info)
{
    return MPIDI_CH4R_mpi_comm_set_info_immutable(comm, info);
}

int MPIDI_NM_mpi_comm_get_info(MPIR_Comm * comm, MPIR_Info ** info_p_p)
{
    return MPIDI_CH4R_mpi_comm_get_info(comm, info_p_p);
}
