/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPICOLLTEST_H_INCLUDED
#define MPICOLLTEST_H_INCLUDED

/* This file defines MTest wrapper functions for MPI collectives.
 * Test uses MTest_Collective calls that runs either the blocking or
 * non-blocking version of the collective depend on the is_blocking
 * parameter.
 */

#include "mpi.h"
#include "mpitestconf.h"

/* Wrap up MTest_Collective calls by non-blocking collectives.
 * It requires MPI_VERSION >= 3. */
#if MPI_VERSION >= 3
static inline int MTest_Barrier(int is_blocking, MPI_Comm comm)
{
    if (is_blocking) {
        return MPI_Barrier(comm);
    } else {
        int mpi_errno;
        MPI_Request req = MPI_REQUEST_NULL;

        mpi_errno = MPI_Ibarrier(comm, &req);
        if (mpi_errno != MPI_SUCCESS)
            return mpi_errno;
        mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
        return mpi_errno;
    }
}

static inline int MTest_Bcast(int is_blocking, void *buffer, int count, MPI_Datatype datatype,
                              int root, MPI_Comm comm)
{
    if (is_blocking) {
        return MPI_Bcast(buffer, count, datatype, root, comm);
    } else {
        int mpi_errno;
        MPI_Request req = MPI_REQUEST_NULL;

        mpi_errno = MPI_Ibcast(buffer, count, datatype, root, comm, &req);
        if (mpi_errno != MPI_SUCCESS)
            return mpi_errno;
        mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
        return mpi_errno;
    }
}

static inline int MTest_Gather(int is_blocking, const void *sendbuf, int sendcount,
                               MPI_Datatype sendtype, void *recvbuf, int recvcount,
                               MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    if (is_blocking) {
        return MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    } else {
        int mpi_errno;
        MPI_Request req = MPI_REQUEST_NULL;

        mpi_errno = MPI_Igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                root, comm, &req);
        if (mpi_errno != MPI_SUCCESS)
            return mpi_errno;
        mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
        return mpi_errno;
    }
}

static inline int MTest_Gatherv(int is_blocking, const void *sendbuf, int sendcount,
                                MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                                const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    if (is_blocking) {
        return MPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                           recvtype, root, comm);
    } else {
        int mpi_errno;
        MPI_Request req = MPI_REQUEST_NULL;

        mpi_errno = MPI_Igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                                 recvtype, root, comm, &req);
        if (mpi_errno != MPI_SUCCESS)
            return mpi_errno;
        mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
        return mpi_errno;
    }
}

static inline int MTest_Scatter(int is_blocking, const void *sendbuf, int sendcount,
                                MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    if (is_blocking) {
        return MPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    } else {
        int mpi_errno;
        MPI_Request req = MPI_REQUEST_NULL;

        mpi_errno = MPI_Iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                 recvtype, root, comm, &req);
        if (mpi_errno != MPI_SUCCESS)
            return mpi_errno;
        mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
        return mpi_errno;
    }
}

static inline int MTest_Scatterv(int is_blocking, const void *sendbuf, const int *sendcounts,
                                 const int *displs, MPI_Datatype sendtype, void *recvbuf,
                                 int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    if (is_blocking) {
        return MPI_Scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                            recvcount, recvtype, root, comm);
    } else {
        int mpi_errno;
        MPI_Request req = MPI_REQUEST_NULL;

        mpi_errno = MPI_Iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                  recvcount, recvtype, root, comm, &req);
        if (mpi_errno != MPI_SUCCESS)
            return mpi_errno;
        mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
        return mpi_errno;
    }
}

static inline int MTest_Allgather(int is_blocking, const void *sendbuf, int sendcount,
                                  MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                  MPI_Datatype recvtype, MPI_Comm comm)
{
    if (is_blocking) {
        return MPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    } else {
        int mpi_errno;
        MPI_Request req = MPI_REQUEST_NULL;

        mpi_errno = MPI_Iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                   recvtype, comm, &req);
        if (mpi_errno != MPI_SUCCESS)
            return mpi_errno;
        mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
        return mpi_errno;
    }
}

static inline int MTest_Allgatherv(int is_blocking, const void *sendbuf, int sendcount,
                                   MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                                   const int *displs, MPI_Datatype recvtype, MPI_Comm comm)
{
    if (is_blocking) {
        return MPI_Allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                              displs, recvtype, comm);
    } else {
        int mpi_errno;
        MPI_Request req = MPI_REQUEST_NULL;

        mpi_errno = MPI_Iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                    displs, recvtype, comm, &req);
        if (mpi_errno != MPI_SUCCESS)
            return mpi_errno;
        mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
        return mpi_errno;
    }
}

static inline int MTest_Alltoall(int is_blocking, const void *sendbuf, int sendcount,
                                 MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                 MPI_Datatype recvtype, MPI_Comm comm)
{
    if (is_blocking) {
        return MPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    } else {
        int mpi_errno;
        MPI_Request req = MPI_REQUEST_NULL;

        mpi_errno = MPI_Ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                  recvtype, comm, &req);
        if (mpi_errno != MPI_SUCCESS)
            return mpi_errno;
        mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
        return mpi_errno;
    }
}

static inline int MTest_Alltoallv(int is_blocking, const void *sendbuf, const int *sendcounts,
                                  const int *sdispls, MPI_Datatype sendtype, void *recvbuf,
                                  const int *recvcounts, const int *rdispls, MPI_Datatype recvtype,
                                  MPI_Comm comm)
{
    if (is_blocking) {
        return MPI_Alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                             recvcounts, rdispls, recvtype, comm);
    } else {
        int mpi_errno;
        MPI_Request req = MPI_REQUEST_NULL;

        mpi_errno = MPI_Ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                   recvcounts, rdispls, recvtype, comm, &req);
        if (mpi_errno != MPI_SUCCESS)
            return mpi_errno;
        mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
        return mpi_errno;
    }
}

static inline int MTest_Alltoallw(int is_blocking, const void *sendbuf, const int *sendcounts,
                                  const int *sdispls, const MPI_Datatype * sendtypes, void *recvbuf,
                                  const int *recvcounts, const int *rdispls,
                                  const MPI_Datatype * recvtypes, MPI_Comm comm)
{
    if (is_blocking) {
        return MPI_Alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                             recvcounts, rdispls, recvtypes, comm);
    } else {
        int mpi_errno;
        MPI_Request req = MPI_REQUEST_NULL;

        mpi_errno = MPI_Ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                   recvcounts, rdispls, recvtypes, comm, &req);
        if (mpi_errno != MPI_SUCCESS)
            return mpi_errno;
        mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
        return mpi_errno;
    }
}

static inline int MTest_Reduce(int is_blocking, const void *sendbuf, void *recvbuf, int count,
                               MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
    if (is_blocking) {
        return MPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
    } else {
        int mpi_errno;
        MPI_Request req = MPI_REQUEST_NULL;

        mpi_errno = MPI_Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, &req);
        if (mpi_errno != MPI_SUCCESS)
            return mpi_errno;
        mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
        return mpi_errno;
    }
}

static inline int MTest_Allreduce(int is_blocking, const void *sendbuf, void *recvbuf, int count,
                                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    if (is_blocking) {
        return MPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
    } else {
        int mpi_errno;
        MPI_Request req = MPI_REQUEST_NULL;

        mpi_errno = MPI_Iallreduce(sendbuf, recvbuf, count, datatype, op, comm, &req);
        if (mpi_errno != MPI_SUCCESS)
            return mpi_errno;
        mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
        return mpi_errno;
    }
}

static inline int MTest_Reduce_scatter(int is_blocking, const void *sendbuf, void *recvbuf,
                                       const int *recvcounts, MPI_Datatype datatype, MPI_Op op,
                                       MPI_Comm comm)
{
    if (is_blocking) {
        return MPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm);
    } else {
        int mpi_errno;
        MPI_Request req = MPI_REQUEST_NULL;

        mpi_errno = MPI_Ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, &req);
        if (mpi_errno != MPI_SUCCESS)
            return mpi_errno;
        mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
        return mpi_errno;
    }
}

static inline int MTest_Reduce_scatter_block(int is_blocking, const void *sendbuf, void *recvbuf,
                                             int recvcount, MPI_Datatype datatype, MPI_Op op,
                                             MPI_Comm comm)
{
    if (is_blocking) {
        return MPI_Reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm);
    } else {
        int mpi_errno;
        MPI_Request req = MPI_REQUEST_NULL;

        mpi_errno =
            MPI_Ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm, &req);
        if (mpi_errno != MPI_SUCCESS)
            return mpi_errno;
        mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
        return mpi_errno;
    }
}

static inline int MTest_Scan(int is_blocking, const void *sendbuf, void *recvbuf, int count,
                             MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    if (is_blocking) {
        return MPI_Scan(sendbuf, recvbuf, count, datatype, op, comm);
    } else {
        int mpi_errno;
        MPI_Request req = MPI_REQUEST_NULL;

        mpi_errno = MPI_Iscan(sendbuf, recvbuf, count, datatype, op, comm, &req);
        if (mpi_errno != MPI_SUCCESS)
            return mpi_errno;
        mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
        return mpi_errno;
    }
}

static inline int MTest_Exscan(int is_blocking, const void *sendbuf, void *recvbuf, int count,
                               MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    if (is_blocking) {
        return MPI_Exscan(sendbuf, recvbuf, count, datatype, op, comm);
    } else {
        int mpi_errno;
        MPI_Request req = MPI_REQUEST_NULL;

        mpi_errno = MPI_Iexscan(sendbuf, recvbuf, count, datatype, op, comm, &req);
        if (mpi_errno != MPI_SUCCESS)
            return mpi_errno;
        mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
        return mpi_errno;
    }
}
#else
/* If MPI_VERSION is less than 3,
 * wrap up MTest_Collective calls by traditional blocking collectives.*/

static inline int MTest_Barrier(int is_blocking, MPI_Comm comm)
{
    return MPI_Barrier(comm);
}

static inline int MTest_Bcast(int is_blocking, void *buffer, int count, MPI_Datatype datatype,
                              int root, MPI_Comm comm)
{
    return MPI_Bcast(buffer, count, datatype, root, comm);
}

static inline int MTest_Gather(int is_blocking, const void *sendbuf, int sendcount,
                               MPI_Datatype sendtype, void *recvbuf, int recvcount,
                               MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    return MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

static inline int MTest_Gatherv(int is_blocking, const void *sendbuf, int sendcount,
                                MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                                const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    return MPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                       recvtype, root, comm);
}

static inline int MTest_Scatter(int is_blocking, const void *sendbuf, int sendcount,
                                MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    return MPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

static inline int MTest_Scatterv(int is_blocking, const void *sendbuf, const int *sendcounts,
                                 const int *displs, MPI_Datatype sendtype, void *recvbuf,
                                 int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    return MPI_Scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                        recvcount, recvtype, root, comm);
}

static inline int MTest_Allgather(int is_blocking, const void *sendbuf, int sendcount,
                                  MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                  MPI_Datatype recvtype, MPI_Comm comm)
{
    return MPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

static inline int MTest_Allgatherv(int is_blocking, const void *sendbuf, int sendcount,
                                   MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                                   const int *displs, MPI_Datatype recvtype, MPI_Comm comm)
{
    return MPI_Allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                          displs, recvtype, comm);
}

static inline int MTest_Alltoall(int is_blocking, const void *sendbuf, int sendcount,
                                 MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                 MPI_Datatype recvtype, MPI_Comm comm)
{
    return MPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

static inline int MTest_Alltoallv(int is_blocking, const void *sendbuf, const int *sendcounts,
                                  const int *sdispls, MPI_Datatype sendtype, void *recvbuf,
                                  const int *recvcounts, const int *rdispls, MPI_Datatype recvtype,
                                  MPI_Comm comm)
{
    return MPI_Alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                         recvcounts, rdispls, recvtype, comm);
}

static inline int MTest_Alltoallw(int is_blocking, const void *sendbuf, const int *sendcounts,
                                  const int *sdispls, const MPI_Datatype * sendtypes, void *recvbuf,
                                  const int *recvcounts, const int *rdispls,
                                  const MPI_Datatype * recvtypes, MPI_Comm comm)
{
    return MPI_Alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                         recvcounts, rdispls, recvtypes, comm);
}

static inline int MTest_Reduce(int is_blocking, const void *sendbuf, void *recvbuf, int count,
                               MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
    return MPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
}

static inline int MTest_Allreduce(int is_blocking, const void *sendbuf, void *recvbuf, int count,
                                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    return MPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
}

static inline int MTest_Reduce_scatter(int is_blocking, const void *sendbuf, void *recvbuf,
                                       const int *recvcounts, MPI_Datatype datatype, MPI_Op op,
                                       MPI_Comm comm)
{
    return MPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm);
}

static inline int MTest_Reduce_scatter_block(int is_blocking, const void *sendbuf, void *recvbuf,
                                             int recvcount, MPI_Datatype datatype, MPI_Op op,
                                             MPI_Comm comm)
{
    return MPI_Reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm);
}

static inline int MTest_Scan(int is_blocking, const void *sendbuf, void *recvbuf, int count,
                             MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    return MPI_Scan(sendbuf, recvbuf, count, datatype, op, comm);
}

static inline int MTest_Exscan(int is_blocking, const void *sendbuf, void *recvbuf, int count,
                               MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    return MPI_Exscan(sendbuf, recvbuf, count, datatype, op, comm);
}

#endif /* MPI_VERSION >= 3 */
#endif /* MPICOLLTEST_H_INCLUDED */
