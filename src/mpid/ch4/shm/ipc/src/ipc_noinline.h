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

int MPIDI_FD_mpi_init_hook(void);
int MPIDI_FD_mpi_finalize_hook(void);

int MPIDI_IPC_mpi_socks_init(void);
int MPIDI_IPC_mpi_fd_init(bool use_drmfd);
int MPIDI_IPC_mpi_fd_finalize(bool use_drmfd);
int MPIDI_IPC_mpi_fd_send(int rank, int fd, void *payload, size_t payload_len);
int MPIDI_IPC_mpi_fd_recv(int rank, int *fd, void *payload, size_t payload_len, int flags);

#endif /* IPC_NOINLINE_H_INCLUDED */
