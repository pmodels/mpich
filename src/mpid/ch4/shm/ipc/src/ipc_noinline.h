/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef IPC_NOINLINE_H_INCLUDED
#define IPC_NOINLINE_H_INCLUDED

#include "mpidimpl.h"
#include "../xpmem/xpmem_post.h"
#include "../gpu/gpu_post.h"

int MPIDI_IPC_init_local(void);
int MPIDI_IPC_comm_bootstrap(MPIR_Comm * comm);
int MPIDI_IPC_mpi_finalize_hook(void);

int MPIDI_IPC_mpi_win_create_hook(MPIR_Win * win);
int MPIDI_IPC_mpi_win_free_hook(MPIR_Win * win);

int MPIDI_FD_comm_bootstrap(MPIR_Comm * comm);

#endif /* IPC_NOINLINE_H_INCLUDED */
