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

#include "stubnm_impl.h"
#include "mpidimpl.h"

int MPIDI_STUBNM_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    return MPIDIG_mpi_win_set_info(win, info);
}

int MPIDI_STUBNM_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    return MPIDIG_mpi_win_get_info(win, info_p_p);
}

int MPIDI_STUBNM_mpi_win_free(MPIR_Win ** win_ptr)
{
    return MPIDIG_mpi_win_free(win_ptr);
}

int MPIDI_STUBNM_mpi_win_create(void *base, MPI_Aint length, int disp_unit, MPIR_Info * info,
                                MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr)
{
    return MPIDIG_mpi_win_create(base, length, disp_unit, info, comm_ptr, win_ptr);
}

int MPIDI_STUBNM_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    return MPIDIG_mpi_win_attach(win, base, size);
}

int MPIDI_STUBNM_mpi_win_allocate_shared(MPI_Aint size, int disp_unit, MPIR_Info * info_ptr,
                                         MPIR_Comm * comm_ptr, void **base_ptr, MPIR_Win ** win_ptr)
{
    return MPIDIG_mpi_win_allocate_shared(size, disp_unit, info_ptr, comm_ptr, base_ptr, win_ptr);
}

int MPIDI_STUBNM_mpi_win_detach(MPIR_Win * win, const void *base)
{
    return MPIDIG_mpi_win_detach(win, base);
}

int MPIDI_STUBNM_mpi_win_allocate(MPI_Aint size, int disp_unit, MPIR_Info * info, MPIR_Comm * comm,
                                  void *baseptr, MPIR_Win ** win)
{
    return MPIDIG_mpi_win_allocate(size, disp_unit, info, comm, baseptr, win);
}

int MPIDI_STUBNM_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm, MPIR_Win ** win)
{
    return MPIDIG_mpi_win_create_dynamic(info, comm, win);
}

int MPIDI_STUBNM_mpi_win_create_hook(MPIR_Win * win)
{
    return MPI_SUCCESS;
}

int MPIDI_STUBNM_mpi_win_allocate_hook(MPIR_Win * win)
{
    return MPI_SUCCESS;
}

int MPIDI_STUBNM_mpi_win_allocate_shared_hook(MPIR_Win * win)
{
    return MPI_SUCCESS;
}

int MPIDI_STUBNM_mpi_win_create_dynamic_hook(MPIR_Win * win)
{
    return MPI_SUCCESS;
}

int MPIDI_STUBNM_mpi_win_attach_hook(MPIR_Win * win, void *base, MPI_Aint size)
{
    return MPI_SUCCESS;
}

int MPIDI_STUBNM_mpi_win_detach_hook(MPIR_Win * win, const void *base)
{
    return MPI_SUCCESS;
}

int MPIDI_STUBNM_mpi_win_free_hook(MPIR_Win * win)
{
    return MPI_SUCCESS;
}
