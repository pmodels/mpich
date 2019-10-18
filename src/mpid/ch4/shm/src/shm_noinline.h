/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef SHM_NOINLINE_H_INCLUDED
#define SHM_NOINLINE_H_INCLUDED

#include "shm_impl.h"

int MPIDI_SHMI_mpi_init_hook(int rank, int size, int *n_vsis_provided, int *tag_bits);
int MPIDI_SHMI_mpi_finalize_hook(void);
MPIDI_vci_resource_t MPIDI_SHMI_vsi_get_resource_info(int vsi);

int MPIDI_SHMI_mpi_comm_create_hook(MPIR_Comm * comm);
int MPIDI_SHMI_mpi_comm_free_hook(MPIR_Comm * comm);
int MPIDI_SHMI_mpi_type_commit_hook(MPIR_Datatype * type);
int MPIDI_SHMI_mpi_type_free_hook(MPIR_Datatype * type);
int MPIDI_SHMI_mpi_op_commit_hook(MPIR_Op * op);
int MPIDI_SHMI_mpi_op_free_hook(MPIR_Op * op);
int MPIDI_SHMI_mpi_win_create_hook(MPIR_Win * win);
int MPIDI_SHMI_mpi_win_allocate_hook(MPIR_Win * win);
int MPIDI_SHMI_mpi_win_allocate_shared_hook(MPIR_Win * win);
int MPIDI_SHMI_mpi_win_create_dynamic_hook(MPIR_Win * win);
int MPIDI_SHMI_mpi_win_attach_hook(MPIR_Win * win, void *base, MPI_Aint size);
int MPIDI_SHMI_mpi_win_detach_hook(MPIR_Win * win, const void *base);
int MPIDI_SHMI_mpi_win_free_hook(MPIR_Win * win);

int MPIDI_SHMI_mpi_comm_connect(const char *port_name, MPIR_Info * info, int root, int timeout,
                                MPIR_Comm * comm, MPIR_Comm ** newcomm_ptr);
int MPIDI_SHMI_mpi_comm_disconnect(MPIR_Comm * comm_ptr);
int MPIDI_SHMI_mpi_open_port(MPIR_Info * info_ptr, char *port_name);
int MPIDI_SHMI_mpi_close_port(const char *port_name);
int MPIDI_SHMI_mpi_comm_accept(const char *port_name, MPIR_Info * info, int root, MPIR_Comm * comm,
                               MPIR_Comm ** newcomm_ptr);

int MPIDI_SHMI_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info);
int MPIDI_SHMI_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p);
int MPIDI_SHMI_mpi_win_free(MPIR_Win ** win_ptr);
int MPIDI_SHMI_mpi_win_create(void *base, MPI_Aint length, int disp_unit, MPIR_Info * info,
                              MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr);
int MPIDI_SHMI_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size);
int MPIDI_SHMI_mpi_win_allocate_shared(MPI_Aint size, int disp_unit, MPIR_Info * info_ptr,
                                       MPIR_Comm * comm_ptr, void **base_ptr, MPIR_Win ** win_ptr);
int MPIDI_SHMI_mpi_win_detach(MPIR_Win * win, const void *base);
int MPIDI_SHMI_mpi_win_allocate(MPI_Aint size, int disp_unit, MPIR_Info * info, MPIR_Comm * comm,
                                void *baseptr, MPIR_Win ** win);
int MPIDI_SHMI_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm, MPIR_Win ** win);

int MPIDI_SHMI_get_local_upids(MPIR_Comm * comm, size_t ** local_upid_size, char **local_upids);
int MPIDI_SHMI_upids_to_lupids(int size, size_t * remote_upid_size, char *remote_upids,
                               int **remote_lupids);
int MPIDI_SHMI_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr, int size, const int lpids[]);

void *MPIDI_SHMI_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr);
int MPIDI_SHMI_mpi_free_mem(void *ptr);
int MPIDI_SHMI_progress(int vsi, int blocking);

#ifdef SHM_INLINE
#define MPIDI_SHM_mpi_init_hook MPIDI_SHMI_mpi_init_hook
#define MPIDI_SHM_mpi_finalize_hook MPIDI_SHMI_mpi_finalize_hook
#define MPIDI_SHM_vsi_get_resource_info MPIDI_SHMI_vsi_get_resource_info
#define MPIDI_SHM_mpi_comm_create_hook MPIDI_SHMI_mpi_comm_create_hook
#define MPIDI_SHM_mpi_comm_free_hook MPIDI_SHMI_mpi_comm_free_hook
#define MPIDI_SHM_mpi_type_commit_hook MPIDI_SHMI_mpi_type_commit_hook
#define MPIDI_SHM_mpi_type_free_hook MPIDI_SHMI_mpi_type_free_hook
#define MPIDI_SHM_mpi_op_commit_hook MPIDI_SHMI_mpi_op_commit_hook
#define MPIDI_SHM_mpi_op_free_hook MPIDI_SHMI_mpi_op_free_hook
#define MPIDI_SHM_mpi_win_create_hook MPIDI_SHMI_mpi_win_create_hook
#define MPIDI_SHM_mpi_win_allocate_hook MPIDI_SHMI_mpi_win_allocate_hook
#define MPIDI_SHM_mpi_win_allocate_shared_hook MPIDI_SHMI_mpi_win_allocate_shared_hook
#define MPIDI_SHM_mpi_win_create_dynamic_hook MPIDI_SHMI_mpi_win_create_dynamic_hook
#define MPIDI_SHM_mpi_win_attach_hook MPIDI_SHMI_mpi_win_attach_hook
#define MPIDI_SHM_mpi_win_detach_hook MPIDI_SHMI_mpi_win_detach_hook
#define MPIDI_SHM_mpi_win_free_hook MPIDI_SHMI_mpi_win_free_hook
#define MPIDI_SHM_mpi_comm_connect MPIDI_SHMI_mpi_comm_connect
#define MPIDI_SHM_mpi_comm_disconnect MPIDI_SHMI_mpi_comm_disconnect
#define MPIDI_SHM_mpi_open_port MPIDI_SHMI_mpi_open_port
#define MPIDI_SHM_mpi_close_port MPIDI_SHMI_mpi_close_port
#define MPIDI_SHM_mpi_comm_accept MPIDI_SHMI_mpi_comm_accept
#define MPIDI_SHM_mpi_win_set_info MPIDI_SHMI_mpi_win_set_info
#define MPIDI_SHM_mpi_win_get_info MPIDI_SHMI_mpi_win_get_info
#define MPIDI_SHM_mpi_win_free MPIDI_SHMI_mpi_win_free
#define MPIDI_SHM_mpi_win_create MPIDI_SHMI_mpi_win_create
#define MPIDI_SHM_mpi_win_attach MPIDI_SHMI_mpi_win_attach
#define MPIDI_SHM_mpi_win_allocate_shared MPIDI_SHMI_mpi_win_allocate_shared
#define MPIDI_SHM_mpi_win_detach MPIDI_SHMI_mpi_win_detach
#define MPIDI_SHM_mpi_win_allocate MPIDI_SHMI_mpi_win_allocate
#define MPIDI_SHM_mpi_win_create_dynamic MPIDI_SHMI_mpi_win_create_dynamic
#define MPIDI_SHM_get_local_upids MPIDI_SHMI_get_local_upids
#define MPIDI_SHM_upids_to_lupids MPIDI_SHMI_upids_to_lupids
#define MPIDI_SHM_create_intercomm_from_lpids MPIDI_SHMI_create_intercomm_from_lpids
#define MPIDI_SHM_mpi_alloc_mem MPIDI_SHMI_mpi_alloc_mem
#define MPIDI_SHM_mpi_free_mem MPIDI_SHMI_mpi_free_mem
#define MPIDI_SHM_progress MPIDI_SHMI_progress
#endif

#endif
