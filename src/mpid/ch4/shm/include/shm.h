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

static inline int MPIDI_SHM_mpi_init_hook(int rank, int size, int *n_vnis_provided);
static inline int MPIDI_SHM_mpi_finalize_hook(void);
static inline int MPIDI_SHM_get_vni_attr(int vni);
static inline int MPIDI_SHM_progress(int vni, int blocking);
static inline int MPIDI_SHM_mpi_comm_connect(const char *port_name, MPIR_Info * info,
                                             int root, int timeout, MPIR_Comm * comm,
                                             MPIR_Comm ** newcomm_ptr);
static inline int MPIDI_SHM_mpi_comm_disconnect(MPIR_Comm * comm_ptr);
static inline int MPIDI_SHM_mpi_open_port(MPIR_Info * info_ptr, char *port_name);
static inline int MPIDI_SHM_mpi_close_port(const char *port_name);
static inline int MPIDI_SHM_mpi_comm_accept(const char *port_name, MPIR_Info * info,
                                            int root, MPIR_Comm * comm, MPIR_Comm ** newcomm_ptr);
static inline int MPIDI_SHM_am_send_hdr(int rank, MPIR_Comm * comm, int handler_id,
                                        const void *am_hdr, size_t am_hdr_sz);
static inline int MPIDI_SHM_am_isend(int rank, MPIR_Comm * comm, int handler_id,
                                     const void *am_hdr, size_t am_hdr_sz,
                                     const void *data, MPI_Count count,
                                     MPI_Datatype datatype, MPIR_Request * sreq);
static inline int MPIDI_SHM_am_isendv(int rank, MPIR_Comm * comm, int handler_id,
                                      struct iovec *am_hdrs, size_t iov_len,
                                      const void *data, MPI_Count count,
                                      MPI_Datatype datatype, MPIR_Request * sreq);
static inline int MPIDI_SHM_am_send_hdr_reply(MPIR_Context_id_t context_id, int src_rank,
                                              int handler_id, const void *am_hdr, size_t am_hdr_sz);
static inline int MPIDI_SHM_am_isend_reply(MPIR_Context_id_t context_id, int src_rank,
                                           int handler_id, const void *am_hdr,
                                           size_t am_hdr_sz, const void *data,
                                           MPI_Count count, MPI_Datatype datatype,
                                           MPIR_Request * sreq);
static inline size_t MPIDI_SHM_am_hdr_max_sz(void);
static inline int MPIDI_SHM_am_recv(MPIR_Request * req);
static inline int MPIDI_SHM_comm_get_lpid(MPIR_Comm * comm_ptr, int idx,
                                          int *lpid_ptr, MPL_bool is_remote);
static inline int MPIDI_SHM_get_local_upids(MPIR_Comm * comm, size_t ** local_upid_size,
                                            char **local_upids);
static inline int MPIDI_SHM_upids_to_lupids(int size, size_t * remote_upid_size,
                                            char *remote_upids, int **remote_lupids);
static inline int MPIDI_SHM_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr,
                                                        int size, const int lpids[]);
static inline int MPIDI_SHM_mpi_comm_create_hook(MPIR_Comm * comm);
static inline int MPIDI_SHM_mpi_comm_free_hook(MPIR_Comm * comm);
static inline int MPIDI_SHM_mpi_type_commit_hook(MPIR_Datatype * type);
static inline int MPIDI_SHM_mpi_type_free_hook(MPIR_Datatype * type);
static inline int MPIDI_SHM_mpi_op_commit_hook(MPIR_Op * op);
static inline int MPIDI_SHM_mpi_op_free_hook(MPIR_Op * op);
static inline void MPIDI_SHM_am_request_init(MPIR_Request * req);
static inline void MPIDI_SHM_am_request_finalize(MPIR_Request * req);
static inline int MPIDI_SHM_mpi_send(const void *buf, MPI_Aint count,
                                     MPI_Datatype datatype, int rank, int tag,
                                     MPIR_Comm * comm, int context_offset, MPIR_Request ** request);
static inline int MPIDI_SHM_mpi_ssend(const void *buf, MPI_Aint count,
                                      MPI_Datatype datatype, int rank, int tag,
                                      MPIR_Comm * comm, int context_offset,
                                      MPIR_Request ** request);
static inline int MPIDI_SHM_mpi_startall(int count, MPIR_Request * requests[]);
static inline int MPIDI_SHM_mpi_send_init(const void *buf, int count,
                                          MPI_Datatype datatype, int rank, int tag,
                                          MPIR_Comm * comm, int context_offset,
                                          MPIR_Request ** request);
static inline int MPIDI_SHM_mpi_ssend_init(const void *buf, int count,
                                           MPI_Datatype datatype, int rank, int tag,
                                           MPIR_Comm * comm, int context_offset,
                                           MPIR_Request ** request);
static inline int MPIDI_SHM_mpi_rsend_init(const void *buf, int count,
                                           MPI_Datatype datatype, int rank, int tag,
                                           MPIR_Comm * comm, int context_offset,
                                           MPIR_Request ** request);
static inline int MPIDI_SHM_mpi_bsend_init(const void *buf, int count,
                                           MPI_Datatype datatype, int rank, int tag,
                                           MPIR_Comm * comm, int context_offset,
                                           MPIR_Request ** request);
static inline int MPIDI_SHM_mpi_isend(const void *buf, MPI_Aint count,
                                      MPI_Datatype datatype, int rank, int tag,
                                      MPIR_Comm * comm, int context_offset,
                                      MPIR_Request ** request);
static inline int MPIDI_SHM_mpi_issend(const void *buf, MPI_Aint count,
                                       MPI_Datatype datatype, int rank, int tag,
                                       MPIR_Comm * comm, int context_offset,
                                       MPIR_Request ** request);
static inline int MPIDI_SHM_mpi_cancel_send(MPIR_Request * sreq);
static inline int MPIDI_SHM_mpi_recv_init(void *buf, int count, MPI_Datatype datatype,
                                          int rank, int tag, MPIR_Comm * comm,
                                          int context_offset, MPIR_Request ** request);
static inline int MPIDI_SHM_mpi_recv(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                     int rank, int tag, MPIR_Comm * comm,
                                     int context_offset, MPI_Status * status,
                                     MPIR_Request ** request);
static inline int MPIDI_SHM_mpi_irecv(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                      int rank, int tag, MPIR_Comm * comm,
                                      int context_offset, MPIR_Request ** request);
static inline int MPIDI_SHM_mpi_imrecv(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                       MPIR_Request * message, MPIR_Request ** rreqp);
static inline int MPIDI_SHM_mpi_cancel_recv(MPIR_Request * rreq);
static inline void *MPIDI_SHM_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr);
static inline int MPIDI_SHM_mpi_free_mem(void *ptr);
static inline int MPIDI_SHM_mpi_improbe(int source, int tag, MPIR_Comm * comm,
                                        int context_offset, int *flag,
                                        MPIR_Request ** message, MPI_Status * status);
static inline int MPIDI_SHM_mpi_iprobe(int source, int tag, MPIR_Comm * comm,
                                       int context_offset, int *flag, MPI_Status * status);
static inline int MPIDI_SHM_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info);
static inline int MPIDI_SHM_mpi_win_shared_query(MPIR_Win * win, int rank,
                                                 MPI_Aint * size, int *disp_unit, void *baseptr);
static inline int MPIDI_SHM_mpi_put(const void *origin_addr, int origin_count,
                                    MPI_Datatype origin_datatype, int target_rank,
                                    MPI_Aint target_disp, int target_count,
                                    MPI_Datatype target_datatype, MPIR_Win * win);
static inline int MPIDI_SHM_mpi_win_start(MPIR_Group * group, int assert, MPIR_Win * win);
static inline int MPIDI_SHM_mpi_win_complete(MPIR_Win * win);
static inline int MPIDI_SHM_mpi_win_post(MPIR_Group * group, int assert, MPIR_Win * win);
static inline int MPIDI_SHM_mpi_win_wait(MPIR_Win * win);
static inline int MPIDI_SHM_mpi_win_test(MPIR_Win * win, int *flag);
static inline int MPIDI_SHM_mpi_win_lock(int lock_type, int rank, int assert, MPIR_Win * win);
static inline int MPIDI_SHM_mpi_win_unlock(int rank, MPIR_Win * win);
static inline int MPIDI_SHM_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p);
static inline int MPIDI_SHM_mpi_get(void *origin_addr, int origin_count,
                                    MPI_Datatype origin_datatype, int target_rank,
                                    MPI_Aint target_disp, int target_count,
                                    MPI_Datatype target_datatype, MPIR_Win * win);
static inline int MPIDI_SHM_mpi_win_free(MPIR_Win ** win_ptr);
static inline int MPIDI_SHM_mpi_win_fence(int assert, MPIR_Win * win);
static inline int MPIDI_SHM_mpi_win_create(void *base, MPI_Aint length, int disp_unit,
                                           MPIR_Info * info, MPIR_Comm * comm_ptr,
                                           MPIR_Win ** win_ptr);
static inline int MPIDI_SHM_mpi_accumulate(const void *origin_addr, int origin_count,
                                           MPI_Datatype origin_datatype,
                                           int target_rank, MPI_Aint target_disp,
                                           int target_count,
                                           MPI_Datatype target_datatype, MPI_Op op, MPIR_Win * win);
static inline int MPIDI_SHM_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size);
static inline int MPIDI_SHM_mpi_win_allocate_shared(MPI_Aint size, int disp_unit,
                                                    MPIR_Info * info_ptr,
                                                    MPIR_Comm * comm_ptr,
                                                    void **base_ptr, MPIR_Win ** win_ptr);
static inline int MPIDI_SHM_mpi_rput(const void *origin_addr, int origin_count,
                                     MPI_Datatype origin_datatype, int target_rank,
                                     MPI_Aint target_disp, int target_count,
                                     MPI_Datatype target_datatype, MPIR_Win * win,
                                     MPIR_Request ** request);
static inline int MPIDI_SHM_mpi_win_flush_local(int rank, MPIR_Win * win);
static inline int MPIDI_SHM_mpi_win_detach(MPIR_Win * win, const void *base);
static inline int MPIDI_SHM_mpi_compare_and_swap(const void *origin_addr,
                                                 const void *compare_addr,
                                                 void *result_addr,
                                                 MPI_Datatype datatype,
                                                 int target_rank, MPI_Aint target_disp,
                                                 MPIR_Win * win);
static inline int MPIDI_SHM_mpi_raccumulate(const void *origin_addr, int origin_count,
                                            MPI_Datatype origin_datatype,
                                            int target_rank, MPI_Aint target_disp,
                                            int target_count,
                                            MPI_Datatype target_datatype, MPI_Op op,
                                            MPIR_Win * win, MPIR_Request ** request);
static inline int MPIDI_SHM_mpi_rget_accumulate(const void *origin_addr,
                                                int origin_count,
                                                MPI_Datatype origin_datatype,
                                                void *result_addr, int result_count,
                                                MPI_Datatype result_datatype,
                                                int target_rank, MPI_Aint target_disp,
                                                int target_count,
                                                MPI_Datatype target_datatype, MPI_Op op,
                                                MPIR_Win * win, MPIR_Request ** request);
static inline int MPIDI_SHM_mpi_fetch_and_op(const void *origin_addr, void *result_addr,
                                             MPI_Datatype datatype, int target_rank,
                                             MPI_Aint target_disp, MPI_Op op, MPIR_Win * win);
static inline int MPIDI_SHM_mpi_win_allocate(MPI_Aint size, int disp_unit,
                                             MPIR_Info * info, MPIR_Comm * comm,
                                             void *baseptr, MPIR_Win ** win);
static inline int MPIDI_SHM_mpi_win_flush(int rank, MPIR_Win * win);
static inline int MPIDI_SHM_mpi_win_flush_local_all(MPIR_Win * win);
static inline int MPIDI_SHM_mpi_win_unlock_all(MPIR_Win * win);
static inline int MPIDI_SHM_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm,
                                                   MPIR_Win ** win);
static inline int MPIDI_SHM_mpi_rget(void *origin_addr, int origin_count,
                                     MPI_Datatype origin_datatype, int target_rank,
                                     MPI_Aint target_disp, int target_count,
                                     MPI_Datatype target_datatype, MPIR_Win * win,
                                     MPIR_Request ** request);
static inline int MPIDI_SHM_mpi_win_sync(MPIR_Win * win);
static inline int MPIDI_SHM_mpi_win_flush_all(MPIR_Win * win);
static inline int MPIDI_SHM_mpi_get_accumulate(const void *origin_addr,
                                               int origin_count,
                                               MPI_Datatype origin_datatype,
                                               void *result_addr, int result_count,
                                               MPI_Datatype result_datatype,
                                               int target_rank, MPI_Aint target_disp,
                                               int target_count,
                                               MPI_Datatype target_datatype, MPI_Op op,
                                               MPIR_Win * win);
static inline int MPIDI_SHM_mpi_win_lock_all(int assert, MPIR_Win * win);
static inline int MPIDI_SHM_mpi_barrier(MPIR_Comm * comm,
                                        MPIR_Errflag_t * errflag, void *algo_parameters_ptr);
static inline int MPIDI_SHM_mpi_bcast(void *buffer, int count, MPI_Datatype datatype,
                                      int root, MPIR_Comm * comm,
                                      MPIR_Errflag_t * errflag, void *algo_parameters_ptr);
static inline int MPIDI_SHM_mpi_allreduce(const void *sendbuf, void *recvbuf, int count,
                                          MPI_Datatype datatype, MPI_Op op,
                                          MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                          void *algo_parameters_ptr);
static inline int MPIDI_SHM_mpi_allgather(const void *sendbuf, int sendcount,
                                          MPI_Datatype sendtype, void *recvbuf,
                                          int recvcount, MPI_Datatype recvtype,
                                          MPIR_Comm * comm, MPIR_Errflag_t * errflag);
static inline int MPIDI_SHM_mpi_allgatherv(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           const int *recvcounts, const int *displs,
                                           MPI_Datatype recvtype, MPIR_Comm * comm,
                                           MPIR_Errflag_t * errflag);
static inline int MPIDI_SHM_mpi_scatter(const void *sendbuf, int sendcount,
                                        MPI_Datatype sendtype, void *recvbuf,
                                        int recvcount, MPI_Datatype recvtype, int root,
                                        MPIR_Comm * comm, MPIR_Errflag_t * errflag);
static inline int MPIDI_SHM_mpi_scatterv(const void *sendbuf, const int *sendcounts,
                                         const int *displs, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcount,
                                         MPI_Datatype recvtype, int root,
                                         MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
static inline int MPIDI_SHM_mpi_gather(const void *sendbuf, int sendcount,
                                       MPI_Datatype sendtype, void *recvbuf,
                                       int recvcount, MPI_Datatype recvtype, int root,
                                       MPIR_Comm * comm, MPIR_Errflag_t * errflag);
static inline int MPIDI_SHM_mpi_gatherv(const void *sendbuf, int sendcount,
                                        MPI_Datatype sendtype, void *recvbuf,
                                        const int *recvcounts, const int *displs,
                                        MPI_Datatype recvtype, int root,
                                        MPIR_Comm * comm, MPIR_Errflag_t * errflag);
static inline int MPIDI_SHM_mpi_alltoall(const void *sendbuf, int sendcount,
                                         MPI_Datatype sendtype, void *recvbuf,
                                         int recvcount, MPI_Datatype recvtype,
                                         MPIR_Comm * comm, MPIR_Errflag_t * errflag);
static inline int MPIDI_SHM_mpi_alltoallv(const void *sendbuf, const int *sendcounts,
                                          const int *sdispls, MPI_Datatype sendtype,
                                          void *recvbuf, const int *recvcounts,
                                          const int *rdispls, MPI_Datatype recvtype,
                                          MPIR_Comm * comm, MPIR_Errflag_t * errflag);
static inline int MPIDI_SHM_mpi_alltoallw(const void *sendbuf, const int *sendcounts,
                                          const int *sdispls,
                                          const MPI_Datatype sendtypes[], void *recvbuf,
                                          const int *recvcounts, const int *rdispls,
                                          const MPI_Datatype recvtypes[],
                                          MPIR_Comm * comm, MPIR_Errflag_t * errflag);
static inline int MPIDI_SHM_mpi_reduce(const void *sendbuf, void *recvbuf, int count,
                                       MPI_Datatype datatype, MPI_Op op, int root,
                                       MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                       void *algo_parameters_ptr);
static inline int MPIDI_SHM_mpi_reduce_scatter(const void *sendbuf, void *recvbuf,
                                               const int *recvcounts,
                                               MPI_Datatype datatype, MPI_Op op,
                                               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
static inline int MPIDI_SHM_mpi_reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                     int recvcount,
                                                     MPI_Datatype datatype, MPI_Op op,
                                                     MPIR_Comm * comm_ptr,
                                                     MPIR_Errflag_t * errflag);
static inline int MPIDI_SHM_mpi_scan(const void *sendbuf, void *recvbuf, int count,
                                     MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                     MPIR_Errflag_t * errflag);
static inline int MPIDI_SHM_mpi_exscan(const void *sendbuf, void *recvbuf, int count,
                                       MPI_Datatype datatype, MPI_Op op,
                                       MPIR_Comm * comm, MPIR_Errflag_t * errflag);
static inline int MPIDI_SHM_mpi_neighbor_allgather(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   int recvcount, MPI_Datatype recvtype,
                                                   MPIR_Comm * comm);
static inline int MPIDI_SHM_mpi_neighbor_allgatherv(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    const int *recvcounts,
                                                    const int *displs,
                                                    MPI_Datatype recvtype, MPIR_Comm * comm);
static inline int MPIDI_SHM_mpi_neighbor_alltoallv(const void *sendbuf,
                                                   const int *sendcounts,
                                                   const int *sdispls,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   const int *recvcounts,
                                                   const int *rdispls,
                                                   MPI_Datatype recvtype, MPIR_Comm * comm);
static inline int MPIDI_SHM_mpi_neighbor_alltoallw(const void *sendbuf,
                                                   const int *sendcounts,
                                                   const MPI_Aint * sdispls,
                                                   const MPI_Datatype * sendtypes,
                                                   void *recvbuf, const int *recvcounts,
                                                   const MPI_Aint * rdispls,
                                                   const MPI_Datatype * recvtypes,
                                                   MPIR_Comm * comm);
static inline int MPIDI_SHM_mpi_neighbor_alltoall(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  int recvcount, MPI_Datatype recvtype,
                                                  MPIR_Comm * comm);
static inline int MPIDI_SHM_mpi_ineighbor_allgather(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    int recvcount, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm, MPIR_Request ** req);
static inline int MPIDI_SHM_mpi_ineighbor_allgatherv(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype,
                                                     void *recvbuf,
                                                     const int *recvcounts,
                                                     const int *displs,
                                                     MPI_Datatype recvtype,
                                                     MPIR_Comm * comm, MPIR_Request ** req);
static inline int MPIDI_SHM_mpi_ineighbor_alltoall(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   int recvcount, MPI_Datatype recvtype,
                                                   MPIR_Comm * comm, MPIR_Request ** req);
static inline int MPIDI_SHM_mpi_ineighbor_alltoallv(const void *sendbuf,
                                                    const int *sendcounts,
                                                    const int *sdispls,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    const int *recvcounts,
                                                    const int *rdispls,
                                                    MPI_Datatype recvtype,
                                                    MPIR_Comm * comm, MPIR_Request ** req);
static inline int MPIDI_SHM_mpi_ineighbor_alltoallw(const void *sendbuf,
                                                    const int *sendcounts,
                                                    const MPI_Aint * sdispls,
                                                    const MPI_Datatype * sendtypes,
                                                    void *recvbuf, const int *recvcounts,
                                                    const MPI_Aint * rdispls,
                                                    const MPI_Datatype * recvtypes,
                                                    MPIR_Comm * comm, MPIR_Request ** req);
static inline int MPIDI_SHM_mpi_ibarrier(MPIR_Comm * comm, MPIR_Request ** req);
static inline int MPIDI_SHM_mpi_ibcast(void *buffer, int count, MPI_Datatype datatype,
                                       int root, MPIR_Comm * comm, MPIR_Request ** req);
static inline int MPIDI_SHM_mpi_iallgather(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           int recvcount, MPI_Datatype recvtype,
                                           MPIR_Comm * comm, MPIR_Request ** req);
static inline int MPIDI_SHM_mpi_iallgatherv(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf,
                                            const int *recvcounts, const int *displs,
                                            MPI_Datatype recvtype, MPIR_Comm * comm,
                                            MPIR_Request ** req);
static inline int MPIDI_SHM_mpi_iallreduce(const void *sendbuf, void *recvbuf, int count,
                                           MPI_Datatype datatype, MPI_Op op,
                                           MPIR_Comm * comm, MPIR_Request ** req);
static inline int MPIDI_SHM_mpi_ialltoall(const void *sendbuf, int sendcount,
                                          MPI_Datatype sendtype, void *recvbuf,
                                          int recvcount, MPI_Datatype recvtype,
                                          MPIR_Comm * comm, MPIR_Request ** req);
static inline int MPIDI_SHM_mpi_ialltoallv(const void *sendbuf, const int *sendcounts,
                                           const int *sdispls, MPI_Datatype sendtype,
                                           void *recvbuf, const int *recvcounts,
                                           const int *rdispls, MPI_Datatype recvtype,
                                           MPIR_Comm * comm, MPIR_Request ** req);
static inline int MPIDI_SHM_mpi_ialltoallw(const void *sendbuf, const int *sendcounts,
                                           const int *sdispls,
                                           const MPI_Datatype sendtypes[], void *recvbuf,
                                           const int *recvcounts, const int *rdispls,
                                           const MPI_Datatype recvtypes[],
                                           MPIR_Comm * comm, MPIR_Request ** req);
static inline int MPIDI_SHM_mpi_iexscan(const void *sendbuf, void *recvbuf, int count,
                                        MPI_Datatype datatype, MPI_Op op,
                                        MPIR_Comm * comm, MPIR_Request ** req);
static inline int MPIDI_SHM_mpi_igather(const void *sendbuf, int sendcount,
                                        MPI_Datatype sendtype, void *recvbuf,
                                        int recvcount, MPI_Datatype recvtype, int root,
                                        MPIR_Comm * comm, MPIR_Request ** req);
static inline int MPIDI_SHM_mpi_igatherv(const void *sendbuf, int sendcount,
                                         MPI_Datatype sendtype, void *recvbuf,
                                         const int *recvcounts, const int *displs,
                                         MPI_Datatype recvtype, int root,
                                         MPIR_Comm * comm, MPIR_Request ** req);
static inline int MPIDI_SHM_mpi_ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                      int recvcount,
                                                      MPI_Datatype datatype, MPI_Op op,
                                                      MPIR_Comm * comm, MPIR_Request ** req);
static inline int MPIDI_SHM_mpi_ireduce_scatter(const void *sendbuf, void *recvbuf,
                                                const int *recvcounts,
                                                MPI_Datatype datatype, MPI_Op op,
                                                MPIR_Comm * comm, MPIR_Request ** req);
static inline int MPIDI_SHM_mpi_ireduce(const void *sendbuf, void *recvbuf, int count,
                                        MPI_Datatype datatype, MPI_Op op, int root,
                                        MPIR_Comm * comm_ptr, MPIR_Request ** req);
static inline int MPIDI_SHM_mpi_iscan(const void *sendbuf, void *recvbuf, int count,
                                      MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                      MPIR_Request ** req);
static inline int MPIDI_SHM_mpi_iscatter(const void *sendbuf, int sendcount,
                                         MPI_Datatype sendtype, void *recvbuf,
                                         int recvcount, MPI_Datatype recvtype, int root,
                                         MPIR_Comm * comm, MPIR_Request ** req);
static inline int MPIDI_SHM_mpi_iscatterv(const void *sendbuf, const int *sendcounts,
                                          const int *displs, MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount,
                                          MPI_Datatype recvtype, int root,
                                          MPIR_Comm * comm_ptr, MPIR_Request ** req);

#endif /* SHM_H_INCLUDED */
