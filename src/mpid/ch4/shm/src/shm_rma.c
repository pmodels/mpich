/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "shm_noinline.h"
#include "../posix/posix_noinline.h"

int MPIDI_SHMI_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    int ret;




    ret = MPIDI_POSIX_mpi_win_set_info(win, info);


    return ret;
}

int MPIDI_SHMI_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    int ret;




    ret = MPIDI_POSIX_mpi_win_get_info(win, info_p_p);


    return ret;
}

int MPIDI_SHMI_mpi_win_free(MPIR_Win ** win_ptr)
{
    int ret;




    ret = MPIDI_POSIX_mpi_win_free(win_ptr);


    return ret;
}

int MPIDI_SHMI_mpi_win_create(void *base, MPI_Aint length, int disp_unit, MPIR_Info * info,
                              MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr)
{
    int ret;




    ret = MPIDI_POSIX_mpi_win_create(base, length, disp_unit, info, comm_ptr, win_ptr);


    return ret;
}

int MPIDI_SHMI_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    int ret;




    ret = MPIDI_POSIX_mpi_win_attach(win, base, size);


    return ret;
}

int MPIDI_SHMI_mpi_win_allocate_shared(MPI_Aint size, int disp_unit, MPIR_Info * info_ptr,
                                       MPIR_Comm * comm_ptr, void **base_ptr, MPIR_Win ** win_ptr)
{
    int ret;




    ret = MPIDI_POSIX_mpi_win_allocate_shared(size, disp_unit, info_ptr, comm_ptr,
                                              base_ptr, win_ptr);


    return ret;
}

int MPIDI_SHMI_mpi_win_detach(MPIR_Win * win, const void *base)
{
    int ret;




    ret = MPIDI_POSIX_mpi_win_detach(win, base);


    return ret;
}

int MPIDI_SHMI_mpi_win_allocate(MPI_Aint size, int disp_unit, MPIR_Info * info, MPIR_Comm * comm,
                                void *baseptr, MPIR_Win ** win)
{
    int ret;




    ret = MPIDI_POSIX_mpi_win_allocate(size, disp_unit, info, comm, baseptr, win);


    return ret;
}

int MPIDI_SHMI_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm, MPIR_Win ** win)
{
    int ret;




    ret = MPIDI_POSIX_mpi_win_create_dynamic(info, comm, win);


    return ret;
}
