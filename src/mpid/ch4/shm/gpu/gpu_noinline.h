/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef GPU_NOINLINE_H_INCLUDED
#define GPU_NOINLINE_H_INCLUDED

#include "gpu_impl.h"

int MPIDI_GPU_mpi_init_hook(int rank, int size, int *tag_bits);
int MPIDI_GPU_mpi_finalize_hook(void);

#endif /* GPU_NOINLINE_H_INCLUDED */
