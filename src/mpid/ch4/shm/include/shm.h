/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
/* ch4 shm functions */
#ifndef SHM_H_INCLUDED
#define SHM_H_INCLUDED

#include <mpidimpl.h>

#define MPIDI_MAX_SHM_STRING_LEN 64

typedef int (*MPIDI_SHM_mpi_init_hook_t) (int rank, int size, int *n_vnis_provided, int *tag_bits);
typedef int (*MPIDI_SHM_mpi_finalize_hook_t) (void);
typedef int (*MPIDI_SHM_get_vni_attr_t) (int vni);
typedef int (*MPIDI_SHM_progress_t) (int vni, int blocking);
typedef int (*MPIDI_SHM_mpi_comm_connect_t) (const char *port_name, MPIR_Info * info,
                                             int root, int timeout, MPIR_Comm * comm,
                                             MPIR_Comm ** newcomm_ptr);
typedef int (*MPIDI_SHM_mpi_comm_disconnect_t) (MPIR_Comm * comm_ptr);
typedef int (*MPIDI_SHM_mpi_open_port_t) (MPIR_Info * info_ptr, char *port_name);
typedef int (*MPIDI_SHM_mpi_close_port_t) (const char *port_name);
typedef int (*MPIDI_SHM_mpi_comm_accept_t) (const char *port_name, MPIR_Info * info,
                                            int root, MPIR_Comm * comm, MPIR_Comm ** newcomm_ptr);
typedef int (*MPIDI_SHM_am_send_hdr_t) (int rank, MPIR_Comm * comm, int handler_id,
                                        const void *am_hdr, size_t am_hdr_sz);
typedef int (*MPIDI_SHM_am_isend_t) (int rank, MPIR_Comm * comm, int handler_id,
                                     const void *am_hdr, size_t am_hdr_sz,
                                     const void *data, MPI_Count count,
                                     MPI_Datatype datatype, MPIR_Request * sreq);
typedef int (*MPIDI_SHM_am_isendv_t) (int rank, MPIR_Comm * comm, int handler_id,
                                      struct iovec * am_hdrs, size_t iov_len,
                                      const void *data, MPI_Count count,
                                      MPI_Datatype datatype, MPIR_Request * sreq);
typedef int (*MPIDI_SHM_am_send_hdr_reply_t) (MPIR_Context_id_t context_id, int src_rank,
                                              int handler_id, const void *am_hdr, size_t am_hdr_sz);
typedef int (*MPIDI_SHM_am_isend_reply_t) (MPIR_Context_id_t context_id, int src_rank,
                                           int handler_id, const void *am_hdr,
                                           size_t am_hdr_sz, const void *data,
                                           MPI_Count count, MPI_Datatype datatype,
                                           MPIR_Request * sreq);
typedef size_t(*MPIDI_SHM_am_hdr_max_sz_t) (void);
typedef int (*MPIDI_SHM_am_recv_t) (MPIR_Request * req);
typedef int (*MPIDI_SHM_comm_get_lpid_t) (MPIR_Comm * comm_ptr, int idx,
                                          int *lpid_ptr, bool is_remote);
typedef int (*MPIDI_SHM_get_local_upids_t) (MPIR_Comm * comm, size_t ** local_upid_size,
                                            char **local_upids);
typedef int (*MPIDI_SHM_upids_to_lupids_t) (int size, size_t * remote_upid_size,
                                            char *remote_upids, int **remote_lupids);
typedef int (*MPIDI_SHM_create_intercomm_from_lpids_t) (MPIR_Comm * newcomm_ptr,
                                                        int size, const int lpids[]);
typedef int (*MPIDI_SHM_mpi_comm_create_hook_t) (MPIR_Comm * comm);
typedef int (*MPIDI_SHM_mpi_comm_free_hook_t) (MPIR_Comm * comm);
typedef int (*MPIDI_SHM_mpi_type_commit_hook_t) (MPIR_Datatype * type);
typedef int (*MPIDI_SHM_mpi_type_free_hook_t) (MPIR_Datatype * type);
typedef int (*MPIDI_SHM_mpi_op_commit_hook_t) (MPIR_Op * op);
typedef int (*MPIDI_SHM_mpi_op_free_hook_t) (MPIR_Op * op);
typedef void (*MPIDI_SHM_am_request_init_t) (MPIR_Request * req);
typedef void (*MPIDI_SHM_am_request_finalize_t) (MPIR_Request * req);
typedef int (*MPIDI_SHM_mpi_send_t) (const void *buf, MPI_Aint count,
                                     MPI_Datatype datatype, int rank, int tag,
                                     MPIR_Comm * comm, int context_offset,
                                     MPIDI_av_entry_t * addr, MPIR_Request ** request);
typedef int (*MPIDI_SHM_mpi_ssend_t) (const void *buf, MPI_Aint count,
                                      MPI_Datatype datatype, int rank, int tag,
                                      MPIR_Comm * comm, int context_offset,
                                      MPIDI_av_entry_t * addr, MPIR_Request ** request);
typedef int (*MPIDI_SHM_mpi_startall_t) (int count, MPIR_Request * requests[]);
typedef int (*MPIDI_SHM_mpi_send_init_t) (const void *buf, int count,
                                          MPI_Datatype datatype, int rank, int tag,
                                          MPIR_Comm * comm, int context_offset,
                                          MPIR_Request ** request);
typedef int (*MPIDI_SHM_mpi_ssend_init_t) (const void *buf, int count,
                                           MPI_Datatype datatype, int rank, int tag,
                                           MPIR_Comm * comm, int context_offset,
                                           MPIR_Request ** request);
typedef int (*MPIDI_SHM_mpi_rsend_init_t) (const void *buf, int count,
                                           MPI_Datatype datatype, int rank, int tag,
                                           MPIR_Comm * comm, int context_offset,
                                           MPIR_Request ** request);
typedef int (*MPIDI_SHM_mpi_bsend_init_t) (const void *buf, int count,
                                           MPI_Datatype datatype, int rank, int tag,
                                           MPIR_Comm * comm, int context_offset,
                                           MPIR_Request ** request);
typedef int (*MPIDI_SHM_mpi_isend_t) (const void *buf, MPI_Aint count,
                                      MPI_Datatype datatype, int rank, int tag,
                                      MPIR_Comm * comm, int context_offset,
                                      MPIDI_av_entry_t * addr, MPIR_Request ** request);
typedef int (*MPIDI_SHM_mpi_issend_t) (const void *buf, MPI_Aint count,
                                       MPI_Datatype datatype, int rank, int tag,
                                       MPIR_Comm * comm, int context_offset,
                                       MPIDI_av_entry_t * addr, MPIR_Request ** request);
typedef int (*MPIDI_SHM_mpi_cancel_send_t) (MPIR_Request * sreq);
typedef int (*MPIDI_SHM_mpi_recv_init_t) (void *buf, int count, MPI_Datatype datatype,
                                          int rank, int tag, MPIR_Comm * comm,
                                          int context_offset, MPIR_Request ** request);
typedef int (*MPIDI_SHM_mpi_recv_t) (void *buf, MPI_Aint count, MPI_Datatype datatype,
                                     int rank, int tag, MPIR_Comm * comm,
                                     int context_offset, MPI_Status * status,
                                     MPIR_Request ** request);
typedef int (*MPIDI_SHM_mpi_irecv_t) (void *buf, MPI_Aint count, MPI_Datatype datatype,
                                      int rank, int tag, MPIR_Comm * comm,
                                      int context_offset, MPIR_Request ** request);
typedef int (*MPIDI_SHM_mpi_imrecv_t) (void *buf, MPI_Aint count, MPI_Datatype datatype,
                                       MPIR_Request * message, MPIR_Request ** rreqp);
typedef int (*MPIDI_SHM_mpi_cancel_recv_t) (MPIR_Request * rreq);
typedef void *(*MPIDI_SHM_mpi_alloc_mem_t) (size_t size, MPIR_Info * info_ptr);
typedef int (*MPIDI_SHM_mpi_free_mem_t) (void *ptr);
typedef int (*MPIDI_SHM_mpi_improbe_t) (int source, int tag, MPIR_Comm * comm,
                                        int context_offset, int *flag,
                                        MPIR_Request ** message, MPI_Status * status);
typedef int (*MPIDI_SHM_mpi_iprobe_t) (int source, int tag, MPIR_Comm * comm,
                                       int context_offset, int *flag, MPI_Status * status);
typedef int (*MPIDI_SHM_mpi_win_create_hook_t) (MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_win_allocate_hook_t) (MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_win_allocate_shared_hook_t) (MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_win_create_dynamic_hook_t) (MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_win_attach_hook_t) (MPIR_Win * win, void *base, MPI_Aint size);
typedef int (*MPIDI_SHM_mpi_win_detach_hook_t) (MPIR_Win * win, const void *base);
typedef int (*MPIDI_SHM_mpi_win_free_hook_t) (MPIR_Win * win);
typedef int (*MPIDI_SHM_rma_win_cmpl_hook_t) (MPIR_Win * win);
typedef int (*MPIDI_SHM_rma_win_local_cmpl_hook_t) (MPIR_Win * win);
typedef int (*MPIDI_SHM_rma_target_cmpl_hook_t) (int rank, MPIR_Win * win);
typedef int (*MPIDI_SHM_rma_target_local_cmpl_hook_t) (int rank, MPIR_Win * win);
typedef int (*MPIDI_SHM_rma_op_cs_enter_hook_t) (MPIR_Win * win);
typedef int (*MPIDI_SHM_rma_op_cs_exit_hook_t) (MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_win_set_info_t) (MPIR_Win * win, MPIR_Info * info);
typedef int (*MPIDI_SHM_mpi_win_shared_query_t) (MPIR_Win * win, int rank,
                                                 MPI_Aint * size, int *disp_unit, void *baseptr);
typedef int (*MPIDI_SHM_mpi_put_t) (const void *origin_addr, int origin_count,
                                    MPI_Datatype origin_datatype, int target_rank,
                                    MPI_Aint target_disp, int target_count,
                                    MPI_Datatype target_datatype, MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_win_start_t) (MPIR_Group * group, int assert, MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_win_complete_t) (MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_win_post_t) (MPIR_Group * group, int assert, MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_win_wait_t) (MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_win_test_t) (MPIR_Win * win, int *flag);
typedef int (*MPIDI_SHM_mpi_win_lock_t) (int lock_type, int rank, int assert, MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_win_unlock_t) (int rank, MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_win_get_info_t) (MPIR_Win * win, MPIR_Info ** info_p_p);
typedef int (*MPIDI_SHM_mpi_get_t) (void *origin_addr, int origin_count,
                                    MPI_Datatype origin_datatype, int target_rank,
                                    MPI_Aint target_disp, int target_count,
                                    MPI_Datatype target_datatype, MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_win_free_t) (MPIR_Win ** win_ptr);
typedef int (*MPIDI_SHM_mpi_win_fence_t) (int assert, MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_win_create_t) (void *base, MPI_Aint length, int disp_unit,
                                           MPIR_Info * info, MPIR_Comm * comm_ptr,
                                           MPIR_Win ** win_ptr);
typedef int (*MPIDI_SHM_mpi_accumulate_t) (const void *origin_addr, int origin_count,
                                           MPI_Datatype origin_datatype,
                                           int target_rank, MPI_Aint target_disp,
                                           int target_count,
                                           MPI_Datatype target_datatype, MPI_Op op, MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_win_attach_t) (MPIR_Win * win, void *base, MPI_Aint size);
typedef int (*MPIDI_SHM_mpi_win_allocate_shared_t) (MPI_Aint size, int disp_unit,
                                                    MPIR_Info * info_ptr,
                                                    MPIR_Comm * comm_ptr,
                                                    void **base_ptr, MPIR_Win ** win_ptr);
typedef int (*MPIDI_SHM_mpi_rput_t) (const void *origin_addr, int origin_count,
                                     MPI_Datatype origin_datatype, int target_rank,
                                     MPI_Aint target_disp, int target_count,
                                     MPI_Datatype target_datatype, MPIR_Win * win,
                                     MPIR_Request ** request);
typedef int (*MPIDI_SHM_mpi_win_flush_local_t) (int rank, MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_win_detach_t) (MPIR_Win * win, const void *base);
typedef int (*MPIDI_SHM_mpi_compare_and_swap_t) (const void *origin_addr,
                                                 const void *compare_addr,
                                                 void *result_addr,
                                                 MPI_Datatype datatype,
                                                 int target_rank, MPI_Aint target_disp,
                                                 MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_raccumulate_t) (const void *origin_addr, int origin_count,
                                            MPI_Datatype origin_datatype,
                                            int target_rank, MPI_Aint target_disp,
                                            int target_count,
                                            MPI_Datatype target_datatype, MPI_Op op,
                                            MPIR_Win * win, MPIR_Request ** request);
typedef int (*MPIDI_SHM_mpi_rget_accumulate_t) (const void *origin_addr,
                                                int origin_count,
                                                MPI_Datatype origin_datatype,
                                                void *result_addr, int result_count,
                                                MPI_Datatype result_datatype,
                                                int target_rank, MPI_Aint target_disp,
                                                int target_count,
                                                MPI_Datatype target_datatype, MPI_Op op,
                                                MPIR_Win * win, MPIR_Request ** request);
typedef int (*MPIDI_SHM_mpi_fetch_and_op_t) (const void *origin_addr, void *result_addr,
                                             MPI_Datatype datatype, int target_rank,
                                             MPI_Aint target_disp, MPI_Op op, MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_win_allocate_t) (MPI_Aint size, int disp_unit,
                                             MPIR_Info * info, MPIR_Comm * comm,
                                             void *baseptr, MPIR_Win ** win);
typedef int (*MPIDI_SHM_mpi_win_flush_t) (int rank, MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_win_flush_local_all_t) (MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_win_unlock_all_t) (MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_win_create_dynamic_t) (MPIR_Info * info, MPIR_Comm * comm,
                                                   MPIR_Win ** win);
typedef int (*MPIDI_SHM_mpi_rget_t) (void *origin_addr, int origin_count,
                                     MPI_Datatype origin_datatype, int target_rank,
                                     MPI_Aint target_disp, int target_count,
                                     MPI_Datatype target_datatype, MPIR_Win * win,
                                     MPIR_Request ** request);
typedef int (*MPIDI_SHM_mpi_win_sync_t) (MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_win_flush_all_t) (MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_get_accumulate_t) (const void *origin_addr,
                                               int origin_count,
                                               MPI_Datatype origin_datatype,
                                               void *result_addr, int result_count,
                                               MPI_Datatype result_datatype,
                                               int target_rank, MPI_Aint target_disp,
                                               int target_count,
                                               MPI_Datatype target_datatype, MPI_Op op,
                                               MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_win_lock_all_t) (int assert, MPIR_Win * win);
typedef int (*MPIDI_SHM_mpi_barrier_t) (MPIR_Comm * comm,
                                        MPIR_Errflag_t * errflag,
                                        const void *algo_parameters_container);
typedef int (*MPIDI_SHM_mpi_bcast_t) (void *buffer, int count, MPI_Datatype datatype, int root,
                                      MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                      const void *algo_parameters_container);
typedef int (*MPIDI_SHM_mpi_allreduce_t) (const void *sendbuf, void *recvbuf, int count,
                                          MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                          MPIR_Errflag_t * errflag,
                                          const void *algo_parameters_container);
typedef int (*MPIDI_SHM_mpi_allgather_t) (const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                          MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                          const void *algo_parameters_container);
typedef int (*MPIDI_SHM_mpi_allgatherv_t) (const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           const int *recvcounts, const int *displs,
                                           MPI_Datatype recvtype, MPIR_Comm * comm,
                                           MPIR_Errflag_t * errflag,
                                           const void *algo_parameters_container);
typedef int (*MPIDI_SHM_mpi_scatter_t) (const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                        int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                        const void *algo_parameters_container);
typedef int (*MPIDI_SHM_mpi_scatterv_t) (const void *sendbuf, const int *sendcounts,
                                         const int *displs, MPI_Datatype sendtype, void *recvbuf,
                                         int recvcount, MPI_Datatype recvtype, int root,
                                         MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                         const void *algo_parameters_container);
typedef int (*MPIDI_SHM_mpi_gather_t) (const void *sendbuf, int sendcount,
                                       MPI_Datatype sendtype, void *recvbuf,
                                       int recvcount, MPI_Datatype recvtype, int root,
                                       MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                       const void *algo_parameters_container);
typedef int (*MPIDI_SHM_mpi_gatherv_t) (const void *sendbuf, int sendcount,
                                        MPI_Datatype sendtype, void *recvbuf,
                                        const int *recvcounts, const int *displs,
                                        MPI_Datatype recvtype, int root,
                                        MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                        const void *algo_parameters_container);
typedef int (*MPIDI_SHM_mpi_alltoall_t) (const void *sendbuf, int sendcount,
                                         MPI_Datatype sendtype, void *recvbuf,
                                         int recvcount, MPI_Datatype recvtype,
                                         MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                         const void *algo_parameters_container);
typedef int (*MPIDI_SHM_mpi_alltoallv_t) (const void *sendbuf, const int *sendcounts,
                                          const int *sdispls, MPI_Datatype sendtype,
                                          void *recvbuf, const int *recvcounts,
                                          const int *rdispls, MPI_Datatype recvtype,
                                          MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                          const void *algo_parameters_container);
typedef int (*MPIDI_SHM_mpi_alltoallw_t) (const void *sendbuf, const int *sendcounts,
                                          const int *sdispls,
                                          const MPI_Datatype sendtypes[], void *recvbuf,
                                          const int *recvcounts, const int *rdispls,
                                          const MPI_Datatype recvtypes[],
                                          MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                          const void *algo_parameters_container);
typedef int (*MPIDI_SHM_mpi_reduce_t) (const void *sendbuf, void *recvbuf, int count,
                                       MPI_Datatype datatype, MPI_Op op, int root,
                                       MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                       const void *algo_parameters_container);
typedef int (*MPIDI_SHM_mpi_reduce_scatter_t) (const void *sendbuf, void *recvbuf,
                                               const int *recvcounts,
                                               MPI_Datatype datatype, MPI_Op op,
                                               MPIR_Comm * comm_ptr,
                                               MPIR_Errflag_t * errflag,
                                               const void *algo_parameters_container);
typedef int (*MPIDI_SHM_mpi_reduce_scatter_block_t) (const void *sendbuf, void *recvbuf,
                                                     int recvcount,
                                                     MPI_Datatype datatype, MPI_Op op,
                                                     MPIR_Comm * comm_ptr,
                                                     MPIR_Errflag_t * errflag,
                                                     const void *algo_parameters_container);
typedef int (*MPIDI_SHM_mpi_scan_t) (const void *sendbuf, void *recvbuf, int count,
                                     MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                     MPIR_Errflag_t * errflag,
                                     const void *algo_parameters_container);
typedef int (*MPIDI_SHM_mpi_exscan_t) (const void *sendbuf, void *recvbuf, int count,
                                       MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                       MPIR_Errflag_t * errflag,
                                       const void *algo_parameters_container);
typedef int (*MPIDI_SHM_mpi_neighbor_allgather_t) (const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   int recvcount, MPI_Datatype recvtype,
                                                   MPIR_Comm * comm);
typedef int (*MPIDI_SHM_mpi_neighbor_allgatherv_t) (const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    const int *recvcounts, const int *displs,
                                                    MPI_Datatype recvtype, MPIR_Comm * comm);
typedef int (*MPIDI_SHM_mpi_neighbor_alltoallv_t) (const void *sendbuf, const int *sendcounts,
                                                   const int *sdispls, MPI_Datatype sendtype,
                                                   void *recvbuf, const int *recvcounts,
                                                   const int *rdispls, MPI_Datatype recvtype,
                                                   MPIR_Comm * comm);
typedef int (*MPIDI_SHM_mpi_neighbor_alltoallw_t) (const void *sendbuf,
                                                   const int *sendcounts,
                                                   const MPI_Aint * sdispls,
                                                   const MPI_Datatype * sendtypes,
                                                   void *recvbuf, const int *recvcounts,
                                                   const MPI_Aint * rdispls,
                                                   const MPI_Datatype * recvtypes,
                                                   MPIR_Comm * comm);
typedef int (*MPIDI_SHM_mpi_neighbor_alltoall_t) (const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  int recvcount, MPI_Datatype recvtype,
                                                  MPIR_Comm * comm);
typedef int (*MPIDI_SHM_mpi_ineighbor_allgather_t) (const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    int recvcount, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm, MPIR_Request ** req);
typedef int (*MPIDI_SHM_mpi_ineighbor_allgatherv_t) (const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype,
                                                     void *recvbuf,
                                                     const int *recvcounts,
                                                     const int *displs,
                                                     MPI_Datatype recvtype,
                                                     MPIR_Comm * comm, MPIR_Request ** req);
typedef int (*MPIDI_SHM_mpi_ineighbor_alltoall_t) (const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   int recvcount, MPI_Datatype recvtype,
                                                   MPIR_Comm * comm, MPIR_Request ** req);
typedef int (*MPIDI_SHM_mpi_ineighbor_alltoallv_t) (const void *sendbuf,
                                                    const int *sendcounts,
                                                    const int *sdispls,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    const int *recvcounts,
                                                    const int *rdispls,
                                                    MPI_Datatype recvtype,
                                                    MPIR_Comm * comm, MPIR_Request ** req);
typedef int (*MPIDI_SHM_mpi_ineighbor_alltoallw_t) (const void *sendbuf,
                                                    const int *sendcounts,
                                                    const MPI_Aint * sdispls,
                                                    const MPI_Datatype * sendtypes,
                                                    void *recvbuf, const int *recvcounts,
                                                    const MPI_Aint * rdispls,
                                                    const MPI_Datatype * recvtypes,
                                                    MPIR_Comm * comm, MPIR_Request ** req);
typedef int (*MPIDI_SHM_mpi_ibarrier_t) (MPIR_Comm * comm, MPIR_Request ** req);
typedef int (*MPIDI_SHM_mpi_ibcast_t) (void *buffer, int count, MPI_Datatype datatype,
                                       int root, MPIR_Comm * comm, MPIR_Request ** req);
typedef int (*MPIDI_SHM_mpi_iallgather_t) (const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           int recvcount, MPI_Datatype recvtype,
                                           MPIR_Comm * comm, MPIR_Request ** req);
typedef int (*MPIDI_SHM_mpi_iallgatherv_t) (const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf,
                                            const int *recvcounts, const int *displs,
                                            MPI_Datatype recvtype, MPIR_Comm * comm,
                                            MPIR_Request ** req);
typedef int (*MPIDI_SHM_mpi_iallreduce_t) (const void *sendbuf, void *recvbuf, int count,
                                           MPI_Datatype datatype, MPI_Op op,
                                           MPIR_Comm * comm, MPIR_Request ** req);
typedef int (*MPIDI_SHM_mpi_ialltoall_t) (const void *sendbuf, int sendcount,
                                          MPI_Datatype sendtype, void *recvbuf,
                                          int recvcount, MPI_Datatype recvtype,
                                          MPIR_Comm * comm, MPIR_Request ** req);
typedef int (*MPIDI_SHM_mpi_ialltoallv_t) (const void *sendbuf, const int *sendcounts,
                                           const int *sdispls, MPI_Datatype sendtype,
                                           void *recvbuf, const int *recvcounts,
                                           const int *rdispls, MPI_Datatype recvtype,
                                           MPIR_Comm * comm, MPIR_Request ** req);
typedef int (*MPIDI_SHM_mpi_ialltoallw_t) (const void *sendbuf, const int *sendcounts,
                                           const int *sdispls,
                                           const MPI_Datatype sendtypes[], void *recvbuf,
                                           const int *recvcounts, const int *rdispls,
                                           const MPI_Datatype recvtypes[],
                                           MPIR_Comm * comm, MPIR_Request ** req);
typedef int (*MPIDI_SHM_mpi_iexscan_t) (const void *sendbuf, void *recvbuf, int count,
                                        MPI_Datatype datatype, MPI_Op op,
                                        MPIR_Comm * comm, MPIR_Request ** req);
typedef int (*MPIDI_SHM_mpi_igather_t) (const void *sendbuf, int sendcount,
                                        MPI_Datatype sendtype, void *recvbuf,
                                        int recvcount, MPI_Datatype recvtype, int root,
                                        MPIR_Comm * comm, MPIR_Request ** req);
typedef int (*MPIDI_SHM_mpi_igatherv_t) (const void *sendbuf, int sendcount,
                                         MPI_Datatype sendtype, void *recvbuf,
                                         const int *recvcounts, const int *displs,
                                         MPI_Datatype recvtype, int root,
                                         MPIR_Comm * comm, MPIR_Request ** req);
typedef int (*MPIDI_SHM_mpi_ireduce_scatter_block_t) (const void *sendbuf, void *recvbuf,
                                                      int recvcount,
                                                      MPI_Datatype datatype, MPI_Op op,
                                                      MPIR_Comm * comm, MPIR_Request ** req);
typedef int (*MPIDI_SHM_mpi_ireduce_scatter_t) (const void *sendbuf, void *recvbuf,
                                                const int *recvcounts,
                                                MPI_Datatype datatype, MPI_Op op,
                                                MPIR_Comm * comm, MPIR_Request ** req);
typedef int (*MPIDI_SHM_mpi_ireduce_t) (const void *sendbuf, void *recvbuf, int count,
                                        MPI_Datatype datatype, MPI_Op op, int root,
                                        MPIR_Comm * comm_ptr, MPIR_Request ** req);
typedef int (*MPIDI_SHM_mpi_iscan_t) (const void *sendbuf, void *recvbuf, int count,
                                      MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                      MPIR_Request ** req);
typedef int (*MPIDI_SHM_mpi_iscatter_t) (const void *sendbuf, int sendcount,
                                         MPI_Datatype sendtype, void *recvbuf,
                                         int recvcount, MPI_Datatype recvtype, int root,
                                         MPIR_Comm * comm, MPIR_Request ** req);
typedef int (*MPIDI_SHM_mpi_iscatterv_t) (const void *sendbuf, const int *sendcounts,
                                          const int *displs, MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount,
                                          MPI_Datatype recvtype, int root,
                                          MPIR_Comm * comm_ptr, MPIR_Request ** req);

typedef struct MPIDI_SHM_funcs {
    MPIDI_SHM_mpi_init_hook_t mpi_init;
    MPIDI_SHM_mpi_finalize_hook_t mpi_finalize;
    MPIDI_SHM_get_vni_attr_t get_vni_attr;
    MPIDI_SHM_progress_t progress;
    MPIDI_SHM_mpi_comm_connect_t mpi_comm_connect;
    MPIDI_SHM_mpi_comm_disconnect_t mpi_comm_disconnect;
    MPIDI_SHM_mpi_open_port_t mpi_open_port;
    MPIDI_SHM_mpi_close_port_t mpi_close_port;
    MPIDI_SHM_mpi_comm_accept_t mpi_comm_accept;
    /* Routines that handle addressing */
    MPIDI_SHM_comm_get_lpid_t comm_get_lpid;
    MPIDI_SHM_get_local_upids_t get_local_upids;
    MPIDI_SHM_upids_to_lupids_t upids_to_lupids;
    MPIDI_SHM_create_intercomm_from_lpids_t create_intercomm_from_lpids;
    MPIDI_SHM_mpi_comm_create_hook_t mpi_comm_create_hook;
    MPIDI_SHM_mpi_comm_free_hook_t mpi_comm_free_hook;
    /* Window initialization/cleanup routines */
    MPIDI_SHM_mpi_win_create_hook_t mpi_win_create_hook;
    MPIDI_SHM_mpi_win_allocate_hook_t mpi_win_allocate_hook;
    MPIDI_SHM_mpi_win_allocate_shared_hook_t mpi_win_allocate_shared_hook;
    MPIDI_SHM_mpi_win_create_dynamic_hook_t mpi_win_create_dynamic_hook;
    MPIDI_SHM_mpi_win_attach_hook_t mpi_win_attach_hook;
    MPIDI_SHM_mpi_win_detach_hook_t mpi_win_detach_hook;
    MPIDI_SHM_mpi_win_free_hook_t mpi_win_free_hook;
    /* RMA synchronization routines */
    MPIDI_SHM_rma_win_cmpl_hook_t rma_win_cmpl_hook;
    MPIDI_SHM_rma_win_local_cmpl_hook_t rma_win_local_cmpl_hook;
    MPIDI_SHM_rma_target_cmpl_hook_t rma_target_cmpl_hook;
    MPIDI_SHM_rma_target_local_cmpl_hook_t rma_target_local_cmpl_hook;
    MPIDI_SHM_rma_op_cs_enter_hook_t rma_op_cs_enter_hook;
    MPIDI_SHM_rma_op_cs_exit_hook_t rma_op_cs_exit_hook;
    /* Request allocation routines */
    MPIDI_SHM_am_request_init_t am_request_init;
    MPIDI_SHM_am_request_finalize_t am_request_finalize;
    /* Active Message Routines */
    MPIDI_SHM_am_send_hdr_t am_send_hdr;
    MPIDI_SHM_am_isend_t am_isend;
    MPIDI_SHM_am_isendv_t am_isendv;
    MPIDI_SHM_am_send_hdr_reply_t am_send_hdr_reply;
    MPIDI_SHM_am_isend_reply_t am_isend_reply;
    MPIDI_SHM_am_hdr_max_sz_t am_hdr_max_sz;
    MPIDI_SHM_am_recv_t am_recv;
} MPIDI_SHM_funcs_t;

typedef struct MPIDI_SHM_native_funcs {
    MPIDI_SHM_mpi_send_t mpi_send;
    MPIDI_SHM_mpi_ssend_t mpi_ssend;
    MPIDI_SHM_mpi_startall_t mpi_startall;
    MPIDI_SHM_mpi_send_init_t mpi_send_init;
    MPIDI_SHM_mpi_ssend_init_t mpi_ssend_init;
    MPIDI_SHM_mpi_rsend_init_t mpi_rsend_init;
    MPIDI_SHM_mpi_bsend_init_t mpi_bsend_init;
    MPIDI_SHM_mpi_isend_t mpi_isend;
    MPIDI_SHM_mpi_issend_t mpi_issend;
    MPIDI_SHM_mpi_cancel_send_t mpi_cancel_send;
    MPIDI_SHM_mpi_recv_init_t mpi_recv_init;
    MPIDI_SHM_mpi_recv_t mpi_recv;
    MPIDI_SHM_mpi_irecv_t mpi_irecv;
    MPIDI_SHM_mpi_imrecv_t mpi_imrecv;
    MPIDI_SHM_mpi_cancel_recv_t mpi_cancel_recv;
    MPIDI_SHM_mpi_alloc_mem_t mpi_alloc_mem;
    MPIDI_SHM_mpi_free_mem_t mpi_free_mem;
    MPIDI_SHM_mpi_improbe_t mpi_improbe;
    MPIDI_SHM_mpi_iprobe_t mpi_iprobe;
    MPIDI_SHM_mpi_win_set_info_t mpi_win_set_info;
    MPIDI_SHM_mpi_win_shared_query_t mpi_win_shared_query;
    MPIDI_SHM_mpi_put_t mpi_put;
    MPIDI_SHM_mpi_win_start_t mpi_win_start;
    MPIDI_SHM_mpi_win_complete_t mpi_win_complete;
    MPIDI_SHM_mpi_win_post_t mpi_win_post;
    MPIDI_SHM_mpi_win_wait_t mpi_win_wait;
    MPIDI_SHM_mpi_win_test_t mpi_win_test;
    MPIDI_SHM_mpi_win_lock_t mpi_win_lock;
    MPIDI_SHM_mpi_win_unlock_t mpi_win_unlock;
    MPIDI_SHM_mpi_win_get_info_t mpi_win_get_info;
    MPIDI_SHM_mpi_get_t mpi_get;
    MPIDI_SHM_mpi_win_free_t mpi_win_free;
    MPIDI_SHM_mpi_win_fence_t mpi_win_fence;
    MPIDI_SHM_mpi_win_create_t mpi_win_create;
    MPIDI_SHM_mpi_accumulate_t mpi_accumulate;
    MPIDI_SHM_mpi_win_attach_t mpi_win_attach;
    MPIDI_SHM_mpi_win_allocate_shared_t mpi_win_allocate_shared;
    MPIDI_SHM_mpi_rput_t mpi_rput;
    MPIDI_SHM_mpi_win_flush_local_t mpi_win_flush_local;
    MPIDI_SHM_mpi_win_detach_t mpi_win_detach;
    MPIDI_SHM_mpi_compare_and_swap_t mpi_compare_and_swap;
    MPIDI_SHM_mpi_raccumulate_t mpi_raccumulate;
    MPIDI_SHM_mpi_rget_accumulate_t mpi_rget_accumulate;
    MPIDI_SHM_mpi_fetch_and_op_t mpi_fetch_and_op;
    MPIDI_SHM_mpi_win_allocate_t mpi_win_allocate;
    MPIDI_SHM_mpi_win_flush_t mpi_win_flush;
    MPIDI_SHM_mpi_win_flush_local_all_t mpi_win_flush_local_all;
    MPIDI_SHM_mpi_win_unlock_all_t mpi_win_unlock_all;
    MPIDI_SHM_mpi_win_create_dynamic_t mpi_win_create_dynamic;
    MPIDI_SHM_mpi_rget_t mpi_rget;
    MPIDI_SHM_mpi_win_sync_t mpi_win_sync;
    MPIDI_SHM_mpi_win_flush_all_t mpi_win_flush_all;
    MPIDI_SHM_mpi_get_accumulate_t mpi_get_accumulate;
    MPIDI_SHM_mpi_win_lock_all_t mpi_win_lock_all;
    /* Collectives */
    MPIDI_SHM_mpi_barrier_t mpi_barrier;
    MPIDI_SHM_mpi_bcast_t mpi_bcast;
    MPIDI_SHM_mpi_allreduce_t mpi_allreduce;
    MPIDI_SHM_mpi_allgather_t mpi_allgather;
    MPIDI_SHM_mpi_allgatherv_t mpi_allgatherv;
    MPIDI_SHM_mpi_scatter_t mpi_scatter;
    MPIDI_SHM_mpi_scatterv_t mpi_scatterv;
    MPIDI_SHM_mpi_gather_t mpi_gather;
    MPIDI_SHM_mpi_gatherv_t mpi_gatherv;
    MPIDI_SHM_mpi_alltoall_t mpi_alltoall;
    MPIDI_SHM_mpi_alltoallv_t mpi_alltoallv;
    MPIDI_SHM_mpi_alltoallw_t mpi_alltoallw;
    MPIDI_SHM_mpi_reduce_t mpi_reduce;
    MPIDI_SHM_mpi_reduce_scatter_t mpi_reduce_scatter;
    MPIDI_SHM_mpi_reduce_scatter_block_t mpi_reduce_scatter_block;
    MPIDI_SHM_mpi_scan_t mpi_scan;
    MPIDI_SHM_mpi_exscan_t mpi_exscan;
    MPIDI_SHM_mpi_neighbor_allgather_t mpi_neighbor_allgather;
    MPIDI_SHM_mpi_neighbor_allgatherv_t mpi_neighbor_allgatherv;
    MPIDI_SHM_mpi_neighbor_alltoall_t mpi_neighbor_alltoall;
    MPIDI_SHM_mpi_neighbor_alltoallv_t mpi_neighbor_alltoallv;
    MPIDI_SHM_mpi_neighbor_alltoallw_t mpi_neighbor_alltoallw;
    MPIDI_SHM_mpi_ineighbor_allgather_t mpi_ineighbor_allgather;
    MPIDI_SHM_mpi_ineighbor_allgatherv_t mpi_ineighbor_allgatherv;
    MPIDI_SHM_mpi_ineighbor_alltoall_t mpi_ineighbor_alltoall;
    MPIDI_SHM_mpi_ineighbor_alltoallv_t mpi_ineighbor_alltoallv;
    MPIDI_SHM_mpi_ineighbor_alltoallw_t mpi_ineighbor_alltoallw;
    MPIDI_SHM_mpi_ibarrier_t mpi_ibarrier;
    MPIDI_SHM_mpi_ibcast_t mpi_ibcast;
    MPIDI_SHM_mpi_iallgather_t mpi_iallgather;
    MPIDI_SHM_mpi_iallgatherv_t mpi_iallgatherv;
    MPIDI_SHM_mpi_iallreduce_t mpi_iallreduce;
    MPIDI_SHM_mpi_ialltoall_t mpi_ialltoall;
    MPIDI_SHM_mpi_ialltoallv_t mpi_ialltoallv;
    MPIDI_SHM_mpi_ialltoallw_t mpi_ialltoallw;
    MPIDI_SHM_mpi_iexscan_t mpi_iexscan;
    MPIDI_SHM_mpi_igather_t mpi_igather;
    MPIDI_SHM_mpi_igatherv_t mpi_igatherv;
    MPIDI_SHM_mpi_ireduce_scatter_block_t mpi_ireduce_scatter_block;
    MPIDI_SHM_mpi_ireduce_scatter_t mpi_ireduce_scatter;
    MPIDI_SHM_mpi_ireduce_t mpi_ireduce;
    MPIDI_SHM_mpi_iscan_t mpi_iscan;
    MPIDI_SHM_mpi_iscatter_t mpi_iscatter;
    MPIDI_SHM_mpi_iscatterv_t mpi_iscatterv;
    /* Datatype hooks */
    MPIDI_SHM_mpi_type_commit_hook_t mpi_type_commit_hook;
    MPIDI_SHM_mpi_type_free_hook_t mpi_type_free_hook;
    /* Op hooks */
    MPIDI_SHM_mpi_op_commit_hook_t mpi_op_commit_hook;
    MPIDI_SHM_mpi_op_free_hook_t mpi_op_free_hook;
} MPIDI_SHM_native_funcs_t;

extern MPIDI_SHM_funcs_t MPIDI_SHM_src_funcs;
extern MPIDI_SHM_native_funcs_t MPIDI_SHM_native_src_funcs;

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_init_hook(int rank, int size,
                                                     int *n_vnis_provided,
                                                     int *tag_bits) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_finalize_hook(void) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_get_vni_attr(int vni) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_progress(int vni, int blocking) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_comm_connect(const char *port_name, MPIR_Info * info,
                                                        int root, int timeout, MPIR_Comm * comm,
                                                        MPIR_Comm **
                                                        newcomm_ptr) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_comm_disconnect(MPIR_Comm *
                                                           comm_ptr) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_open_port(MPIR_Info * info_ptr,
                                                     char *port_name) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_close_port(const char *port_name)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_comm_accept(const char *port_name, MPIR_Info * info,
                                                       int root, MPIR_Comm * comm,
                                                       MPIR_Comm **
                                                       newcomm_ptr) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_am_send_hdr(int rank, MPIR_Comm * comm, int handler_id,
                                                   const void *am_hdr,
                                                   size_t am_hdr_sz) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_am_isend(int rank, MPIR_Comm * comm, int handler_id,
                                                const void *am_hdr, size_t am_hdr_sz,
                                                const void *data, MPI_Count count,
                                                MPI_Datatype datatype,
                                                MPIR_Request * sreq) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_am_isendv(int rank, MPIR_Comm * comm, int handler_id,
                                                 struct iovec *am_hdrs, size_t iov_len,
                                                 const void *data, MPI_Count count,
                                                 MPI_Datatype datatype,
                                                 MPIR_Request * sreq) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_am_send_hdr_reply(MPIR_Context_id_t context_id, int src_rank,
                                                         int handler_id, const void *am_hdr,
                                                         size_t am_hdr_sz) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_am_isend_reply(MPIR_Context_id_t context_id, int src_rank,
                                                      int handler_id, const void *am_hdr,
                                                      size_t am_hdr_sz, const void *data,
                                                      MPI_Count count, MPI_Datatype datatype,
                                                      MPIR_Request * sreq) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX size_t MPIDI_SHM_am_hdr_max_sz(void) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_am_recv(MPIR_Request * req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_comm_get_lpid(MPIR_Comm * comm_ptr, int idx,
                                                     int *lpid_ptr,
                                                     bool is_remote) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_get_local_upids(MPIR_Comm * comm, size_t ** local_upid_size,
                                                       char **local_upids) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_upids_to_lupids(int size, size_t * remote_upid_size,
                                                       char *remote_upids,
                                                       int **remote_lupids)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr,
                                                                   int size,
                                                                   const int lpids[])
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_comm_create_hook(MPIR_Comm *
                                                            comm) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_comm_free_hook(MPIR_Comm *
                                                          comm) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_type_commit_hook(MPIR_Datatype *
                                                            type) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_type_free_hook(MPIR_Datatype *
                                                          type) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_op_commit_hook(MPIR_Op * op) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_op_free_hook(MPIR_Op * op) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX void MPIDI_SHM_am_request_init(MPIR_Request *
                                                        req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX void MPIDI_SHM_am_request_finalize(MPIR_Request *
                                                            req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_send(const void *buf, MPI_Aint count,
                                                MPI_Datatype datatype, int rank, int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIDI_av_entry_t * addr,
                                                MPIR_Request ** request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ssend(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIDI_av_entry_t * addr,
                                                 MPIR_Request ** request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_startall(int count,
                                                    MPIR_Request *
                                                    requests[]) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_send_init(const void *buf, int count,
                                                     MPI_Datatype datatype, int rank, int tag,
                                                     MPIR_Comm * comm, int context_offset,
                                                     MPIR_Request **
                                                     request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ssend_init(const void *buf, int count,
                                                      MPI_Datatype datatype, int rank, int tag,
                                                      MPIR_Comm * comm, int context_offset,
                                                      MPIR_Request **
                                                      request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_rsend_init(const void *buf, int count,
                                                      MPI_Datatype datatype, int rank, int tag,
                                                      MPIR_Comm * comm, int context_offset,
                                                      MPIR_Request **
                                                      request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_bsend_init(const void *buf, int count,
                                                      MPI_Datatype datatype, int rank, int tag,
                                                      MPIR_Comm * comm, int context_offset,
                                                      MPIR_Request **
                                                      request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_isend(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIDI_av_entry_t * addr,
                                                 MPIR_Request ** request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_issend(const void *buf, MPI_Aint count,
                                                  MPI_Datatype datatype, int rank, int tag,
                                                  MPIR_Comm * comm, int context_offset,
                                                  MPIDI_av_entry_t * addr,
                                                  MPIR_Request ** request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_cancel_send(MPIR_Request *
                                                       sreq) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_recv_init(void *buf, int count, MPI_Datatype datatype,
                                                     int rank, int tag, MPIR_Comm * comm,
                                                     int context_offset,
                                                     MPIR_Request **
                                                     request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_recv(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                                int rank, int tag, MPIR_Comm * comm,
                                                int context_offset, MPI_Status * status,
                                                MPIR_Request ** request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_irecv(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                                 int rank, int tag, MPIR_Comm * comm,
                                                 int context_offset,
                                                 MPIR_Request ** request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_imrecv(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                                  MPIR_Request * message,
                                                  MPIR_Request ** rreqp) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_cancel_recv(MPIR_Request *
                                                       rreq) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX void *MPIDI_SHM_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_free_mem(void *ptr) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_improbe(int source, int tag, MPIR_Comm * comm,
                                                   int context_offset, int *flag,
                                                   MPIR_Request ** message,
                                                   MPI_Status * status) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iprobe(int source, int tag, MPIR_Comm * comm,
                                                  int context_offset, int *flag,
                                                  MPI_Status * status) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_create_hook(MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_allocate_hook(MPIR_Win *
                                                             win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_allocate_shared_hook(MPIR_Win *
                                                                    win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_create_dynamic_hook(MPIR_Win *
                                                                   win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_attach_hook(MPIR_Win * win, void *base,
                                                           MPI_Aint size) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_detach_hook(MPIR_Win * win,
                                                           const void *base)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_free_hook(MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_win_cmpl_hook(MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_win_local_cmpl_hook(MPIR_Win *
                                                               win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_target_cmpl_hook(int rank,
                                                            MPIR_Win *
                                                            win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_target_local_cmpl_hook(int rank,
                                                                  MPIR_Win *
                                                                  win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_op_cs_enter_hook(MPIR_Win *
                                                            win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_op_cs_exit_hook(MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_set_info(MPIR_Win * win,
                                                        MPIR_Info * info) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_shared_query(MPIR_Win * win, int rank,
                                                            MPI_Aint * size, int *disp_unit,
                                                            void *baseptr) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_put(const void *origin_addr, int origin_count,
                                               MPI_Datatype origin_datatype, int target_rank,
                                               MPI_Aint target_disp, int target_count,
                                               MPI_Datatype target_datatype,
                                               MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_start(MPIR_Group * group, int assert,
                                                     MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_complete(MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_post(MPIR_Group * group, int assert,
                                                    MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_wait(MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_test(MPIR_Win * win,
                                                    int *flag) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_lock(int lock_type, int rank, int assert,
                                                    MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_unlock(int rank,
                                                      MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_get_info(MPIR_Win * win,
                                                        MPIR_Info **
                                                        info_p_p) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_get(void *origin_addr, int origin_count,
                                               MPI_Datatype origin_datatype, int target_rank,
                                               MPI_Aint target_disp, int target_count,
                                               MPI_Datatype target_datatype,
                                               MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_free(MPIR_Win ** win_ptr) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_fence(int assert,
                                                     MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_create(void *base, MPI_Aint length, int disp_unit,
                                                      MPIR_Info * info, MPIR_Comm * comm_ptr,
                                                      MPIR_Win ** win_ptr) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_accumulate(const void *origin_addr, int origin_count,
                                                      MPI_Datatype origin_datatype,
                                                      int target_rank, MPI_Aint target_disp,
                                                      int target_count,
                                                      MPI_Datatype target_datatype, MPI_Op op,
                                                      MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_attach(MPIR_Win * win, void *base,
                                                      MPI_Aint size) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_allocate_shared(MPI_Aint size, int disp_unit,
                                                               MPIR_Info * info_ptr,
                                                               MPIR_Comm * comm_ptr,
                                                               void **base_ptr,
                                                               MPIR_Win **
                                                               win_ptr) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_rput(const void *origin_addr, int origin_count,
                                                MPI_Datatype origin_datatype, int target_rank,
                                                MPI_Aint target_disp, int target_count,
                                                MPI_Datatype target_datatype, MPIR_Win * win,
                                                MPIR_Request ** request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_flush_local(int rank,
                                                           MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_detach(MPIR_Win * win,
                                                      const void *base) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_compare_and_swap(const void *origin_addr,
                                                            const void *compare_addr,
                                                            void *result_addr,
                                                            MPI_Datatype datatype,
                                                            int target_rank, MPI_Aint target_disp,
                                                            MPIR_Win *
                                                            win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_raccumulate(const void *origin_addr, int origin_count,
                                                       MPI_Datatype origin_datatype,
                                                       int target_rank, MPI_Aint target_disp,
                                                       int target_count,
                                                       MPI_Datatype target_datatype, MPI_Op op,
                                                       MPIR_Win * win,
                                                       MPIR_Request **
                                                       request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_rget_accumulate(const void *origin_addr,
                                                           int origin_count,
                                                           MPI_Datatype origin_datatype,
                                                           void *result_addr, int result_count,
                                                           MPI_Datatype result_datatype,
                                                           int target_rank, MPI_Aint target_disp,
                                                           int target_count,
                                                           MPI_Datatype target_datatype, MPI_Op op,
                                                           MPIR_Win * win,
                                                           MPIR_Request **
                                                           request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_fetch_and_op(const void *origin_addr, void *result_addr,
                                                        MPI_Datatype datatype, int target_rank,
                                                        MPI_Aint target_disp, MPI_Op op,
                                                        MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_allocate(MPI_Aint size, int disp_unit,
                                                        MPIR_Info * info, MPIR_Comm * comm,
                                                        void *baseptr,
                                                        MPIR_Win ** win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_flush(int rank,
                                                     MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_flush_local_all(MPIR_Win *
                                                               win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_unlock_all(MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm,
                                                              MPIR_Win **
                                                              win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_rget(void *origin_addr, int origin_count,
                                                MPI_Datatype origin_datatype, int target_rank,
                                                MPI_Aint target_disp, int target_count,
                                                MPI_Datatype target_datatype, MPIR_Win * win,
                                                MPIR_Request ** request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_sync(MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_flush_all(MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_get_accumulate(const void *origin_addr,
                                                          int origin_count,
                                                          MPI_Datatype origin_datatype,
                                                          void *result_addr, int result_count,
                                                          MPI_Datatype result_datatype,
                                                          int target_rank, MPI_Aint target_disp,
                                                          int target_count,
                                                          MPI_Datatype target_datatype, MPI_Op op,
                                                          MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_lock_all(int assert,
                                                        MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_barrier(MPIR_Comm * comm,
                                                   MPIR_Errflag_t * errflag,
                                                   const void *algo_parameters_container)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_bcast(void *buffer, int count, MPI_Datatype datatype,
                                                 int root, MPIR_Comm * comm,
                                                 MPIR_Errflag_t * errflag,
                                                 const void *algo_parameters_container)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_allreduce(const void *sendbuf, void *recvbuf, int count,
                                                     MPI_Datatype datatype, MPI_Op op,
                                                     MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                     const void *algo_parameters_container)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_allgather(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     int recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm,
                                                     MPIR_Errflag_t *
                                                     errflag,
                                                     const void *algo_parameters_container)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_allgatherv(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      const int *recvcounts, const int *displs,
                                                      MPI_Datatype recvtype, MPIR_Comm * comm,
                                                      MPIR_Errflag_t * errflag,
                                                      const void *algo_parameters_container)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_scatter(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   int recvcount, MPI_Datatype recvtype, int root,
                                                   MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                   const void *algo_parameters_container)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_scatterv(const void *sendbuf, const int *sendcounts,
                                                    const int *displs, MPI_Datatype sendtype,
                                                    void *recvbuf, int recvcount,
                                                    MPI_Datatype recvtype, int root,
                                                    MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                    const void *algo_parameters_container)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_gather(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  int recvcount, MPI_Datatype recvtype, int root,
                                                  MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                  const void *algo_parameters_container)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_gatherv(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   const int *recvcounts, const int *displs,
                                                   MPI_Datatype recvtype, int root,
                                                   MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                   const void *algo_parameters_container)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_alltoall(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    int recvcount, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                    const void *algo_parameters_container)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_alltoallv(const void *sendbuf, const int *sendcounts,
                                                     const int *sdispls, MPI_Datatype sendtype,
                                                     void *recvbuf, const int *recvcounts,
                                                     const int *rdispls, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                     const void *algo_parameters_container)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_alltoallw(const void *sendbuf, const int *sendcounts,
                                                     const int *sdispls,
                                                     const MPI_Datatype sendtypes[], void *recvbuf,
                                                     const int *recvcounts, const int *rdispls,
                                                     const MPI_Datatype recvtypes[],
                                                     MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                     const void *algo_parameters_container)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_reduce(const void *sendbuf, void *recvbuf, int count,
                                                  MPI_Datatype datatype, MPI_Op op, int root,
                                                  MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                  const void *algo_parameters_container)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_reduce_scatter(const void *sendbuf, void *recvbuf,
                                                          const int *recvcounts,
                                                          MPI_Datatype datatype, MPI_Op op,
                                                          MPIR_Comm * comm_ptr,
                                                          MPIR_Errflag_t * errflag,
                                                          const void *algo_parameters_container)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                                int recvcount,
                                                                MPI_Datatype datatype, MPI_Op op,
                                                                MPIR_Comm * comm_ptr,
                                                                MPIR_Errflag_t * errflag, const void
                                                                *algo_parameters_container)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_scan(const void *sendbuf, void *recvbuf, int count,
                                                MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                                MPIR_Errflag_t * errflag,
                                                const void *algo_parameters_container)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_exscan(const void *sendbuf, void *recvbuf, int count,
                                                  MPI_Datatype datatype, MPI_Op op,
                                                  MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                  const void *algo_parameters_container)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_neighbor_allgather(const void *sendbuf, int sendcount,
                                                              MPI_Datatype sendtype, void *recvbuf,
                                                              int recvcount, MPI_Datatype recvtype,
                                                              MPIR_Comm *
                                                              comm) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_neighbor_allgatherv(const void *sendbuf, int sendcount,
                                                               MPI_Datatype sendtype, void *recvbuf,
                                                               const int *recvcounts,
                                                               const int *displs,
                                                               MPI_Datatype recvtype,
                                                               MPIR_Comm *
                                                               comm) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_neighbor_alltoallv(const void *sendbuf,
                                                              const int *sendcounts,
                                                              const int *sdispls,
                                                              MPI_Datatype sendtype, void *recvbuf,
                                                              const int *recvcounts,
                                                              const int *rdispls,
                                                              MPI_Datatype recvtype,
                                                              MPIR_Comm *
                                                              comm) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_neighbor_alltoallw(const void *sendbuf,
                                                              const int *sendcounts,
                                                              const MPI_Aint * sdispls,
                                                              const MPI_Datatype * sendtypes,
                                                              void *recvbuf, const int *recvcounts,
                                                              const MPI_Aint * rdispls,
                                                              const MPI_Datatype * recvtypes,
                                                              MPIR_Comm *
                                                              comm) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_neighbor_alltoall(const void *sendbuf, int sendcount,
                                                             MPI_Datatype sendtype, void *recvbuf,
                                                             int recvcount, MPI_Datatype recvtype,
                                                             MPIR_Comm *
                                                             comm) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ineighbor_allgather(const void *sendbuf, int sendcount,
                                                               MPI_Datatype sendtype, void *recvbuf,
                                                               int recvcount, MPI_Datatype recvtype,
                                                               MPIR_Comm * comm,
                                                               MPIR_Request **
                                                               req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ineighbor_allgatherv(const void *sendbuf, int sendcount,
                                                                MPI_Datatype sendtype,
                                                                void *recvbuf,
                                                                const int *recvcounts,
                                                                const int *displs,
                                                                MPI_Datatype recvtype,
                                                                MPIR_Comm * comm,
                                                                MPIR_Request **
                                                                req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ineighbor_alltoall(const void *sendbuf, int sendcount,
                                                              MPI_Datatype sendtype, void *recvbuf,
                                                              int recvcount, MPI_Datatype recvtype,
                                                              MPIR_Comm * comm,
                                                              MPIR_Request **
                                                              req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ineighbor_alltoallv(const void *sendbuf,
                                                               const int *sendcounts,
                                                               const int *sdispls,
                                                               MPI_Datatype sendtype, void *recvbuf,
                                                               const int *recvcounts,
                                                               const int *rdispls,
                                                               MPI_Datatype recvtype,
                                                               MPIR_Comm * comm,
                                                               MPIR_Request **
                                                               req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ineighbor_alltoallw(const void *sendbuf,
                                                               const int *sendcounts,
                                                               const MPI_Aint * sdispls,
                                                               const MPI_Datatype * sendtypes,
                                                               void *recvbuf, const int *recvcounts,
                                                               const MPI_Aint * rdispls,
                                                               const MPI_Datatype * recvtypes,
                                                               MPIR_Comm * comm,
                                                               MPIR_Request **
                                                               req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ibarrier(MPIR_Comm * comm,
                                                    MPIR_Request ** req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ibcast(void *buffer, int count, MPI_Datatype datatype,
                                                  int root, MPIR_Comm * comm,
                                                  MPIR_Request ** req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iallgather(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      int recvcount, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm,
                                                      MPIR_Request ** req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iallgatherv(const void *sendbuf, int sendcount,
                                                       MPI_Datatype sendtype, void *recvbuf,
                                                       const int *recvcounts, const int *displs,
                                                       MPI_Datatype recvtype, MPIR_Comm * comm,
                                                       MPIR_Request **
                                                       req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iallreduce(const void *sendbuf, void *recvbuf, int count,
                                                      MPI_Datatype datatype, MPI_Op op,
                                                      MPIR_Comm * comm,
                                                      MPIR_Request ** req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ialltoall(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     int recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm,
                                                     MPIR_Request ** req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ialltoallv(const void *sendbuf, const int *sendcounts,
                                                      const int *sdispls, MPI_Datatype sendtype,
                                                      void *recvbuf, const int *recvcounts,
                                                      const int *rdispls, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm,
                                                      MPIR_Request ** req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ialltoallw(const void *sendbuf, const int *sendcounts,
                                                      const int *sdispls,
                                                      const MPI_Datatype sendtypes[], void *recvbuf,
                                                      const int *recvcounts, const int *rdispls,
                                                      const MPI_Datatype recvtypes[],
                                                      MPIR_Comm * comm,
                                                      MPIR_Request ** req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iexscan(const void *sendbuf, void *recvbuf, int count,
                                                   MPI_Datatype datatype, MPI_Op op,
                                                   MPIR_Comm * comm,
                                                   MPIR_Request ** req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_igather(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   int recvcount, MPI_Datatype recvtype, int root,
                                                   MPIR_Comm * comm,
                                                   MPIR_Request ** req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_igatherv(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    const int *recvcounts, const int *displs,
                                                    MPI_Datatype recvtype, int root,
                                                    MPIR_Comm * comm,
                                                    MPIR_Request ** req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                                 int recvcount,
                                                                 MPI_Datatype datatype, MPI_Op op,
                                                                 MPIR_Comm * comm,
                                                                 MPIR_Request **
                                                                 req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ireduce_scatter(const void *sendbuf, void *recvbuf,
                                                           const int *recvcounts,
                                                           MPI_Datatype datatype, MPI_Op op,
                                                           MPIR_Comm * comm,
                                                           MPIR_Request **
                                                           req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ireduce(const void *sendbuf, void *recvbuf, int count,
                                                   MPI_Datatype datatype, MPI_Op op, int root,
                                                   MPIR_Comm * comm_ptr,
                                                   MPIR_Request ** req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iscan(const void *sendbuf, void *recvbuf, int count,
                                                 MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                                 MPIR_Request ** req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iscatter(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    int recvcount, MPI_Datatype recvtype, int root,
                                                    MPIR_Comm * comm,
                                                    MPIR_Request ** req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iscatterv(const void *sendbuf, const int *sendcounts,
                                                     const int *displs, MPI_Datatype sendtype,
                                                     void *recvbuf, int recvcount,
                                                     MPI_Datatype recvtype, int root,
                                                     MPIR_Comm * comm_ptr,
                                                     MPIR_Request ** req) MPL_STATIC_INLINE_SUFFIX;

#endif /* SHM_H_INCLUDED */
