/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_COLL_H_INCLUDED
#define MPIR_COLL_H_INCLUDED

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
int MPIC_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype,
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
int MPIR_Allgather_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                        MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Allgather_intra(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                         MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Allgather_inter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                         MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Allgatherv_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                         void *recvbuf, const int *recvcounts, const int *displs,
                         MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                    void *recvbuf, const int *recvcounts, const int *displs,
                    MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Allgatherv_intra(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                          void *recvbuf, const int *recvcounts, const int *displs,
                          MPI_Datatype recvtype, MPIR_Comm *comm_pt, MPIR_Errflag_t *errflag );
int MPIR_Allgatherv_inter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                          void *recvbuf, const int *recvcounts, const int *displs,
                          MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Allreduce_impl(const void *sendbuf, void *recvbuf, int count,
                        MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Allreduce(const void *sendbuf, void *recvbuf, int count,
                   MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Allreduce_recursive_doubling(const void *sendbuf, void *recvbuf, int count,
                                      MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Allreduce_reduce_scatter_allgather(const void *sendbuf, void *recvbuf, int count,
                                            MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Allreduce_intra(const void *sendbuf, void *recvbuf, int count,
                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Allreduce_inter(const void *sendbuf, void *recvbuf, int count,
                        MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Alltoall_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                       MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Alltoall_intra(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                        MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Alltoall_inter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                        MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Alltoallv_impl(const void *sendbuf, const int *sendcnts, const int *sdispls,
                        MPI_Datatype sendtype, void *recvbuf, const int *recvcnts,
                        const int *rdispls, MPI_Datatype recvtype, MPIR_Comm *comm_ptr,
                        MPIR_Errflag_t *errflag);
int MPIR_Alltoallv(const void *sendbuf, const int *sendcnts, const int *sdispls,
                   MPI_Datatype sendtype, void *recvbuf, const int *recvcnts,
                   const int *rdispls, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Alltoallv_intra(const void *sendbuf, const int *sendcnts, const int *sdispls,
                         MPI_Datatype sendtype, void *recvbuf, const int *recvcnts,
                         const int *rdispls, MPI_Datatype recvtype, MPIR_Comm *comm_ptr,
                         MPIR_Errflag_t *errflag);
int MPIR_Alltoallv_inter(const void *sendbuf, const int *sendcnts, const int *sdispls,
                         MPI_Datatype sendtype, void *recvbuf, const int *recvcnts,
                         const int *rdispls, MPI_Datatype recvtype,
                         MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Alltoallw_impl(const void *sendbuf, const int *sendcnts, const int *sdispls,
                        const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcnts,
                        const int *rdispls, const MPI_Datatype *recvtypes, MPIR_Comm *comm_ptr,
                        MPIR_Errflag_t *errflag);
int MPIR_Alltoallw(const void *sendbuf, const int *sendcnts, const int *sdispls,
                   const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcnts,
                   const int *rdispls, const MPI_Datatype *recvtypes, MPIR_Comm *comm_ptr,
                   MPIR_Errflag_t *errflag);
int MPIR_Alltoallw_intra(const void *sendbuf, const int *sendcnts, const int *sdispls,
                         const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcnts,
                         const int *rdispls, const MPI_Datatype *recvtypes, MPIR_Comm *comm_ptr,
                         MPIR_Errflag_t *errflag);
int MPIR_Alltoallw_inter(const void *sendbuf, const int *sendcnts, const int *sdispls,
                         const MPI_Datatype *sendtypes, void *recvbuf,
                         const int *recvcnts, const int *rdispls, const MPI_Datatype *recvtypes,
                         MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Bcast_inter(void *buffer, int count, MPI_Datatype datatype,
		     int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Bcast_intra (void *buffer, int count, MPI_Datatype datatype, int
                      root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Bcast (void *buffer, int count, MPI_Datatype datatype, int
                root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Bcast_binomial(void *buffer, int count, MPI_Datatype datatype, int root,
                        MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Bcast_scatter_ring_allgather(void *buffer, int count, MPI_Datatype datatype, int root,
                                      MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Bcast_scatter_doubling_allgather(void *buffer, int count, MPI_Datatype datatype, int root,
                                          MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Bcast_impl (void *buffer, int count, MPI_Datatype datatype, int
                root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Exscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Exscan_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                     MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Gather_impl (const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                      void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                      int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Gather (const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                 void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                 int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Gather_intra (const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                       void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                       int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Gather_inter (const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                       void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                       int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Gatherv (const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                  void *recvbuf, const int *recvcnts, const int *displs,
                  MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Gatherv_impl (const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                       void *recvbuf, const int *recvcnts, const int *displs,
                       MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Reduce_scatter_impl(const void *sendbuf, void *recvbuf, const int *recvcnts,
                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Reduce_scatter(const void *sendbuf, void *recvbuf, const int *recvcnts,
                        MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Reduce_scatter_intra(const void *sendbuf, void *recvbuf, const int *recvcnts,
                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Reduce_scatter_inter(const void *sendbuf, void *recvbuf, const int *recvcnts,
                              MPI_Datatype datatype, MPI_Op op,
                              MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Reduce_scatter_block_impl(const void *sendbuf, void *recvbuf, int recvcount,
                                   MPI_Datatype datatype, MPI_Op op, MPIR_Comm
                                   *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount,
                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm
                              *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Reduce_scatter_block_intra(const void *sendbuf, void *recvbuf, int recvcount,
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm
                                    *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Reduce_scatter_block_inter(const void *sendbuf, void *recvbuf, int recvcount,
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm
                                    *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Reduce_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                     MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Reduce_redscat_gather(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                               MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Reduce_binomial(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                         MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Reduce_intra(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                      MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Reduce_inter (const void *sendbuf, void *recvbuf, int count, MPI_Datatype
                       datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Scan_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                   MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
              MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Scatter_impl(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                      void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                      int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Scatter(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                 void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                 int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Scatter_intra(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                       void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                       int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Scatter_inter(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                       void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                       int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag );
int MPIR_Scatterv_impl (const void *sendbuf, const int *sendcnts, const int *displs,
                        MPI_Datatype sendtype, void *recvbuf, int recvcnt,
                        MPI_Datatype recvtype, int root, MPIR_Comm
                        *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Scatterv (const void *sendbuf, const int *sendcnts, const int *displs,
                   MPI_Datatype sendtype, void *recvbuf, int recvcnt,
                   MPI_Datatype recvtype, int root, MPIR_Comm
                   *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Barrier_impl( MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Barrier( MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Barrier_intra( MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Barrier_inter( MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Barrier_recursive_doubling(MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag);
int MPIR_Reduce_local_impl(const void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype, MPI_Op op);


/* group collectives */
int MPIR_Allreduce_group(void *sendbuf, void *recvbuf, int count,
                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr,
                         MPIR_Group *group_ptr, int tag, MPIR_Errflag_t *errflag);
int MPIR_Allreduce_group_intra(void *sendbuf, void *recvbuf, int count,
                               MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr,
                               MPIR_Group *group_ptr, int tag, MPIR_Errflag_t *errflag);


int MPIR_Barrier_group(MPIR_Comm *comm_ptr, MPIR_Group *group_ptr, int tag, MPIR_Errflag_t *errflag);


/* impl functions for NBC */
int MPIR_Ibarrier_impl(MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Ibcast_impl(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Igather_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Igatherv_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Iscatter_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Iscatterv_impl(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Iallgather_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Iallgatherv_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Ialltoall_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Ialltoallv_impl(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Ialltoallw_impl(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Ireduce_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Iallreduce_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Ireduce_scatter_impl(const void *sendbuf, void *recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Ireduce_scatter_block_impl(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Iscan_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Iexscan_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPI_Request *request);

/* sched functions for NBC */
int MPIR_Ibarrier_sched(MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_sched(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Igather_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Igatherv_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscatter_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscatterv_sched(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallv_sched(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallw_sched(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
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
int MPIR_Ineighbor_allgather_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Ineighbor_allgatherv_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Ineighbor_alltoall_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Ineighbor_alltoallv_impl(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Ineighbor_alltoallw_impl(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Neighbor_allgather_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr);
int MPIR_Neighbor_allgatherv_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr);
int MPIR_Neighbor_alltoall_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr);
int MPIR_Neighbor_alltoallv_impl(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr);
int MPIR_Neighbor_alltoallw_impl(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPIR_Comm *comm_ptr);


/* neighborhood collective default algorithms */
int MPIR_Neighbor_allgather_default(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr);
int MPIR_Neighbor_allgatherv_default(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr);
int MPIR_Neighbor_alltoall_default(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr);
int MPIR_Neighbor_alltoallv_default(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr);
int MPIR_Neighbor_alltoallw_default(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPIR_Comm *comm_ptr);
int MPIR_Ineighbor_allgather_default_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ineighbor_allgatherv_default_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ineighbor_alltoall_default_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ineighbor_alltoallv_default_sched(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ineighbor_alltoallw_default_sched(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPIR_Comm *comm_ptr, MPIR_Sched_t s);


/* nonblocking collective default algorithms */
int MPIR_Ibcast_intra_sched(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_inter_sched(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_binomial_sched(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_SMP_sched(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscatter_for_bcast_sched(void *tmp_buf, int root, MPIR_Comm *comm_ptr, int nbytes, MPIR_Sched_t s);
int MPIR_Ibcast_scatter_rec_dbl_allgather_sched(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_scatter_ring_allgather_sched(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibarrier_intra_sched(MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibarrier_inter_sched(MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_intra_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_inter_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_binomial_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_redscat_gather_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_SMP_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallv_intra_sched(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallv_inter_sched(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_intra_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_inter_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_naive_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_SMP_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_redscat_allgather_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_rec_dbl_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Igather_binomial_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Igather_intra_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Igather_inter_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscatter_intra_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscatter_inter_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_intra_sched(const void *sendbuf, void *recvbuf, const int *recvcnts, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_inter_sched(const void *sendbuf, void *recvbuf, const int *recvcnts, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_rec_dbl_sched(const void *sendbuf, void *recvbuf, const int *recvcnts, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_rec_hlv_sched(const void *sendbuf, void *recvbuf, const int *recvcnts, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_pairwise_sched(const void *sendbuf, void *recvbuf, const int *recvcnts, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_intra_sched(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_inter_sched(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_rec_hlv_sched(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_pairwise_sched(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_rec_dbl_sched(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_noncomm_sched(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_intra_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_inter_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_inplace_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_bruck_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_perm_sr_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_pairwise_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_intra_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_inter_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_rec_dbl_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_bruck_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_ring_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_rec_dbl_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_bruck_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_ring_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_intra_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_inter_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscan_rec_dbl_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscan_SMP_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallw_intra_sched(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallw_inter_sched(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPIR_Comm *comm_ptr, MPIR_Sched_t s);

#endif /* MPIR_COLL_H_INCLUDED */
