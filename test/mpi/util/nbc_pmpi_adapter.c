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
#include "mpitest.h"

/* Temporary hack to make this test work with the option to define MPICH2_CONST
   Used to test the MPI-3 binding that added const to some declarations. */
#ifdef MPICH2_CONST
#define MPITEST_CONST MPICH2_CONST
#else
#define MPITEST_CONST
#endif

#if !defined(USE_STRICT_MPI) && defined(MPICH2)

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

int MPI_Gather(MPITEST_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Gatherv(MPITEST_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, MPITEST_CONST int *recvcounts, MPITEST_CONST int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Scatter(MPITEST_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Scatterv(MPITEST_CONST void *sendbuf, MPITEST_CONST int *sendcounts, MPITEST_CONST int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Allgather(MPITEST_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Allgatherv(MPITEST_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, MPITEST_CONST int *recvcounts, MPITEST_CONST int *displs, MPI_Datatype recvtype, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Alltoall(MPITEST_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Alltoallv(MPITEST_CONST void *sendbuf, MPITEST_CONST int *sendcounts, MPITEST_CONST int *sdispls, MPI_Datatype sendtype, void *recvbuf, MPITEST_CONST int *recvcounts, MPITEST_CONST int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Alltoallw(MPITEST_CONST void *sendbuf, MPITEST_CONST int *sendcounts, MPITEST_CONST int *sdispls, MPITEST_CONST MPI_Datatype *sendtypes, void *recvbuf, MPITEST_CONST int *recvcounts, MPITEST_CONST int *rdispls, MPITEST_CONST MPI_Datatype *recvtypes, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Reduce(MPITEST_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Allreduce(MPITEST_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Iallreduce(sendbuf, recvbuf, count, datatype, op, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Reduce_scatter(MPITEST_CONST void *sendbuf, void *recvbuf, MPITEST_CONST int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Reduce_scatter_block(MPITEST_CONST void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Scan(MPITEST_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Iscan(sendbuf, recvbuf, count, datatype, op, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

int MPI_Exscan(MPITEST_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno;
    MPI_Request req = MPI_REQUEST_NULL;

    mpi_errno = PMPIX_Iexscan(sendbuf, recvbuf, count, datatype, op, comm, &req);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = PMPI_Wait(&req, MPI_STATUS_IGNORE);
    return mpi_errno;
}

#endif
