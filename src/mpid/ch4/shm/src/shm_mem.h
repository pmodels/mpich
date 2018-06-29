/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef SHM_MEM_H_INCLUDED
#define SHM_MEM_H_INCLUDED

#include <shm.h>
#include "../posix/shm_inline.h"

MPL_STATIC_INLINE_PREFIX void *MPIDI_SHM_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    void *ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_ALLOC_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_ALLOC_MEM);

    ret = MPIDI_POSIX_mpi_alloc_mem(size, info_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_ALLOC_MEM);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_free_mem(void *ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_FREE_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_FREE_MEM);

    ret = MPIDI_POSIX_mpi_free_mem(ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_FREE_MEM);
    return ret;
}

#endif /* SHM_MEM_H_INCLUDED */
