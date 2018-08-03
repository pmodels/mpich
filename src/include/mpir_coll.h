/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_COLL_H_INCLUDED
#define MPIR_COLL_H_INCLUDED

#include "coll_impl.h"

/* Internal point-to-point communication for collectives */
/* These functions are used in the implementation of collective and
   other internal operations. They are wrappers around MPID send/recv
   functions. They do sends/receives by setting the context offset to
   MPIR_CONTEXT_INTRA(INTER)_COLL. */
int MPIC_Wait(MPIR_Request * request_ptr, MPIR_Errflag_t * errflag);
int MPIC_Probe(int source, int tag, MPI_Comm comm, MPI_Status * status);

int MPIC_Send(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_Recv(void *buf, MPI_Aint count, MPI_Datatype datatype, int source, int tag,
              MPIR_Comm * comm_ptr, MPI_Status * status, MPIR_Errflag_t * errflag);
int MPIC_Ssend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_Sendrecv(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                  int dest, int sendtag, void *recvbuf, MPI_Aint recvcount,
                  MPI_Datatype recvtype, int source, int recvtag,
                  MPIR_Comm * comm_ptr, MPI_Status * status, MPIR_Errflag_t * errflag);
int MPIC_Sendrecv_replace(void *buf, MPI_Aint count, MPI_Datatype datatype,
                          int dest, int sendtag,
                          int source, int recvtag,
                          MPIR_Comm * comm_ptr, MPI_Status * status, MPIR_Errflag_t * errflag);
int MPIC_Isend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
               MPIR_Comm * comm_ptr, MPIR_Request ** request, MPIR_Errflag_t * errflag);
int MPIC_Issend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
                MPIR_Comm * comm_ptr, MPIR_Request ** request, MPIR_Errflag_t * errflag);
int MPIC_Irecv(void *buf, MPI_Aint count, MPI_Datatype datatype, int source,
               int tag, MPIR_Comm * comm_ptr, MPIR_Request ** request);
int MPIC_Waitall(int numreq, MPIR_Request * requests[], MPI_Status statuses[],
                 MPIR_Errflag_t * errflag);


/******************************** Allgather ********************************/
int MPIR_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                   int recvcount, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                   MPIR_Errflag_t * errflag);
int MPIR_Allgather_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                        int recvcount, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                        MPIR_Errflag_t * errflag);

/* intracomm-only functions */
int MPIR_Allgather_intra_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                              void *recvbuf, int recvcount, MPI_Datatype recvtype,
                              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Allgather_intra_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Allgather_intra_recursive_doubling(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                            MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                            MPIR_Errflag_t * errflag);
int MPIR_Allgather_intra_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                              void *recvbuf, int recvcount, MPI_Datatype recvtype,
                              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Allgather_inter_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                              void *recvbuf, int recvcount, MPI_Datatype recvtype,
                              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Allgather_inter_local_gather_remote_bcast(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   int recvcount, MPI_Datatype recvtype,
                                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Allgather_allcomm_nb(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                              void *recvbuf, int recvcount, MPI_Datatype recvtype,
                              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);


/******************************** Allgatherv ********************************/
int MPIR_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                    const int *recvcounts, const int *displs, MPI_Datatype recvtype,
                    MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Allgatherv_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                         const int *recvcounts, const int *displs, MPI_Datatype recvtype,
                         MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intracomm-only functions */
int MPIR_Allgatherv_intra_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                               void *recvbuf, const int *recvcounts, const int *displs,
                               MPI_Datatype recvtype, MPIR_Comm * comm_pt,
                               MPIR_Errflag_t * errflag);
int MPIR_Allgatherv_intra_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                 void *recvbuf, const int *recvcounts, const int *displs,
                                 MPI_Datatype recvtype, MPIR_Comm * comm_pt,
                                 MPIR_Errflag_t * errflag);
int MPIR_Allgatherv_intra_recursive_doubling(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf,
                                             const int *recvcounts, const int *displs,
                                             MPI_Datatype recvtype, MPIR_Comm * comm_pt,
                                             MPIR_Errflag_t * errflag);
int MPIR_Allgatherv_intra_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                               void *recvbuf, const int *recvcounts, const int *displs,
                               MPI_Datatype recvtype, MPIR_Comm * comm_pt,
                               MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Allgatherv_inter_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                               void *recvbuf, const int *recvcounts, const int *displs,
                               MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                               MPIR_Errflag_t * errflag);
int MPIR_Allgatherv_inter_remote_gather_local_bcast(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    const int *recvcounts, const int *displs,
                                                    MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                                    MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Allgatherv_allcomm_nb(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                               void *recvbuf, const int *recvcounts, const int *displs,
                               MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                               MPIR_Errflag_t * errflag);


/******************************** Allreduce ********************************/
int MPIR_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Allreduce_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                        MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intracomm-only functions */
int MPIR_Allreduce_intra_auto(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                              MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Allreduce_intra_recursive_doubling(const void *sendbuf, void *recvbuf, int count,
                                            MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                            MPIR_Errflag_t * errflag);
int MPIR_Allreduce_intra_reduce_scatter_allgather(const void *sendbuf, void *recvbuf, int count,
                                                  MPI_Datatype datatype, MPI_Op op,
                                                  MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Allreduce_intra_smp(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                             MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Allreduce_inter_auto(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                              MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Allreduce_inter_reduce_exchange_bcast(const void *sendbuf, void *recvbuf, int count,
                                               MPI_Datatype datatype, MPI_Op op,
                                               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Allreduce_allcomm_nb(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                              MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);


/******************************** Alltoall ********************************/
int MPIR_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                  int recvcount, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                  MPIR_Errflag_t * errflag);
int MPIR_Alltoall_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                       int recvcount, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                       MPIR_Errflag_t * errflag);

/* intracomm-only functions */
int MPIR_Alltoall_intra_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                             void *recvbuf, int recvcount, MPI_Datatype recvtype,
                             MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Alltoall_intra_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                               void *recvbuf, int recvcount, MPI_Datatype recvtype,
                               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Alltoall_intra_pairwise(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                 void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Alltoall_intra_pairwise_sendrecv_replace(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  int recvcount, MPI_Datatype recvtype,
                                                  MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Alltoall_intra_scattered(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                  MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Alltoall_inter_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                             void *recvbuf, int recvcount, MPI_Datatype recvtype,
                             MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Alltoall_inter_pairwise_exchange(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                          MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Alltoall_allcomm_nb(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                             void *recvbuf, int recvcount, MPI_Datatype recvtype,
                             MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);


/******************************** Alltoallv ********************************/
int MPIR_Alltoallv(const void *sendbuf, const int *sendcnts, const int *sdispls,
                   MPI_Datatype sendtype, void *recvbuf, const int *recvcnts, const int *rdispls,
                   MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Alltoallv_impl(const void *sendbuf, const int *sendcnts, const int *sdispls,
                        MPI_Datatype sendtype, void *recvbuf, const int *recvcnts,
                        const int *rdispls, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                        MPIR_Errflag_t * errflag);

/* intracomm-only functions */
int MPIR_Alltoallv_intra_auto(const void *sendbuf, const int *sendcnts, const int *sdispls,
                              MPI_Datatype sendtype, void *recvbuf, const int *recvcnts,
                              const int *rdispls, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                              MPIR_Errflag_t * errflag);
int MPIR_Alltoallv_intra_pairwise_sendrecv_replace(const void *sendbuf, const int *sendcnts,
                                                   const int *sdispls, MPI_Datatype sendtype,
                                                   void *recvbuf, const int *recvcnts,
                                                   const int *rdispls, MPI_Datatype recvtype,
                                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Alltoallv_intra_scattered(const void *sendbuf, const int *sendcnts, const int *sdispls,
                                   MPI_Datatype sendtype, void *recvbuf, const int *recvcnts,
                                   const int *rdispls, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                   MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Alltoallv_inter_auto(const void *sendbuf, const int *sendcnts, const int *sdispls,
                              MPI_Datatype sendtype, void *recvbuf, const int *recvcnts,
                              const int *rdispls, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                              MPIR_Errflag_t * errflag);
int MPIR_Alltoallv_inter_pairwise_exchange(const void *sendbuf, const int *sendcnts,
                                           const int *sdispls, MPI_Datatype sendtype, void *recvbuf,
                                           const int *recvcnts, const int *rdispls,
                                           MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                           MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Alltoallv_allcomm_nb(const void *sendbuf, const int *sendcnts, const int *sdispls,
                              MPI_Datatype sendtype, void *recvbuf, const int *recvcnts,
                              const int *rdispls, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                              MPIR_Errflag_t * errflag);


/******************************** Alltoallw ********************************/
int MPIR_Alltoallw(const void *sendbuf, const int *sendcnts, const int *sdispls,
                   const MPI_Datatype * sendtypes, void *recvbuf, const int *recvcnts,
                   const int *rdispls, const MPI_Datatype * recvtypes, MPIR_Comm * comm_ptr,
                   MPIR_Errflag_t * errflag);
int MPIR_Alltoallw_impl(const void *sendbuf, const int *sendcnts, const int *sdispls,
                        const MPI_Datatype * sendtypes, void *recvbuf, const int *recvcnts,
                        const int *rdispls, const MPI_Datatype * recvtypes, MPIR_Comm * comm_ptr,
                        MPIR_Errflag_t * errflag);

/* intracomm-only functions */
int MPIR_Alltoallw_intra_auto(const void *sendbuf, const int *sendcnts, const int *sdispls,
                              const MPI_Datatype * sendtypes, void *recvbuf, const int *recvcnts,
                              const int *rdispls, const MPI_Datatype * recvtypes,
                              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Alltoallw_intra_pairwise_sendrecv_replace(const void *sendbuf, const int *sendcnts,
                                                   const int *sdispls,
                                                   const MPI_Datatype * sendtypes, void *recvbuf,
                                                   const int *recvcnts, const int *rdispls,
                                                   const MPI_Datatype * recvtypes,
                                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Alltoallw_intra_scattered(const void *sendbuf, const int *sendcnts, const int *sdispls,
                                   const MPI_Datatype * sendtypes, void *recvbuf,
                                   const int *recvcnts, const int *rdispls,
                                   const MPI_Datatype * recvtypes, MPIR_Comm * comm_ptr,
                                   MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Alltoallw_inter_auto(const void *sendbuf, const int *sendcnts, const int *sdispls,
                              const MPI_Datatype * sendtypes, void *recvbuf, const int *recvcnts,
                              const int *rdispls, const MPI_Datatype * recvtypes,
                              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Alltoallw_inter_pairwise_exchange(const void *sendbuf, const int *sendcnts,
                                           const int *sdispls, const MPI_Datatype * sendtypes,
                                           void *recvbuf, const int *recvcnts, const int *rdispls,
                                           const MPI_Datatype * recvtypes, MPIR_Comm * comm_ptr,
                                           MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Alltoallw_allcomm_nb(const void *sendbuf, const int *sendcnts, const int *sdispls,
                              const MPI_Datatype * sendtypes, void *recvbuf, const int *recvcnts,
                              const int *rdispls, const MPI_Datatype * recvtypes,
                              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);


/******************************** Barrier ********************************/
int MPIR_Barrier(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Barrier_impl(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intracomm-only functions */
int MPIR_Barrier_intra_auto(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Barrier_intra_dissemination(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Barrier_intra_smp(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Barrier_inter_auto(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Barrier_inter_bcast(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Barrier_allcomm_nb(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);


/******************************** Bcast ********************************/
int MPIR_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm * comm_ptr,
               MPIR_Errflag_t * errflag);
int MPIR_Bcast_impl(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm * comm_ptr,
                    MPIR_Errflag_t * errflag);

/* intracomm-only functions */
int MPIR_Bcast_intra_auto(void *buffer, int count, MPI_Datatype datatype, int root,
                          MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Bcast_intra_binomial(void *buffer, int count, MPI_Datatype datatype, int root,
                              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Bcast_intra_scatter_recursive_doubling_allgather(void *buffer, int count,
                                                          MPI_Datatype datatype, int root,
                                                          MPIR_Comm * comm_ptr,
                                                          MPIR_Errflag_t * errflag);
int MPIR_Bcast_intra_scatter_ring_allgather(void *buffer, int count, MPI_Datatype datatype,
                                            int root, MPIR_Comm * comm_ptr,
                                            MPIR_Errflag_t * errflag);
int MPIR_Bcast_intra_smp(void *buffer, int count, MPI_Datatype datatype, int root,
                         MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Bcast_inter_auto(void *buffer, int count, MPI_Datatype datatype, int root,
                          MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Bcast_inter_remote_send_local_bcast(void *buffer, int count, MPI_Datatype datatype,
                                             int root, MPIR_Comm * comm_ptr,
                                             MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Bcast_allcomm_nb(void *buffer, int count, MPI_Datatype datatype, int root,
                          MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);


/******************************** Exscan ********************************/
int MPIR_Exscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Exscan_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                     MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intracomm-only functions */
int MPIR_Exscan_intra_auto(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                           MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Exscan_intra_recursive_doubling(const void *sendbuf, void *recvbuf, int count,
                                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                         MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Exscan_allcomm_nb(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                           MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);


/******************************** Gather ********************************/
int MPIR_Gather(const void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf, int recvcnt,
                MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Gather_impl(const void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf,
                     int recvcnt, MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                     MPIR_Errflag_t * errflag);

/* intracomm-only functions */
int MPIR_Gather_intra_auto(const void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf,
                           int recvcnt, MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                           MPIR_Errflag_t * errflag);
int MPIR_Gather_intra_binomial(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                               void *recvbuf, int recvcnt, MPI_Datatype recvtype, int root,
                               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Gather_inter_auto(const void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf,
                           int recvcnt, MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                           MPIR_Errflag_t * errflag);
int MPIR_Gather_inter_linear(const void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf,
                             int recvcnt, MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                             MPIR_Errflag_t * errflag);
int MPIR_Gather_inter_local_gather_remote_send(const void *sendbuf, int sendcnt,
                                               MPI_Datatype sendtype, void *recvbuf, int recvcnt,
                                               MPI_Datatype recvtype, int root,
                                               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Gather_allcomm_nb(const void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf,
                           int recvcnt, MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                           MPIR_Errflag_t * errflag);


/******************************** Gatherv ********************************/
int MPIR_Gatherv(const void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf,
                 const int *recvcnts, const int *displs, MPI_Datatype recvtype, int root,
                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Gatherv_impl(const void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf,
                      const int *recvcnts, const int *displs, MPI_Datatype recvtype, int root,
                      MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intracomm-only functions */
int MPIR_Gatherv_intra_auto(const void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf,
                            const int *recvcnts, const int *displs, MPI_Datatype recvtype, int root,
                            MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Gatherv_inter_auto(const void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf,
                            const int *recvcnts, const int *displs, MPI_Datatype recvtype, int root,
                            MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Gatherv_allcomm_linear(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                                void *recvbuf, const int *recvcnts, const int *displs,
                                MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                MPIR_Errflag_t * errflag);
int MPIR_Gatherv_allcomm_nb(const void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf,
                            const int *recvcnts, const int *displs, MPI_Datatype recvtype, int root,
                            MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);


/******************************** Iallgather ********************************/
/* request-based functions */
int MPIR_Iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                    int recvcount, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                    MPIR_Request ** request);
int MPIR_Iallgather_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                         int recvcount, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                         MPIR_Request ** request);
int MPIR_Iallgather_intra_gentran_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                         MPIR_Comm * comm_ptr, MPIR_Request ** request);

/* sched-based functions */
int MPIR_Iallgather_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                          int recvcount, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                          MPIR_Sched_t s);
int MPIR_Iallgather_sched_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                               void *recvbuf, int recvcount, MPI_Datatype recvtype,
                               MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Iallgather_sched_intra_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                     void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                     MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_sched_intra_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                       MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_sched_intra_recursive_doubling(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   int recvcount, MPI_Datatype recvtype,
                                                   MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_sched_intra_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                     void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                     MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_intra_recexch_distance_doubling(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    int recvcount, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm, MPIR_Request ** req);
int MPIR_Iallgather_intra_recexch_distance_halving(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   int recvcount, MPI_Datatype recvtype,
                                                   MPIR_Comm * comm, MPIR_Request ** req);

/* sched-based intercomm-only functions */
int MPIR_Iallgather_sched_inter_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                     void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                     MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_sched_inter_local_gather_remote_bcast(const void *sendbuf, int sendcount,
                                                          MPI_Datatype sendtype, void *recvbuf,
                                                          int recvcount, MPI_Datatype recvtype,
                                                          MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Iallgatherv ********************************/
/* request-based functions */
int MPIR_Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                     const int *recvcounts, const int *displs, MPI_Datatype recvtype,
                     MPIR_Comm * comm_ptr, MPIR_Request ** request);
int MPIR_Iallgatherv_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                          const int *recvcounts, const int *displs, MPI_Datatype recvtype,
                          MPIR_Comm * comm_ptr, MPIR_Request ** request);

/* sched-based functions */
int MPIR_Iallgatherv_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                           const int *recvcounts, const int *displs, MPI_Datatype recvtype,
                           MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_sched_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                void *recvbuf, const int *recvcounts, const int *displs,
                                MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Iallgatherv_sched_intra_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, const int *recvcounts, const int *displs,
                                      MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_sched_intra_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, const int recvcounts[], const int displs[],
                                        MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                        MPIR_Sched_t s);
int MPIR_Iallgatherv_sched_intra_recursive_doubling(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    const int recvcounts[], const int displs[],
                                                    MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                                    MPIR_Sched_t s);
int MPIR_Iallgatherv_sched_intra_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, const int recvcounts[], const int displs[],
                                      MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_intra_recexch_distance_doubling(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     const int *recvcounts, const int *displs,
                                                     MPI_Datatype recvtype, MPIR_Comm * comm,
                                                     MPIR_Request ** req);
int MPIR_Iallgatherv_intra_recexch_distance_halving(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    const int *recvcounts, const int *displs,
                                                    MPI_Datatype recvtype, MPIR_Comm * comm,
                                                    MPIR_Request ** req);
int MPIR_Iallgatherv_intra_gentran_ring(const void *sendbuf, int sendcount,
                                        MPI_Datatype sendtype, void *recvbuf,
                                        const int *recvcounts, const int *displs,
                                        MPI_Datatype recvtype, MPIR_Comm * comm,
                                        MPIR_Request ** req);

/* sched-based intercomm-only functions */
int MPIR_Iallgatherv_sched_inter_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, const int *recvcounts, const int *displs,
                                      MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_sched_inter_remote_gather_local_bcast(const void *sendbuf, int sendcount,
                                                           MPI_Datatype sendtype, void *recvbuf,
                                                           const int *recvcounts, const int *displs,
                                                           MPI_Datatype recvtype,
                                                           MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Iallreduce ********************************/
/* request-based functions */
int MPIR_Iallreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                    MPIR_Comm * comm_ptr, MPIR_Request ** request);
int MPIR_Iallreduce_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                         MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Request ** request);

/* sched-based functions */
int MPIR_Iallreduce_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                          MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_sched_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                               MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Iallreduce_sched_intra_auto(const void *sendbuf, void *recvbuf, int count,
                                     MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                     MPIR_Sched_t s);
int MPIR_Iallreduce_sched_intra_naive(const void *sendbuf, void *recvbuf, int count,
                                      MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                      MPIR_Sched_t s);
int MPIR_Iallreduce_sched_intra_recursive_doubling(const void *sendbuf, void *recvbuf, int count,
                                                   MPI_Datatype datatype, MPI_Op op,
                                                   MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_sched_intra_reduce_scatter_allgather(const void *sendbuf, void *recvbuf,
                                                         int count, MPI_Datatype datatype,
                                                         MPI_Op op, MPIR_Comm * comm_ptr,
                                                         MPIR_Sched_t s);
int MPIR_Iallreduce_intra_recexch_single_buffer(const void *sendbuf, void *recvbuf, int count,
                                                MPI_Datatype datatype, MPI_Op op,
                                                MPIR_Comm * comm, MPIR_Request ** req);
int MPIR_Iallreduce_intra_recexch_multiple_buffer(const void *sendbuf, void *recvbuf, int count,
                                                  MPI_Datatype datatype, MPI_Op op,
                                                  MPIR_Comm * comm, MPIR_Request ** req);
int MPIR_Iallreduce_intra_tree_kary(const void *sendbuf, void *recvbuf, int count,
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                    MPIR_Request ** request);
int MPIR_Iallreduce_intra_tree_knomial(const void *sendbuf, void *recvbuf, int count,
                                       MPI_Datatype datatype, MPI_Op op,
                                       MPIR_Comm * comm_ptr, MPIR_Request ** request);
int MPIR_Iallreduce_sched_intra_smp(const void *sendbuf, void *recvbuf, int count,
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                    MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Iallreduce_sched_inter_auto(const void *sendbuf, void *recvbuf, int count,
                                     MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                     MPIR_Sched_t s);
int MPIR_Iallreduce_sched_inter_remote_reduce_local_bcast(const void *sendbuf, void *recvbuf,
                                                          int count, MPI_Datatype datatype,
                                                          MPI_Op op, MPIR_Comm * comm_ptr,
                                                          MPIR_Sched_t s);


/******************************** Ialltoall ********************************/
/* request-based functions */
int MPIR_Ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                   int recvcount, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                   MPIR_Request ** request);
int MPIR_Ialltoall_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                        int recvcount, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                        MPIR_Request ** request);

/* sched-based functions */
int MPIR_Ialltoall_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                         int recvcount, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                         MPIR_Sched_t s);
int MPIR_Ialltoall_sched_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                              void *recvbuf, int recvcount, MPI_Datatype recvtype,
                              MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ialltoall_sched_intra_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                    void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                    MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_sched_intra_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                      MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_sched_intra_inplace(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                       MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_sched_intra_pairwise(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                        MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_sched_intra_permuted_sendrecv(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 int recvcount, MPI_Datatype recvtype,
                                                 MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ialltoall_sched_inter_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                    void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                    MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_sched_inter_pairwise_exchange(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 int recvcount, MPI_Datatype recvtype,
                                                 MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Ialltoallv ********************************/
/* request-based functions */
int MPIR_Ialltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls,
                    MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls,
                    MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Request ** request);
int MPIR_Ialltoallv_impl(const void *sendbuf, const int *sendcounts, const int *sdispls,
                         MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                         const int *rdispls, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                         MPIR_Request ** request);

/* sched-based functions */
int MPIR_Ialltoallv_sched(const void *sendbuf, const int *sendcounts, const int *sdispls,
                          MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                          const int *rdispls, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                          MPIR_Sched_t s);
int MPIR_Ialltoallv_sched_impl(const void *sendbuf, const int *sendcounts, const int *sdispls,
                               MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                               const int *rdispls, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                               MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ialltoallv_sched_intra_auto(const void *sendbuf, const int *sendcounts, const int *sdispls,
                                     MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                                     const int *rdispls, MPI_Datatype recvtype,
                                     MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallv_sched_intra_blocked(const void *sendbuf, const int sendcounts[],
                                        const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                                        const int recvcounts[], const int rdispls[],
                                        MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                        MPIR_Sched_t s);
int MPIR_Ialltoallv_sched_intra_inplace(const void *sendbuf, const int sendcounts[],
                                        const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                                        const int recvcounts[], const int rdispls[],
                                        MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                        MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ialltoallv_sched_inter_auto(const void *sendbuf, const int *sendcounts, const int *sdispls,
                                     MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                                     const int *rdispls, MPI_Datatype recvtype,
                                     MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallv_sched_inter_pairwise_exchange(const void *sendbuf, const int sendcounts[],
                                                  const int sdispls[], MPI_Datatype sendtype,
                                                  void *recvbuf, const int recvcounts[],
                                                  const int rdispls[], MPI_Datatype recvtype,
                                                  MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Ialltoallw ********************************/
/* request-based functions */
int MPIR_Ialltoallw(const void *sendbuf, const int *sendcounts, const int *sdispls,
                    const MPI_Datatype * sendtypes, void *recvbuf, const int *recvcounts,
                    const int *rdispls, const MPI_Datatype * recvtypes, MPIR_Comm * comm_ptr,
                    MPIR_Request ** request);
int MPIR_Ialltoallw_impl(const void *sendbuf, const int *sendcounts, const int *sdispls,
                         const MPI_Datatype * sendtypes, void *recvbuf, const int *recvcounts,
                         const int *rdispls, const MPI_Datatype * recvtypes, MPIR_Comm * comm_ptr,
                         MPIR_Request ** request);

/* sched-based functions */
int MPIR_Ialltoallw_sched(const void *sendbuf, const int *sendcounts, const int *sdispls,
                          const MPI_Datatype * sendtypes, void *recvbuf, const int *recvcounts,
                          const int *rdispls, const MPI_Datatype * recvtypes, MPIR_Comm * comm_ptr,
                          MPIR_Sched_t s);
int MPIR_Ialltoallw_sched_impl(const void *sendbuf, const int *sendcounts, const int *sdispls,
                               const MPI_Datatype * sendtypes, void *recvbuf, const int *recvcounts,
                               const int *rdispls, const MPI_Datatype * recvtypes,
                               MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ialltoallw_sched_intra_auto(const void *sendbuf, const int *sendcounts, const int *sdispls,
                                     const MPI_Datatype * sendtypes, void *recvbuf,
                                     const int *recvcounts, const int *rdispls,
                                     const MPI_Datatype * recvtypes, MPIR_Comm * comm_ptr,
                                     MPIR_Sched_t s);
int MPIR_Ialltoallw_sched_intra_blocked(const void *sendbuf, const int sendcounts[],
                                        const int sdispls[], const MPI_Datatype sendtypes[],
                                        void *recvbuf, const int recvcounts[], const int rdispls[],
                                        const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr,
                                        MPIR_Sched_t s);
int MPIR_Ialltoallw_sched_intra_inplace(const void *sendbuf, const int sendcounts[],
                                        const int sdispls[], const MPI_Datatype sendtypes[],
                                        void *recvbuf, const int recvcounts[], const int rdispls[],
                                        const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr,
                                        MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ialltoallw_sched_inter_auto(const void *sendbuf, const int *sendcounts, const int *sdispls,
                                     const MPI_Datatype * sendtypes, void *recvbuf,
                                     const int *recvcounts, const int *rdispls,
                                     const MPI_Datatype * recvtypes, MPIR_Comm * comm_ptr,
                                     MPIR_Sched_t s);
int MPIR_Ialltoallw_sched_inter_pairwise_exchange(const void *sendbuf, const int sendcounts[],
                                                  const int sdispls[],
                                                  const MPI_Datatype sendtypes[], void *recvbuf,
                                                  const int recvcounts[], const int rdispls[],
                                                  const MPI_Datatype recvtypes[],
                                                  MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Ibarrier ********************************/
/* request-based functions */
int MPIR_Ibarrier(MPIR_Comm * comm_ptr, MPIR_Request ** request);
int MPIR_Ibarrier_impl(MPIR_Comm * comm_ptr, MPIR_Request ** request);

/* sched-based functions */
int MPIR_Ibarrier_sched(MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ibarrier_sched_impl(MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ibarrier_sched_intra_auto(MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ibarrier_sched_intra_recursive_doubling(MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ibarrier_intra_recexch(MPIR_Comm * comm, MPIR_Request ** req);

/* sched-based intercomm-only functions */
int MPIR_Ibarrier_sched_inter_auto(MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ibarrier_sched_inter_bcast(MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Ibcast ********************************/
/* request-based functions */
int MPIR_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm * comm_ptr,
                MPIR_Request ** request);
int MPIR_Ibcast_impl(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm * comm_ptr,
                     MPIR_Request ** request);
int MPIR_Ibcast_intra_tree(void *buffer, int count, MPI_Datatype datatype, int root,
                           MPIR_Comm * comm_ptr, MPIR_Request ** request);
int MPIR_Ibcast_intra_scatter_recexch_allgather(void *buffer, int count, MPI_Datatype datatype,
                                                int root, MPIR_Comm * comm_ptr,
                                                MPIR_Request ** request);
int MPIR_Ibcast_intra_ring(void *buffer, int count, MPI_Datatype datatype, int root,
                           MPIR_Comm * comm_ptr, MPIR_Request ** request);

/* sched-based functions */
int MPIR_Ibcast_sched(void *buffer, int count, MPI_Datatype datatype, int root,
                      MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_sched_impl(void *buffer, int count, MPI_Datatype datatype, int root,
                           MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ibcast_sched_intra_auto(void *buffer, int count, MPI_Datatype datatype, int root,
                                 MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_sched_intra_binomial(void *buffer, int count, MPI_Datatype datatype, int root,
                                     MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_sched_intra_scatter_recursive_doubling_allgather(void *buffer, int count,
                                                                 MPI_Datatype datatype, int root,
                                                                 MPIR_Comm * comm_ptr,
                                                                 MPIR_Sched_t s);
int MPIR_Ibcast_sched_intra_scatter_ring_allgather(void *buffer, int count, MPI_Datatype datatype,
                                                   int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_sched_intra_smp(void *buffer, int count, MPI_Datatype datatype, int root,
                                MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ibcast_sched_inter_auto(void *buffer, int count, MPI_Datatype datatype, int root,
                                 MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_sched_inter_flat(void *buffer, int count, MPI_Datatype datatype, int root,
                                 MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Iexscan ********************************/
/* request-based functions */
int MPIR_Iexscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                 MPIR_Comm * comm_ptr, MPIR_Request ** request);
int MPIR_Iexscan_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                      MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Request ** request);

/* sched-based functions */
int MPIR_Iexscan_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                       MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iexscan_sched_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                            MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Iexscan_sched_intra_auto(const void *sendbuf, void *recvbuf, int count,
                                  MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                  MPIR_Sched_t s);
int MPIR_Iexscan_sched_intra_recursive_doubling(const void *sendbuf, void *recvbuf, int count,
                                                MPI_Datatype datatype, MPI_Op op,
                                                MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Igather ********************************/
/* request-based functions */
int MPIR_Igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                 int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                 MPIR_Request ** request);
int MPIR_Igather_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                      int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                      MPIR_Request ** request);
int MPIR_Igather_intra_tree(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                            void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                            MPIR_Comm * comm_ptr, MPIR_Request ** request);

/* sched-based functions */
int MPIR_Igather_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                       int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                       MPIR_Sched_t s);
int MPIR_Igather_sched_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                            void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                            MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Igather_sched_intra_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                                  MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Igather_sched_intra_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                                      MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Igather_sched_inter_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                                  MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Igather_sched_inter_long(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                                  MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Igather_sched_inter_short(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                                   MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Igatherv ********************************/
/* request-based functions */
int MPIR_Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                  const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root,
                  MPIR_Comm * comm_ptr, MPIR_Request ** request);
int MPIR_Igatherv_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                       const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root,
                       MPIR_Comm * comm_ptr, MPIR_Request ** request);

/* sched-based functions */
int MPIR_Igatherv_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                        const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root,
                        MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Igatherv_sched_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                             void *recvbuf, const int *recvcounts, const int *displs,
                             MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Igatherv_sched_intra_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, const int *recvcounts, const int *displs,
                                   MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                   MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Igatherv_sched_inter_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, const int *recvcounts, const int *displs,
                                   MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                   MPIR_Sched_t s);

/* sched-based anycomm functions */
int MPIR_Igatherv_sched_allcomm_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, const int *recvcounts, const int *displs,
                                       MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                       MPIR_Sched_t s);


/******************************** Ineighbor_allgather ********************************/
/* request-based functions */
int MPIR_Ineighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                             void *recvbuf, int recvcount, MPI_Datatype recvtype,
                             MPIR_Comm * comm_ptr, MPIR_Request ** request);
int MPIR_Ineighbor_allgather_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                  MPIR_Comm * comm_ptr, MPIR_Request ** request);

/* sched-based functions */
int MPIR_Ineighbor_allgather_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                   MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ineighbor_allgather_sched_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                        MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ineighbor_allgather_sched_intra_auto(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                              MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                              MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ineighbor_allgather_sched_inter_auto(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                              MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                              MPIR_Sched_t s);

/* sched-based anycomm functions */
int MPIR_Ineighbor_allgather_sched_allcomm_linear(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  int recvcount, MPI_Datatype recvtype,
                                                  MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Ineighbor_allgatherv ********************************/
/* request-based functions */
int MPIR_Ineighbor_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                              void *recvbuf, const int recvcounts[], const int displs[],
                              MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Request ** request);
int MPIR_Ineighbor_allgatherv_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, const int recvcounts[], const int displs[],
                                   MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                   MPIR_Request ** request);

/* sched-based functions */
int MPIR_Ineighbor_allgatherv_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                    void *recvbuf, const int recvcounts[], const int displs[],
                                    MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ineighbor_allgatherv_sched_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                         void *recvbuf, const int recvcounts[], const int displs[],
                                         MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                         MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ineighbor_allgatherv_sched_intra_auto(const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               const int recvcounts[], const int displs[],
                                               MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                               MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ineighbor_allgatherv_sched_inter_auto(const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               const int recvcounts[], const int displs[],
                                               MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                               MPIR_Sched_t s);

/* sched-based anycomm functions */
int MPIR_Ineighbor_allgatherv_sched_allcomm_linear(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   const int recvcounts[], const int displs[],
                                                   MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                                   MPIR_Sched_t s);


/******************************** Ineighbor_alltoall ********************************/
/* request-based functions */
int MPIR_Ineighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                            void *recvbuf, int recvcount, MPI_Datatype recvtype,
                            MPIR_Comm * comm_ptr, MPIR_Request ** request);
int MPIR_Ineighbor_alltoall_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                 void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                 MPIR_Comm * comm_ptr, MPIR_Request ** request);

/* sched-based functions */
int MPIR_Ineighbor_alltoall_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                  MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ineighbor_alltoall_sched_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                       MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ineighbor_alltoall_sched_intra_auto(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                             MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                             MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ineighbor_alltoall_sched_inter_auto(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                             MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                             MPIR_Sched_t s);

/* sched-based anycomm functions */
int MPIR_Ineighbor_alltoall_sched_allcomm_linear(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 int recvcount, MPI_Datatype recvtype,
                                                 MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Ineighbor_alltoallv ********************************/
/* request-based functions */
int MPIR_Ineighbor_alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[],
                             MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                             const int rdispls[], MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                             MPIR_Request ** request);
int MPIR_Ineighbor_alltoallv_impl(const void *sendbuf, const int sendcounts[], const int sdispls[],
                                  MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                                  const int rdispls[], MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                  MPIR_Request ** request);

/* sched-based functions */
int MPIR_Ineighbor_alltoallv_sched(const void *sendbuf, const int sendcounts[], const int sdispls[],
                                   MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                                   const int rdispls[], MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                   MPIR_Sched_t s);
int MPIR_Ineighbor_alltoallv_sched_impl(const void *sendbuf, const int sendcounts[],
                                        const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                                        const int recvcounts[], const int rdispls[],
                                        MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                        MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ineighbor_alltoallv_sched_intra_auto(const void *sendbuf, const int sendcounts[],
                                              const int sdispls[], MPI_Datatype sendtype,
                                              void *recvbuf, const int recvcounts[],
                                              const int rdispls[], MPI_Datatype recvtype,
                                              MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ineighbor_alltoallv_sched_inter_auto(const void *sendbuf, const int sendcounts[],
                                              const int sdispls[], MPI_Datatype sendtype,
                                              void *recvbuf, const int recvcounts[],
                                              const int rdispls[], MPI_Datatype recvtype,
                                              MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based anycomm functions */
int MPIR_Ineighbor_alltoallv_sched_allcomm_linear(const void *sendbuf, const int sendcounts[],
                                                  const int sdispls[], MPI_Datatype sendtype,
                                                  void *recvbuf, const int recvcounts[],
                                                  const int rdispls[], MPI_Datatype recvtype,
                                                  MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Ineighbor_alltoallw ********************************/
/* request-based functions */
int MPIR_Ineighbor_alltoallw(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[],
                             const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                             const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                             MPIR_Comm * comm_ptr, MPIR_Request ** request);
int MPIR_Ineighbor_alltoallw_impl(const void *sendbuf, const int sendcounts[],
                                  const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                  void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[],
                                  const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr,
                                  MPIR_Request ** request);

/* sched-based functions */
int MPIR_Ineighbor_alltoallw_sched(const void *sendbuf, const int sendcounts[],
                                   const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                   void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[],
                                   const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr,
                                   MPIR_Sched_t s);
int MPIR_Ineighbor_alltoallw_sched_impl(const void *sendbuf, const int sendcounts[],
                                        const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                        void *recvbuf, const int recvcounts[],
                                        const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                        MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ineighbor_alltoallw_sched_intra_auto(const void *sendbuf, const int sendcounts[],
                                              const MPI_Aint sdispls[],
                                              const MPI_Datatype sendtypes[], void *recvbuf,
                                              const int recvcounts[], const MPI_Aint rdispls[],
                                              const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr,
                                              MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ineighbor_alltoallw_sched_inter_auto(const void *sendbuf, const int sendcounts[],
                                              const MPI_Aint sdispls[],
                                              const MPI_Datatype sendtypes[], void *recvbuf,
                                              const int recvcounts[], const MPI_Aint rdispls[],
                                              const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr,
                                              MPIR_Sched_t s);

/* sched-based anycomm functions */
int MPIR_Ineighbor_alltoallw_sched_allcomm_linear(const void *sendbuf, const int sendcounts[],
                                                  const MPI_Aint sdispls[],
                                                  const MPI_Datatype sendtypes[], void *recvbuf,
                                                  const int recvcounts[], const MPI_Aint rdispls[],
                                                  const MPI_Datatype recvtypes[],
                                                  MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Ireduce ********************************/
/* request-based functions */
int MPIR_Ireduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                 int root, MPIR_Comm * comm_ptr, MPIR_Request ** request);
int MPIR_Ireduce_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                      MPI_Op op, int root, MPIR_Comm * comm_ptr, MPIR_Request ** request);
int MPIR_Ireduce_intra_tree(const void *sendbuf, void *recvbuf, int count,
                            MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                            MPIR_Request ** request);
int MPIR_Ireduce_intra_ring(const void *sendbuf, void *recvbuf, int count,
                            MPI_Datatype datatype, MPI_Op op, int root,
                            MPIR_Comm * comm_ptr, MPIR_Request ** request);

/* sched-based functions */
int MPIR_Ireduce_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                       MPI_Op op, int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_sched_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                            MPI_Op op, int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ireduce_sched_intra_auto(const void *sendbuf, void *recvbuf, int count,
                                  MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                                  MPIR_Sched_t s);
int MPIR_Ireduce_sched_intra_binomial(const void *sendbuf, void *recvbuf, int count,
                                      MPI_Datatype datatype, MPI_Op op, int root,
                                      MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_sched_intra_reduce_scatter_gather(const void *sendbuf, void *recvbuf, int count,
                                                   MPI_Datatype datatype, MPI_Op op, int root,
                                                   MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_sched_intra_smp(const void *sendbuf, void *recvbuf, int count,
                                 MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                                 MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ireduce_sched_inter_auto(const void *sendbuf, void *recvbuf, int count,
                                  MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                                  MPIR_Sched_t s);
int MPIR_Ireduce_sched_inter_local_reduce_remote_send(const void *sendbuf, void *recvbuf, int count,
                                                      MPI_Datatype datatype, MPI_Op op, int root,
                                                      MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Ireduce_scatter ********************************/
/* request-based functions */
int MPIR_Ireduce_scatter(const void *sendbuf, void *recvbuf, const int *recvcounts,
                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                         MPIR_Request ** request);
int MPIR_Ireduce_scatter_impl(const void *sendbuf, void *recvbuf, const int *recvcounts,
                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                              MPIR_Request ** request);

/* sched-based functions */
int MPIR_Ireduce_scatter_sched(const void *sendbuf, void *recvbuf, const int *recvcounts,
                               MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                               MPIR_Sched_t s);
int MPIR_Ireduce_scatter_sched_impl(const void *sendbuf, void *recvbuf, const int *recvcounts,
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                    MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ireduce_scatter_sched_intra_auto(const void *sendbuf, void *recvbuf, const int *recvcnts,
                                          MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                          MPIR_Sched_t s);
int MPIR_Ireduce_scatter_sched_intra_noncommutative(const void *sendbuf, void *recvbuf,
                                                    const int *recvcnts, MPI_Datatype datatype,
                                                    MPI_Op op, MPIR_Comm * comm_ptr,
                                                    MPIR_Sched_t s);
int MPIR_Ireduce_scatter_sched_intra_pairwise(const void *sendbuf, void *recvbuf,
                                              const int *recvcnts, MPI_Datatype datatype, MPI_Op op,
                                              MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_sched_intra_recursive_doubling(const void *sendbuf, void *recvbuf,
                                                        const int *recvcnts, MPI_Datatype datatype,
                                                        MPI_Op op, MPIR_Comm * comm_ptr,
                                                        MPIR_Sched_t s);
int MPIR_Ireduce_scatter_sched_intra_recursive_halving(const void *sendbuf, void *recvbuf,
                                                       const int *recvcnts, MPI_Datatype datatype,
                                                       MPI_Op op, MPIR_Comm * comm_ptr,
                                                       MPIR_Sched_t s);
int MPIR_Ireduce_scatter_intra_recexch(const void *sendbuf, void *recvbuf,
                                       const int *recvcounts, MPI_Datatype datatype,
                                       MPI_Op op, MPIR_Comm * comm, MPIR_Request ** req);

/* sched-based intercomm-only functions */
int MPIR_Ireduce_scatter_sched_inter_auto(const void *sendbuf, void *recvbuf, const int *recvcnts,
                                          MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                          MPIR_Sched_t s);
int MPIR_Ireduce_scatter_sched_inter_remote_reduce_local_scatterv(const void *sendbuf,
                                                                  void *recvbuf,
                                                                  const int *recvcnts,
                                                                  MPI_Datatype datatype, MPI_Op op,
                                                                  MPIR_Comm * comm_ptr,
                                                                  MPIR_Sched_t s);


/******************************** Ireduce_scatter_block ********************************/
/* request-based functions */
int MPIR_Ireduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount,
                               MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                               MPIR_Request ** request);
int MPIR_Ireduce_scatter_block_impl(const void *sendbuf, void *recvbuf, int recvcount,
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                    MPIR_Request ** request);
int MPIR_Ireduce_scatter_block_intra_recexch(const void *sendbuf, void *recvbuf,
                                             int recvcount, MPI_Datatype datatype,
                                             MPI_Op op, MPIR_Comm * comm, MPIR_Request ** req);

/* sched-based functions */
int MPIR_Ireduce_scatter_block_sched(const void *sendbuf, void *recvbuf, int recvcount,
                                     MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                     MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_sched_impl(const void *sendbuf, void *recvbuf, int recvcount,
                                          MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                          MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ireduce_scatter_block_sched_intra_auto(const void *sendbuf, void *recvbuf, int recvcount,
                                                MPI_Datatype datatype, MPI_Op op,
                                                MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_sched_intra_noncommutative(const void *sendbuf, void *recvbuf,
                                                          int recvcount, MPI_Datatype datatype,
                                                          MPI_Op op, MPIR_Comm * comm_ptr,
                                                          MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_sched_intra_pairwise(const void *sendbuf, void *recvbuf,
                                                    int recvcount, MPI_Datatype datatype, MPI_Op op,
                                                    MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_sched_intra_recursive_doubling(const void *sendbuf, void *recvbuf,
                                                              int recvcount, MPI_Datatype datatype,
                                                              MPI_Op op, MPIR_Comm * comm_ptr,
                                                              MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_sched_intra_recursive_halving(const void *sendbuf, void *recvbuf,
                                                             int recvcount, MPI_Datatype datatype,
                                                             MPI_Op op, MPIR_Comm * comm_ptr,
                                                             MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ireduce_scatter_block_sched_inter_auto(const void *sendbuf, void *recvbuf, int recvcount,
                                                MPI_Datatype datatype, MPI_Op op,
                                                MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_sched_inter_remote_reduce_local_scatterv(const void *sendbuf,
                                                                        void *recvbuf,
                                                                        int recvcount,
                                                                        MPI_Datatype datatype,
                                                                        MPI_Op op,
                                                                        MPIR_Comm * comm_ptr,
                                                                        MPIR_Sched_t s);


/******************************** Iscan ********************************/
/* request-based functions */
int MPIR_Iscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
               MPIR_Comm * comm_ptr, MPIR_Request ** request);
int MPIR_Iscan_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                    MPIR_Comm * comm_ptr, MPIR_Request ** request);

/* sched-based functions */
int MPIR_Iscan_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                     MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iscan_sched_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                          MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Iscan_sched_intra_auto(const void *sendbuf, void *recvbuf, int count,
                                MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                MPIR_Sched_t s);
int MPIR_Iscan_sched_intra_recursive_doubling(const void *sendbuf, void *recvbuf, int count,
                                              MPI_Datatype datatype, MPI_Op op,
                                              MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iscan_sched_intra_smp(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                               MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Iscatter ********************************/
/* request-based functions */
int MPIR_Iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                  int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                  MPIR_Request ** request);
int MPIR_Iscatter_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                       int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                       MPIR_Request ** request);
int MPIR_Iscatter_intra_tree(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                             void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                             MPIR_Comm * comm_ptr, MPIR_Request ** request);

/* sched-based functions */
int MPIR_Iscatter_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                        int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                        MPIR_Sched_t s);
int MPIR_Iscatter_sched_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                             void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                             MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Iscatter_sched_intra_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                                   MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iscatter_sched_intra_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                       int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Iscatter_sched_inter_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                                   MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iscatter_sched_inter_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                     void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                                     MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iscatter_sched_inter_remote_send_local_scatter(const void *sendbuf, int sendcount,
                                                        MPI_Datatype sendtype, void *recvbuf,
                                                        int recvcount, MPI_Datatype recvtype,
                                                        int root, MPIR_Comm * comm_ptr,
                                                        MPIR_Sched_t s);


/******************************** Iscatterv ********************************/
/* request-based functions */
int MPIR_Iscatterv(const void *sendbuf, const int *sendcounts, const int *displs,
                   MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   int root, MPIR_Comm * comm_ptr, MPIR_Request ** request);
int MPIR_Iscatterv_impl(const void *sendbuf, const int *sendcounts, const int *displs,
                        MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                        int root, MPIR_Comm * comm_ptr, MPIR_Request ** request);

/* sched-based functions */
int MPIR_Iscatterv_sched(const void *sendbuf, const int *sendcounts, const int *displs,
                         MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                         int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iscatterv_sched_impl(const void *sendbuf, const int *sendcounts, const int *displs,
                              MPI_Datatype sendtype, void *recvbuf, int recvcount,
                              MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                              MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Iscatterv_sched_intra_auto(const void *sendbuf, const int *sendcounts, const int *displs,
                                    MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                    MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                    MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Iscatterv_sched_inter_auto(const void *sendbuf, const int *sendcounts, const int *displs,
                                    MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                    MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                    MPIR_Sched_t s);

/* sched-based anycomm functions */
int MPIR_Iscatterv_sched_allcomm_linear(const void *sendbuf, const int *sendcounts,
                                        const int *displs, MPI_Datatype sendtype, void *recvbuf,
                                        int recvcount, MPI_Datatype recvtype, int root,
                                        MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Neighbor_allgather ********************************/
int MPIR_Neighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                            void *recvbuf, int recvcount, MPI_Datatype recvtype,
                            MPIR_Comm * comm_ptr);
int MPIR_Neighbor_allgather_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                 void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                 MPIR_Comm * comm_ptr);

/* intracomm-only functions */
int MPIR_Neighbor_allgather_intra_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                       MPIR_Comm * comm_ptr);

/* intercomm-only functions */
int MPIR_Neighbor_allgather_inter_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                       MPIR_Comm * comm_ptr);

/* anycomm functions */
int MPIR_Neighbor_allgather_allcomm_nb(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                       MPIR_Comm * comm_ptr);


/******************************** Neighbor_allgatherv ********************************/
int MPIR_Neighbor_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                             void *recvbuf, const int recvcounts[], const int displs[],
                             MPI_Datatype recvtype, MPIR_Comm * comm_ptr);
int MPIR_Neighbor_allgatherv_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, const int recvcounts[], const int displs[],
                                  MPI_Datatype recvtype, MPIR_Comm * comm_ptr);

/* intracomm-only functions */
int MPIR_Neighbor_allgatherv_intra_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, const int recvcounts[], const int displs[],
                                        MPI_Datatype recvtype, MPIR_Comm * comm_ptr);

/* intercomm-only functions */
int MPIR_Neighbor_allgatherv_inter_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, const int recvcounts[], const int displs[],
                                        MPI_Datatype recvtype, MPIR_Comm * comm_ptr);

/* anycomm functions */
int MPIR_Neighbor_allgatherv_allcomm_nb(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, const int recvcounts[], const int displs[],
                                        MPI_Datatype recvtype, MPIR_Comm * comm_ptr);


/******************************** Neighbor_alltoall ********************************/
int MPIR_Neighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                           int recvcount, MPI_Datatype recvtype, MPIR_Comm * comm_ptr);
int MPIR_Neighbor_alltoall_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                MPIR_Comm * comm_ptr);

/* intracomm-only functions */
int MPIR_Neighbor_alltoall_intra_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                      MPIR_Comm * comm_ptr);

/* intercomm-only functions */
int MPIR_Neighbor_alltoall_inter_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                      MPIR_Comm * comm_ptr);

/* anycomm functions */
int MPIR_Neighbor_alltoall_allcomm_nb(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                      MPIR_Comm * comm_ptr);


/******************************** Neighbor_alltoallv ********************************/
int MPIR_Neighbor_alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[],
                            MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                            const int rdispls[], MPI_Datatype recvtype, MPIR_Comm * comm_ptr);
int MPIR_Neighbor_alltoallv_impl(const void *sendbuf, const int sendcounts[], const int sdispls[],
                                 MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                                 const int rdispls[], MPI_Datatype recvtype, MPIR_Comm * comm_ptr);

/* intracomm-only functions */
int MPIR_Neighbor_alltoallv_intra_auto(const void *sendbuf, const int sendcounts[],
                                       const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                                       const int recvcounts[], const int rdispls[],
                                       MPI_Datatype recvtype, MPIR_Comm * comm_ptr);

/* intercomm-only functions */
int MPIR_Neighbor_alltoallv_inter_auto(const void *sendbuf, const int sendcounts[],
                                       const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                                       const int recvcounts[], const int rdispls[],
                                       MPI_Datatype recvtype, MPIR_Comm * comm_ptr);

/* anycomm functions */
int MPIR_Neighbor_alltoallv_allcomm_nb(const void *sendbuf, const int sendcounts[],
                                       const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                                       const int recvcounts[], const int rdispls[],
                                       MPI_Datatype recvtype, MPIR_Comm * comm_ptr);


/******************************** Neighbor_alltoallw ********************************/
int MPIR_Neighbor_alltoallw(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[],
                            const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                            const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                            MPIR_Comm * comm_ptr);
int MPIR_Neighbor_alltoallw_impl(const void *sendbuf, const int sendcounts[],
                                 const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                 void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[],
                                 const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr);

/* intracomm-only functions */
int MPIR_Neighbor_alltoallw_intra_auto(const void *sendbuf, const int sendcounts[],
                                       const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                       void *recvbuf, const int recvcounts[],
                                       const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                       MPIR_Comm * comm_ptr);

/* intercomm-only functions */
int MPIR_Neighbor_alltoallw_inter_auto(const void *sendbuf, const int sendcounts[],
                                       const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                       void *recvbuf, const int recvcounts[],
                                       const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                       MPIR_Comm * comm_ptr);

/* anycomm functions */
int MPIR_Neighbor_alltoallw_allcomm_nb(const void *sendbuf, const int sendcounts[],
                                       const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                       void *recvbuf, const int recvcounts[],
                                       const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                       MPIR_Comm * comm_ptr);


/******************************** Reduce ********************************/
int MPIR_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Reduce_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                     MPI_Op op, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intracomm-only functions */
int MPIR_Reduce_intra_auto(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                           MPI_Op op, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Reduce_intra_binomial(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                               MPI_Op op, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Reduce_intra_reduce_scatter_gather(const void *sendbuf, void *recvbuf, int count,
                                            MPI_Datatype datatype, MPI_Op op, int root,
                                            MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Reduce_intra_smp(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                          MPI_Op op, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Reduce_inter_auto(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                           MPI_Op op, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Reduce_inter_local_reduce_remote_send(const void *sendbuf, void *recvbuf, int count,
                                               MPI_Datatype datatype, MPI_Op op, int root,
                                               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Reduce_allcomm_nb(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                           MPI_Op op, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);


/******************************** Reduce_local ********************************/
int MPIR_Reduce_local(const void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype,
                      MPI_Op op);


/******************************** Reduce_scatter ********************************/
int MPIR_Reduce_scatter(const void *sendbuf, void *recvbuf, const int *recvcnts,
                        MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                        MPIR_Errflag_t * errflag);
int MPIR_Reduce_scatter_impl(const void *sendbuf, void *recvbuf, const int *recvcnts,
                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                             MPIR_Errflag_t * errflag);

/* intracomm-only functions */
int MPIR_Reduce_scatter_intra_auto(const void *sendbuf, void *recvbuf, const int *recvcnts,
                                   MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                   MPIR_Errflag_t * errflag);
int MPIR_Reduce_scatter_intra_noncommutative(const void *sendbuf, void *recvbuf,
                                             const int *recvcnts, MPI_Datatype datatype, MPI_Op op,
                                             MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Reduce_scatter_intra_pairwise(const void *sendbuf, void *recvbuf, const int *recvcnts,
                                       MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                       MPIR_Errflag_t * errflag);
int MPIR_Reduce_scatter_intra_recursive_doubling(const void *sendbuf, void *recvbuf,
                                                 const int *recvcnts, MPI_Datatype datatype,
                                                 MPI_Op op, MPIR_Comm * comm_ptr,
                                                 MPIR_Errflag_t * errflag);
int MPIR_Reduce_scatter_intra_recursive_halving(const void *sendbuf, void *recvbuf,
                                                const int *recvcnts, MPI_Datatype datatype,
                                                MPI_Op op, MPIR_Comm * comm_ptr,
                                                MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Reduce_scatter_inter_auto(const void *sendbuf, void *recvbuf, const int *recvcnts,
                                   MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                   MPIR_Errflag_t * errflag);
int MPIR_Reduce_scatter_inter_remote_reduce_local_scatter(const void *sendbuf, void *recvbuf,
                                                          const int *recvcnts,
                                                          MPI_Datatype datatype, MPI_Op op,
                                                          MPIR_Comm * comm_ptr,
                                                          MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Reduce_scatter_allcomm_nb(const void *sendbuf, void *recvbuf, const int *recvcnts,
                                   MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                   MPIR_Errflag_t * errflag);


/******************************** Reduce_scatter_block ********************************/
int MPIR_Reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount,
                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                              MPIR_Errflag_t * errflag);
int MPIR_Reduce_scatter_block_impl(const void *sendbuf, void *recvbuf, int recvcount,
                                   MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                   MPIR_Errflag_t * errflag);

/* intracomm-only functions */
int MPIR_Reduce_scatter_block_intra_auto(const void *sendbuf, void *recvbuf, int recvcount,
                                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                         MPIR_Errflag_t * errflag);
int MPIR_Reduce_scatter_block_intra_noncommutative(const void *sendbuf, void *recvbuf,
                                                   int recvcount, MPI_Datatype datatype, MPI_Op op,
                                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Reduce_scatter_block_intra_pairwise(const void *sendbuf, void *recvbuf, int recvcount,
                                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                             MPIR_Errflag_t * errflag);
int MPIR_Reduce_scatter_block_intra_recursive_doubling(const void *sendbuf, void *recvbuf,
                                                       int recvcount, MPI_Datatype datatype,
                                                       MPI_Op op, MPIR_Comm * comm_ptr,
                                                       MPIR_Errflag_t * errflag);
int MPIR_Reduce_scatter_block_intra_recursive_halving(const void *sendbuf, void *recvbuf,
                                                      int recvcount, MPI_Datatype datatype,
                                                      MPI_Op op, MPIR_Comm * comm_ptr,
                                                      MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Reduce_scatter_block_inter_auto(const void *sendbuf, void *recvbuf, int recvcount,
                                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                         MPIR_Errflag_t * errflag);
int MPIR_Reduce_scatter_block_inter_remote_reduce_local_scatter(const void *sendbuf, void *recvbuf,
                                                                int recvcount,
                                                                MPI_Datatype datatype, MPI_Op op,
                                                                MPIR_Comm * comm_ptr,
                                                                MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Reduce_scatter_block_allcomm_nb(const void *sendbuf, void *recvbuf, int recvcount,
                                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                         MPIR_Errflag_t * errflag);


/******************************** Scan ********************************/
int MPIR_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Scan_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intracomm-only functions */
int MPIR_Scan_intra_auto(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                         MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Scan_intra_recursive_doubling(const void *sendbuf, void *recvbuf, int count,
                                       MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                       MPIR_Errflag_t * errflag);
int MPIR_Scan_intra_smp(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                        MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Scan_allcomm_nb(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                         MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);


/******************************** Scatter ********************************/
int MPIR_Scatter(const void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf,
                 int recvcnt, MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                 MPIR_Errflag_t * errflag);
int MPIR_Scatter_impl(const void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf,
                      int recvcnt, MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                      MPIR_Errflag_t * errflag);

/* intracomm-only functions */
int MPIR_Scatter_intra_auto(const void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf,
                            int recvcnt, MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                            MPIR_Errflag_t * errflag);
int MPIR_Scatter_intra_binomial(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                                void *recvbuf, int recvcnt, MPI_Datatype recvtype, int root,
                                MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Scatter_inter_auto(const void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf,
                            int recvcnt, MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                            MPIR_Errflag_t * errflag);
int MPIR_Scatter_inter_linear(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                              void *recvbuf, int recvcnt, MPI_Datatype recvtype, int root,
                              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Scatter_inter_remote_send_local_scatter(const void *sendbuf, int sendcnt,
                                                 MPI_Datatype sendtype, void *recvbuf, int recvcnt,
                                                 MPI_Datatype recvtype, int root,
                                                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Scatter_allcomm_nb(const void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf,
                            int recvcnt, MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                            MPIR_Errflag_t * errflag);


/******************************** Scatterv ********************************/
int MPIR_Scatterv(const void *sendbuf, const int *sendcnts, const int *displs,
                  MPI_Datatype sendtype, void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                  int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Scatterv_impl(const void *sendbuf, const int *sendcnts, const int *displs,
                       MPI_Datatype sendtype, void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                       int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intracomm-only functions */
int MPIR_Scatterv_intra_auto(const void *sendbuf, const int *sendcnts, const int *displs,
                             MPI_Datatype sendtype, void *recvbuf, int recvcnt,
                             MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                             MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Scatterv_inter_auto(const void *sendbuf, const int *sendcnts, const int *displs,
                             MPI_Datatype sendtype, void *recvbuf, int recvcnt,
                             MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                             MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Scatterv_allcomm_linear(const void *sendbuf, const int *sendcnts, const int *displs,
                                 MPI_Datatype sendtype, void *recvbuf, int recvcnt,
                                 MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                 MPIR_Errflag_t * errflag);
int MPIR_Scatterv_allcomm_nb(const void *sendbuf, const int *sendcnts, const int *displs,
                             MPI_Datatype sendtype, void *recvbuf, int recvcnt,
                             MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                             MPIR_Errflag_t * errflag);

#endif /* MPIR_COLL_H_INCLUDED */
