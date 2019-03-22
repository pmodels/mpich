/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef SHM_INIT_H_INCLUDED
#define SHM_INIT_H_INCLUDED

#include <shm.h>
#include "../posix/shm_inline.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_init_hook(int rank, int size, int *n_vsis_provided,
                                                     int *tag_bits)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_INIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_INIT_HOOK);

    ret = MPIDI_POSIX_mpi_init_hook(rank, size, n_vsis_provided, tag_bits);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_INIT_HOOK);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_finalize_hook(void)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_FINALIZE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_FINALIZE_HOOK);

    ret = MPIDI_POSIX_mpi_finalize_hook();

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_FINALIZE_HOOK);
    return ret;
}

static inline MPIDI_vci_resource_t MPIDI_SHM_vsi_get_resource_info(int vsi)
{
    MPIDI_vci_resource_t ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_QUERY_VSI);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_QUERY_VSI);

    ret = MPIDI_POSIX_vsi_get_resource_info(vsi);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_QUERY_VSI);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_progress(int vsi, int blocking)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_PROGRESS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_PROGRESS);

    ret = MPIDI_POSIX_progress(blocking);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_PROGRESS);
    return ret;
}

#endif /* SHM_INIT_H_INCLUDED */
