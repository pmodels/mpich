/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2015 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */

#ifndef MPICOLLTEST_H_INCLUDED
#define MPICOLLTEST_H_INCLUDED

/* This file defines MTest wrapper functions for MPI collectives.
 * Test uses MTest_Collective calls that are defined as blocking collective by
 * default, or as non-blocking version by compiling with -DUSE_MTEST_NBC. */

#include "mpi.h"
#include "mpitestconf.h"

/* Wrap up MTest_Collective calls by non-blocking collectives.
 * It requires MPI_VERSION >= 3. */
#if defined(USE_MTEST_NBC) && MPI_VERSION >= 3
static inline int MTest_Barrier(MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = MPI_Ibarrier(comm, &req);
    if (mpi_errno != MPI_SUCCESS)
        return mpi_errno;
    mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

static inline int MTest_Bcast(void *buffer, int count, MPI_Datatype datatype, int root,
                              MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = MPI_Ibcast(buffer, count, datatype, root, comm, &req);
    if (mpi_errno != MPI_SUCCESS)
        return mpi_errno;
    mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

static inline int MTest_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                               void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                               MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = MPI_Igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                            root, comm, &req);
    if (mpi_errno != MPI_SUCCESS)
        return mpi_errno;
    mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

static inline int MTest_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                void *recvbuf, const int *recvcounts, const int *displs,
                                MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = MPI_Igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                             recvtype, root, comm, &req);
    if (mpi_errno != MPI_SUCCESS)
        return mpi_errno;
    mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

static inline int MTest_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                                MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = MPI_Iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                             recvtype, root, comm, &req);
    if (mpi_errno != MPI_SUCCESS)
        return mpi_errno;
    mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

static inline int MTest_Scatterv(const void *sendbuf, const int *sendcounts, const int *displs,
                                 MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                 MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = MPI_Iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                              recvcount, recvtype, root, comm, &req);
    if (mpi_errno != MPI_SUCCESS)
        return mpi_errno;
    mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

static inline int MTest_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                  MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = MPI_Iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                               recvtype, comm, &req);
    if (mpi_errno != MPI_SUCCESS)
        return mpi_errno;
    mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

static inline int MTest_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, const int *recvcounts, const int *displs,
                                   MPI_Datatype recvtype, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = MPI_Iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                displs, recvtype, comm, &req);
    if (mpi_errno != MPI_SUCCESS)
        return mpi_errno;
    mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

static inline int MTest_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                 void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = MPI_Ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                              recvtype, comm, &req);
    if (mpi_errno != MPI_SUCCESS)
        return mpi_errno;
    mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

static inline int MTest_Alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls,
                                  MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                                  const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = MPI_Ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                               recvcounts, rdispls, recvtype, comm, &req);
    if (mpi_errno != MPI_SUCCESS)
        return mpi_errno;
    mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

static inline int MTest_Alltoallw(const void *sendbuf, const int *sendcounts, const int *sdispls,
                                  const MPI_Datatype * sendtypes, void *recvbuf,
                                  const int *recvcounts, const int *rdispls,
                                  const MPI_Datatype * recvtypes, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = MPI_Ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                               recvcounts, rdispls, recvtypes, comm, &req);
    if (mpi_errno != MPI_SUCCESS)
        return mpi_errno;
    mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

static inline int MTest_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                               MPI_Op op, int root, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = MPI_Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, &req);
    if (mpi_errno != MPI_SUCCESS)
        return mpi_errno;
    mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

static inline int MTest_Allreduce(const void *sendbuf, void *recvbuf, int count,
                                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = MPI_Iallreduce(sendbuf, recvbuf, count, datatype, op, comm, &req);
    if (mpi_errno != MPI_SUCCESS)
        return mpi_errno;
    mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

static inline int MTest_Reduce_scatter(const void *sendbuf, void *recvbuf, const int *recvcounts,
                                       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = MPI_Ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, &req);
    if (mpi_errno != MPI_SUCCESS)
        return mpi_errno;
    mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

static inline int MTest_Reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount,
                                             MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = MPI_Ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm, &req);
    if (mpi_errno != MPI_SUCCESS)
        return mpi_errno;
    mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

static inline int MTest_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                             MPI_Op op, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = MPI_Iscan(sendbuf, recvbuf, count, datatype, op, comm, &req);
    if (mpi_errno != MPI_SUCCESS)
        return mpi_errno;
    mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

static inline int MTest_Exscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                               MPI_Op op, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = MPI_Iexscan(sendbuf, recvbuf, count, datatype, op, comm, &req);
    if (mpi_errno != MPI_SUCCESS)
        return mpi_errno;
    mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}
#else
/* If USE_MTEST_NBC is not specified, or MPI_VERSION is less than 3,
 * wrap up MTest_Collective calls by traditional blocking collectives.*/

static inline int MTest_Barrier(MPI_Comm comm)
{
    return MPI_Barrier(comm);
}

static inline int MTest_Bcast(void *buffer, int count, MPI_Datatype datatype, int root,
                              MPI_Comm comm)
{
    return MPI_Bcast(buffer, count, datatype, root, comm);
}

static inline int MTest_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                               void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                               MPI_Comm comm)
{
    return MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

static inline int MTest_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                void *recvbuf, const int *recvcounts, const int *displs,
                                MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    return MPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                       recvtype, root, comm);
}

static inline int MTest_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                                MPI_Comm comm)
{
    return MPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

static inline int MTest_Scatterv(const void *sendbuf, const int *sendcounts, const int *displs,
                                 MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                 MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    return MPI_Scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                        recvcount, recvtype, root, comm);
}

static inline int MTest_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                  MPI_Comm comm)
{
    return MPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

static inline int MTest_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, const int *recvcounts, const int *displs,
                                   MPI_Datatype recvtype, MPI_Comm comm)
{
    return MPI_Allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                          displs, recvtype, comm);
}

static inline int MTest_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                 void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    return MPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

static inline int MTest_Alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls,
                                  MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                                  const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
{
    return MPI_Alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                         recvcounts, rdispls, recvtype, comm);
}

static inline int MTest_Alltoallw(const void *sendbuf, const int *sendcounts, const int *sdispls,
                                  const MPI_Datatype * sendtypes, void *recvbuf,
                                  const int *recvcounts, const int *rdispls,
                                  const MPI_Datatype * recvtypes, MPI_Comm comm)
{
    return MPI_Alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                         recvcounts, rdispls, recvtypes, comm);
}

static inline int MTest_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                               MPI_Op op, int root, MPI_Comm comm)
{
    return MPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
}

static inline int MTest_Allreduce(const void *sendbuf, void *recvbuf, int count,
                                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    return MPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
}

static inline int MTest_Reduce_scatter(const void *sendbuf, void *recvbuf, const int *recvcounts,
                                       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    return MPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm);
}

static inline int MTest_Reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount,
                                             MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    return MPI_Reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm);
}

static inline int MTest_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                             MPI_Op op, MPI_Comm comm)
{
    return MPI_Scan(sendbuf, recvbuf, count, datatype, op, comm);
}

static inline int MTest_Exscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                               MPI_Op op, MPI_Comm comm)
{
    return MPI_Exscan(sendbuf, recvbuf, count, datatype, op, comm);
}

#endif /* USE_MTEST_NBC */
#endif /* MPICOLLTEST_H_INCLUDED */
