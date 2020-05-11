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
#define MPIDI_XPMEM_mpi_init_hook(...) MPI_SUCCESS
#define MPIDI_XPMEM_mpi_finalize_hook(...) MPI_SUCCESS
#endif

#endif /* XPMEM_NOINLINE_H_INCLUDED */
