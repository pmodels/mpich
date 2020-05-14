/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef STUBNM_NOINLINE_H_INCLUDED
#define STUBNM_NOINLINE_H_INCLUDED

#include "stubnm_impl.h"

/* stubnm_comm.h */
int MPIDI_STUBNM_mpi_comm_commit_pre_hook(MPIR_Comm * comm);
int MPIDI_STUBNM_mpi_comm_commit_post_hook(MPIR_Comm * comm);
int MPIDI_STUBNM_mpi_comm_free_hook(MPIR_Comm * comm);

#ifdef NETMOD_INLINE
#define MPIDI_NM_mpi_comm_commit_pre_hook MPIDI_STUBNM_mpi_comm_commit_pre_hook
#define MPIDI_NM_mpi_comm_commit_post_hook MPIDI_STUBNM_mpi_comm_commit_post_hook
#define MPIDI_NM_mpi_comm_free_hook MPIDI_STUBNM_mpi_comm_free_hook
#endif

/* stubnm_spawn.h */
int MPIDI_STUBNM_mpi_comm_connect(const char *port_name, MPIR_Info * info, int root, int timeout,
                                  MPIR_Comm * comm_ptr, MPIR_Comm ** newcomm);
int MPIDI_STUBNM_mpi_comm_disconnect(MPIR_Comm * comm_ptr);
int MPIDI_STUBNM_mpi_open_port(MPIR_Info * info_ptr, char *port_name);
int MPIDI_STUBNM_mpi_close_port(const char *port_name);
int MPIDI_STUBNM_mpi_comm_accept(const char *port_name, MPIR_Info * info, int root,
                                 MPIR_Comm * comm_ptr, MPIR_Comm ** newcomm);

#ifdef NETMOD_INLINE
#define MPIDI_NM_mpi_comm_connect MPIDI_STUBNM_mpi_comm_connect
#define MPIDI_NM_mpi_comm_disconnect MPIDI_STUBNM_mpi_comm_disconnect
#define MPIDI_NM_mpi_open_port MPIDI_STUBNM_mpi_open_port
#define MPIDI_NM_mpi_close_port MPIDI_STUBNM_mpi_close_port
#define MPIDI_NM_mpi_comm_accept MPIDI_STUBNM_mpi_comm_accept
#endif

int MPIDI_STUBNM_mpi_op_free_hook(MPIR_Op * op_p);
int MPIDI_STUBNM_mpi_op_commit_hook(MPIR_Op * op_p);

#ifdef NETMOD_INLINE
#define MPIDI_NM_mpi_op_free_hook MPIDI_STUBNM_mpi_op_free_hook
#define MPIDI_NM_mpi_op_commit_hook MPIDI_STUBNM_mpi_op_commit_hook
#endif

/* stubnm_win.h */
int MPIDI_STUBNM_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info);
int MPIDI_STUBNM_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p);
int MPIDI_STUBNM_mpi_win_free(MPIR_Win ** win_ptr);
int MPIDI_STUBNM_mpi_win_create(void *base, MPI_Aint length, int disp_unit, MPIR_Info * info,
                                MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr);
int MPIDI_STUBNM_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size);
int MPIDI_STUBNM_mpi_win_allocate_shared(MPI_Aint size, int disp_unit, MPIR_Info * info_ptr,
                                         MPIR_Comm * comm_ptr, void **base_ptr,
                                         MPIR_Win ** win_ptr);
int MPIDI_STUBNM_mpi_win_detach(MPIR_Win * win, const void *base);
int MPIDI_STUBNM_mpi_win_allocate(MPI_Aint size, int disp_unit, MPIR_Info * info, MPIR_Comm * comm,
                                  void *baseptr, MPIR_Win ** win_ptr);
int MPIDI_STUBNM_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm, MPIR_Win ** win_ptr);
int MPIDI_STUBNM_mpi_win_create_hook(MPIR_Win * win);
int MPIDI_STUBNM_mpi_win_allocate_hook(MPIR_Win * win);
int MPIDI_STUBNM_mpi_win_allocate_shared_hook(MPIR_Win * win);
int MPIDI_STUBNM_mpi_win_create_dynamic_hook(MPIR_Win * win);
int MPIDI_STUBNM_mpi_win_attach_hook(MPIR_Win * win, void *base, MPI_Aint size);
int MPIDI_STUBNM_mpi_win_detach_hook(MPIR_Win * win, const void *base);
int MPIDI_STUBNM_mpi_win_free_hook(MPIR_Win * win);

#ifdef NETMOD_INLINE
#define MPIDI_NM_mpi_win_set_info MPIDI_STUBNM_mpi_win_set_info
#define MPIDI_NM_mpi_win_get_info MPIDI_STUBNM_mpi_win_get_info
#define MPIDI_NM_mpi_win_free MPIDI_STUBNM_mpi_win_free
#define MPIDI_NM_mpi_win_create MPIDI_STUBNM_mpi_win_create
#define MPIDI_NM_mpi_win_attach MPIDI_STUBNM_mpi_win_attach
#define MPIDI_NM_mpi_win_allocate_shared MPIDI_STUBNM_mpi_win_allocate_shared
#define MPIDI_NM_mpi_win_detach MPIDI_STUBNM_mpi_win_detach
#define MPIDI_NM_mpi_win_allocate MPIDI_STUBNM_mpi_win_allocate
#define MPIDI_NM_mpi_win_create_dynamic MPIDI_STUBNM_mpi_win_create_dynamic
#define MPIDI_NM_mpi_win_create_hook MPIDI_STUBNM_mpi_win_create_hook
#define MPIDI_NM_mpi_win_allocate_hook MPIDI_STUBNM_mpi_win_allocate_hook
#define MPIDI_NM_mpi_win_allocate_shared_hook MPIDI_STUBNM_mpi_win_allocate_shared_hook
#define MPIDI_NM_mpi_win_create_dynamic_hook MPIDI_STUBNM_mpi_win_create_dynamic_hook
#define MPIDI_NM_mpi_win_attach_hook MPIDI_STUBNM_mpi_win_attach_hook
#define MPIDI_NM_mpi_win_detach_hook MPIDI_STUBNM_mpi_win_detach_hook
#define MPIDI_NM_mpi_win_free_hook MPIDI_STUBNM_mpi_win_free_hook
#endif

/* stubnm_init.h */
int MPIDI_STUBNM_mpi_init_hook(int rank, int size, int appnum, int *tag_bits,
                               MPIR_Comm * init_comm);
int MPIDI_STUBNM_mpi_finalize_hook(void);
int MPIDI_STUBNM_get_vci_attr(int vci);
void *MPIDI_STUBNM_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr);
int MPIDI_STUBNM_mpi_free_mem(void *ptr);
int MPIDI_STUBNM_get_local_upids(MPIR_Comm * comm, size_t ** local_upid_size, char **local_upids);
int MPIDI_STUBNM_upids_to_lupids(int size, size_t * remote_upid_size, char *remote_upids,
                                 int **remote_lupids);
int MPIDI_STUBNM_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr, int size, const int lpids[]);

#ifdef NETMOD_INLINE
#define MPIDI_NM_mpi_init_hook MPIDI_STUBNM_mpi_init_hook
#define MPIDI_NM_mpi_finalize_hook MPIDI_STUBNM_mpi_finalize_hook
#define MPIDI_NM_get_vci_attr MPIDI_STUBNM_get_vci_attr
#define MPIDI_NM_mpi_alloc_mem MPIDI_STUBNM_mpi_alloc_mem
#define MPIDI_NM_mpi_free_mem MPIDI_STUBNM_mpi_free_mem
#define MPIDI_NM_get_local_upids MPIDI_STUBNM_get_local_upids
#define MPIDI_NM_upids_to_lupids MPIDI_STUBNM_upids_to_lupids
#define MPIDI_NM_create_intercomm_from_lpids MPIDI_STUBNM_create_intercomm_from_lpids
#endif

int MPIDI_STUBNM_mpi_type_free_hook(MPIR_Datatype * datatype_p);
int MPIDI_STUBNM_mpi_type_commit_hook(MPIR_Datatype * datatype_p);

#ifdef NETMOD_INLINE
#define MPIDI_NM_mpi_type_free_hook MPIDI_STUBNM_mpi_type_free_hook
#define MPIDI_NM_mpi_type_commit_hook MPIDI_STUBNM_mpi_type_commit_hook
#endif

int MPIDI_STUBNM_progress(int vci, int blocking);
int MPIDI_STUBNM_progress_test(void);
int MPIDI_STUBNM_progress_poke(void);
void MPIDI_STUBNM_progress_start(MPID_Progress_state * state);
void MPIDI_STUBNM_progress_end(MPID_Progress_state * state);
int MPIDI_STUBNM_progress_wait(MPID_Progress_state * state);
int MPIDI_STUBNM_progress_register(int (*progress_fn) (int *), int *id);
int MPIDI_STUBNM_progress_deregister(int id);
int MPIDI_STUBNM_progress_activate(int id);
int MPIDI_STUBNM_progress_deactivate(int id);

#ifdef NETMOD_INLINE
#define MPIDI_NM_progress MPIDI_STUBNM_progress
#define MPIDI_NM_progress_test MPIDI_STUBNM_progress_test
#define MPIDI_NM_progress_poke MPIDI_STUBNM_progress_poke
#define MPIDI_NM_progress_start MPIDI_STUBNM_progress_start
#define MPIDI_NM_progress_end MPIDI_STUBNM_progress_end
#define MPIDI_NM_progress_wait MPIDI_STUBNM_progress_wait
#define MPIDI_NM_progress_register MPIDI_STUBNM_progress_register
#define MPIDI_NM_progress_deregister MPIDI_STUBNM_progress_deregister
#define MPIDI_NM_progress_activate MPIDI_STUBNM_progress_activate
#define MPIDI_NM_progress_deactivate MPIDI_STUBNM_progress_deactivate
#endif

#endif
