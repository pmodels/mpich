/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_COLL_H_INCLUDED
#define POSIX_COLL_H_INCLUDED

#include "posix_impl.h"
#include "ch4_impl.h"
#include "ch4_coll_select.h"
#include "posix_coll_params.h"
#include "posix_coll_select.h"
#include "posix_coll_release_gather.h"

static inline int MPIDI_POSIX_mpi_barrier(MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                          const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Barrier_select(comm, errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Barrier_intra_dissemination_id:
            mpi_errno = MPIR_Barrier_intra_dissemination(comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Barrier_impl(comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_POSIX_mpi_bcast(void *buffer, int count, MPI_Datatype datatype,
                                        int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                        const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Bcast_select(buffer, count, datatype, root, comm, errflag,
                                 ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Bcast_intra_binomial_id:
            mpi_errno = MPIR_Bcast_intra_binomial(buffer, count, datatype, root, comm, errflag);
            break;
        case MPIDI_POSIX_Bcast_intra_scatter_recursive_doubling_allgather_id:
            mpi_errno =
                MPIR_Bcast_intra_scatter_recursive_doubling_allgather(buffer, count, datatype,
                                                                      root, comm, errflag);
            break;
        case MPIDI_POSIX_Bcast_intra_scatter_ring_allgather_id:
            mpi_errno =
                MPIR_Bcast_intra_scatter_ring_allgather(buffer, count, datatype,
                                                        root, comm, errflag);
            break;
        case MPIDI_POSIX_Bcast_intra_release_gather_id:
            mpi_errno =
                MPIDI_POSIX_mpi_bcast_release_gather(buffer, count, datatype, root, comm, errflag);
            break;
        case MPIDI_POSIX_Bcast_intra_invalid_id:
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**noizem");
        default:
            mpi_errno = MPIR_Bcast_impl(buffer, count, datatype, root, comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_POSIX_mpi_allreduce(const void *sendbuf, void *recvbuf, int count,
                                            MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                            MPIR_Errflag_t * errflag,
                                            const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Allreduce_select(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                     ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Allreduce_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Allreduce_intra_recursive_doubling(sendbuf, recvbuf, count, datatype,
                                                        op, comm, errflag);
            break;
        case MPIDI_POSIX_Allreduce_intra_reduce_scatter_allgather_id:
            mpi_errno =
                MPIR_Allreduce_intra_reduce_scatter_allgather(sendbuf, recvbuf, count,
                                                              datatype, op, comm, errflag);
            break;
        case MPIDI_POSIX_Allreduce_intra_release_gather_id:
            mpi_errno =
                MPIDI_POSIX_mpi_allreduce_release_gather(sendbuf, recvbuf, count,
                                                         datatype, op, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Allreduce_impl(sendbuf, recvbuf, count, datatype, op, comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_POSIX_mpi_allgather(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                            MPI_Datatype recvtype, MPIR_Comm * comm,
                                            MPIR_Errflag_t * errflag,
                                            const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Allgather_select(sendbuf, sendcount, sendtype,
                                     recvbuf, recvcount, recvtype,
                                     comm, errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Allgather_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Allgather_intra_recursive_doubling(sendbuf, sendcount, sendtype,
                                                        recvbuf, recvcount, recvtype,
                                                        comm, errflag);
            break;
        case MPIDI_POSIX_Allgather_intra_brucks_id:
            mpi_errno =
                MPIR_Allgather_intra_brucks(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcount, recvtype, comm, errflag);
            break;
        case MPIDI_POSIX_Allgather_intra_ring_id:
            mpi_errno =
                MPIR_Allgather_intra_ring(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcount, recvtype, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Allgather_impl(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcount, recvtype, comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_POSIX_mpi_allgatherv(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf,
                                             const int *recvcounts, const int *displs,
                                             MPI_Datatype recvtype, MPIR_Comm * comm,
                                             MPIR_Errflag_t * errflag,
                                             const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Allgatherv_select(sendbuf, sendcount, sendtype,
                                      recvbuf, recvcounts, displs,
                                      recvtype, comm, errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Allgatherv_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Allgatherv_intra_recursive_doubling(sendbuf, sendcount, sendtype,
                                                         recvbuf, recvcounts, displs,
                                                         recvtype, comm, errflag);
            break;
        case MPIDI_POSIX_Allgatherv_intra_brucks_id:
            mpi_errno =
                MPIR_Allgatherv_intra_brucks(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcounts, displs, recvtype, comm, errflag);
            break;
        case MPIDI_POSIX_Allgatherv_intra_ring_id:
            mpi_errno =
                MPIR_Allgatherv_intra_ring(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcounts, displs, recvtype, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Allgatherv_impl(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcounts, displs, recvtype, comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_POSIX_mpi_gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                         int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                         const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Gather_select(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                  recvtype, root, comm, errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Gather_intra_binomial_id:
            mpi_errno =
                MPIR_Gather_intra_binomial(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, root, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Gather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                         recvtype, root, comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_POSIX_mpi_gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, const int *recvcounts, const int *displs,
                                          MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                          MPIR_Errflag_t * errflag,
                                          const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Gatherv_select(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcounts, displs, recvtype, root,
                                   comm, errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Gatherv_allcomm_linear_id:
            mpi_errno =
                MPIR_Gatherv_allcomm_linear(sendbuf, sendcount, sendtype, recvbuf,
                                            recvcounts, displs, recvtype, root, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Gatherv_impl(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                          displs, recvtype, root, comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

static inline int MPIDI_POSIX_mpi_scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                          int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                          const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Scatter_select(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                   recvtype, root, comm, errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Scatter_intra_binomial_id:
            mpi_errno =
                MPIR_Scatter_intra_binomial(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcount, recvtype, root, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Scatter_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                          recvtype, root, comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_POSIX_mpi_scatterv(const void *sendbuf, const int *sendcounts,
                                           const int *displs, MPI_Datatype sendtype,
                                           void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                           int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                           const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Scatterv_select(sendbuf, sendcounts, displs, sendtype,
                                    recvbuf, recvcount, recvtype, root,
                                    comm, errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Scatterv_allcomm_linear_id:
            mpi_errno =
                MPIR_Scatterv_allcomm_linear(sendbuf, sendcounts, displs, sendtype,
                                             recvbuf, recvcount, recvtype, root, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Scatterv_impl(sendbuf, sendcounts, displs, sendtype,
                                           recvbuf, recvcount, recvtype, root, comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_POSIX_mpi_alltoall(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                           MPI_Datatype recvtype, MPIR_Comm * comm,
                                           MPIR_Errflag_t * errflag,
                                           const void *ch4_algo_parameters_container_in)
{
    int mpi_errno;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Alltoall_select(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                    recvtype, comm, errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Alltoall_intra_brucks_id:
            mpi_errno =
                MPIR_Alltoall_intra_brucks(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, comm, errflag);
            break;
        case MPIDI_POSIX_Alltoall_intra_scattered_id:
            mpi_errno =
                MPIR_Alltoall_intra_scattered(sendbuf, sendcount, sendtype,
                                              recvbuf, recvcount, recvtype, comm, errflag);
            break;
        case MPIDI_POSIX_Alltoall_intra_pairwise_id:
            mpi_errno =
                MPIR_Alltoall_intra_pairwise(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcount, recvtype, comm, errflag);
            break;
        case MPIDI_POSIX_Alltoall_intra_pairwise_sendrecv_replace_id:
            mpi_errno =
                MPIR_Alltoall_intra_pairwise_sendrecv_replace(sendbuf, sendcount, sendtype,
                                                              recvbuf, recvcount, recvtype,
                                                              comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Alltoall_impl(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_POSIX_mpi_alltoallv(const void *sendbuf, const int *sendcounts,
                                            const int *sdispls, MPI_Datatype sendtype,
                                            void *recvbuf, const int *recvcounts,
                                            const int *rdispls, MPI_Datatype recvtype,
                                            MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                            const void *ch4_algo_parameters_container_in)
{
    int mpi_errno;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Alltoallv_select(sendbuf, sendcounts, sdispls,
                                     sendtype, recvbuf, recvcounts,
                                     rdispls, recvtype, comm,
                                     errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Alltoallv_intra_pairwise_sendrecv_replace_id:
            mpi_errno =
                MPIR_Alltoallv_intra_pairwise_sendrecv_replace(sendbuf, sendcounts, sdispls,
                                                               sendtype, recvbuf, recvcounts,
                                                               rdispls, recvtype, comm, errflag);
            break;
        case MPIDI_POSIX_Alltoallv_intra_scattered_id:
            mpi_errno =
                MPIR_Alltoallv_intra_scattered(sendbuf, sendcounts, sdispls,
                                               sendtype, recvbuf, recvcounts,
                                               rdispls, recvtype, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Alltoallv_impl(sendbuf, sendcounts, sdispls,
                                            sendtype, recvbuf, recvcounts,
                                            rdispls, recvtype, comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_POSIX_mpi_alltoallw(const void *sendbuf, const int sendcounts[],
                                            const int sdispls[], const MPI_Datatype sendtypes[],
                                            void *recvbuf, const int recvcounts[],
                                            const int rdispls[], const MPI_Datatype recvtypes[],
                                            MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                            const void *ch4_algo_parameters_container_in)
{
    int mpi_errno;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Alltoallw_select(sendbuf, sendcounts, sdispls,
                                     sendtypes, recvbuf, recvcounts,
                                     rdispls, recvtypes, comm,
                                     errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Alltoallw_intra_pairwise_sendrecv_replace_id:
            mpi_errno =
                MPIR_Alltoallw_intra_pairwise_sendrecv_replace(sendbuf, sendcounts, sdispls,
                                                               sendtypes, recvbuf,
                                                               recvcounts, rdispls,
                                                               recvtypes, comm, errflag);
            break;
        case MPIDI_POSIX_Alltoallw_intra_scattered_id:
            mpi_errno =
                MPIR_Alltoallw_intra_scattered(sendbuf, sendcounts, sdispls,
                                               sendtypes, recvbuf, recvcounts,
                                               rdispls, recvtypes, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Alltoallw_impl(sendbuf, sendcounts, sdispls,
                                            sendtypes, recvbuf, recvcounts,
                                            rdispls, recvtypes, comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_POSIX_mpi_reduce(const void *sendbuf, void *recvbuf, int count,
                                         MPI_Datatype datatype, MPI_Op op, int root,
                                         MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                         const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Reduce_select(sendbuf, recvbuf, count, datatype, op, root, comm, errflag,
                                  ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Reduce_intra_reduce_scatter_gather_id:
            mpi_errno =
                MPIR_Reduce_intra_reduce_scatter_gather(sendbuf, recvbuf, count, datatype,
                                                        op, root, comm, errflag);
            break;
        case MPIDI_POSIX_Reduce_intra_binomial_id:
            mpi_errno =
                MPIR_Reduce_intra_binomial(sendbuf, recvbuf, count, datatype,
                                           op, root, comm, errflag);
            break;
        case MPIDI_POSIX_Reduce_intra_release_gather_id:
            mpi_errno =
                MPIDI_POSIX_mpi_reduce_release_gather(sendbuf, recvbuf, count, datatype,
                                                      op, root, comm, errflag);
            break;
        case MPIDI_POSIX_Reduce_intra_invalid_id:
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**noizem");
        default:
            mpi_errno = MPIR_Reduce_impl(sendbuf, recvbuf, count, datatype, op,
                                         root, comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_POSIX_mpi_reduce_scatter(const void *sendbuf, void *recvbuf,
                                                 const int recvcounts[], MPI_Datatype datatype,
                                                 MPI_Op op, MPIR_Comm * comm,
                                                 MPIR_Errflag_t * errflag,
                                                 const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Reduce_scatter_select(sendbuf, recvbuf, recvcounts, datatype, op, comm,
                                          errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Reduce_scatter_intra_noncommutative_id:
            mpi_errno =
                MPIR_Reduce_scatter_intra_noncommutative(sendbuf, recvbuf, recvcounts,
                                                         datatype, op, comm, errflag);
            break;
        case MPIDI_POSIX_Reduce_scatter_intra_pairwise_id:
            mpi_errno =
                MPIR_Reduce_scatter_intra_pairwise(sendbuf, recvbuf, recvcounts,
                                                   datatype, op, comm, errflag);
            break;
        case MPIDI_POSIX_Reduce_scatter_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Reduce_scatter_intra_recursive_doubling(sendbuf, recvbuf, recvcounts,
                                                             datatype, op, comm, errflag);
            break;
        case MPIDI_POSIX_Reduce_scatter_intra_recursive_halving_id:
            mpi_errno =
                MPIR_Reduce_scatter_intra_recursive_halving(sendbuf, recvbuf, recvcounts,
                                                            datatype, op, comm, errflag);
            break;
        default:
            mpi_errno =
                MPIR_Reduce_scatter_impl(sendbuf, recvbuf, recvcounts, datatype, op, comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_POSIX_mpi_reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                       int recvcount, MPI_Datatype datatype,
                                                       MPI_Op op, MPIR_Comm * comm,
                                                       MPIR_Errflag_t * errflag,
                                                       const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Reduce_scatter_block_select(sendbuf, recvbuf, recvcount, datatype, op, comm,
                                                errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Reduce_scatter_block_intra_noncommutative_id:
            mpi_errno =
                MPIR_Reduce_scatter_block_intra_noncommutative(sendbuf, recvbuf, recvcount,
                                                               datatype, op, comm, errflag);
            break;
        case MPIDI_POSIX_Reduce_scatter_block_intra_pairwise_id:
            mpi_errno =
                MPIR_Reduce_scatter_block_intra_pairwise(sendbuf, recvbuf, recvcount,
                                                         datatype, op, comm, errflag);
            break;
        case MPIDI_POSIX_Reduce_scatter_block_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Reduce_scatter_block_intra_recursive_doubling(sendbuf, recvbuf,
                                                                   recvcount, datatype, op,
                                                                   comm, errflag);
            break;
        case MPIDI_POSIX_Reduce_scatter_block_intra_recursive_halving_id:
            mpi_errno =
                MPIR_Reduce_scatter_block_intra_recursive_halving(sendbuf, recvbuf,
                                                                  recvcount, datatype, op,
                                                                  comm, errflag);
            break;
        default:
            mpi_errno =
                MPIR_Reduce_scatter_block_impl(sendbuf, recvbuf, recvcount, datatype, op, comm,
                                               errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_POSIX_mpi_scan(const void *sendbuf, void *recvbuf, int count,
                                       MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                       MPIR_Errflag_t * errflag,
                                       const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Scan_select(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Scan_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Scan_intra_recursive_doubling(sendbuf, recvbuf, count, datatype,
                                                   op, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Scan_impl(sendbuf, recvbuf, count, datatype, op, comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_POSIX_mpi_exscan(const void *sendbuf, void *recvbuf, int count,
                                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                         MPIR_Errflag_t * errflag,
                                         const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Exscan_select(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                  ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Exscan_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Exscan_intra_recursive_doubling(sendbuf, recvbuf, count, datatype,
                                                     op, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Exscan_impl(sendbuf, recvbuf, count, datatype, op, comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_POSIX_mpi_neighbor_allgather(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     int recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm)
{
    int mpi_errno;

    mpi_errno =
        MPIR_Neighbor_allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_neighbor_allgatherv(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      const int recvcounts[], const int displs[],
                                                      MPI_Datatype recvtype, MPIR_Comm * comm)
{
    int mpi_errno;

    mpi_errno = MPIR_Neighbor_allgatherv(sendbuf, sendcount, sendtype,
                                         recvbuf, recvcounts, displs, recvtype, comm);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_neighbor_alltoall(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    int recvcount, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm)
{
    int mpi_errno;

    mpi_errno = MPIR_Neighbor_alltoall(sendbuf, sendcount, sendtype,
                                       recvbuf, recvcount, recvtype, comm);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_neighbor_alltoallv(const void *sendbuf, const int sendcounts[],
                                                     const int sdispls[], MPI_Datatype sendtype,
                                                     void *recvbuf, const int recvcounts[],
                                                     const int rdispls[], MPI_Datatype recvtype,
                                                     MPIR_Comm * comm)
{
    int mpi_errno;

    mpi_errno = MPIR_Neighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                        recvbuf, recvcounts, rdispls, recvtype, comm);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_neighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                                                     const MPI_Aint sdispls[],
                                                     const MPI_Datatype sendtypes[], void *recvbuf,
                                                     const int recvcounts[],
                                                     const MPI_Aint rdispls[],
                                                     const MPI_Datatype recvtypes[],
                                                     MPIR_Comm * comm)
{
    int mpi_errno;

    mpi_errno = MPIR_Neighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                        recvbuf, recvcounts, rdispls, recvtypes, comm);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_ineighbor_allgather(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      int recvcount, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ineighbor_allgather(sendbuf, sendcount, sendtype,
                                         recvbuf, recvcount, recvtype, comm, req);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_ineighbor_allgatherv(const void *sendbuf, int sendcount,
                                                       MPI_Datatype sendtype, void *recvbuf,
                                                       const int recvcounts[], const int displs[],
                                                       MPI_Datatype recvtype, MPIR_Comm * comm,
                                                       MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ineighbor_allgatherv(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcounts, displs, recvtype, comm, req);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_ineighbor_alltoall(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     int recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ineighbor_alltoall(sendbuf, sendcount, sendtype,
                                        recvbuf, recvcount, recvtype, comm, req);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_ineighbor_alltoallv(const void *sendbuf, const int sendcounts[],
                                                      const int sdispls[], MPI_Datatype sendtype,
                                                      void *recvbuf, const int recvcounts[],
                                                      const int rdispls[], MPI_Datatype recvtype,
                                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ineighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                         recvbuf, recvcounts, rdispls, recvtype, comm, req);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_ineighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                                                      const MPI_Aint sdispls[],
                                                      const MPI_Datatype sendtypes[], void *recvbuf,
                                                      const int recvcounts[],
                                                      const MPI_Aint rdispls[],
                                                      const MPI_Datatype recvtypes[],
                                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ineighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                         recvbuf, recvcounts, rdispls, recvtypes, comm, req);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_ibarrier(MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ibarrier(comm, req);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_ibcast(void *buffer, int count, MPI_Datatype datatype,
                                         int root, MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ibcast(buffer, count, datatype, root, comm, req);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_iallgather(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                             MPI_Datatype recvtype, MPIR_Comm * comm,
                                             MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Iallgather(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, comm, req);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_iallgatherv(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              const int *recvcounts, const int *displs,
                                              MPI_Datatype recvtype, MPIR_Comm * comm,
                                              MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Iallgatherv(sendbuf, sendcount, sendtype,
                                 recvbuf, recvcounts, displs, recvtype, comm, req);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_ialltoall(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                            MPI_Datatype recvtype, MPIR_Comm * comm,
                                            MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ialltoall(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, comm, req);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_ialltoallv(const void *sendbuf, const int *sendcounts,
                                             const int *sdispls, MPI_Datatype sendtype,
                                             void *recvbuf, const int *recvcounts,
                                             const int *rdispls, MPI_Datatype recvtype,
                                             MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ialltoallv(sendbuf, sendcounts, sdispls,
                                sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, req);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_ialltoallw(const void *sendbuf, const int *sendcounts,
                                             const int *sdispls, const MPI_Datatype sendtypes[],
                                             void *recvbuf, const int *recvcounts,
                                             const int *rdispls, const MPI_Datatype recvtypes[],
                                             MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ialltoallw(sendbuf, sendcounts, sdispls,
                                sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, req);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_iexscan(const void *sendbuf, void *recvbuf, int count,
                                          MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                          MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Iexscan(sendbuf, recvbuf, count, datatype, op, comm, req);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                          int root, MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Igather(sendbuf, sendcount, sendtype, recvbuf,
                             recvcount, recvtype, root, comm, req);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_igatherv(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           const int *recvcounts, const int *displs,
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                           MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Igatherv(sendbuf, sendcount, sendtype,
                              recvbuf, recvcounts, displs, recvtype, root, comm, req);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                        int recvcount, MPI_Datatype datatype,
                                                        MPI_Op op, MPIR_Comm * comm,
                                                        MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm, req);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_ireduce_scatter(const void *sendbuf, void *recvbuf,
                                                  const int recvcounts[], MPI_Datatype datatype,
                                                  MPI_Op op, MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, req);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_ireduce(const void *sendbuf, void *recvbuf, int count,
                                          MPI_Datatype datatype, MPI_Op op, int root,
                                          MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, req);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_iallreduce(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op,
                                             MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Iallreduce(sendbuf, recvbuf, count, datatype, op, comm, req);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_iscan(const void *sendbuf, void *recvbuf, int count,
                                        MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                        MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Iscan(sendbuf, recvbuf, count, datatype, op, comm, req);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_iscatter(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           int recvcount, MPI_Datatype recvtype,
                                           int root, MPIR_Comm * comm, MPIR_Request ** request)
{
    int mpi_errno;

    mpi_errno = MPIR_Iscatter(sendbuf, sendcount, sendtype, recvbuf,
                              recvcount, recvtype, root, comm, request);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_iscatterv(const void *sendbuf, const int *sendcounts,
                                            const int *displs, MPI_Datatype sendtype,
                                            void *recvbuf, int recvcount,
                                            MPI_Datatype recvtype, int root,
                                            MPIR_Comm * comm, MPIR_Request ** request)
{
    int mpi_errno;

    mpi_errno = MPIR_Iscatterv(sendbuf, sendcounts, displs, sendtype,
                               recvbuf, recvcount, recvtype, root, comm, request);

    return mpi_errno;
}

#endif /* POSIX_COLL_H_INCLUDED */
