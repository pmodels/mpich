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
/* ch4 netmod functions */
#ifndef NETMOD_PROTOTYPES_H_INCLUDED
#define NETMOD_PROTOTYPES_H_INCLUDED

#include <mpidimpl.h>

#define MPIDI_MAX_NETMOD_STRING_LEN 64

typedef int (*MPIDI_NM_am_completion_handler_fn) (MPIR_Request * req);
typedef int (*MPIDI_NM_am_origin_handler_fn) (MPIR_Request * req);

/* Callback function setup by handler register function */
/* for short cases, output arguments are NULL */
typedef int (*MPIDI_NM_am_target_handler_fn)
 (int handler_id, void *am_hdr, void **data,    /* data should be iovs if *is_contig is false */
  size_t * data_sz, int *is_contig, MPIDI_NM_am_completion_handler_fn * cmpl_handler_fn,        /* completion handler */
  MPIR_Request ** req);         /* if allocated, need pointer to completion function */

typedef int (*MPIDI_NM_init_t) (int rank, int size, int appnum, int *tag_ub, MPIR_Comm * comm_world,
                                MPIR_Comm * comm_self, int spawned, int num_contexts,
                                void **netmod_contexts);
typedef int (*MPIDI_NM_finalize_t) (void);
typedef int (*MPIDI_NM_progress_t) (void *netmod_context, int blocking);
typedef int (*MPIDI_NM_am_reg_handler_t) (int handler_id,
                                          MPIDI_NM_am_origin_handler_fn origin_handler_fn,
                                          MPIDI_NM_am_target_handler_fn target_handler_fn);
typedef int (*MPIDI_NM_comm_connect_t) (const char *port_name, MPIR_Info * info, int root,
                                        MPIR_Comm * comm, MPIR_Comm ** newcomm_ptr);
typedef int (*MPIDI_NM_comm_disconnect_t) (MPIR_Comm * comm_ptr);
typedef int (*MPIDI_NM_open_port_t) (MPIR_Info * info_ptr, char *port_name);
typedef int (*MPIDI_NM_close_port_t) (const char *port_name);
typedef int (*MPIDI_NM_comm_accept_t) (const char *port_name, MPIR_Info * info, int root,
                                       MPIR_Comm * comm, MPIR_Comm ** newcomm_ptr);
typedef int (*MPIDI_NM_am_send_hdr_t) (int rank, MPIR_Comm * comm, int handler_id,
                                       const void *am_hdr, size_t am_hdr_sz, void *netmod_context);
typedef int (*MPIDI_NM_am_isend_t) (int rank, MPIR_Comm * comm, int handler_id, const void *am_hdr,
                                    size_t am_hdr_sz, const void *data, MPI_Count count,
                                    MPI_Datatype datatype, MPIR_Request * sreq,
                                    void *netmod_context);
typedef int (*MPIDI_NM_am_isendv_t) (int rank, MPIR_Comm * comm, int handler_id,
                                     struct iovec * am_hdrs, size_t iov_len, const void *data,
                                     MPI_Count count, MPI_Datatype datatype, MPIR_Request * sreq,
                                     void *netmod_context);
typedef int (*MPIDI_NM_am_send_hdr_reply_t) (MPIR_Context_id_t context_id, int src_rank,
                                             int handler_id, const void *am_hdr, size_t am_hdr_sz);
typedef int (*MPIDI_NM_am_isend_reply_t) (MPIR_Context_id_t context_id, int src_rank,
                                          int handler_id, const void *am_hdr, size_t am_hdr_sz,
                                          const void *data, MPI_Count count, MPI_Datatype datatype,
                                          MPIR_Request * sreq);
typedef size_t(*MPIDI_NM_am_hdr_max_sz_t) (void);
typedef int (*MPIDI_NM_am_recv_t) (MPIR_Request * req);
typedef int (*MPIDI_NM_comm_get_lpid_t) (MPIR_Comm * comm_ptr, int idx, int *lpid_ptr,
                                         MPL_bool is_remote);
typedef int (*MPIDI_NM_gpid_get_t) (MPIR_Comm * comm_ptr, int rank, MPIR_Gpid * gpid);
typedef int (*MPIDI_NM_getallincomm_t) (MPIR_Comm * comm_ptr, int local_size,
                                        MPIR_Gpid local_gpids[], int *singleAVT);
typedef int (*MPIDI_NM_gpid_tolpidarray_t) (int size, MPIR_Gpid gpid[], int lpid[]);
typedef int (*MPIDI_NM_create_intercomm_from_lpids_t) (MPIR_Comm * newcomm_ptr, int size,
                                                       const int lpids[]);
typedef int (*MPIDI_NM_comm_create_hook_t) (MPIR_Comm * comm);
typedef int (*MPIDI_NM_comm_free_hook_t) (MPIR_Comm * comm);
typedef void (*MPIDI_NM_am_request_init_t) (MPIR_Request * req);
typedef void (*MPIDI_NM_am_request_finalize_t) (MPIR_Request * req);
typedef int (*MPIDI_NM_send_t) (const void *buf, int count, MPI_Datatype datatype, int rank,
                                int tag, MPIR_Comm * comm, int context_offset,
                                MPIR_Request ** request);
typedef int (*MPIDI_NM_ssend_t) (const void *buf, int count, MPI_Datatype datatype, int rank,
                                 int tag, MPIR_Comm * comm, int context_offset,
                                 MPIR_Request ** request);
typedef int (*MPIDI_NM_startall_t) (int count, MPIR_Request * requests[]);
typedef int (*MPIDI_NM_send_init_t) (const void *buf, int count, MPI_Datatype datatype, int rank,
                                     int tag, MPIR_Comm * comm, int context_offset,
                                     MPIR_Request ** request);
typedef int (*MPIDI_NM_ssend_init_t) (const void *buf, int count, MPI_Datatype datatype, int rank,
                                      int tag, MPIR_Comm * comm, int context_offset,
                                      MPIR_Request ** request);
typedef int (*MPIDI_NM_rsend_init_t) (const void *buf, int count, MPI_Datatype datatype, int rank,
                                      int tag, MPIR_Comm * comm, int context_offset,
                                      MPIR_Request ** request);
typedef int (*MPIDI_NM_bsend_init_t) (const void *buf, int count, MPI_Datatype datatype, int rank,
                                      int tag, MPIR_Comm * comm, int context_offset,
                                      MPIR_Request ** request);
typedef int (*MPIDI_NM_isend_t) (const void *buf, int count, MPI_Datatype datatype, int rank,
                                 int tag, MPIR_Comm * comm, int context_offset,
                                 MPIR_Request ** request);
typedef int (*MPIDI_NM_issend_t) (const void *buf, int count, MPI_Datatype datatype, int rank,
                                  int tag, MPIR_Comm * comm, int context_offset,
                                  MPIR_Request ** request);
typedef int (*MPIDI_NM_cancel_send_t) (MPIR_Request * sreq);
typedef int (*MPIDI_NM_recv_init_t) (void *buf, int count, MPI_Datatype datatype, int rank, int tag,
                                     MPIR_Comm * comm, int context_offset, MPIR_Request ** request);
typedef int (*MPIDI_NM_recv_t) (void *buf, int count, MPI_Datatype datatype, int rank, int tag,
                                MPIR_Comm * comm, int context_offset, MPI_Status * status,
                                MPIR_Request ** request);
typedef int (*MPIDI_NM_irecv_t) (void *buf, int count, MPI_Datatype datatype, int rank, int tag,
                                 MPIR_Comm * comm, int context_offset, MPIR_Request ** request);
typedef int (*MPIDI_NM_imrecv_t) (void *buf, int count, MPI_Datatype datatype,
                                  MPIR_Request * message, MPIR_Request ** rreqp);
typedef int (*MPIDI_NM_cancel_recv_t) (MPIR_Request * rreq);
typedef void *(*MPIDI_NM_alloc_mem_t) (size_t size, MPIR_Info * info_ptr);
typedef int (*MPIDI_NM_free_mem_t) (void *ptr);
typedef int (*MPIDI_NM_improbe_t) (int source, int tag, MPIR_Comm * comm, int context_offset,
                                   int *flag, MPIR_Request ** message, MPI_Status * status);
typedef int (*MPIDI_NM_iprobe_t) (int source, int tag, MPIR_Comm * comm, int context_offset,
                                  int *flag, MPI_Status * status);
typedef int (*MPIDI_NM_win_set_info_t) (MPIR_Win * win, MPIR_Info * info);
typedef int (*MPIDI_NM_win_shared_query_t) (MPIR_Win * win, int rank, MPI_Aint * size,
                                            int *disp_unit, void *baseptr);
typedef int (*MPIDI_NM_put_t) (const void *origin_addr, int origin_count,
                               MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
                               int target_count, MPI_Datatype target_datatype, MPIR_Win * win);
typedef int (*MPIDI_NM_win_start_t) (MPIR_Group * group, int assert, MPIR_Win * win);
typedef int (*MPIDI_NM_win_complete_t) (MPIR_Win * win);
typedef int (*MPIDI_NM_win_post_t) (MPIR_Group * group, int assert, MPIR_Win * win);
typedef int (*MPIDI_NM_win_wait_t) (MPIR_Win * win);
typedef int (*MPIDI_NM_win_test_t) (MPIR_Win * win, int *flag);
typedef int (*MPIDI_NM_win_lock_t) (int lock_type, int rank, int assert, MPIR_Win * win);
typedef int (*MPIDI_NM_win_unlock_t) (int rank, MPIR_Win * win);
typedef int (*MPIDI_NM_win_get_info_t) (MPIR_Win * win, MPIR_Info ** info_p_p);
typedef int (*MPIDI_NM_get_t) (void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
                               int target_rank, MPI_Aint target_disp, int target_count,
                               MPI_Datatype target_datatype, MPIR_Win * win);
typedef int (*MPIDI_NM_win_free_t) (MPIR_Win ** win_ptr);
typedef int (*MPIDI_NM_win_fence_t) (int assert, MPIR_Win * win);
typedef int (*MPIDI_NM_win_create_t) (void *base, MPI_Aint length, int disp_unit, MPIR_Info * info,
                                      MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr);
typedef int (*MPIDI_NM_accumulate_t) (const void *origin_addr, int origin_count,
                                      MPI_Datatype origin_datatype, int target_rank,
                                      MPI_Aint target_disp, int target_count,
                                      MPI_Datatype target_datatype, MPI_Op op, MPIR_Win * win);
typedef int (*MPIDI_NM_win_attach_t) (MPIR_Win * win, void *base, MPI_Aint size);
typedef int (*MPIDI_NM_win_allocate_shared_t) (MPI_Aint size, int disp_unit, MPIR_Info * info_ptr,
                                               MPIR_Comm * comm_ptr, void **base_ptr,
                                               MPIR_Win ** win_ptr);
typedef int (*MPIDI_NM_rput_t) (const void *origin_addr, int origin_count,
                                MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
                                int target_count, MPI_Datatype target_datatype, MPIR_Win * win,
                                MPIR_Request ** request);
typedef int (*MPIDI_NM_win_flush_local_t) (int rank, MPIR_Win * win);
typedef int (*MPIDI_NM_win_detach_t) (MPIR_Win * win, const void *base);
typedef int (*MPIDI_NM_compare_and_swap_t) (const void *origin_addr, const void *compare_addr,
                                            void *result_addr, MPI_Datatype datatype,
                                            int target_rank, MPI_Aint target_disp, MPIR_Win * win);
typedef int (*MPIDI_NM_raccumulate_t) (const void *origin_addr, int origin_count,
                                       MPI_Datatype origin_datatype, int target_rank,
                                       MPI_Aint target_disp, int target_count,
                                       MPI_Datatype target_datatype, MPI_Op op, MPIR_Win * win,
                                       MPIR_Request ** request);
typedef int (*MPIDI_NM_rget_accumulate_t) (const void *origin_addr, int origin_count,
                                           MPI_Datatype origin_datatype, void *result_addr,
                                           int result_count, MPI_Datatype result_datatype,
                                           int target_rank, MPI_Aint target_disp, int target_count,
                                           MPI_Datatype target_datatype, MPI_Op op, MPIR_Win * win,
                                           MPIR_Request ** request);
typedef int (*MPIDI_NM_fetch_and_op_t) (const void *origin_addr, void *result_addr,
                                        MPI_Datatype datatype, int target_rank,
                                        MPI_Aint target_disp, MPI_Op op, MPIR_Win * win);
typedef int (*MPIDI_NM_win_allocate_t) (MPI_Aint size, int disp_unit, MPIR_Info * info,
                                        MPIR_Comm * comm, void *baseptr, MPIR_Win ** win);
typedef int (*MPIDI_NM_win_flush_t) (int rank, MPIR_Win * win);
typedef int (*MPIDI_NM_win_flush_local_all_t) (MPIR_Win * win);
typedef int (*MPIDI_NM_win_unlock_all_t) (MPIR_Win * win);
typedef int (*MPIDI_NM_win_create_dynamic_t) (MPIR_Info * info, MPIR_Comm * comm, MPIR_Win ** win);
typedef int (*MPIDI_NM_rget_t) (void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
                                int target_rank, MPI_Aint target_disp, int target_count,
                                MPI_Datatype target_datatype, MPIR_Win * win,
                                MPIR_Request ** request);
typedef int (*MPIDI_NM_win_sync_t) (MPIR_Win * win);
typedef int (*MPIDI_NM_win_flush_all_t) (MPIR_Win * win);
typedef int (*MPIDI_NM_get_accumulate_t) (const void *origin_addr, int origin_count,
                                          MPI_Datatype origin_datatype, void *result_addr,
                                          int result_count, MPI_Datatype result_datatype,
                                          int target_rank, MPI_Aint target_disp, int target_count,
                                          MPI_Datatype target_datatype, MPI_Op op, MPIR_Win * win);
typedef int (*MPIDI_NM_win_lock_all_t) (int assert, MPIR_Win * win);
typedef int (*MPIDI_NM_rank_is_local_t) (int target, MPIR_Comm * comm);
typedef int (*MPIDI_NM_barrier_t) (MPIR_Comm * comm, MPIR_Errflag_t * errflag);
typedef int (*MPIDI_NM_bcast_t) (void *buffer, int count, MPI_Datatype datatype, int root,
                                 MPIR_Comm * comm, MPIR_Errflag_t * errflag);
typedef int (*MPIDI_NM_allreduce_t) (const void *sendbuf, void *recvbuf, int count,
                                     MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                     MPIR_Errflag_t * errflag);
typedef int (*MPIDI_NM_allgather_t) (const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                     void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                     MPIR_Comm * comm, MPIR_Errflag_t * errflag);
typedef int (*MPIDI_NM_allgatherv_t) (const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, const int *recvcounts, const int *displs,
                                      MPI_Datatype recvtype, MPIR_Comm * comm,
                                      MPIR_Errflag_t * errflag);
typedef int (*MPIDI_NM_scatter_t) (const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                                   MPIR_Comm * comm, MPIR_Errflag_t * errflag);
typedef int (*MPIDI_NM_scatterv_t) (const void *sendbuf, const int *sendcounts, const int *displs,
                                    MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                    MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                    MPIR_Errflag_t * errflag);
typedef int (*MPIDI_NM_gather_t) (const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                                  MPIR_Comm * comm, MPIR_Errflag_t * errflag);
typedef int (*MPIDI_NM_gatherv_t) (const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, const int *recvcounts, const int *displs,
                                   MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                   MPIR_Errflag_t * errflag);
typedef int (*MPIDI_NM_alltoall_t) (const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                    void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                    MPIR_Comm * comm, MPIR_Errflag_t * errflag);
typedef int (*MPIDI_NM_alltoallv_t) (const void *sendbuf, const int *sendcounts, const int *sdispls,
                                     MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                                     const int *rdispls, MPI_Datatype recvtype, MPIR_Comm * comm,
                                     MPIR_Errflag_t * errflag);
typedef int (*MPIDI_NM_alltoallw_t) (const void *sendbuf, const int *sendcounts, const int *sdispls,
                                     const MPI_Datatype sendtypes[], void *recvbuf,
                                     const int *recvcounts, const int *rdispls,
                                     const MPI_Datatype recvtypes[], MPIR_Comm * comm,
                                     MPIR_Errflag_t * errflag);
typedef int (*MPIDI_NM_reduce_t) (const void *sendbuf, void *recvbuf, int count,
                                  MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                                  MPIR_Errflag_t * errflag);
typedef int (*MPIDI_NM_reduce_scatter_t) (const void *sendbuf, void *recvbuf, const int *recvcounts,
                                          MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                          MPIR_Errflag_t * errflag);
typedef int (*MPIDI_NM_reduce_scatter_block_t) (const void *sendbuf, void *recvbuf, int recvcount,
                                                MPI_Datatype datatype, MPI_Op op,
                                                MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
typedef int (*MPIDI_NM_scan_t) (const void *sendbuf, void *recvbuf, int count,
                                MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                MPIR_Errflag_t * errflag);
typedef int (*MPIDI_NM_exscan_t) (const void *sendbuf, void *recvbuf, int count,
                                  MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                  MPIR_Errflag_t * errflag);
typedef int (*MPIDI_NM_neighbor_allgather_t) (const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                              MPI_Datatype recvtype, MPIR_Comm * comm);
typedef int (*MPIDI_NM_neighbor_allgatherv_t) (const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               const int *recvcounts, const int *displs,
                                               MPI_Datatype recvtype, MPIR_Comm * comm);
typedef int (*MPIDI_NM_neighbor_alltoallv_t) (const void *sendbuf, const int *sendcounts,
                                              const int *sdispls, MPI_Datatype sendtype,
                                              void *recvbuf, const int *recvcounts,
                                              const int *rdispls, MPI_Datatype recvtype,
                                              MPIR_Comm * comm);
typedef int (*MPIDI_NM_neighbor_alltoallw_t) (const void *sendbuf, const int *sendcounts,
                                              const MPI_Aint * sdispls,
                                              const MPI_Datatype * sendtypes, void *recvbuf,
                                              const int *recvcounts, const MPI_Aint * rdispls,
                                              const MPI_Datatype * recvtypes, MPIR_Comm * comm);
typedef int (*MPIDI_NM_neighbor_alltoall_t) (const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                             MPI_Datatype recvtype, MPIR_Comm * comm);
typedef int (*MPIDI_NM_ineighbor_allgather_t) (const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                               MPI_Datatype recvtype, MPIR_Comm * comm,
                                               MPI_Request * req);
typedef int (*MPIDI_NM_ineighbor_allgatherv_t) (const void *sendbuf, int sendcount,
                                                MPI_Datatype sendtype, void *recvbuf,
                                                const int *recvcounts, const int *displs,
                                                MPI_Datatype recvtype, MPIR_Comm * comm,
                                                MPI_Request * req);
typedef int (*MPIDI_NM_ineighbor_alltoall_t) (const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                              MPI_Datatype recvtype, MPIR_Comm * comm,
                                              MPI_Request * req);
typedef int (*MPIDI_NM_ineighbor_alltoallv_t) (const void *sendbuf, const int *sendcounts,
                                               const int *sdispls, MPI_Datatype sendtype,
                                               void *recvbuf, const int *recvcounts,
                                               const int *rdispls, MPI_Datatype recvtype,
                                               MPIR_Comm * comm, MPI_Request * req);
typedef int (*MPIDI_NM_ineighbor_alltoallw_t) (const void *sendbuf, const int *sendcounts,
                                               const MPI_Aint * sdispls,
                                               const MPI_Datatype * sendtypes, void *recvbuf,
                                               const int *recvcounts, const MPI_Aint * rdispls,
                                               const MPI_Datatype * recvtypes, MPIR_Comm * comm,
                                               MPI_Request * req);
typedef int (*MPIDI_NM_ibarrier_t) (MPIR_Comm * comm, MPI_Request * req);
typedef int (*MPIDI_NM_ibcast_t) (void *buffer, int count, MPI_Datatype datatype, int root,
                                  MPIR_Comm * comm, MPI_Request * req);
typedef int (*MPIDI_NM_iallgather_t) (const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                      MPIR_Comm * comm, MPI_Request * req);
typedef int (*MPIDI_NM_iallgatherv_t) (const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, const int *recvcounts, const int *displs,
                                       MPI_Datatype recvtype, MPIR_Comm * comm, MPI_Request * req);
typedef int (*MPIDI_NM_iallreduce_t) (const void *sendbuf, void *recvbuf, int count,
                                      MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                      MPI_Request * req);
typedef int (*MPIDI_NM_ialltoall_t) (const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                     void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                     MPIR_Comm * comm, MPI_Request * req);
typedef int (*MPIDI_NM_ialltoallv_t) (const void *sendbuf, const int *sendcounts,
                                      const int *sdispls, MPI_Datatype sendtype, void *recvbuf,
                                      const int *recvcounts, const int *rdispls,
                                      MPI_Datatype recvtype, MPIR_Comm * comm, MPI_Request * req);
typedef int (*MPIDI_NM_ialltoallw_t) (const void *sendbuf, const int *sendcounts,
                                      const int *sdispls, const MPI_Datatype sendtypes[],
                                      void *recvbuf, const int *recvcounts, const int *rdispls,
                                      const MPI_Datatype recvtypes[], MPIR_Comm * comm,
                                      MPI_Request * req);
typedef int (*MPIDI_NM_iexscan_t) (const void *sendbuf, void *recvbuf, int count,
                                   MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                   MPI_Request * req);
typedef int (*MPIDI_NM_igather_t) (const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                                   MPIR_Comm * comm, MPI_Request * req);
typedef int (*MPIDI_NM_igatherv_t) (const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                    void *recvbuf, const int *recvcounts, const int *displs,
                                    MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                    MPI_Request * req);
typedef int (*MPIDI_NM_ireduce_scatter_block_t) (const void *sendbuf, void *recvbuf, int recvcount,
                                                 MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                                 MPI_Request * req);
typedef int (*MPIDI_NM_ireduce_scatter_t) (const void *sendbuf, void *recvbuf,
                                           const int *recvcounts, MPI_Datatype datatype, MPI_Op op,
                                           MPIR_Comm * comm, MPI_Request * req);
typedef int (*MPIDI_NM_ireduce_t) (const void *sendbuf, void *recvbuf, int count,
                                   MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                                   MPI_Request * req);
typedef int (*MPIDI_NM_iscan_t) (const void *sendbuf, void *recvbuf, int count,
                                 MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                 MPI_Request * req);
typedef int (*MPIDI_NM_iscatter_t) (const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                    void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                                    MPIR_Comm * comm, MPI_Request * req);
typedef int (*MPIDI_NM_iscatterv_t) (const void *sendbuf, const int *sendcounts, const int *displs,
                                     MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                     MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                     MPI_Request * req);
typedef int (*MPIDI_NM_type_create_hook_t) (MPIR_Datatype * datatype_p);
typedef int (*MPIDI_NM_type_free_hook_t) (MPIR_Datatype * datatype_p);
typedef int (*MPIDI_NM_op_create_hook_t) (MPIR_Op * op_p);
typedef int (*MPIDI_NM_op_free_hook_t) (MPIR_Op * op_p);

typedef struct MPIDI_NM_funcs {
    MPIDI_NM_init_t init;
    MPIDI_NM_finalize_t finalize;
    MPIDI_NM_progress_t progress;
    MPIDI_NM_comm_connect_t comm_connect;
    MPIDI_NM_comm_disconnect_t comm_disconnect;
    MPIDI_NM_open_port_t open_port;
    MPIDI_NM_close_port_t close_port;
    MPIDI_NM_comm_accept_t comm_accept;
    /* Routines that handle addressing */
    MPIDI_NM_comm_get_lpid_t comm_get_lpid;
    MPIDI_NM_gpid_get_t gpid_get;
    MPIDI_NM_getallincomm_t getallincomm;
    MPIDI_NM_gpid_tolpidarray_t gpid_tolpidarray;
    MPIDI_NM_create_intercomm_from_lpids_t create_intercomm_from_lpids;
    MPIDI_NM_comm_create_hook_t comm_create_hook;
    MPIDI_NM_comm_free_hook_t comm_free_hook;
    /* Request allocation routines */
    MPIDI_NM_am_request_init_t am_request_init;
    MPIDI_NM_am_request_finalize_t am_request_finalize;
    /* Active Message Routines */
    MPIDI_NM_am_reg_handler_t am_reg_handler;
    MPIDI_NM_am_send_hdr_t am_send_hdr;
    MPIDI_NM_am_isend_t am_isend;
    MPIDI_NM_am_isendv_t am_isendv;
    MPIDI_NM_am_send_hdr_reply_t am_send_hdr_reply;
    MPIDI_NM_am_isend_reply_t am_isend_reply;
    MPIDI_NM_am_hdr_max_sz_t am_hdr_max_sz;
    MPIDI_NM_am_recv_t am_recv;
} MPIDI_NM_funcs_t;

typedef struct MPIDI_NM_native_funcs {
    MPIDI_NM_send_t send;
    MPIDI_NM_ssend_t ssend;
    MPIDI_NM_startall_t startall;
    MPIDI_NM_send_init_t send_init;
    MPIDI_NM_ssend_init_t ssend_init;
    MPIDI_NM_rsend_init_t rsend_init;
    MPIDI_NM_bsend_init_t bsend_init;
    MPIDI_NM_isend_t isend;
    MPIDI_NM_issend_t issend;
    MPIDI_NM_cancel_send_t cancel_send;
    MPIDI_NM_recv_init_t recv_init;
    MPIDI_NM_recv_t recv;
    MPIDI_NM_irecv_t irecv;
    MPIDI_NM_imrecv_t imrecv;
    MPIDI_NM_cancel_recv_t cancel_recv;
    MPIDI_NM_alloc_mem_t alloc_mem;
    MPIDI_NM_free_mem_t free_mem;
    MPIDI_NM_improbe_t improbe;
    MPIDI_NM_iprobe_t iprobe;
    MPIDI_NM_win_set_info_t win_set_info;
    MPIDI_NM_win_shared_query_t win_shared_query;
    MPIDI_NM_put_t put;
    MPIDI_NM_win_start_t win_start;
    MPIDI_NM_win_complete_t win_complete;
    MPIDI_NM_win_post_t win_post;
    MPIDI_NM_win_wait_t win_wait;
    MPIDI_NM_win_test_t win_test;
    MPIDI_NM_win_lock_t win_lock;
    MPIDI_NM_win_unlock_t win_unlock;
    MPIDI_NM_win_get_info_t win_get_info;
    MPIDI_NM_get_t get;
    MPIDI_NM_win_free_t win_free;
    MPIDI_NM_win_fence_t win_fence;
    MPIDI_NM_win_create_t win_create;
    MPIDI_NM_accumulate_t accumulate;
    MPIDI_NM_win_attach_t win_attach;
    MPIDI_NM_win_allocate_shared_t win_allocate_shared;
    MPIDI_NM_rput_t rput;
    MPIDI_NM_win_flush_local_t win_flush_local;
    MPIDI_NM_win_detach_t win_detach;
    MPIDI_NM_compare_and_swap_t compare_and_swap;
    MPIDI_NM_raccumulate_t raccumulate;
    MPIDI_NM_rget_accumulate_t rget_accumulate;
    MPIDI_NM_fetch_and_op_t fetch_and_op;
    MPIDI_NM_win_allocate_t win_allocate;
    MPIDI_NM_win_flush_t win_flush;
    MPIDI_NM_win_flush_local_all_t win_flush_local_all;
    MPIDI_NM_win_unlock_all_t win_unlock_all;
    MPIDI_NM_win_create_dynamic_t win_create_dynamic;
    MPIDI_NM_rget_t rget;
    MPIDI_NM_win_sync_t win_sync;
    MPIDI_NM_win_flush_all_t win_flush_all;
    MPIDI_NM_get_accumulate_t get_accumulate;
    MPIDI_NM_win_lock_all_t win_lock_all;
    MPIDI_NM_rank_is_local_t rank_is_local;
    /* Collectives */
    MPIDI_NM_barrier_t barrier;
    MPIDI_NM_bcast_t bcast;
    MPIDI_NM_allreduce_t allreduce;
    MPIDI_NM_allgather_t allgather;
    MPIDI_NM_allgatherv_t allgatherv;
    MPIDI_NM_scatter_t scatter;
    MPIDI_NM_scatterv_t scatterv;
    MPIDI_NM_gather_t gather;
    MPIDI_NM_gatherv_t gatherv;
    MPIDI_NM_alltoall_t alltoall;
    MPIDI_NM_alltoallv_t alltoallv;
    MPIDI_NM_alltoallw_t alltoallw;
    MPIDI_NM_reduce_t reduce;
    MPIDI_NM_reduce_scatter_t reduce_scatter;
    MPIDI_NM_reduce_scatter_block_t reduce_scatter_block;
    MPIDI_NM_scan_t scan;
    MPIDI_NM_exscan_t exscan;
    MPIDI_NM_neighbor_allgather_t neighbor_allgather;
    MPIDI_NM_neighbor_allgatherv_t neighbor_allgatherv;
    MPIDI_NM_neighbor_alltoall_t neighbor_alltoall;
    MPIDI_NM_neighbor_alltoallv_t neighbor_alltoallv;
    MPIDI_NM_neighbor_alltoallw_t neighbor_alltoallw;
    MPIDI_NM_ineighbor_allgather_t ineighbor_allgather;
    MPIDI_NM_ineighbor_allgatherv_t ineighbor_allgatherv;
    MPIDI_NM_ineighbor_alltoall_t ineighbor_alltoall;
    MPIDI_NM_ineighbor_alltoallv_t ineighbor_alltoallv;
    MPIDI_NM_ineighbor_alltoallw_t ineighbor_alltoallw;
    MPIDI_NM_ibarrier_t ibarrier;
    MPIDI_NM_ibcast_t ibcast;
    MPIDI_NM_iallgather_t iallgather;
    MPIDI_NM_iallgatherv_t iallgatherv;
    MPIDI_NM_iallreduce_t iallreduce;
    MPIDI_NM_ialltoall_t ialltoall;
    MPIDI_NM_ialltoallv_t ialltoallv;
    MPIDI_NM_ialltoallw_t ialltoallw;
    MPIDI_NM_iexscan_t iexscan;
    MPIDI_NM_igather_t igather;
    MPIDI_NM_igatherv_t igatherv;
    MPIDI_NM_ireduce_scatter_block_t ireduce_scatter_block;
    MPIDI_NM_ireduce_scatter_t ireduce_scatter;
    MPIDI_NM_ireduce_t ireduce;
    MPIDI_NM_iscan_t iscan;
    MPIDI_NM_iscatter_t iscatter;
    MPIDI_NM_iscatterv_t iscatterv;
    /* Datatype hooks */
    MPIDI_NM_type_create_hook_t type_create_hook;
    MPIDI_NM_type_free_hook_t type_free_hook;
    /* Op hooks */
    MPIDI_NM_op_create_hook_t op_create_hook;
    MPIDI_NM_op_free_hook_t op_free_hook;
} MPIDI_NM_native_funcs_t;

extern MPIDI_NM_funcs_t *MPIDI_NM_funcs[];
extern MPIDI_NM_funcs_t *MPIDI_NM_func;
extern MPIDI_NM_native_funcs_t *MPIDI_NM_native_funcs[];
extern MPIDI_NM_native_funcs_t *MPIDI_NM_native_func;
extern int MPIDI_num_netmods;
extern char MPIDI_NM_strings[][MPIDI_MAX_NETMOD_STRING_LEN];

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_init(int rank, int size, int appnum, int *tag_ub,
                                           MPIR_Comm * comm_world, MPIR_Comm * comm_self,
                                           int spawned, int num_contexts,
                                           void **netmod_contexts) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_finalize(void) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_progress(void *netmod_context,
                                               int blocking) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_reg_handler(int handler_id,
                                                     MPIDI_NM_am_origin_handler_fn
                                                     origin_handler_fn,
                                                     MPIDI_NM_am_target_handler_fn
                                                     target_handler_fn) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_comm_connect(const char *port_name, MPIR_Info * info,
                                                   int root, MPIR_Comm * comm,
                                                   MPIR_Comm **
                                                   newcomm_ptr) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_comm_disconnect(MPIR_Comm *
                                                      comm_ptr) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_open_port(MPIR_Info * info_ptr,
                                                char *port_name) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_close_port(const char *port_name) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_comm_accept(const char *port_name, MPIR_Info * info,
                                                  int root, MPIR_Comm * comm,
                                                  MPIR_Comm **
                                                  newcomm_ptr) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr(int rank, MPIR_Comm * comm, int handler_id,
                                                  const void *am_hdr, size_t am_hdr_sz,
                                                  void *netmod_context) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend(int rank, MPIR_Comm * comm, int handler_id,
                                               const void *am_hdr, size_t am_hdr_sz,
                                               const void *data, MPI_Count count,
                                               MPI_Datatype datatype, MPIR_Request * sreq,
                                               void *netmod_context) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isendv(int rank, MPIR_Comm * comm, int handler_id,
                                                struct iovec *am_hdrs, size_t iov_len,
                                                const void *data, MPI_Count count,
                                                MPI_Datatype datatype, MPIR_Request * sreq,
                                                void *netmod_context) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr_reply(MPIR_Context_id_t context_id,
                                                        int src_rank, int handler_id,
                                                        const void *am_hdr,
                                                        size_t am_hdr_sz) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend_reply(MPIR_Context_id_t context_id, int src_rank,
                                                     int handler_id, const void *am_hdr,
                                                     size_t am_hdr_sz, const void *data,
                                                     MPI_Count count, MPI_Datatype datatype,
                                                     MPIR_Request * sreq) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX size_t MPIDI_NM_am_hdr_max_sz(void) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_recv(MPIR_Request * req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_comm_get_lpid(MPIR_Comm * comm_ptr, int idx,
                                                    int *lpid_ptr,
                                                    MPL_bool is_remote) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_gpid_get(MPIR_Comm * comm_ptr, int rank,
                                               MPIR_Gpid * gpid) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_getallincomm(MPIR_Comm * comm_ptr, int local_size,
                                                   MPIR_Gpid local_gpids[],
                                                   int *singleAVT) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_gpid_tolpidarray(int size, MPIR_Gpid gpid[],
                                                       int lpid[]) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr,
                                                                  int size,
                                                                  const int lpids[])
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_comm_create_hook(MPIR_Comm * comm) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_comm_free_hook(MPIR_Comm * comm) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX void MPIDI_NM_am_request_init(MPIR_Request * req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX void MPIDI_NM_am_request_finalize(MPIR_Request *
                                                           req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_send(const void *buf, int count, MPI_Datatype datatype,
                                           int rank, int tag, MPIR_Comm * comm,
                                           int context_offset,
                                           MPIR_Request ** request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ssend(const void *buf, int count, MPI_Datatype datatype,
                                            int rank, int tag, MPIR_Comm * comm,
                                            int context_offset,
                                            MPIR_Request ** request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_startall(int count,
                                               MPIR_Request * requests[]) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_send_init(const void *buf, int count,
                                                MPI_Datatype datatype, int rank, int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIR_Request ** request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ssend_init(const void *buf, int count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIR_Request ** request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rsend_init(const void *buf, int count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIR_Request ** request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_bsend_init(const void *buf, int count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIR_Request ** request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_isend(const void *buf, int count, MPI_Datatype datatype,
                                            int rank, int tag, MPIR_Comm * comm,
                                            int context_offset,
                                            MPIR_Request ** request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_issend(const void *buf, int count, MPI_Datatype datatype,
                                             int rank, int tag, MPIR_Comm * comm,
                                             int context_offset,
                                             MPIR_Request ** request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_cancel_send(MPIR_Request * sreq) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_recv_init(void *buf, int count, MPI_Datatype datatype,
                                                int rank, int tag, MPIR_Comm * comm,
                                                int context_offset,
                                                MPIR_Request ** request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_recv(void *buf, int count, MPI_Datatype datatype,
                                           int rank, int tag, MPIR_Comm * comm,
                                           int context_offset, MPI_Status * status,
                                           MPIR_Request ** request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_irecv(void *buf, int count, MPI_Datatype datatype,
                                            int rank, int tag, MPIR_Comm * comm,
                                            int context_offset,
                                            MPIR_Request ** request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_imrecv(void *buf, int count, MPI_Datatype datatype,
                                             MPIR_Request * message,
                                             MPIR_Request ** rreqp) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_cancel_recv(MPIR_Request * rreq) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX void *MPIDI_NM_alloc_mem(size_t size,
                                                  MPIR_Info * info_ptr) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_free_mem(void *ptr) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_improbe(int source, int tag, MPIR_Comm * comm,
                                              int context_offset, int *flag,
                                              MPIR_Request ** message,
                                              MPI_Status * status) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_iprobe(int source, int tag, MPIR_Comm * comm,
                                             int context_offset, int *flag,
                                             MPI_Status * status) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_set_info(MPIR_Win * win,
                                                   MPIR_Info * info) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_shared_query(MPIR_Win * win, int rank,
                                                       MPI_Aint * size, int *disp_unit,
                                                       void *baseptr) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_put(const void *origin_addr, int origin_count,
                                          MPI_Datatype origin_datatype, int target_rank,
                                          MPI_Aint target_disp, int target_count,
                                          MPI_Datatype target_datatype,
                                          MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_start(MPIR_Group * group, int assert,
                                                MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_complete(MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_post(MPIR_Group * group, int assert,
                                               MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_wait(MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_test(MPIR_Win * win, int *flag) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_lock(int lock_type, int rank, int assert,
                                               MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_unlock(int rank, MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_get_info(MPIR_Win * win,
                                                   MPIR_Info ** info_p_p) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_get(void *origin_addr, int origin_count,
                                          MPI_Datatype origin_datatype, int target_rank,
                                          MPI_Aint target_disp, int target_count,
                                          MPI_Datatype target_datatype,
                                          MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_free(MPIR_Win ** win_ptr) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_fence(int assert,
                                                MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_create(void *base, MPI_Aint length, int disp_unit,
                                                 MPIR_Info * info, MPIR_Comm * comm_ptr,
                                                 MPIR_Win ** win_ptr) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_accumulate(const void *origin_addr, int origin_count,
                                                 MPI_Datatype origin_datatype, int target_rank,
                                                 MPI_Aint target_disp, int target_count,
                                                 MPI_Datatype target_datatype, MPI_Op op,
                                                 MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_attach(MPIR_Win * win, void *base,
                                                 MPI_Aint size) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_allocate_shared(MPI_Aint size, int disp_unit,
                                                          MPIR_Info * info_ptr,
                                                          MPIR_Comm * comm_ptr,
                                                          void **base_ptr,
                                                          MPIR_Win **
                                                          win_ptr) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rput(const void *origin_addr, int origin_count,
                                           MPI_Datatype origin_datatype, int target_rank,
                                           MPI_Aint target_disp, int target_count,
                                           MPI_Datatype target_datatype, MPIR_Win * win,
                                           MPIR_Request ** request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_flush_local(int rank,
                                                      MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_detach(MPIR_Win * win,
                                                 const void *base) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_compare_and_swap(const void *origin_addr,
                                                       const void *compare_addr,
                                                       void *result_addr,
                                                       MPI_Datatype datatype, int target_rank,
                                                       MPI_Aint target_disp,
                                                       MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_raccumulate(const void *origin_addr, int origin_count,
                                                  MPI_Datatype origin_datatype,
                                                  int target_rank, MPI_Aint target_disp,
                                                  int target_count,
                                                  MPI_Datatype target_datatype, MPI_Op op,
                                                  MPIR_Win * win,
                                                  MPIR_Request ** request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rget_accumulate(const void *origin_addr,
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
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_fetch_and_op(const void *origin_addr, void *result_addr,
                                                   MPI_Datatype datatype, int target_rank,
                                                   MPI_Aint target_disp, MPI_Op op,
                                                   MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_allocate(MPI_Aint size, int disp_unit,
                                                   MPIR_Info * info, MPIR_Comm * comm,
                                                   void *baseptr,
                                                   MPIR_Win ** win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_flush(int rank, MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_flush_local_all(MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_unlock_all(MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm,
                                                         MPIR_Win ** win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rget(void *origin_addr, int origin_count,
                                           MPI_Datatype origin_datatype, int target_rank,
                                           MPI_Aint target_disp, int target_count,
                                           MPI_Datatype target_datatype, MPIR_Win * win,
                                           MPIR_Request ** request) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_sync(MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_flush_all(MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_get_accumulate(const void *origin_addr, int origin_count,
                                                     MPI_Datatype origin_datatype,
                                                     void *result_addr, int result_count,
                                                     MPI_Datatype result_datatype,
                                                     int target_rank, MPI_Aint target_disp,
                                                     int target_count,
                                                     MPI_Datatype target_datatype, MPI_Op op,
                                                     MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_lock_all(int assert,
                                                   MPIR_Win * win) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rank_is_local(int target,
                                                    MPIR_Comm * comm) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_barrier(MPIR_Comm * comm,
                                              MPIR_Errflag_t * errflag) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_bcast(void *buffer, int count, MPI_Datatype datatype,
                                            int root, MPIR_Comm * comm,
                                            MPIR_Errflag_t * errflag) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_allreduce(const void *sendbuf, void *recvbuf, int count,
                                                MPI_Datatype datatype, MPI_Op op,
                                                MPIR_Comm * comm,
                                                MPIR_Errflag_t * errflag) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_allgather(const void *sendbuf, int sendcount,
                                                MPI_Datatype sendtype, void *recvbuf,
                                                int recvcount, MPI_Datatype recvtype,
                                                MPIR_Comm * comm,
                                                MPIR_Errflag_t * errflag) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_allgatherv(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 const int *recvcounts, const int *displs,
                                                 MPI_Datatype recvtype, MPIR_Comm * comm,
                                                 MPIR_Errflag_t * errflag) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_scatter(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              int recvcount, MPI_Datatype recvtype, int root,
                                              MPIR_Comm * comm,
                                              MPIR_Errflag_t * errflag) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_scatterv(const void *sendbuf, const int *sendcounts,
                                               const int *displs, MPI_Datatype sendtype,
                                               void *recvbuf, int recvcount,
                                               MPI_Datatype recvtype, int root,
                                               MPIR_Comm * comm_ptr,
                                               MPIR_Errflag_t * errflag) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_gather(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf,
                                             int recvcount, MPI_Datatype recvtype, int root,
                                             MPIR_Comm * comm,
                                             MPIR_Errflag_t * errflag) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_gatherv(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              const int *recvcounts, const int *displs,
                                              MPI_Datatype recvtype, int root,
                                              MPIR_Comm * comm,
                                              MPIR_Errflag_t * errflag) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_alltoall(const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               int recvcount, MPI_Datatype recvtype,
                                               MPIR_Comm * comm,
                                               MPIR_Errflag_t * errflag) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_alltoallv(const void *sendbuf, const int *sendcounts,
                                                const int *sdispls, MPI_Datatype sendtype,
                                                void *recvbuf, const int *recvcounts,
                                                const int *rdispls, MPI_Datatype recvtype,
                                                MPIR_Comm * comm,
                                                MPIR_Errflag_t * errflag) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_alltoallw(const void *sendbuf, const int *sendcounts,
                                                const int *sdispls,
                                                const MPI_Datatype sendtypes[], void *recvbuf,
                                                const int *recvcounts, const int *rdispls,
                                                const MPI_Datatype recvtypes[],
                                                MPIR_Comm * comm,
                                                MPIR_Errflag_t * errflag) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_reduce(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op, int root,
                                             MPIR_Comm * comm_ptr,
                                             MPIR_Errflag_t * errflag) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_reduce_scatter(const void *sendbuf, void *recvbuf,
                                                     const int *recvcounts,
                                                     MPI_Datatype datatype, MPI_Op op,
                                                     MPIR_Comm * comm_ptr,
                                                     MPIR_Errflag_t *
                                                     errflag) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                           int recvcount,
                                                           MPI_Datatype datatype, MPI_Op op,
                                                           MPIR_Comm * comm_ptr,
                                                           MPIR_Errflag_t *
                                                           errflag) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_scan(const void *sendbuf, void *recvbuf, int count,
                                           MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                           MPIR_Errflag_t * errflag) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_exscan(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op,
                                             MPIR_Comm * comm,
                                             MPIR_Errflag_t * errflag) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_neighbor_allgather(const void *sendbuf, int sendcount,
                                                         MPI_Datatype sendtype, void *recvbuf,
                                                         int recvcount, MPI_Datatype recvtype,
                                                         MPIR_Comm * comm) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_neighbor_allgatherv(const void *sendbuf, int sendcount,
                                                          MPI_Datatype sendtype, void *recvbuf,
                                                          const int *recvcounts,
                                                          const int *displs,
                                                          MPI_Datatype recvtype,
                                                          MPIR_Comm *
                                                          comm) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_neighbor_alltoallv(const void *sendbuf,
                                                         const int *sendcounts,
                                                         const int *sdispls,
                                                         MPI_Datatype sendtype, void *recvbuf,
                                                         const int *recvcounts,
                                                         const int *rdispls,
                                                         MPI_Datatype recvtype,
                                                         MPIR_Comm * comm) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_neighbor_alltoallw(const void *sendbuf,
                                                         const int *sendcounts,
                                                         const MPI_Aint * sdispls,
                                                         const MPI_Datatype * sendtypes,
                                                         void *recvbuf, const int *recvcounts,
                                                         const MPI_Aint * rdispls,
                                                         const MPI_Datatype * recvtypes,
                                                         MPIR_Comm * comm) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_neighbor_alltoall(const void *sendbuf, int sendcount,
                                                        MPI_Datatype sendtype, void *recvbuf,
                                                        int recvcount, MPI_Datatype recvtype,
                                                        MPIR_Comm * comm) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ineighbor_allgather(const void *sendbuf, int sendcount,
                                                          MPI_Datatype sendtype, void *recvbuf,
                                                          int recvcount, MPI_Datatype recvtype,
                                                          MPIR_Comm * comm,
                                                          MPI_Request *
                                                          req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ineighbor_allgatherv(const void *sendbuf, int sendcount,
                                                           MPI_Datatype sendtype,
                                                           void *recvbuf,
                                                           const int *recvcounts,
                                                           const int *displs,
                                                           MPI_Datatype recvtype,
                                                           MPIR_Comm * comm,
                                                           MPI_Request *
                                                           req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ineighbor_alltoall(const void *sendbuf, int sendcount,
                                                         MPI_Datatype sendtype, void *recvbuf,
                                                         int recvcount, MPI_Datatype recvtype,
                                                         MPIR_Comm * comm,
                                                         MPI_Request *
                                                         req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ineighbor_alltoallv(const void *sendbuf,
                                                          const int *sendcounts,
                                                          const int *sdispls,
                                                          MPI_Datatype sendtype, void *recvbuf,
                                                          const int *recvcounts,
                                                          const int *rdispls,
                                                          MPI_Datatype recvtype,
                                                          MPIR_Comm * comm,
                                                          MPI_Request *
                                                          req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ineighbor_alltoallw(const void *sendbuf,
                                                          const int *sendcounts,
                                                          const MPI_Aint * sdispls,
                                                          const MPI_Datatype * sendtypes,
                                                          void *recvbuf, const int *recvcounts,
                                                          const MPI_Aint * rdispls,
                                                          const MPI_Datatype * recvtypes,
                                                          MPIR_Comm * comm,
                                                          MPI_Request *
                                                          req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ibarrier(MPIR_Comm * comm,
                                               MPI_Request * req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ibcast(void *buffer, int count, MPI_Datatype datatype,
                                             int root, MPIR_Comm * comm,
                                             MPI_Request * req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_iallgather(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 int recvcount, MPI_Datatype recvtype,
                                                 MPIR_Comm * comm,
                                                 MPI_Request * req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_iallgatherv(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  const int *recvcounts, const int *displs,
                                                  MPI_Datatype recvtype, MPIR_Comm * comm,
                                                  MPI_Request * req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_iallreduce(const void *sendbuf, void *recvbuf, int count,
                                                 MPI_Datatype datatype, MPI_Op op,
                                                 MPIR_Comm * comm,
                                                 MPI_Request * req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ialltoall(const void *sendbuf, int sendcount,
                                                MPI_Datatype sendtype, void *recvbuf,
                                                int recvcount, MPI_Datatype recvtype,
                                                MPIR_Comm * comm,
                                                MPI_Request * req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ialltoallv(const void *sendbuf, const int *sendcounts,
                                                 const int *sdispls, MPI_Datatype sendtype,
                                                 void *recvbuf, const int *recvcounts,
                                                 const int *rdispls, MPI_Datatype recvtype,
                                                 MPIR_Comm * comm,
                                                 MPI_Request * req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ialltoallw(const void *sendbuf, const int *sendcounts,
                                                 const int *sdispls,
                                                 const MPI_Datatype sendtypes[], void *recvbuf,
                                                 const int *recvcounts, const int *rdispls,
                                                 const MPI_Datatype recvtypes[],
                                                 MPIR_Comm * comm,
                                                 MPI_Request * req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_iexscan(const void *sendbuf, void *recvbuf, int count,
                                              MPI_Datatype datatype, MPI_Op op,
                                              MPIR_Comm * comm,
                                              MPI_Request * req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_igather(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              int recvcount, MPI_Datatype recvtype, int root,
                                              MPIR_Comm * comm,
                                              MPI_Request * req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_igatherv(const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               const int *recvcounts, const int *displs,
                                               MPI_Datatype recvtype, int root,
                                               MPIR_Comm * comm,
                                               MPI_Request * req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                            int recvcount,
                                                            MPI_Datatype datatype, MPI_Op op,
                                                            MPIR_Comm * comm,
                                                            MPI_Request *
                                                            req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ireduce_scatter(const void *sendbuf, void *recvbuf,
                                                      const int *recvcounts,
                                                      MPI_Datatype datatype, MPI_Op op,
                                                      MPIR_Comm * comm,
                                                      MPI_Request * req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ireduce(const void *sendbuf, void *recvbuf, int count,
                                              MPI_Datatype datatype, MPI_Op op, int root,
                                              MPIR_Comm * comm_ptr,
                                              MPI_Request * req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_iscan(const void *sendbuf, void *recvbuf, int count,
                                            MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                            MPI_Request * req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_iscatter(const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               int recvcount, MPI_Datatype recvtype, int root,
                                               MPIR_Comm * comm,
                                               MPI_Request * req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_iscatterv(const void *sendbuf, const int *sendcounts,
                                                const int *displs, MPI_Datatype sendtype,
                                                void *recvbuf, int recvcount,
                                                MPI_Datatype recvtype, int root,
                                                MPIR_Comm * comm_ptr,
                                                MPI_Request * req) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_type_create_hook(MPIR_Datatype *
                                                       datatype_p) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_type_free_hook(MPIR_Datatype *
                                                     datatype_p) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_op_create_hook(MPIR_Op * op_p) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_op_free_hook(MPIR_Op * op_p) MPL_STATIC_INLINE_SUFFIX;

#endif
