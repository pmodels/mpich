
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
/* ch4 netmod functions */


#include "netmod_impl.h"

int MPIDI_NM_mpi_comm_set_info_mutable(MPIR_Comm * comm, MPIR_Info * info)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_COMM_SET_INFO_MUTABLE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_COMM_SET_INFO_MUTABLE);

    ret = MPIDI_NM_func->mpi_comm_set_info_mutable(comm, info);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_COMM_SET_INFO_MUTABLE);
    return ret;
}

int MPIDI_NM_mpi_comm_set_info_immutable(MPIR_Comm * comm, MPIR_Info * info)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_COMM_SET_INFO_IMMUTABLE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_COMM_SET_INFO_IMMUTABLE);

    ret = MPIDI_NM_func->mpi_comm_set_info_immutable(comm, info);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_COMM_SET_INFO_IMMUTABLE);
    return ret;
}

int MPIDI_NM_mpi_comm_get_info(MPIR_Comm * comm, MPIR_Info ** info_p_p)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_COMM_GET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_COMM_GET_INFO);

    ret = MPIDI_NM_func->mpi_comm_get_info(comm, info_p_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_COMMGET_INFO);
    return ret;
}
