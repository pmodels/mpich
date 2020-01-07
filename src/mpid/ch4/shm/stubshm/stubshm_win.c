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

int MPIDI_STUBSHM_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

int MPIDI_STUBSHM_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

int MPIDI_STUBSHM_mpi_win_free(MPIR_Win ** win_ptr)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

int MPIDI_STUBSHM_mpi_win_create(void *base, MPI_Aint length, int disp_unit, MPIR_Info * info,
                                 MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

int MPIDI_STUBSHM_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

int MPIDI_STUBSHM_mpi_win_allocate_shared(MPI_Aint size, int disp_unit, MPIR_Info * info_ptr,
                                          MPIR_Comm * comm_ptr, void **base_ptr,
                                          MPIR_Win ** win_ptr)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

int MPIDI_STUBSHM_mpi_win_detach(MPIR_Win * win, const void *base)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

int MPIDI_STUBSHM_mpi_win_allocate(MPI_Aint size, int disp_unit, MPIR_Info * info, MPIR_Comm * comm,
                                   void *baseptr, MPIR_Win ** win)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

int MPIDI_STUBSHM_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm, MPIR_Win ** win)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}
