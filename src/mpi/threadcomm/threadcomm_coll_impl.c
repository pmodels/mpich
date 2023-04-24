/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#ifdef ENABLE_THREADCOMM

#define BARRIER_COUNTER(i, j) (MPL_atomic_int_t *) ((char *)(threadcomm->in_counters) + (i*P+j) * MPL_CACHELINE_SIZE)

static void thread_barrier(MPIR_Threadcomm * threadcomm)
{
    int P = threadcomm->num_threads;
    int tid = MPIR_threadcomm_get_tid(threadcomm);

    /* dissemination */
    int mask = 0x1;
    while (mask < P) {
        int dst = (tid + mask) % P;
        int src = (tid - mask + P) % P;
        MPL_atomic_fetch_add_int(BARRIER_COUNTER(tid, dst), 1);
        while (MPL_atomic_load_int(BARRIER_COUNTER(src, tid)) < 1) {
        }
        MPL_atomic_fetch_add_int(BARRIER_COUNTER(src, tid), -1);
        mask <<= 1;
    }
}

int MPIR_Threadcomm_barrier_impl(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm->local_size == 1) {
        thread_barrier(comm->threadcomm);
    } else {
        mpi_errno = MPIR_Barrier_intra_dissemination(comm, MPIR_ERR_NONE);
    }

    return mpi_errno;
}

int MPIR_Threadcomm_bcast_impl(void *buffer, MPI_Aint count, MPI_Datatype datatype,
                               int root, MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Bcast_intra_binomial(buffer, count, datatype, root, comm, MPIR_ERR_NONE);

    return mpi_errno;
}

int MPIR_Threadcomm_gather_impl(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                int root, MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Gather_intra_binomial(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, root, comm, MPIR_ERR_NONE);

    return mpi_errno;
}

int MPIR_Threadcomm_gatherv_impl(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                 void *recvbuf, const MPI_Aint * recvcounts,
                                 const MPI_Aint * displs, MPI_Datatype recvtype,
                                 int root, MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Gatherv_allcomm_linear(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcounts, displs, recvtype, root,
                                            comm, MPIR_ERR_NONE);

    return mpi_errno;
}

int MPIR_Threadcomm_scatter_impl(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                 void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                 int root, MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Scatter_intra_binomial(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcount, recvtype, root,
                                            comm, MPIR_ERR_NONE);

    return mpi_errno;
}

int MPIR_Threadcomm_scatterv_impl(const void *sendbuf, const MPI_Aint * sendcounts,
                                  const MPI_Aint * displs, MPI_Datatype sendtype,
                                  void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                  int root, MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Scatterv_allcomm_linear(sendbuf, sendcounts, displs, sendtype,
                                             recvbuf, recvcount, recvtype, root,
                                             comm, MPIR_ERR_NONE);

    return mpi_errno;
}

int MPIR_Threadcomm_allgather_impl(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                   MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgather_intra_brucks(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcount, recvtype, comm, MPIR_ERR_NONE);

    return mpi_errno;
}

int MPIR_Threadcomm_allgatherv_impl(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                    void *recvbuf, const MPI_Aint * recvcounts,
                                    const MPI_Aint * displs, MPI_Datatype recvtype,
                                    MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgatherv_intra_brucks(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcounts, displs, recvtype,
                                             comm, MPIR_ERR_NONE);

    return mpi_errno;
}

int MPIR_Threadcomm_alltoall_impl(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                  MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(sendbuf != MPI_IN_PLACE);
    mpi_errno = MPIR_Alltoall_intra_brucks(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, comm, MPIR_ERR_NONE);

    return mpi_errno;
}

int MPIR_Threadcomm_alltoallv_impl(const void *sendbuf, const MPI_Aint * sendcounts,
                                   const MPI_Aint * sdispls, MPI_Datatype sendtype,
                                   void *recvbuf, const MPI_Aint * recvcounts,
                                   const MPI_Aint * rdispls, MPI_Datatype recvtype,
                                   MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(sendbuf != MPI_IN_PLACE);
    mpi_errno = MPIR_Alltoallv_intra_scattered(sendbuf, sendcounts, sdispls, sendtype,
                                               recvbuf, recvcounts, rdispls, recvtype,
                                               comm, MPIR_ERR_NONE);

    return mpi_errno;
}

int MPIR_Threadcomm_alltoallw_impl(const void *sendbuf, const MPI_Aint * sendcounts,
                                   const MPI_Aint * sdispls, const MPI_Datatype * sendtypes,
                                   void *recvbuf, const MPI_Aint * recvcounts,
                                   const MPI_Aint * rdispls, const MPI_Datatype * recvtypes,
                                   MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(sendbuf != MPI_IN_PLACE);
    mpi_errno = MPIR_Alltoallw_intra_scattered(sendbuf, sendcounts, sdispls, sendtypes,
                                               recvbuf, recvcounts, rdispls, recvtypes,
                                               comm, MPIR_ERR_NONE);

    return mpi_errno;
}

int MPIR_Threadcomm_allreduce_impl(const void *sendbuf, void *recvbuf,
                                   MPI_Aint count, MPI_Datatype datatype,
                                   MPI_Op op, MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allreduce_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op,
                                                        comm, MPIR_ERR_NONE);

    return mpi_errno;
}

int MPIR_Threadcomm_reduce_impl(const void *sendbuf, void *recvbuf,
                                MPI_Aint count, MPI_Datatype datatype,
                                MPI_Op op, int root, MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_intra_binomial(sendbuf, recvbuf, count, datatype, op, root,
                                           comm, MPIR_ERR_NONE);

    return mpi_errno;
}

int MPIR_Threadcomm_reduce_scatter_impl(const void *sendbuf, void *recvbuf,
                                        const MPI_Aint * recvcounts, MPI_Datatype datatype,
                                        MPI_Op op, MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(MPIR_Op_is_commutative(op));
    mpi_errno = MPIR_Reduce_scatter_intra_recursive_halving(sendbuf, recvbuf, recvcounts,
                                                            datatype, op, comm, MPIR_ERR_NONE);

    return mpi_errno;
}

int MPIR_Threadcomm_reduce_scatter_block_impl(const void *sendbuf, void *recvbuf,
                                              MPI_Aint recvcount, MPI_Datatype datatype,
                                              MPI_Op op, MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(MPIR_Op_is_commutative(op));
    mpi_errno = MPIR_Reduce_scatter_block_intra_recursive_halving(sendbuf, recvbuf, recvcount,
                                                                  datatype, op,
                                                                  comm, MPIR_ERR_NONE);

    return mpi_errno;
}

int MPIR_Threadcomm_scan_impl(const void *sendbuf, void *recvbuf,
                              MPI_Aint count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Scan_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op,
                                                   comm, MPIR_ERR_NONE);

    return mpi_errno;
}

int MPIR_Threadcomm_exscan_impl(const void *sendbuf, void *recvbuf,
                                MPI_Aint count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Exscan_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op,
                                                     comm, MPIR_ERR_NONE);

    return mpi_errno;
}

#endif
