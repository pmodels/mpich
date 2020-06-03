/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_NOINLINE_H_INCLUDED
#define OFI_NOINLINE_H_INCLUDED

#include "ofi_impl.h"

/* ofi_comm.h */
int MPIDI_OFI_mpi_comm_commit_pre_hook(MPIR_Comm * comm);
int MPIDI_OFI_mpi_comm_commit_post_hook(MPIR_Comm * comm);
int MPIDI_OFI_mpi_comm_free_hook(MPIR_Comm * comm);

#ifdef NETMOD_INLINE
#define MPIDI_NM_mpi_comm_commit_pre_hook MPIDI_OFI_mpi_comm_commit_pre_hook
#define MPIDI_NM_mpi_comm_commit_post_hook MPIDI_OFI_mpi_comm_commit_post_hook
#define MPIDI_NM_mpi_comm_free_hook MPIDI_OFI_mpi_comm_free_hook
#endif

/* ofi_spawn.h */
int MPIDI_OFI_mpi_comm_connect(const char *port_name, MPIR_Info * info, int root, int timeout,
                               MPIR_Comm * comm_ptr, MPIR_Comm ** newcomm);
int MPIDI_OFI_mpi_comm_disconnect(MPIR_Comm * comm_ptr);
int MPIDI_OFI_mpi_open_port(MPIR_Info * info_ptr, char *port_name);
int MPIDI_OFI_mpi_close_port(const char *port_name);
int MPIDI_OFI_mpi_comm_accept(const char *port_name, MPIR_Info * info, int root,
                              MPIR_Comm * comm_ptr, MPIR_Comm ** newcomm);

#ifdef NETMOD_INLINE
#define MPIDI_NM_mpi_comm_connect MPIDI_OFI_mpi_comm_connect
#define MPIDI_NM_mpi_comm_disconnect MPIDI_OFI_mpi_comm_disconnect
#define MPIDI_NM_mpi_open_port MPIDI_OFI_mpi_open_port
#define MPIDI_NM_mpi_close_port MPIDI_OFI_mpi_close_port
#define MPIDI_NM_mpi_comm_accept MPIDI_OFI_mpi_comm_accept
#endif

int MPIDI_OFI_mpi_op_free_hook(MPIR_Op * op_p);
int MPIDI_OFI_mpi_op_commit_hook(MPIR_Op * op_p);

#ifdef NETMOD_INLINE
#define MPIDI_NM_mpi_op_free_hook MPIDI_OFI_mpi_op_free_hook
#define MPIDI_NM_mpi_op_commit_hook MPIDI_OFI_mpi_op_commit_hook
#endif

/* ofi_win.h */
int MPIDI_OFI_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info);
int MPIDI_OFI_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p);
int MPIDI_OFI_mpi_win_free(MPIR_Win ** win_ptr);
int MPIDI_OFI_mpi_win_create(void *base, MPI_Aint length, int disp_unit, MPIR_Info * info,
                             MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr);
int MPIDI_OFI_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size);
int MPIDI_OFI_mpi_win_allocate_shared(MPI_Aint size, int disp_unit, MPIR_Info * info_ptr,
                                      MPIR_Comm * comm_ptr, void **base_ptr, MPIR_Win ** win_ptr);
int MPIDI_OFI_mpi_win_detach(MPIR_Win * win, const void *base);
int MPIDI_OFI_mpi_win_allocate(MPI_Aint size, int disp_unit, MPIR_Info * info, MPIR_Comm * comm,
                               void *baseptr, MPIR_Win ** win_ptr);
int MPIDI_OFI_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm, MPIR_Win ** win_ptr);
int MPIDI_OFI_mpi_win_create_hook(MPIR_Win * win);
int MPIDI_OFI_mpi_win_allocate_hook(MPIR_Win * win);
int MPIDI_OFI_mpi_win_allocate_shared_hook(MPIR_Win * win);
int MPIDI_OFI_mpi_win_create_dynamic_hook(MPIR_Win * win);
int MPIDI_OFI_mpi_win_attach_hook(MPIR_Win * win, void *base, MPI_Aint size);
int MPIDI_OFI_mpi_win_detach_hook(MPIR_Win * win, const void *base);
int MPIDI_OFI_mpi_win_free_hook(MPIR_Win * win);

#ifdef NETMOD_INLINE
#define MPIDI_NM_mpi_win_set_info MPIDI_OFI_mpi_win_set_info
#define MPIDI_NM_mpi_win_get_info MPIDI_OFI_mpi_win_get_info
#define MPIDI_NM_mpi_win_free MPIDI_OFI_mpi_win_free
#define MPIDI_NM_mpi_win_create MPIDI_OFI_mpi_win_create
#define MPIDI_NM_mpi_win_attach MPIDI_OFI_mpi_win_attach
#define MPIDI_NM_mpi_win_allocate_shared MPIDI_OFI_mpi_win_allocate_shared
#define MPIDI_NM_mpi_win_detach MPIDI_OFI_mpi_win_detach
#define MPIDI_NM_mpi_win_allocate MPIDI_OFI_mpi_win_allocate
#define MPIDI_NM_mpi_win_create_dynamic MPIDI_OFI_mpi_win_create_dynamic
#define MPIDI_NM_mpi_win_create_hook MPIDI_OFI_mpi_win_create_hook
#define MPIDI_NM_mpi_win_allocate_hook MPIDI_OFI_mpi_win_allocate_hook
#define MPIDI_NM_mpi_win_allocate_shared_hook MPIDI_OFI_mpi_win_allocate_shared_hook
#define MPIDI_NM_mpi_win_create_dynamic_hook MPIDI_OFI_mpi_win_create_dynamic_hook
#define MPIDI_NM_mpi_win_attach_hook MPIDI_OFI_mpi_win_attach_hook
#define MPIDI_NM_mpi_win_detach_hook MPIDI_OFI_mpi_win_detach_hook
#define MPIDI_NM_mpi_win_free_hook MPIDI_OFI_mpi_win_free_hook
#endif

/* ofi_init.h */
int MPIDI_OFI_mpi_init_hook(int rank, int size, int appnum, int *tag_bits, MPIR_Comm * init_comm);
int MPIDI_OFI_mpi_finalize_hook(void);
int MPIDI_OFI_post_init(void);
int MPIDI_OFI_get_vci_attr(int vci);
void *MPIDI_OFI_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr);
int MPIDI_OFI_mpi_free_mem(void *ptr);
int MPIDI_OFI_get_local_upids(MPIR_Comm * comm, size_t ** local_upid_size, char **local_upids);
int MPIDI_OFI_upids_to_lupids(int size, size_t * remote_upid_size, char *remote_upids,
                              int **remote_lupids);
int MPIDI_OFI_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr, int size, const int lpids[]);

#ifdef NETMOD_INLINE
#define MPIDI_NM_mpi_init_hook MPIDI_OFI_mpi_init_hook
#define MPIDI_NM_mpi_finalize_hook MPIDI_OFI_mpi_finalize_hook
#define MPIDI_NM_post_init MPIDI_OFI_post_init
#define MPIDI_NM_get_vci_attr MPIDI_OFI_get_vci_attr
#define MPIDI_NM_mpi_alloc_mem MPIDI_OFI_mpi_alloc_mem
#define MPIDI_NM_mpi_free_mem MPIDI_OFI_mpi_free_mem
#define MPIDI_NM_get_local_upids MPIDI_OFI_get_local_upids
#define MPIDI_NM_upids_to_lupids MPIDI_OFI_upids_to_lupids
#define MPIDI_NM_create_intercomm_from_lpids MPIDI_OFI_create_intercomm_from_lpids
#endif

int MPIDI_OFI_mpi_type_free_hook(MPIR_Datatype * datatype_p);
int MPIDI_OFI_mpi_type_commit_hook(MPIR_Datatype * datatype_p);

#ifdef NETMOD_INLINE
#define MPIDI_NM_mpi_type_free_hook MPIDI_OFI_mpi_type_free_hook
#define MPIDI_NM_mpi_type_commit_hook MPIDI_OFI_mpi_type_commit_hook
#endif

int MPIDI_OFI_progress(int vci, int blocking);

#ifdef NETMOD_INLINE
#define MPIDI_NM_progress MPIDI_OFI_progress
#endif

#endif
