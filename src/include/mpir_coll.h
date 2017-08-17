/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_COLL_H_INCLUDED
#define MPIR_COLL_H_INCLUDED

/* Collective operations */
typedef struct MPIR_Collops {
    int ref_count;   /* Supports lazy copies */
    /* Contains pointers to the functions for the MPI collectives */
    int (*Barrier) (MPIR_Comm *, MPIR_Errflag_t *);
    int (*Bcast) (void*, int, MPI_Datatype, int, MPIR_Comm *, MPIR_Errflag_t *);
    int (*Gather) (const void*, int, MPI_Datatype, void*, int, MPI_Datatype,
                   int, MPIR_Comm *, MPIR_Errflag_t *);
    int (*Gatherv) (const void*, int, MPI_Datatype, void*, const int *, const int *,
                    MPI_Datatype, int, MPIR_Comm *, MPIR_Errflag_t *);
    int (*Scatter) (const void*, int, MPI_Datatype, void*, int, MPI_Datatype,
                    int, MPIR_Comm *, MPIR_Errflag_t *);
    int (*Scatterv) (const void*, const int *, const int *, MPI_Datatype,
                     void*, int, MPI_Datatype, int, MPIR_Comm *, MPIR_Errflag_t *);
    int (*Allgather) (const void*, int, MPI_Datatype, void*, int,
                      MPI_Datatype, MPIR_Comm *, MPIR_Errflag_t *);
    int (*Allgatherv) (const void*, int, MPI_Datatype, void*, const int *,
                       const int *, MPI_Datatype, MPIR_Comm *, MPIR_Errflag_t *);
    int (*Alltoall) (const void*, int, MPI_Datatype, void*, int, MPI_Datatype,
                               MPIR_Comm *, MPIR_Errflag_t *);
    int (*Alltoallv) (const void*, const int *, const int *, MPI_Datatype,
                      void*, const int *, const int *, MPI_Datatype, MPIR_Comm *,
                      MPIR_Errflag_t *);
    int (*Alltoallw) (const void*, const int *, const int *, const MPI_Datatype *, void*,
                      const int *, const int *, const MPI_Datatype *, MPIR_Comm *, MPIR_Errflag_t *);
    int (*Reduce) (const void*, void*, int, MPI_Datatype, MPI_Op, int,
                   MPIR_Comm *, MPIR_Errflag_t *);
    int (*Allreduce) (const void*, void*, int, MPI_Datatype, MPI_Op,
                      MPIR_Comm *, MPIR_Errflag_t *);
    int (*Reduce_scatter) (const void*, void*, const int *, MPI_Datatype, MPI_Op,
                           MPIR_Comm *, MPIR_Errflag_t *);
    int (*Scan) (const void*, void*, int, MPI_Datatype, MPI_Op, MPIR_Comm *, MPIR_Errflag_t * );
    int (*Exscan) (const void*, void*, int, MPI_Datatype, MPI_Op, MPIR_Comm *, MPIR_Errflag_t * );
    int (*Reduce_scatter_block) (const void*, void*, int, MPI_Datatype, MPI_Op,
                           MPIR_Comm *, MPIR_Errflag_t *);

    /* MPI-3 nonblocking collectives */
    int (*Ibarrier_sched)(MPIR_Comm *comm_ptr, MPIR_Sched_t s);
    int (*Ibcast_sched)(void *buffer, int count, MPI_Datatype datatype, int root,
                  MPIR_Comm *comm_ptr, MPIR_Sched_t s);
    int (*Igather_sched)(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                   int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr,
                   MPIR_Sched_t s);
    int (*Igatherv_sched)(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                    const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root,
                    MPIR_Comm *comm_ptr, MPIR_Sched_t s);
    int (*Iscatter_sched)(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                    int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr,
                    MPIR_Sched_t s);
    int (*Iscatterv_sched)(const void *sendbuf, const int *sendcounts, const int *displs,
                     MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                     int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
    int (*Iallgather_sched)(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                      int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr,
                      MPIR_Sched_t s);
    int (*Iallgatherv_sched)(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                       const int *recvcounts, const int *displs, MPI_Datatype recvtype,
                       MPIR_Comm *comm_ptr, MPIR_Sched_t s);
    int (*Ialltoall_sched)(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                     int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr,
                     MPIR_Sched_t s);
    int (*Ialltoallv_sched)(const void *sendbuf, const int *sendcounts, const int *sdispls,
                      MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                      const int *rdispls, MPI_Datatype recvtype, MPIR_Comm *comm_ptr,
                      MPIR_Sched_t s);
    int (*Ialltoallw_sched)(const void *sendbuf, const int *sendcounts, const int *sdispls,
                      const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts,
                      const int *rdispls, const MPI_Datatype *recvtypes,
                      MPIR_Comm *comm_ptr, MPIR_Sched_t s);
    int (*Ireduce_sched)(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                   int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
    int (*Iallreduce_sched)(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                      MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
    int (*Ireduce_scatter_sched)(const void *sendbuf, void *recvbuf, const int *recvcounts,
                           MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
    int (*Ireduce_scatter_block_sched)(const void *sendbuf, void *recvbuf, int recvcount,
                                 MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr,
                                 MPIR_Sched_t s);
    int (*Iscan_sched)(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                 MPIR_Comm *comm_ptr, MPIR_Sched_t s);
    int (*Iexscan_sched)(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                   MPIR_Comm *comm_ptr, MPIR_Sched_t s);

    struct MPIR_Collops *prev_coll_fns; /* when overriding this table, set this to point to the old table */

    /* MPI-3 neighborhood collectives (blocking & nonblocking) */
    int (*Neighbor_allgather)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                              void *recvbuf, int recvcount, MPI_Datatype recvtype,
                              MPIR_Comm *comm_ptr);
    int (*Neighbor_allgatherv)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                               void *recvbuf, const int recvcounts[], const int displs[],
                               MPI_Datatype recvtype, MPIR_Comm *comm_ptr);
    int (*Neighbor_alltoall)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                             void *recvbuf, int recvcount, MPI_Datatype recvtype,
                             MPIR_Comm *comm_ptr);
    int (*Neighbor_alltoallv)(const void *sendbuf, const int sendcounts[], const int sdispls[],
                              MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                              const int rdispls[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr);
    int (*Neighbor_alltoallw)(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[],
                              const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                              const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                              MPIR_Comm *comm_ptr);
    int (*Ineighbor_allgather)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                               void *recvbuf, int recvcount, MPI_Datatype recvtype,
                               MPIR_Comm *comm_ptr, MPIR_Sched_t s);
    int (*Ineighbor_allgatherv)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                void *recvbuf, const int recvcounts[], const int displs[],
                                MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
    int (*Ineighbor_alltoall)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                              void *recvbuf, int recvcount, MPI_Datatype recvtype,
                              MPIR_Comm *comm_ptr, MPIR_Sched_t s);
    int (*Ineighbor_alltoallv)(const void *sendbuf, const int sendcounts[], const int sdispls[],
                               MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                               const int rdispls[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr,
                               MPIR_Sched_t s);
    int (*Ineighbor_alltoallw)(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[],
                               const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                               const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                               MPIR_Comm *comm_ptr, MPIR_Sched_t s);
} MPIR_Collops;


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
int MPIR_Ineighbor_allgather_default(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ineighbor_allgatherv_default(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ineighbor_alltoall_default(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ineighbor_alltoallv_default(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ineighbor_alltoallw_default(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPIR_Comm *comm_ptr, MPIR_Sched_t s);


/* nonblocking collective default algorithms */
int MPIR_Ibcast_intra(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_inter(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_binomial(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_SMP(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscatter_for_bcast(void *tmp_buf, int root, MPIR_Comm *comm_ptr, int nbytes, MPIR_Sched_t s);
int MPIR_Ibcast_scatter_rec_dbl_allgather(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_scatter_ring_allgather(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibarrier_intra(MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ibarrier_inter(MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_intra(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_inter(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_binomial(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_redscat_gather(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_SMP(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallv_intra(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallv_inter(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_intra(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_inter(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_naive(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_SMP(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_redscat_allgather(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_rec_dbl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Igather_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Igather_intra(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Igather_inter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscatter_intra(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscatter_inter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscatterv(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_intra(const void *sendbuf, void *recvbuf, const int *recvcnts, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_inter(const void *sendbuf, void *recvbuf, const int *recvcnts, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_rec_dbl(const void *sendbuf, void *recvbuf, const int *recvcnts, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_rec_hlv(const void *sendbuf, void *recvbuf, const int *recvcnts, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_pairwise(const void *sendbuf, void *recvbuf, const int *recvcnts, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_intra(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_inter(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_rec_hlv(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_pairwise(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_rec_dbl(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_noncomm(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_intra(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_inter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_inplace(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_bruck(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_perm_sr(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_pairwise(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_intra(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_inter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_rec_dbl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_bruck(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_rec_dbl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_bruck(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_intra(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_inter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscan_rec_dbl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iscan_SMP(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Iexscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallw_intra(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPIR_Comm *comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallw_inter(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPIR_Comm *comm_ptr, MPIR_Sched_t s);

#endif /* MPIR_COLL_H_INCLUDED */
