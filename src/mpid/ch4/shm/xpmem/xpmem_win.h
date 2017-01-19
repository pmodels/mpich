/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */
#ifndef XPMEM_WIN_H_INCLUDED
#define XPMEM_WIN_H_INCLUDED

#include "xpmem.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_mpi_win_allocate(MPI_Aint size, int disp_unit,
                                                          MPIR_Info * info, MPIR_Comm * comm,
                                                          void *baseptr, MPIR_Win ** win)
{
    void *local_buf, **all_bufs;
    int *all_sizes;
    xpmem_segid_t local_segid, *all_segids;
    struct xpmem_addr addr;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_MPI_WIN_ALLOCATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_MPI_WIN_ALLOCATE);

    local_buf = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    if (local_buf == MAP_FAILED) {
        mpi_errno = MPI_ERR_INTERNAL;
        MPIR_ERR_POP(mpi_errno);
    }

    local_segid = xpmem_make(local_buf, size, XPMEM_PERMIT_MODE, (void *) 0666);

    all_segids = MPL_malloc(comm->local_comm->local_size * sizeof(void *));
    all_bufs = MPL_malloc(comm->local_comm->local_size * sizeof(void *));
    all_sizes = MPL_malloc(comm->local_comm->local_size * sizeof(void *));

    /* FIXME: allgather the local segids from all processes on the node */
    /* FIXME: allgather the local sizes from all processes on the node */

    for (i = 0; i < comm->local_comm->local_size; i++) {
        addr.apid = xpmem_get(all_segids[i], XPMEM_RDWR, XPMEM_PERMIT_MODE, NULL);
        addr.offset = 0;
        all_bufs[i] = xpmem_attach(addr, all_sizes[i], NULL);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_MPI_WIN_ALLOCATE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_XPMEM_mpi_win_free(MPIR_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_FREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_FREE);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_FREE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#endif /* XPMEM_WIN_H_INCLUDED */
