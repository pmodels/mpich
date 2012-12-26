/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* A PMPI-based "adapter" that intercepts the traditional blocking collective
 * operations and implements them via PMPI_Icollective/PMPI_Wait.  This permits
 * the use of a wide variety of MPI collective test suites and benchmarking
 * programs with the newer nonblocking collectives.
 *
 * Author: Dave Goodell <goodell@mcs.anl.gov>
 */

#include "mpi.h"
#include "mpitest.h"

/* The test on the MPI_VERISON is needed to check for pre MPI-3 versions
   of MPICH, which will not include const in declarations or 
   the non-blocking collectives.  
 */
#if !defined(USE_STRICT_MPI) && defined(MPICH) && MPI_VERSION >= 3

int MPI_Barrier(MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPI_Ibarrier(comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPI_Ibcast(buffer, count, datatype, root, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPI_Igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPI_Igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPI_Iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Scatterv(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPI_Iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPI_Iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPI_Iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPI_Ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPI_Ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Alltoallw(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPI_Ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPI_Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPI_Iallreduce(sendbuf, recvbuf, count, datatype, op, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Reduce_scatter(const void *sendbuf, void *recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPI_Ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPI_Ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPI_Iscan(sendbuf, recvbuf, count, datatype, op, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Exscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPI_Iexscan(sendbuf, recvbuf, count, datatype, op, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

#endif
