/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* A PMPI-based "adapter" that intercepts the traditional blocking collective
 * operations and implements them via PMPIX_Icollective/PMPI_Wait.  This permits
 * the use of a wide variety of MPI collective test suites and benchmarking
 * programs with the newer nonblocking collectives.
 *
 * Author: Dave Goodell <goodell@mcs.anl.gov>
 */

#include "mpi.h"

int MPI_Barrier(MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Ibarrier(comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Ibcast(buffer, count, datatype, root, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Gather(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Gatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Scatterv(void *sendbuf, int *sendcounts, int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *displs, MPI_Datatype recvtype, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Alltoall(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Alltoallv(void *sendbuf, int *sendcounts, int *sdispls, MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Alltoallw(void *sendbuf, int *sendcounts, int *sdispls, MPI_Datatype *sendtypes, void *recvbuf, int *recvcounts, int *rdispls, MPI_Datatype *recvtypes, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Allreduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Iallreduce(sendbuf, recvbuf, count, datatype, op, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Reduce_scatter(void *sendbuf, void *recvbuf, int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Reduce_scatter_block(void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Scan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Iscan(sendbuf, recvbuf, count, datatype, op, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Exscan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Iexscan(sendbuf, recvbuf, count, datatype, op, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

