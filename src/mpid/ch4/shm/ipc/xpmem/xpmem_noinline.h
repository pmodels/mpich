/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef XPMEM_NOINLINE_H_INCLUDED
#define XPMEM_NOINLINE_H_INCLUDED

#include "mpidimpl.h"

#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
int MPIDI_XPMEM_mpi_init_hook(int rank, int size, int *tag_bits);
int MPIDI_XPMEM_mpi_finalize_hook(void);
#else

/* stub noninline functions */
MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_mpi_init_hook(int rank, int size, int *tag_bits)
{
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_mpi_finalize_hook(void)
{
    return MPI_SUCCESS;
}
#endif

#endif /* XPMEM_NOINLINE_H_INCLUDED */
