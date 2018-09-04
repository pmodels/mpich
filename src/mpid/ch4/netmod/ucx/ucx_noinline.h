/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef OFI_NOINLINE_H_INCLUDED
#define OFI_NOINLINE_H_INCLUDED

#include "ucx_impl.h"

/* ucx_init.c */

int MPIDI_UCX_mpi_init_hook(int rank, int size, int appnum, int *tag_ub, MPIR_Comm * comm_world,
                            MPIR_Comm * comm_self, int spawned, int *n_vnis_provided);
int MPIDI_UCX_mpi_finalize_hook(void);
int MPIDI_UCX_get_vni_attr(int vni);
int MPIDI_UCX_get_local_upids(MPIR_Comm * comm, size_t ** local_upid_size, char **local_upids);
int MPIDI_UCX_upids_to_lupids(int size, size_t * remote_upid_size, char *remote_upids,
                              int **remote_lupids);
int MPIDI_UCX_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr, int size, const int lpids[]);
int MPIDI_UCX_mpi_free_mem(void *ptr);
void *MPIDI_UCX_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr);

#ifdef NETMOD_INLINE
#define MPIDI_NM_mpi_init_hook MPIDI_UCX_mpi_init_hook
#define MPIDI_NM_mpi_finalize_hook MPIDI_UCX_mpi_finalize_hook
#define MPIDI_NM_get_vni_attr MPIDI_UCX_get_vni_attr
#define MPIDI_NM_get_local_upids MPIDI_UCX_get_local_upids
#define MPIDI_NM_upids_to_lupids MPIDI_UCX_upids_to_lupids
#define MPIDI_NM_create_intercomm_from_lpids MPIDI_UCX_create_intercomm_from_lpids
#define MPIDI_NM_mpi_free_mem MPIDI_UCX_mpi_free_mem
#define MPIDI_NM_mpi_alloc_mem MPIDI_UCX_mpi_alloc_mem
#endif

/* ucx_comm.c */
int MPIDI_UCX_mpi_comm_create_hook(MPIR_Comm * comm);
int MPIDI_UCX_mpi_comm_free_hook(MPIR_Comm * comm);

#ifdef NETMOD_INLINE
#define MPIDI_NM_mpi_comm_create_hook MPIDI_UCX_mpi_comm_create_hook
#define MPIDI_NM_mpi_comm_free_hook MPIDI_UCX_mpi_comm_free_hook
#endif

/* ucx_datatype.c */
int MPIDI_UCX_mpi_type_free_hook(MPIR_Datatype * datatype_p);
int MPIDI_UCX_mpi_type_commit_hook(MPIR_Datatype * datatype_p);

#ifdef NETMOD_INLINE
#define MPIDI_NM_mpi_type_free_hook MPIDI_UCX_mpi_type_free_hook
#define MPIDI_NM_mpi_type_commit_hook MPIDI_UCX_mpi_type_commit_hook
#endif

/* ucx_op.c */
int MPIDI_UCX_mpi_op_commit_hook(MPIR_Op * op_p);
int MPIDI_UCX_mpi_op_free_hook(MPIR_Op * op_p);

#ifdef NETMOD_INLINE
#define MPIDI_NM_mpi_op_commit_hook MPIDI_UCX_mpi_op_commit_hook
#define MPIDI_NM_mpi_op_free_hook MPIDI_UCX_mpi_op_free_hook
#endif


/* ucx_spawn.c */
int MPIDI_UCX_mpi_comm_connect(const char *port_name, MPIR_Info * info, int root, int timeout,
                               MPIR_Comm * comm_ptr, MPIR_Comm ** newcomm);
int MPIDI_UCX_mpi_comm_disconnect(MPIR_Comm * comm_ptr);
int MPIDI_UCX_mpi_open_port(MPIR_Info * info_ptr, char *port_name);
int MPIDI_UCX_mpi_close_port(const char *port_name);
int MPIDI_UCX_mpi_comm_accept(const char *port_name, MPIR_Info * info, int root,
                              MPIR_Comm * comm_ptr, MPIR_Comm ** newcomm);
#ifdef NETMOD_INLINE
#define MPIDI_NM_mpi_comm_connect MPIDI_UCX_mpi_comm_connect
#define MPIDI_NM_mpi_comm_disconnect MPIDI_UCX_mpi_comm_disconnect
#define MPIDI_NM_mpi_open_port MPIDI_UCX_mpi_open_port
#define MPIDI_NM_mpi_close_port MPIDI_UCX_mpi_close_port
#define MPIDI_NM_mpi_comm_accept MPIDI_UCX_mpi_comm_accept
#endif

/* ucx_win.c */
int MPIDI_UCX_mpi_win_free(MPIR_Win ** win_ptr);
int MPIDI_UCX_mpi_win_create(void *base, MPI_Aint length, int disp_unit, MPIR_Info * info,
                             MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr);
int MPIDI_UCX_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size);
int MPIDI_UCX_mpi_win_allocate_shared(MPI_Aint size, int disp_unit, MPIR_Info * info_ptr,
                                      MPIR_Comm * comm_ptr, void **base_ptr, MPIR_Win ** win_ptr);
int MPIDI_UCX_mpi_win_detach(MPIR_Win * win, const void *base);
int MPIDI_UCX_mpi_win_shared_query(MPIR_Win * win, int rank, MPI_Aint * size, int *disp_unit,
                                   void *baseptr);
int MPIDI_UCX_mpi_win_allocate(MPI_Aint size, int disp_unit, MPIR_Info * info, MPIR_Comm * comm,
                               void *baseptr, MPIR_Win ** win_ptr);
int MPIDI_UCX_mpi_win_create_hook(MPIR_Win * win);
int MPIDI_UCX_mpi_win_allocate_hook(MPIR_Win * win);
int MPIDI_UCX_mpi_win_allocate_shared_hook(MPIR_Win * win);
int MPIDI_UCX_mpi_win_create_dynamic_hook(MPIR_Win * win);
int MPIDI_UCX_mpi_win_attach_hook(MPIR_Win * win, void *base, MPI_Aint size);
int MPIDI_UCX_mpi_win_detach_hook(MPIR_Win * win, const void *base);
int MPIDI_UCX_mpi_win_free_hook(MPIR_Win * win);
int MPIDI_UCX_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm, MPIR_Win ** win_ptr);

#ifdef NETMOD_INLINE
#define MPIDI_NM_mpi_win_free MPIDI_UCX_mpi_win_free
#define MPIDI_NM_mpi_win_create MPIDI_UCX_mpi_win_create
#define MPIDI_NM_mpi_win_attach MPIDI_UCX_mpi_win_attach
#define MPIDI_NM_mpi_win_allocate_shared MPIDI_UCX_mpi_win_allocate_shared
#define MPIDI_NM_mpi_win_detach MPIDI_UCX_mpi_win_detach
#define MPIDI_NM_mpi_win_shared_query MPIDI_UCX_mpi_win_shared_query
#define MPIDI_NM_mpi_win_allocate MPIDI_UCX_mpi_win_allocate
#define MPIDI_NM_mpi_win_create_hook MPIDI_UCX_mpi_win_create_hook
#define MPIDI_NM_mpi_win_allocate_hook MPIDI_UCX_mpi_win_allocate_hook
#define MPIDI_NM_mpi_win_allocate_shared_hook MPIDI_UCX_mpi_win_allocate_shared_hook
#define MPIDI_NM_mpi_win_create_dynamic_hook MPIDI_UCX_mpi_win_create_dynamic_hook
#define MPIDI_NM_mpi_win_attach_hook MPIDI_UCX_mpi_win_attach_hook
#define MPIDI_NM_mpi_win_detach_hook MPIDI_UCX_mpi_win_detach_hook
#define MPIDI_NM_mpi_win_free_hook MPIDI_UCX_mpi_win_free_hook
#define MPIDI_NM_mpi_win_create_dynamic MPIDI_UCX_mpi_win_create_dynamic
#endif

#endif /* UCX_NOINLINE_H_INCLUDED */
