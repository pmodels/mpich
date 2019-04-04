/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "../posix/posix_noinline.h"

int MPIDI_SHMI_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_MPI_WIN_SET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_MPI_WIN_SET_INFO);

    ret = MPIDI_POSIX_mpi_win_set_info(win, info);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_MPI_WIN_SET_INFO);
    return ret;
}

int MPIDI_SHMI_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_MPI_WIN_GET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_MPI_WIN_GET_INFO);

    ret = MPIDI_POSIX_mpi_win_get_info(win, info_p_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_MPI_WIN_GET_INFO);
    return ret;
}

int MPIDI_SHMI_mpi_win_free(MPIR_Win ** win_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_MPI_WIN_FREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_MPI_WIN_FREE);

    ret = MPIDI_POSIX_mpi_win_free(win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_MPI_WIN_FREE);
    return ret;
}

int MPIDI_SHMI_mpi_win_create(void *base, MPI_Aint length, int disp_unit, MPIR_Info * info,
                              MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_MPI_WIN_CREATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_MPI_WIN_CREATE);

    ret = MPIDI_POSIX_mpi_win_create(base, length, disp_unit, info, comm_ptr, win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_MPI_WIN_CREATE);
    return ret;
}

int MPIDI_SHMI_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_MPI_WIN_ATTACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_MPI_WIN_ATTACH);

    ret = MPIDI_POSIX_mpi_win_attach(win, base, size);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_MPI_WIN_ATTACH);
    return ret;
}

int MPIDI_SHMI_mpi_win_allocate_shared(MPI_Aint size, int disp_unit, MPIR_Info * info_ptr,
                                       MPIR_Comm * comm_ptr, void **base_ptr, MPIR_Win ** win_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_MPI_WIN_ALLOCATE_SHARED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_MPI_WIN_ALLOCATE_SHARED);

    ret = MPIDI_POSIX_mpi_win_allocate_shared(size, disp_unit, info_ptr, comm_ptr,
                                              base_ptr, win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_MPI_WIN_ALLOCATE_SHARED);
    return ret;
}

int MPIDI_SHMI_mpi_win_detach(MPIR_Win * win, const void *base)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_MPI_WIN_DETACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_MPI_WIN_DETACH);

    ret = MPIDI_POSIX_mpi_win_detach(win, base);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_MPI_WIN_DETACH);
    return ret;
}

int MPIDI_SHMI_mpi_win_allocate(MPI_Aint size, int disp_unit, MPIR_Info * info, MPIR_Comm * comm,
                                void *baseptr, MPIR_Win ** win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_MPI_WIN_ALLOCATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_MPI_WIN_ALLOCATE);

    ret = MPIDI_POSIX_mpi_win_allocate(size, disp_unit, info, comm, baseptr, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_MPI_WIN_ALLOCATE);
    return ret;
}

int MPIDI_SHMI_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm, MPIR_Win ** win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_MPI_WIN_CREATE_DYNAMIC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_MPI_WIN_CREATE_DYNAMIC);

    ret = MPIDI_POSIX_mpi_win_create_dynamic(info, comm, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_MPI_WIN_CREATE_DYNAMIC);
    return ret;
}
