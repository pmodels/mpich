/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "mpidimpl.h"
#include "shm_noinline.h"
#include "../posix/posix_noinline.h"

int MPIDI_SHMI_mpi_init_hook(int rank, int size, int *n_vsis_provided, int *tag_bits)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_MPI_INIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_MPI_INIT_HOOK);

    ret = MPIDI_POSIX_mpi_init_hook(rank, size, n_vsis_provided, tag_bits);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_MPI_INIT_HOOK);
    return ret;
}

int MPIDI_SHMI_mpi_finalize_hook(void)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_MPI_FINALIZE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_MPI_FINALIZE_HOOK);

    ret = MPIDI_POSIX_mpi_finalize_hook();

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_MPI_FINALIZE_HOOK);
    return ret;
}

MPIDI_vci_resource_t MPIDI_SHMI_vsi_get_resource_info(int vsi)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_QUERY_VSI);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_QUERY_VSI);

    ret = MPIDI_POSIX_vsi_get_resource_info(vsi);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_QUERY_VSI);
    return ret;
}
