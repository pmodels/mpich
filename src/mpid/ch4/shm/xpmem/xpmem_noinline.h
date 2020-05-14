/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef XPMEM_NOINLINE_H_INCLUDED
#define XPMEM_NOINLINE_H_INCLUDED

#include "xpmem_impl.h"

int MPIDI_XPMEM_mpi_init_hook(int rank, int size, int *tag_bits);
int MPIDI_XPMEM_mpi_finalize_hook(void);

int MPIDI_XPMEM_mpi_win_create_hook(MPIR_Win * win);
int MPIDI_XPMEM_mpi_win_free_hook(MPIR_Win * win);

#endif /* XPMEM_NOINLINE_H_INCLUDED */
