/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Barrier
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Barrier(MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Barrier(comm, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Bcast(void *buffer, int count, MPI_Datatype datatype, int root,
                             MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Bcast(buffer, count, datatype, root, comm, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Allreduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Allreduce(const void *sendbuf, void *recvbuf, int count,
                                 MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                 MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                 void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                 MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgather(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, comm, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Allgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, const int *recvcounts, const int *displs,
                                  MPI_Datatype recvtype, MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                recvcounts, displs, recvtype, comm, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                               void *recvbuf, int recvcount, MPI_Datatype recvtype,
                               int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                             recvtype, root, comm, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Scatterv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Scatterv(const void *sendbuf, const int *sendcounts, const int *displs,
                                MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                              recvcount, recvtype, root, comm, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Gather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                              void *recvbuf, int recvcount, MPI_Datatype recvtype,
                              int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                            recvtype, root, comm, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Gatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                               void *recvbuf, const int *recvcounts, const int *displs,
                               MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                               MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                             displs, recvtype, root, comm, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Alltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                              recvtype, comm, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Alltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls,
                                 MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                                 const int *rdispls, MPI_Datatype recvtype, MPIR_Comm * comm,
                                 MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                               recvcounts, rdispls, recvtype, comm, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Alltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Alltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[],
                                 const MPI_Datatype sendtypes[], void *recvbuf,
                                 const int recvcounts[], const int rdispls[],
                                 const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr,
                                 MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoallw(sendbuf, sendcounts, sdispls,
                               sendtypes, recvbuf, recvcounts,
                               rdispls, recvtypes, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Reduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Reduce(const void *sendbuf, void *recvbuf, int count,
                              MPI_Datatype datatype, MPI_Op op, int root,
                              MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Reduce_scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[],
                                      MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                      MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_scatter(sendbuf, recvbuf, recvcounts,
                                    datatype, op, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Reduce_scatter_block
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                            int recvcount, MPI_Datatype datatype,
                                            MPI_Op op, MPIR_Comm * comm_ptr,
                                            MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_scatter_block(sendbuf, recvbuf, recvcount,
                                          datatype, op, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Scan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Scan(const void *sendbuf, void *recvbuf, int count,
                            MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                            MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Scan(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Exscan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Exscan(const void *sendbuf, void *recvbuf, int count,
                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                              MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Exscan(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Neighbor_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Neighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                          MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Neighbor_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                        recvcount, recvtype, comm);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Neighbor_allgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Neighbor_allgatherv(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           const int recvcounts[], const int displs[],
                                           MPI_Datatype recvtype, MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Neighbor_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                         recvcounts, displs, recvtype, comm);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Neighbor_alltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Neighbor_alltoallv(const void *sendbuf, const int sendcounts[],
                                          const int sdispls[], MPI_Datatype sendtype,
                                          void *recvbuf, const int recvcounts[],
                                          const int rdispls[], MPI_Datatype recvtype,
                                          MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Neighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                        recvbuf, recvcounts, rdispls, recvtype, comm);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Neighbor_alltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Neighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                                          const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                          void *recvbuf, const int recvcounts[],
                                          const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                          MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Neighbor_alltoallw(sendbuf, sendcounts, sdispls,
                                        sendtypes, recvbuf, recvcounts, rdispls, recvtypes,
                                        comm);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Neighbor_alltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Neighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                         MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Neighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                       recvcount, recvtype, comm);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Ineighbor_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Ineighbor_allgather(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                           MPI_Datatype recvtype, MPIR_Comm * comm,
                                           MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ineighbor_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                         recvcount, recvtype, comm, request);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Ineighbor_allgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Ineighbor_allgatherv(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf,
                                            const int recvcounts[], const int displs[],
                                            MPI_Datatype recvtype, MPIR_Comm * comm,
                                            MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ineighbor_allgatherv(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcounts, displs, recvtype, comm,
                                          request);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Ineighbor_alltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Ineighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                          MPIR_Comm * comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ineighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                        recvcount, recvtype, comm, request);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Ineighbor_alltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Ineighbor_alltoallv(const void *sendbuf, const int sendcounts[],
                                           const int sdispls[], MPI_Datatype sendtype,
                                           void *recvbuf, const int recvcounts[],
                                           const int rdispls[], MPI_Datatype recvtype,
                                           MPIR_Comm * comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ineighbor_alltoallv(sendbuf, sendcounts, sdispls,
                                         sendtype, recvbuf, recvcounts, rdispls, recvtype,
                                         comm, request);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Ineighbor_alltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Ineighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                                           const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                           void *recvbuf, const int recvcounts[],
                                           const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                           MPIR_Comm * comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ineighbor_alltoallw(sendbuf, sendcounts, sdispls,
                                         sendtypes, recvbuf, recvcounts, rdispls, recvtypes,
                                         comm, request);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Ibarrier
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Ibarrier(MPIR_Comm * comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ibarrier(comm, request);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Ibcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root,
                              MPIR_Comm * comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ibcast(buffer, count, datatype, root, comm, request);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Iallgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                  MPIR_Comm * comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Iallgather(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, comm, request);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Iallgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, const int *recvcounts, const int *displs,
                                   MPI_Datatype recvtype, MPIR_Comm * comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Iallgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcounts, displs, recvtype, comm, request);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Iallreduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Iallreduce(const void *sendbuf, void *recvbuf, int count,
                                  MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                  MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Iallreduce(sendbuf, recvbuf, count, datatype, op, comm, request);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Ialltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                 void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                 MPIR_Comm * comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ialltoall(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, comm, request);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Ialltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Ialltoallv(const void *sendbuf, const int sendcounts[],
                                  const int sdispls[], MPI_Datatype sendtype,
                                  void *recvbuf, const int recvcounts[],
                                  const int rdispls[], MPI_Datatype recvtype,
                                  MPIR_Comm * comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ialltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                recvbuf, recvcounts, rdispls, recvtype, comm, request);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Ialltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Ialltoallw(const void *sendbuf, const int sendcounts[],
                                  const int sdispls[], const MPI_Datatype sendtypes[],
                                  void *recvbuf, const int recvcounts[],
                                  const int rdispls[], const MPI_Datatype recvtypes[],
                                  MPIR_Comm * comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ialltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                recvbuf, recvcounts, rdispls, recvtypes, comm, request);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Iexscan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Iexscan(const void *sendbuf, void *recvbuf, int count,
                               MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                               MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Iexscan(sendbuf, recvbuf, count, datatype, op, comm, request);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Igather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                               void *recvbuf, int recvcount, MPI_Datatype recvtype,
                               int root, MPIR_Comm * comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Igather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                             recvtype, root, comm, request);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Igatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                void *recvbuf, const int *recvcounts, const int *displs,
                                MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                              displs, recvtype, root, comm, request);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Ireduce_scatter_block
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Ireduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount,
                                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                             MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ireduce_scatter_block(sendbuf, recvbuf, recvcount,
                                           datatype, op, comm, request);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Ireduce_scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Ireduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[],
                                       MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                       MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype,
                                     op, comm, request);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Ireduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Ireduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                               MPI_Op op, int root, MPIR_Comm * comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, request);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Iscan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Iscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                             MPI_Op op, MPIR_Comm * comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Iscan(sendbuf, recvbuf, count, datatype, op, comm, request);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Iscatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                int root, MPIR_Comm * comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                              recvtype, root, comm, request);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Iscatterv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPID_Iscatterv(const void *sendbuf, const int *sendcounts,
                                 const int *displs, MPI_Datatype sendtype,
                                 void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                 int root, MPIR_Comm * comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                               recvcount, recvtype, root, comm, request);

    return mpi_errno;
}
