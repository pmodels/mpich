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
int MPIC_Wait(MPIR_Request * request_ptr, MPIR_Errflag_t *errflag);
int MPIC_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status);

int MPIC_Send(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
                 MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIC_Recv(void *buf, MPI_Aint count, MPI_Datatype datatype, int source, int tag,
                 MPIR_Comm *comm_ptr, MPI_Status *status, MPIR_Errflag_t *errflag);
int MPIC_Ssend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
                  MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIC_Sendrecv(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                     int dest, int sendtag, void *recvbuf, MPI_Aint recvcount,
                     MPI_Datatype recvtype, int source, int recvtag,
                     MPIR_Comm *comm_ptr, MPI_Status *status, MPIR_Errflag_t *errflag);
int MPIC_Sendrecv_replace(void *buf, MPI_Aint count, MPI_Datatype datatype,
                             int dest, int sendtag,
                             int source, int recvtag,
                             MPIR_Comm *comm_ptr, MPI_Status *status, MPIR_Errflag_t *errflag);
int MPIC_Isend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
                  MPIR_Comm *comm_ptr, MPIR_Request **request, MPIR_Errflag_t *errflag);
int MPIC_Issend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
                  MPIR_Comm *comm_ptr, MPIR_Request **request, MPIR_Errflag_t *errflag);
int MPIC_Irecv(void *buf, MPI_Aint count, MPI_Datatype datatype, int source,
                  int tag, MPIR_Comm *comm_ptr, MPIR_Request **request);
int MPIC_Waitall(int numreq, MPIR_Request *requests[], MPI_Status statuses[], MPIR_Errflag_t *errflag);


/* Collective fallback implementations */
/* Allgather functions */
int MPIR_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Allgather_intra(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                         MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Allgather_intra_recursive_doubling(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                         MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Allgather_intra_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                         MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Allgather_intra_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                         MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Allgather_inter_generic(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                 void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                 MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Allgather_inter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                         MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Allgather_nb(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                      void *recvbuf, int recvcount, MPI_Datatype recvtype,
                      MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );

/* Allgatherv functions */
int MPIR_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                    void *recvbuf, const int *recvcounts, const int *displs,
                    MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Allgatherv_intra(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                          void *recvbuf, const int *recvcounts, const int *displs,
                          MPI_Datatype recvtype, MPIR_Comm *comm_pt, MPIR_Errflag_t *errflag );
int MPIR_Allgatherv_intra_recursive_doubling(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                          void *recvbuf, const int *recvcounts, const int *displs,
                          MPI_Datatype recvtype, MPIR_Comm *comm_pt, MPIR_Errflag_t *errflag );
int MPIR_Allgatherv_intra_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                          void *recvbuf, const int *recvcounts, const int *displs,
                          MPI_Datatype recvtype, MPIR_Comm *comm_pt, MPIR_Errflag_t *errflag );
int MPIR_Allgatherv_intra_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                          void *recvbuf, const int *recvcounts, const int *displs,
                          MPI_Datatype recvtype, MPIR_Comm *comm_pt, MPIR_Errflag_t *errflag );
int MPIR_Allgatherv_inter_generic(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, const int *recvcounts, const int *displs,
                                  MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Allgatherv_inter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                          void *recvbuf, const int *recvcounts, const int *displs,
                          MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Allgatherv_nb(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                       void *recvbuf, const int *recvcounts, const int *displs,
                       MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );

/* Allreduce functions */
int MPIR_Allreduce(const void *sendbuf, void *recvbuf, int count,
                   MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Allreduce_intra_recursive_doubling(const void *sendbuf, void *recvbuf, int count,
                                      MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Allreduce_intra_reduce_scatter_allgather(const void *sendbuf, void *recvbuf, int count,
                                            MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Allreduce_intra_smp(const void *sendbuf, void *recvbuf, int count,
                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Allreduce_intra(const void *sendbuf, void *recvbuf, int count,
                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Allreduce_inter_generic(const void *sendbuf, void *recvbuf, int count,
                                 MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Allreduce_inter(const void *sendbuf, void *recvbuf, int count,
                        MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Allreduce_nb(const void *sendbuf, void *recvbuf, int count,
                      MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);

/* Alltoall functions */
int MPIR_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Alltoall_intra(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                        MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Alltoall_intra_pairwise_sendrecv_replace(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                        MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Alltoall_intra_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                        MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Alltoall_intra_scattered(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                        MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Alltoall_intra_pairwise(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                        MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Alltoall_inter_generic(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Alltoall_inter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                        MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Alltoall_nb(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                     void *recvbuf, int recvcount, MPI_Datatype recvtype,
                     MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);

/* Alltoallv functions */
int MPIR_Alltoallv(const void *sendbuf, const int *sendcnts, const int *sdispls,
                   MPI_Datatype sendtype, void *recvbuf, const int *recvcnts,
                   const int *rdispls, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Alltoallv_intra(const void *sendbuf, const int *sendcnts, const int *sdispls,
                         MPI_Datatype sendtype, void *recvbuf, const int *recvcnts,
                         const int *rdispls, MPI_Datatype recvtype, MPIR_Comm *comm_ptr,
                         MPIR_Errflag_t *errflag);
int MPIR_Alltoallv_intra_pairwise_sendrecv_replace(const void *sendbuf, const int *sendcnts, const int *sdispls,
                         MPI_Datatype sendtype, void *recvbuf, const int *recvcnts,
                         const int *rdispls, MPI_Datatype recvtype, MPIR_Comm *comm_ptr,
                         MPIR_Errflag_t *errflag);
int MPIR_Alltoallv_intra_scattered(const void *sendbuf, const int *sendcnts, const int *sdispls,
                         MPI_Datatype sendtype, void *recvbuf, const int *recvcnts,
                         const int *rdispls, MPI_Datatype recvtype, MPIR_Comm *comm_ptr,
                         MPIR_Errflag_t *errflag);
int MPIR_Alltoallv_inter_generic(const void *sendbuf, const int *sendcnts, const int *sdispls,
                                 MPI_Datatype sendtype, void *recvbuf, const int *recvcnts,
                                 const int *rdispls, MPI_Datatype recvtype,
                                 MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Alltoallv_inter(const void *sendbuf, const int *sendcnts, const int *sdispls,
                         MPI_Datatype sendtype, void *recvbuf, const int *recvcnts,
                         const int *rdispls, MPI_Datatype recvtype,
                         MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Alltoallv_nb(const void *sendbuf, const int *sendcnts, const int *sdispls,
                      MPI_Datatype sendtype, void *recvbuf, const int *recvcnts,
                      const int *rdispls, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);

/* Alltoallw functions */
int MPIR_Alltoallw(const void *sendbuf, const int *sendcnts, const int *sdispls,
                   const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcnts,
                   const int *rdispls, const MPI_Datatype *recvtypes, MPIR_Comm *comm_ptr,
                   MPIR_Errflag_t *errflag);
int MPIR_Alltoallw_intra(const void *sendbuf, const int *sendcnts, const int *sdispls,
                         const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcnts,
                         const int *rdispls, const MPI_Datatype *recvtypes, MPIR_Comm *comm_ptr,
                         MPIR_Errflag_t *errflag);
int MPIR_Alltoallw_intra_pairwise_sendrecv_replace(const void *sendbuf, const int *sendcnts, const int *sdispls,
                         const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcnts,
                         const int *rdispls, const MPI_Datatype *recvtypes, MPIR_Comm *comm_ptr,
                         MPIR_Errflag_t *errflag);
int MPIR_Alltoallw_intra_scattered(const void *sendbuf, const int *sendcnts, const int *sdispls,
                         const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcnts,
                         const int *rdispls, const MPI_Datatype *recvtypes, MPIR_Comm *comm_ptr,
                         MPIR_Errflag_t *errflag);
int MPIR_Alltoallw_inter_generic(const void *sendbuf, const int *sendcnts, const int *sdispls,
                                 const MPI_Datatype *sendtypes, void *recvbuf,
                                 const int *recvcnts, const int *rdispls, const MPI_Datatype *recvtypes,
                                 MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Alltoallw_inter(const void *sendbuf, const int *sendcnts, const int *sdispls,
                         const MPI_Datatype *sendtypes, void *recvbuf,
                         const int *recvcnts, const int *rdispls, const MPI_Datatype *recvtypes,
                         MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Alltoallw_nb(const void *sendbuf, const int *sendcnts, const int *sdispls,
                      const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcnts,
                      const int *rdispls, const MPI_Datatype *recvtypes, MPIR_Comm *comm_ptr,
                      MPIR_Errflag_t *errflag);

/* Bcast functions */
int MPIR_Bcast_intra (void *buffer, int count, MPI_Datatype datatype, int
                      root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Bcast (void *buffer, int count, MPI_Datatype datatype, int
                root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Bcast_intra_binomial(void *buffer, int count, MPI_Datatype datatype, int root,
                        MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Bcast_intra_scatter_ring_allgather(void *buffer, int count, MPI_Datatype datatype, int root,
                                      MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Bcast_intra_scatter_doubling_allgather(void *buffer, int count, MPI_Datatype datatype, int root,
                                          MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Bcast_intra_smp(void *buffer, int count, MPI_Datatype datatype, int root,
                         MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Bcast_inter_generic(void *buffer, int count, MPI_Datatype datatype,
                             int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Bcast_inter(void *buffer, int count, MPI_Datatype datatype,
                     int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Bcast_nb(void *buffer, int count, MPI_Datatype datatype, int
                  root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);

/* Exscan functions */
int MPIR_Exscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Exscan_intra_recursive_doubling(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Exscan_nb(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                   MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );

/* Gather functions */
int MPIR_Gather (const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                 void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                 int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Gather_intra (const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                       void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                       int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Gather_intra_binomial (const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                       void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                       int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Gather_inter_generic (const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                       void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                       int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Gather_inter (const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                       void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                       int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Gather_nb(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                   void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                   int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);

/* Gatherv functions */
int MPIR_Gatherv (const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                  void *recvbuf, const int *recvcnts, const int *displs,
                  MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Gatherv_linear (const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                      void *recvbuf, const int *recvcnts, const int *displs,
                      MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Gatherv_nb(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                    void *recvbuf, const int *recvcnts, const int *displs,
                    MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);

/* Reduce_scatter functions */
int MPIR_Reduce_scatter(const void *sendbuf, void *recvbuf, const int *recvcnts,
                        MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Reduce_scatter_intra(const void *sendbuf, void *recvbuf, const int *recvcnts,
                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Reduce_scatter_intra_recursive_halving(const void *sendbuf, void *recvbuf, const int *recvcnts,
                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Reduce_scatter_intra_pairwise(const void *sendbuf, void *recvbuf, const int *recvcnts,
                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Reduce_scatter_intra_recursive_doubling(const void *sendbuf, void *recvbuf, const int *recvcnts,
                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Reduce_scatter_intra_noncommutative(const void *sendbuf, void *recvbuf, const int *recvcnts,
                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Reduce_scatter_inter_generic(const void *sendbuf, void *recvbuf, const int *recvcnts,
                                      MPI_Datatype datatype, MPI_Op op,
                                      MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Reduce_scatter_inter(const void *sendbuf, void *recvbuf, const int *recvcnts,
                              MPI_Datatype datatype, MPI_Op op,
                              MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Reduce_scatter_nb(const void *sendbuf, void *recvbuf, const int *recvcnts,
                           MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);

/* Reduce_scatter_block functions */
int MPIR_Reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount,
                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm
                              *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Reduce_scatter_block_intra(const void *sendbuf, void *recvbuf, int recvcount,
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm
                                    *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Reduce_scatter_block_intra_noncommutative(const void *sendbuf, void *recvbuf, int recvcount,
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm
                                    *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Reduce_scatter_block_intra_recursive_halving(const void *sendbuf, void *recvbuf, int recvcount,
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm
                                    *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Reduce_scatter_block_intra_pairwise(const void *sendbuf, void *recvbuf, int recvcount,
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm
                                    *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Reduce_scatter_block_intra_recursive_doubling(const void *sendbuf, void *recvbuf, int recvcount,
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm
                                    *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Reduce_scatter_block_inter_generic(const void *sendbuf, void *recvbuf, int recvcount,
                                            MPI_Datatype datatype, MPI_Op op, MPIR_Comm
                                            *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Reduce_scatter_block_inter(const void *sendbuf, void *recvbuf, int recvcount,
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm
                                    *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Reduce_scatter_block_nb(const void *sendbuf, void *recvbuf, int recvcount,
                                 MPI_Datatype datatype, MPI_Op op, MPIR_Comm
                                 *comm_ptr, MPIR_Errflag_t *errflag );

/* Reduce functions */
int MPIR_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Reduce_intra_reduce_scatter_gather(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                               MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Reduce_intra_binomial(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                         MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Reduce_intra_smp(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                          MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Reduce_intra(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                      MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Reduce_inter_generic (const void *sendbuf, void *recvbuf, int count, MPI_Datatype
                       datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Reduce_inter (const void *sendbuf, void *recvbuf, int count, MPI_Datatype
                       datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Reduce_nb(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                   MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );

/* Scan functions */
int MPIR_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
              MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Scan_intra(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
              MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Scan_intra_smp(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
              MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Scan_intra_generic(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
              MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Scan_nb(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                 MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);

/* Scatter functions */
int MPIR_Scatter(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                 void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                 int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Scatter_intra(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                       void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                       int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Scatter_intra_binomial(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                       void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                       int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Scatter_inter_generic(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                       void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                       int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Scatter_inter(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                       void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                       int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Scatter_nb(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                    void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                    int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );

/* Scatterv functions */
int MPIR_Scatterv (const void *sendbuf, const int *sendcnts, const int *displs,
                   MPI_Datatype sendtype, void *recvbuf, int recvcnt,
                   MPI_Datatype recvtype, int root, MPIR_Comm
                   *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Scatterv_linear (const void *sendbuf, const int *sendcnts, const int *displs,
                   MPI_Datatype sendtype, void *recvbuf, int recvcnt,
                   MPI_Datatype recvtype, int root, MPIR_Comm
                   *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Scatterv_nb(const void *sendbuf, const int *sendcnts, const int *displs,
                     MPI_Datatype sendtype, void *recvbuf, int recvcnt,
                     MPI_Datatype recvtype, int root, MPIR_Comm
                     *comm_ptr, MPIR_Errflag_t *errflag);

/* Barrier functions */
int MPIR_Barrier( MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Barrier_intra( MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Barrier_inter_generic( MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Barrier_inter( MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Barrier_intra_recursive_doubling(MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Barrier_nb( MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Barrier_intra_smp(MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);

/* Reduce_local implementation */
int MPIR_Reduce_local(const void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype, MPI_Op op);

/* impl functions for NBC */
int MPIR_Ibarrier(MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Iscatterv(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Ialltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Ialltoallw(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Ireduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Iallreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Ireduce_scatter(const void *sendbuf, void *recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Ireduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Iscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Iexscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPI_Request *request);

/* sched functions for NBC */
int MPIR_Ibarrier_sched(MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibarrier_intra_recursive_doubling_sched(MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibarrier_intra_bcast_sched(MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_sched(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Igather_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Igatherv_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscatter_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscatterv_sched(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_intra_recursive_doubling_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_intra_brucks_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_intra_ring_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_intra_recursive_doubling_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_intra_brucks_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_intra_ring_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallv_sched(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallv_intra_inplace_sched(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallv_intra_blocked_sched(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallv_intra_pairwise_exchange_sched(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallw_sched(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallw_intra_inplace_sched(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallw_intra_blocked_sched(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallw_intra_pairwise_exchange_sched(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_intra_naive_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_intra_reduce_scatter_allgather_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_intra_recursive_doubling_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_intra_smp_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_sched(const void *sendbuf, void *recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_sched(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscan_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iexscan_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);

/* sched functions for neighborhood collectives */
int MPIR_Ineighbor_allgather_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ineighbor_allgatherv_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ineighbor_alltoall_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ineighbor_alltoallv_sched(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ineighbor_alltoallw_sched(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPIR_Comm *comm_ptr, MPIR_Sched_t s);

/* impl functions for neighborhood collectives */
int MPIR_Ineighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Ineighbor_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Ineighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Ineighbor_alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Ineighbor_alltoallw(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPIR_Comm *comm_ptr, MPI_Request *request);

/* neighborhood collective default algorithms */
int MPIR_Neighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr);
int MPIR_Neighbor_allgather_nb(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr);
int MPIR_Neighbor_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr);
int MPIR_Neighbor_allgatherv_nb(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr);
int MPIR_Neighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr);
int MPIR_Neighbor_alltoall_nb(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr);
int MPIR_Neighbor_alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr);
int MPIR_Neighbor_alltoallv_nb(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr);
int MPIR_Neighbor_alltoallw(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPIR_Comm *comm_ptr);
int MPIR_Neighbor_alltoallw_nb(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPIR_Comm *comm_ptr);
int MPIR_Ineighbor_allgather_generic_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ineighbor_allgatherv_generic_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ineighbor_alltoall_generic_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ineighbor_alltoallv_generic_sched(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ineighbor_alltoallw_generic_sched(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPIR_Comm *comm_ptr, MPIR_Sched_t s);


/* nonblocking collective default algorithms */
int MPIR_Ibcast_intra_sched(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_generic_inter_sched(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_inter_sched(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_intra_binomial_sched(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_intra_smp_sched(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_flat_sched(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibarrier_intra_sched(MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibarrier_generic_inter_sched(MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibarrier_inter_sched(MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_intra_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_inter_generic_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_inter_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_intra_binomial_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_intra_reduce_scatter_gather_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_intra_smp_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallv_intra_sched(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallv_generic_inter_sched(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallv_inter_sched(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_intra_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_inter_generic_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_inter_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_intra_naive_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_intra_reduce_scatter_allgather_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_intra_recursive_doubling_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Igather_intra_binomial_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Igather_inter_short_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Igather_inter_long_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Igather_intra_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Igather_inter_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscatter_intra_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscatter_intra_binomial_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscatter_inter_generic_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscatter_inter_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscatterv_linear_sched(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_intra_sched(const void *sendbuf, void *recvbuf, const int *recvcnts, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_inter_sched(const void *sendbuf, void *recvbuf, const int *recvcnts, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_intra_recursive_doubling_sched(const void *sendbuf, void *recvbuf, const int *recvcnts, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_intra_recursive_halving_sched(const void *sendbuf, void *recvbuf, const int *recvcnts, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_pairwise_sched(const void *sendbuf, void *recvbuf, const int *recvcnts, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_intra_noncommutative_sched(const void *sendbuf, void *recvbuf, const int *recvcnts, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_inter_generic_sched(const void *sendbuf, void *recvbuf, const int *recvcnts, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Igatherv_generic_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_intra_sched(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_inter_sched(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_intra_recursive_halving_sched(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_pairwise_sched(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_intra_recursive_doubling_sched(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_noncommutative_sched(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_inter_generic_sched(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_intra_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_inter_generic_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_inter_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_intra_inplace_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_intra_brucks_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_intra_permuted_sendrecv_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_intra_pairwise_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_intra_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_inter_generic_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_inter_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_intra_recursive_doubling_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_intra_brucks_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_intra_ring_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_intra_recursive_doubling_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_intra_brucks_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_intra_ring_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_intra_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_inter_generic_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_inter_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscan_intra_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscan_intra_recursive_doubling_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscan_intra_smp_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iexscan_intra_recursive_doubling_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallw_intra_sched(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallw_generic_inter_sched(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallw_inter_sched(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPIR_Comm *comm_ptr, MPIR_Sched_t s);

#endif /* MPIR_COLL_H_INCLUDED */
