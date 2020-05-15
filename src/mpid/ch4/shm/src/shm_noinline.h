/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SHM_NOINLINE_H_INCLUDED
#define SHM_NOINLINE_H_INCLUDED

#include "shm_impl.h"

int MPIDI_SHM_mpi_init_hook(int rank, int size, int *tag_bits);
int MPIDI_SHM_mpi_finalize_hook(void);
int MPIDI_SHM_get_vci_attr(int vci);

int MPIDI_SHM_mpi_comm_create_hook(MPIR_Comm * comm);
int MPIDI_SHM_mpi_comm_free_hook(MPIR_Comm * comm);
int MPIDI_SHM_mpi_type_commit_hook(MPIR_Datatype * type);
int MPIDI_SHM_mpi_type_free_hook(MPIR_Datatype * type);
int MPIDI_SHM_mpi_op_commit_hook(MPIR_Op * op);
int MPIDI_SHM_mpi_op_free_hook(MPIR_Op * op);
int MPIDI_SHM_mpi_win_create_hook(MPIR_Win * win);
int MPIDI_SHM_mpi_win_allocate_hook(MPIR_Win * win);
int MPIDI_SHM_mpi_win_allocate_shared_hook(MPIR_Win * win);
int MPIDI_SHM_mpi_win_create_dynamic_hook(MPIR_Win * win);
int MPIDI_SHM_mpi_win_attach_hook(MPIR_Win * win, void *base, MPI_Aint size);
int MPIDI_SHM_mpi_win_detach_hook(MPIR_Win * win, const void *base);
int MPIDI_SHM_mpi_win_free_hook(MPIR_Win * win);

int MPIDI_SHM_mpi_comm_connect(const char *port_name, MPIR_Info * info, int root, int timeout,
                               MPIR_Comm * comm, MPIR_Comm ** newcomm_ptr);
int MPIDI_SHM_mpi_comm_disconnect(MPIR_Comm * comm_ptr);
int MPIDI_SHM_mpi_open_port(MPIR_Info * info_ptr, char *port_name);
int MPIDI_SHM_mpi_close_port(const char *port_name);
int MPIDI_SHM_mpi_comm_accept(const char *port_name, MPIR_Info * info, int root, MPIR_Comm * comm,
                              MPIR_Comm ** newcomm_ptr);

int MPIDI_SHM_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info);
int MPIDI_SHM_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p);
int MPIDI_SHM_mpi_win_free(MPIR_Win ** win_ptr);
int MPIDI_SHM_mpi_win_create(void *base, MPI_Aint length, int disp_unit, MPIR_Info * info,
                             MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr);
int MPIDI_SHM_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size);
int MPIDI_SHM_mpi_win_allocate_shared(MPI_Aint size, int disp_unit, MPIR_Info * info_ptr,
                                      MPIR_Comm * comm_ptr, void **base_ptr, MPIR_Win ** win_ptr);
int MPIDI_SHM_mpi_win_detach(MPIR_Win * win, const void *base);
int MPIDI_SHM_mpi_win_allocate(MPI_Aint size, int disp_unit, MPIR_Info * info, MPIR_Comm * comm,
                               void *baseptr, MPIR_Win ** win);
int MPIDI_SHM_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm, MPIR_Win ** win);

int MPIDI_SHM_get_local_upids(MPIR_Comm * comm, size_t ** local_upid_size, char **local_upids);
int MPIDI_SHM_upids_to_lupids(int size, size_t * remote_upid_size, char *remote_upids,
                              int **remote_lupids);
int MPIDI_SHM_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr, int size, const int lpids[]);

void *MPIDI_SHM_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr);
int MPIDI_SHM_mpi_free_mem(void *ptr);
int MPIDI_SHM_progress(int vci, int blocking);

#endif
