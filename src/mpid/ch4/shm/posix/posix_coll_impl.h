/* -*- Mode: C ; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_COLL_IMPL_H_INCLUDED
#define POSIX_COLL_IMPL_H_INCLUDED

#include "posix_coll_params.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Barrier_recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Barrier_recursive_doubling(MPIR_Comm * comm_ptr,
                                           MPIR_Errflag_t * errflag,
                                           MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Barrier_recursive_doubling(comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Bcast_binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Bcast_binomial(void *buffer,
                               int count,
                               MPI_Datatype datatype,
                               int root,
                               MPIR_Comm * comm_ptr,
                               MPIR_Errflag_t * errflag,
                               MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Bcast_binomial(buffer, count, datatype, root, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Bcast_scatter_doubling_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Bcast_scatter_doubling_allgather(void *buffer,
                                                 int count,
                                                 MPI_Datatype datatype,
                                                 int root,
                                                 MPIR_Comm * comm_ptr,
                                                 MPIR_Errflag_t * errflag,
                                                 MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Bcast_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Bcast_scatter_doubling_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Bcast_scatter_ring_allgather(void *buffer,
                                             int count,
                                             MPI_Datatype datatype,
                                             int root,
                                             MPIR_Comm * comm_ptr,
                                             MPIR_Errflag_t * errflag,
                                             MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Bcast_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_allreduce_recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_allreduce_recursive_doubling(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op,
                                             MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                             MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allreduce_recursive_doubling(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_allreduce_reduce_scatter_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_allreduce_reduce_scatter_allgather(const void *sendbuf, void *recvbuf, int count,
                                                   MPI_Datatype datatype, MPI_Op op,
                                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                   MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allreduce_reduce_scatter_allgather(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_reduce_redscat_gather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_reduce_redscat_gather(const void *sendbuf, void *recvbuf, int count,
                                      MPI_Datatype datatype, MPI_Op op, int root,
                                      MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                      MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_redscat_gather(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_reduce_binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_reduce_binomial(const void *sendbuf, void *recvbuf, int count,
                                MPI_Datatype datatype, MPI_Op op, int root,
                                MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_binomial(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_alltoall_bruck
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
    int MPIDI_POSIX_alltoall_bruck(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                   MPIDI_POSIX_coll_algo_container_t *
                                   params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoall_bruck(sendbuf, sendcount, sendtype,
                                    recvbuf, recvcount, recvtype, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_alltoall_isend_irecv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
    int MPIDI_POSIX_alltoall_isend_irecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                         MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                         MPIDI_POSIX_coll_algo_container_t *
                                         params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoall_isend_irecv(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcount, recvtype, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_alltoall_pairwise_exchange
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
    int MPIDI_POSIX_alltoall_pairwise_exchange(const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                               MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                               MPIR_Errflag_t * errflag,
                                               MPIDI_POSIX_coll_algo_container_t *
                                               params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoall_pairwise_exchange(sendbuf, sendcount, sendtype,
                                                recvbuf, recvcount, recvtype, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_alltoall_sendrecv_replace
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
    int MPIDI_POSIX_alltoall_sendrecv_replace(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                              MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                              MPIR_Errflag_t * errflag,
                                              MPIDI_POSIX_coll_algo_container_t *
                                              params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoall_sendrecv_replace(sendbuf, sendcount, sendtype,
                                               recvbuf, recvcount, recvtype, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_alltoallv_sendrecv_replace
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
    int MPIDI_POSIX_alltoallv_sendrecv_replace(const void *sendbuf, const int *sendcounts,
                                               const int *sdispls, MPI_Datatype sendtype,
                                               void *recvbuf, const int *recvcounts,
                                               const int *rdispls, MPI_Datatype recvtype,
                                               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                               MPIDI_POSIX_coll_algo_container_t *
                                               params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoallv_sendrecv_replace(sendbuf, sendcounts, sdispls, sendtype,
                                                recvbuf, recvcounts, rdispls, recvtype,
                                                comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_alltoallv_isend_irecv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
    int MPIDI_POSIX_alltoallv_isend_irecv(const void *sendbuf, const int *sendcounts,
                                          const int *sdispls, MPI_Datatype sendtype,
                                          void *recvbuf, const int *recvcounts,
                                          const int *rdispls, MPI_Datatype recvtype,
                                          MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                          MPIDI_POSIX_coll_algo_container_t *
                                          params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoallv_isend_irecv(sendbuf, sendcounts, sdispls, sendtype,
                                           recvbuf, recvcounts, rdispls, recvtype,
                                           comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_alltoallw_sendrecv_replace
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
    int MPIDI_POSIX_alltoallw_sendrecv_replace(const void *sendbuf, const int sendcounts[],
                                               const int sdispls[], const MPI_Datatype sendtypes[],
                                               void *recvbuf, const int recvcounts[],
                                               const int rdispls[], const MPI_Datatype recvtypes[],
                                               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                               MPIDI_POSIX_coll_algo_container_t *
                                               params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoallw_sendrecv_replace(sendbuf, sendcounts, sdispls, sendtypes,
                                                recvbuf, recvcounts, rdispls, recvtypes,
                                                comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_alltoallw_isend_irecv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
    int MPIDI_POSIX_alltoallw_isend_irecv(const void *sendbuf, const int sendcounts[],
                                          const int sdispls[], const MPI_Datatype sendtypes[],
                                          void *recvbuf, const int recvcounts[],
                                          const int rdispls[], const MPI_Datatype recvtypes[],
                                          MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                          MPIDI_POSIX_coll_algo_container_t *
                                          params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoallw_isend_irecv(sendbuf, sendcounts, sdispls, sendtypes,
                                           recvbuf, recvcounts, rdispls, recvtypes,
                                           comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_alltoallw_pairwise_exchange
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
    int MPIDI_POSIX_alltoallw_pairwise_exchange(const void *sendbuf, const int sendcounts[],
                                                const int sdispls[], const MPI_Datatype sendtypes[],
                                                void *recvbuf, const int recvcounts[],
                                                const int rdispls[], const MPI_Datatype recvtypes[],
                                                MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                MPIDI_POSIX_coll_algo_container_t *
                                                params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoallw_pairwise_exchange(sendbuf, sendcounts, sdispls, sendtypes,
                                                 recvbuf, recvcounts, rdispls, recvtypes,
                                                 comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_allgather_recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
    int MPIDI_POSIX_allgather_recursive_doubling(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 int recvcount, MPI_Datatype recvtype,
                                                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                 MPIDI_POSIX_coll_algo_container_t *
                                                 params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgather_recursive_doubling(sendbuf, sendcount, sendtype,
                                                  recvbuf, recvcount, recvtype, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_allgather_bruck
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
    int MPIDI_POSIX_allgather_bruck(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                    void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                    MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                    MPIDI_POSIX_coll_algo_container_t *
                                    params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgather_bruck(sendbuf, sendcount, sendtype,
                                     recvbuf, recvcount, recvtype, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_allgather_ring
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
    int MPIDI_POSIX_allgather_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                   MPIDI_POSIX_coll_algo_container_t *
                                   params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgather_ring(sendbuf, sendcount, sendtype,
                                    recvbuf, recvcount, recvtype, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_allgatherv_recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
    int MPIDI_POSIX_allgatherv_recursive_doubling(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  const int *recvcounts, const int *displs,
                                                  MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                                  MPIR_Errflag_t * errflag,
                                                  MPIDI_POSIX_coll_algo_container_t *
                                                  params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgatherv_recursive_doubling(sendbuf, sendcount, sendtype,
                                                   recvbuf, recvcounts, displs,
                                                   recvtype, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_allgatherv_bruck
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
    int MPIDI_POSIX_allgatherv_bruck(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                     void *recvbuf, const int *recvcounts, const int *displs,
                                     MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                     MPIR_Errflag_t * errflag,
                                     MPIDI_POSIX_coll_algo_container_t *
                                     params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgatherv_bruck(sendbuf, sendcount, sendtype,
                                      recvbuf, recvcounts, displs, recvtype, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_allgatherv_ring
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
    int MPIDI_POSIX_allgatherv_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                    void *recvbuf, const int *recvcounts, const int *displs,
                                    MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                    MPIR_Errflag_t * errflag,
                                    MPIDI_POSIX_coll_algo_container_t *
                                    params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgatherv_ring(sendbuf, sendcount, sendtype,
                                     recvbuf, recvcounts, displs, recvtype, comm_ptr, errflag);
    return mpi_errno;
}


#endif /* POSIX_COLL_IMPL_H_INCLUDED */
