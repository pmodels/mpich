/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef SHM_INIT_H_INCLUDED
#define SHM_INIT_H_INCLUDED

#include <shm.h>
#include "../posix/shm_direct.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_init_hook(int rank, int size, int *n_vnis_provided)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_INIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_INIT_HOOK);

    ret = MPIDI_POSIX_mpi_init_hook(rank, size, n_vnis_provided);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_INIT_HOOK);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_finalize_hook(void)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_FINALIZE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_FINALIZE_HOOK);

    ret = MPIDI_POSIX_mpi_finalize_hook();

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_FINALIZE_HOOK);
    return ret;
}

static inline int MPIDI_SHM_get_vni_attr(int vni)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_QUERY_VNI);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_QUERY_VNI);

    ret = MPIDI_POSIX_get_vni_attr(vni);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_QUERY_VNI);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_progress(int vni, int blocking)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_PROGRESS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_PROGRESS);

    ret = MPIDI_POSIX_progress(blocking);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_PROGRESS);
    return ret;
}

#endif /* SHM_INIT_H_INCLUDED */
