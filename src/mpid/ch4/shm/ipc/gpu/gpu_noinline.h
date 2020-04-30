/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef GPU_NOINLINE_H_INCLUDED
#define GPU_NOINLINE_H_INCLUDED

#include "mpidimpl.h"

#ifdef MPIDI_CH4_SHM_ENABLE_GPU
int MPIDI_GPU_mpi_init_hook(int rank, int size, int *tag_bits);
int MPIDI_GPU_mpi_finalize_hook(void);
#else
#define MPIDI_GPU_mpi_init_hook(...) MPI_SUCCESS
#define MPIDI_GPU_mpi_finalize_hook(...) MPI_SUCCESS
#endif

#endif /* GPU_NOINLINE_H_INCLUDED */
