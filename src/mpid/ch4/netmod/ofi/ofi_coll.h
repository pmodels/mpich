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
#ifndef OFI_COLL_H_INCLUDED
#define OFI_COLL_H_INCLUDED

#include "ofi_impl.h"
#include "ch4_coll_select.h"
#include "ch4_coll_params.h"
#include "ofi_coll_select.h"

static inline int MPIDI_NM_mpi_barrier(MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                       const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;


    nm_algo_parameters_container_out =
        MPIDI_OFI_Barrier_select(comm, errflag, ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Barrier_intra_dissemination_id:
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

static inline int MPIDI_NM_mpi_bcast(void *buffer, int count, MPI_Datatype datatype,
                                     int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                     const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;


    nm_algo_parameters_container_out =
        MPIDI_OFI_Bcast_select(buffer, count, datatype, root, comm, errflag,
                               ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Bcast_intra_binomial_id:
            mpi_errno = MPIR_Bcast_intra_binomial(buffer, count, datatype, root, comm, errflag);
            break;
        case MPIDI_OFI_Bcast_intra_scatter_recursive_doubling_allgather_id:
            mpi_errno =
                MPIR_Bcast_intra_scatter_recursive_doubling_allgather(buffer, count, datatype,
                                                                      root, comm, errflag);
            break;
        case MPIDI_OFI_Bcast_intra_scatter_ring_allgather_id:
            mpi_errno =
                MPIR_Bcast_intra_scatter_ring_allgather(buffer, count, datatype,
                                                        root, comm, errflag);
            break;
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

static inline int MPIDI_NM_mpi_allreduce(const void *sendbuf, void *recvbuf, int count,
                                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                         MPIR_Errflag_t * errflag,
                                         const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;


    nm_algo_parameters_container_out =
        MPIDI_OFI_Allreduce_select(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                   ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Allreduce_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Allreduce_intra_recursive_doubling(sendbuf, recvbuf, count,
                                                        datatype, op, comm, errflag);
            break;
        case MPIDI_OFI_Allreduce_intra_reduce_scatter_allgather_id:
            mpi_errno =
                MPIR_Allreduce_intra_reduce_scatter_allgather(sendbuf, recvbuf, count,
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

static inline int MPIDI_NM_mpi_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                         MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                         const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;


    nm_algo_parameters_container_out =
        MPIDI_OFI_Allgather_select(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcount, recvtype, comm, errflag,
                                   ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Allgather_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Allgather_intra_recursive_doubling(sendbuf, sendcount, sendtype,
                                                        recvbuf, recvcount, recvtype,
                                                        comm, errflag);
            break;
        case MPIDI_OFI_Allgather_intra_brucks_id:
            mpi_errno =
                MPIR_Allgather_intra_brucks(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcount, recvtype, comm, errflag);
            break;
        case MPIDI_OFI_Allgather_intra_ring_id:
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

static inline int MPIDI_NM_mpi_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, const int *recvcounts, const int *displs,
                                          MPI_Datatype recvtype, MPIR_Comm * comm,
                                          MPIR_Errflag_t * errflag,
                                          const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;


    nm_algo_parameters_container_out =
        MPIDI_OFI_Allgatherv_select(sendbuf, sendcount, sendtype,
                                    recvbuf, recvcounts, displs, recvtype, comm, errflag,
                                    ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Allgatherv_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Allgatherv_intra_recursive_doubling(sendbuf, sendcount, sendtype,
                                                         recvbuf, recvcounts, displs,
                                                         recvtype, comm, errflag);
            break;
        case MPIDI_OFI_Allgatherv_intra_brucks_id:
            mpi_errno =
                MPIR_Allgatherv_intra_brucks(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcounts, displs, recvtype, comm, errflag);
            break;
        case MPIDI_OFI_Allgatherv_intra_ring_id:
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

static inline int MPIDI_NM_mpi_gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                      int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                      const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;


    nm_algo_parameters_container_out =
        MPIDI_OFI_Gather_select(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                recvtype, root, comm, errflag, ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Gather_intra_binomial_id:
            mpi_errno =
                MPIR_Gather_intra_binomial(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, root, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Gather_impl(sendbuf, sendcount, sendtype, recvbuf,
                                         recvcount, recvtype, root, comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);


  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_mpi_gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, const int *recvcounts, const int *displs,
                                       MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                       MPIR_Errflag_t * errflag,
                                       const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;


    nm_algo_parameters_container_out =
        MPIDI_OFI_Gatherv_select(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                 displs, recvtype, root, comm, errflag,
                                 ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Gatherv_allcomm_linear_id:
            mpi_errno =
                MPIR_Gatherv_allcomm_linear(sendbuf, sendcount, sendtype, recvbuf,
                                            recvcounts, displs, recvtype, root, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Gatherv_impl(sendbuf, sendcount, sendtype, recvbuf,
                                          recvcounts, displs, recvtype, root, comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);


  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

static inline int MPIDI_NM_mpi_scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                       int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                       const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;


    nm_algo_parameters_container_out =
        MPIDI_OFI_Scatter_select(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                 recvtype, root, comm, errflag, ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Scatter_intra_binomial_id:
            mpi_errno =
                MPIR_Scatter_intra_binomial(sendbuf, sendcount, sendtype, recvbuf,
                                            recvcount, recvtype, root, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Scatter_impl(sendbuf, sendcount, sendtype, recvbuf,
                                          recvcount, recvtype, root, comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);


  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_mpi_scatterv(const void *sendbuf, const int *sendcounts,
                                        const int *displs, MPI_Datatype sendtype,
                                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                        int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                        const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;


    nm_algo_parameters_container_out =
        MPIDI_OFI_Scatterv_select(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                  recvcount, recvtype, root, comm, errflag,
                                  ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Scatterv_allcomm_linear_id:
            mpi_errno =
                MPIR_Scatterv_allcomm_linear(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                             recvcount, recvtype, root, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Scatterv_impl(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                           recvcount, recvtype, root, comm, errflag);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);


  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_mpi_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                        MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                        const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;


    nm_algo_parameters_container_out =
        MPIDI_OFI_Alltoall_select(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcount, recvtype, comm, errflag,
                                  ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Alltoall_intra_brucks_id:
            mpi_errno =
                MPIR_Alltoall_intra_brucks(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, comm, errflag);
            break;
        case MPIDI_OFI_Alltoall_intra_scattered_id:
            mpi_errno =
                MPIR_Alltoall_intra_scattered(sendbuf, sendcount, sendtype,
                                              recvbuf, recvcount, recvtype, comm, errflag);
            break;
        case MPIDI_OFI_Alltoall_intra_pairwise_id:
            mpi_errno =
                MPIR_Alltoall_intra_pairwise(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcount, recvtype, comm, errflag);
            break;
        case MPIDI_OFI_Alltoall_intra_pairwise_sendrecv_replace_id:
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

static inline int MPIDI_NM_mpi_alltoallv(const void *sendbuf, const int *sendcounts,
                                         const int *sdispls, MPI_Datatype sendtype,
                                         void *recvbuf, const int *recvcounts,
                                         const int *rdispls, MPI_Datatype recvtype,
                                         MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                         const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;


    nm_algo_parameters_container_out =
        MPIDI_OFI_Alltoallv_select(sendbuf, sendcounts, sdispls,
                                   sendtype, recvbuf, recvcounts,
                                   rdispls, recvtype, comm, errflag,
                                   ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Alltoallv_intra_pairwise_sendrecv_replace_id:
            mpi_errno =
                MPIR_Alltoallv_intra_pairwise_sendrecv_replace(sendbuf, sendcounts, sdispls,
                                                               sendtype, recvbuf, recvcounts,
                                                               rdispls, recvtype, comm, errflag);
            break;
        case MPIDI_OFI_Alltoallv_intra_scattered_id:
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

static inline int MPIDI_NM_mpi_alltoallw(const void *sendbuf, const int sendcounts[],
                                         const int sdispls[], const MPI_Datatype sendtypes[],
                                         void *recvbuf, const int recvcounts[],
                                         const int rdispls[], const MPI_Datatype recvtypes[],
                                         MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                         const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;


    nm_algo_parameters_container_out =
        MPIDI_OFI_Alltoallw_select(sendbuf, sendcounts, sdispls,
                                   sendtypes, recvbuf, recvcounts,
                                   rdispls, recvtypes, comm, errflag,
                                   ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Alltoallw_intra_pairwise_sendrecv_replace_id:
            mpi_errno =
                MPIR_Alltoallw_intra_pairwise_sendrecv_replace(sendbuf, sendcounts, sdispls,
                                                               sendtypes, recvbuf, recvcounts,
                                                               rdispls, recvtypes, comm, errflag);
            break;
        case MPIDI_OFI_Alltoallw_intra_scattered_id:
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

static inline int MPIDI_NM_mpi_reduce(const void *sendbuf, void *recvbuf, int count,
                                      MPI_Datatype datatype, MPI_Op op, int root,
                                      MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                      const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;


    nm_algo_parameters_container_out =
        MPIDI_OFI_Reduce_select(sendbuf, recvbuf, count, datatype, op, root, comm, errflag,
                                ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Reduce_intra_reduce_scatter_gather_id:
            mpi_errno =
                MPIR_Reduce_intra_reduce_scatter_gather(sendbuf, recvbuf, count,
                                                        datatype, op, root, comm, errflag);
            break;
        case MPIDI_OFI_Reduce_intra_binomial_id:
            mpi_errno =
                MPIR_Reduce_intra_binomial(sendbuf, recvbuf, count,
                                           datatype, op, root, comm, errflag);
            break;
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

static inline int MPIDI_NM_mpi_reduce_scatter(const void *sendbuf, void *recvbuf,
                                              const int recvcounts[], MPI_Datatype datatype,
                                              MPI_Op op, MPIR_Comm * comm,
                                              MPIR_Errflag_t * errflag,
                                              const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;


    nm_algo_parameters_container_out =
        MPIDI_OFI_Reduce_scatter_select(sendbuf, recvbuf, recvcounts, datatype, op, comm,
                                        errflag, ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Reduce_scatter_intra_noncommutative_id:
            mpi_errno =
                MPIR_Reduce_scatter_intra_noncommutative(sendbuf, recvbuf, recvcounts,
                                                         datatype, op, comm, errflag);
            break;
        case MPIDI_OFI_Reduce_scatter_intra_pairwise_id:
            mpi_errno =
                MPIR_Reduce_scatter_intra_pairwise(sendbuf, recvbuf, recvcounts,
                                                   datatype, op, comm, errflag);
            break;
        case MPIDI_OFI_Reduce_scatter_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Reduce_scatter_intra_recursive_doubling(sendbuf, recvbuf, recvcounts,
                                                             datatype, op, comm, errflag);
            break;
        case MPIDI_OFI_Reduce_scatter_intra_recursive_halving_id:
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

static inline int MPIDI_NM_mpi_reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                    int recvcount, MPI_Datatype datatype,
                                                    MPI_Op op, MPIR_Comm * comm,
                                                    MPIR_Errflag_t * errflag,
                                                    const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;


    nm_algo_parameters_container_out =
        MPIDI_OFI_Reduce_scatter_block_select(sendbuf, recvbuf, recvcount, datatype, op, comm,
                                              errflag, ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Reduce_scatter_block_intra_noncommutative_id:
            mpi_errno =
                MPIR_Reduce_scatter_block_intra_noncommutative(sendbuf, recvbuf, recvcount,
                                                               datatype, op, comm, errflag);
            break;
        case MPIDI_OFI_Reduce_scatter_block_intra_pairwise_id:
            mpi_errno =
                MPIR_Reduce_scatter_block_intra_pairwise(sendbuf, recvbuf, recvcount,
                                                         datatype, op, comm, errflag);
            break;
        case MPIDI_OFI_Reduce_scatter_block_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Reduce_scatter_block_intra_recursive_doubling(sendbuf, recvbuf, recvcount,
                                                                   datatype, op, comm, errflag);
            break;
        case MPIDI_OFI_Reduce_scatter_block_intra_recursive_halving_id:
            mpi_errno =
                MPIR_Reduce_scatter_block_intra_recursive_halving(sendbuf, recvbuf, recvcount,
                                                                  datatype, op, comm, errflag);
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

static inline int MPIDI_NM_mpi_scan(const void *sendbuf, void *recvbuf, int count,
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                    MPIR_Errflag_t * errflag,
                                    const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;


    nm_algo_parameters_container_out =
        MPIDI_OFI_Scan_select(sendbuf, recvbuf, count, datatype, op, comm,
                              errflag, ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Scan_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Scan_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op,
                                                   comm, errflag);
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

static inline int MPIDI_NM_mpi_exscan(const void *sendbuf, void *recvbuf, int count,
                                      MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                      MPIR_Errflag_t * errflag,
                                      const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;


    nm_algo_parameters_container_out =
        MPIDI_OFI_Exscan_select(sendbuf, recvbuf, count, datatype, op, comm,
                                errflag, ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Exscan_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Exscan_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op,
                                                     comm, errflag);
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

static inline int MPIDI_NM_mpi_neighbor_allgather(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  int recvcount, MPI_Datatype recvtype,
                                                  MPIR_Comm * comm)
{
    int mpi_errno;

    mpi_errno =
        MPIR_Neighbor_allgather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                     comm);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_neighbor_allgatherv(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   const int recvcounts[], const int displs[],
                                                   MPI_Datatype recvtype, MPIR_Comm * comm)
{
    int mpi_errno;

    mpi_errno = MPIR_Neighbor_allgatherv_impl(sendbuf, sendcount, sendtype,
                                              recvbuf, recvcounts, displs, recvtype, comm);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_neighbor_alltoall(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 int recvcount, MPI_Datatype recvtype,
                                                 MPIR_Comm * comm)
{
    int mpi_errno;

    mpi_errno = MPIR_Neighbor_alltoall_impl(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcount, recvtype, comm);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_neighbor_alltoallv(const void *sendbuf, const int sendcounts[],
                                                  const int sdispls[], MPI_Datatype sendtype,
                                                  void *recvbuf, const int recvcounts[],
                                                  const int rdispls[], MPI_Datatype recvtype,
                                                  MPIR_Comm * comm)
{
    int mpi_errno;

    mpi_errno = MPIR_Neighbor_alltoallv_impl(sendbuf, sendcounts, sdispls, sendtype,
                                             recvbuf, recvcounts, rdispls, recvtype, comm);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_neighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                                                  const MPI_Aint sdispls[],
                                                  const MPI_Datatype sendtypes[], void *recvbuf,
                                                  const int recvcounts[], const MPI_Aint rdispls[],
                                                  const MPI_Datatype recvtypes[], MPIR_Comm * comm)
{
    int mpi_errno;

    mpi_errno = MPIR_Neighbor_alltoallw_impl(sendbuf, sendcounts, sdispls, sendtypes,
                                             recvbuf, recvcounts, rdispls, recvtypes, comm);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ineighbor_allgather(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   int recvcount, MPI_Datatype recvtype,
                                                   MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ineighbor_allgather_impl(sendbuf, sendcount, sendtype,
                                              recvbuf, recvcount, recvtype, comm, req);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ineighbor_allgatherv(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    const int recvcounts[], const int displs[],
                                                    MPI_Datatype recvtype, MPIR_Comm * comm,
                                                    MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ineighbor_allgatherv_impl(sendbuf, sendcount, sendtype,
                                               recvbuf, recvcounts, displs, recvtype, comm, req);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ineighbor_alltoall(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  int recvcount, MPI_Datatype recvtype,
                                                  MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ineighbor_alltoall_impl(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcount, recvtype, comm, req);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ineighbor_alltoallv(const void *sendbuf, const int sendcounts[],
                                                   const int sdispls[], MPI_Datatype sendtype,
                                                   void *recvbuf, const int recvcounts[],
                                                   const int rdispls[], MPI_Datatype recvtype,
                                                   MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ineighbor_alltoallv_impl(sendbuf, sendcounts, sdispls, sendtype,
                                              recvbuf, recvcounts, rdispls, recvtype, comm, req);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ineighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                                                   const MPI_Aint sdispls[],
                                                   const MPI_Datatype sendtypes[], void *recvbuf,
                                                   const int recvcounts[], const MPI_Aint rdispls[],
                                                   const MPI_Datatype recvtypes[],
                                                   MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ineighbor_alltoallw_impl(sendbuf, sendcounts, sdispls, sendtypes,
                                              recvbuf, recvcounts, rdispls, recvtypes, comm, req);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ibarrier(MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ibarrier_impl(comm, req);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ibcast(void *buffer, int count, MPI_Datatype datatype,
                                      int root, MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ibcast_impl(buffer, count, datatype, root, comm, req);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                          MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Iallgather_impl(sendbuf, sendcount, sendtype, recvbuf,
                                     recvcount, recvtype, comm, req);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_iallgatherv(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           const int *recvcounts, const int *displs,
                                           MPI_Datatype recvtype, MPIR_Comm * comm,
                                           MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Iallgatherv_impl(sendbuf, sendcount, sendtype,
                                      recvbuf, recvcounts, displs, recvtype, comm, req);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_iallreduce(const void *sendbuf, void *recvbuf, int count,
                                          MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                          MPIR_Request ** request)
{
    int mpi_errno;

    mpi_errno = MPIR_Iallreduce_impl(sendbuf, recvbuf, count, datatype, op, comm, request);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                         MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ialltoall_impl(sendbuf, sendcount, sendtype, recvbuf,
                                    recvcount, recvtype, comm, req);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ialltoallv(const void *sendbuf, const int *sendcounts,
                                          const int *sdispls, MPI_Datatype sendtype,
                                          void *recvbuf, const int *recvcounts,
                                          const int *rdispls, MPI_Datatype recvtype,
                                          MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ialltoallv_impl(sendbuf, sendcounts, sdispls,
                                     sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, req);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ialltoallw(const void *sendbuf, const int *sendcounts,
                                          const int *sdispls, const MPI_Datatype sendtypes[],
                                          void *recvbuf, const int *recvcounts,
                                          const int *rdispls, const MPI_Datatype recvtypes[],
                                          MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ialltoallw_impl(sendbuf, sendcounts, sdispls,
                                     sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, req);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_iexscan(const void *sendbuf, void *recvbuf, int count,
                                       MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                       MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Iexscan_impl(sendbuf, recvbuf, count, datatype, op, comm, req);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                       int root, MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Igather_impl(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcount, recvtype, root, comm, req);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, const int *recvcounts, const int *displs,
                                        MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                        MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Igatherv_impl(sendbuf, sendcount, sendtype,
                                   recvbuf, recvcounts, displs, recvtype, root, comm, req);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                     int recvcount, MPI_Datatype datatype,
                                                     MPI_Op op, MPIR_Comm * comm,
                                                     MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ireduce_scatter_block_impl(sendbuf, recvbuf, recvcount,
                                                datatype, op, comm, req);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ireduce_scatter(const void *sendbuf, void *recvbuf,
                                               const int recvcounts[], MPI_Datatype datatype,
                                               MPI_Op op, MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ireduce_scatter_impl(sendbuf, recvbuf, recvcounts, datatype, op, comm, req);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ireduce(const void *sendbuf, void *recvbuf, int count,
                                       MPI_Datatype datatype, MPI_Op op, int root,
                                       MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Ireduce_impl(sendbuf, recvbuf, count, datatype, op, root, comm, req);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_iscan(const void *sendbuf, void *recvbuf, int count,
                                     MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                     MPIR_Request ** req)
{
    int mpi_errno;

    mpi_errno = MPIR_Iscan_impl(sendbuf, recvbuf, count, datatype, op, comm, req);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_iscatter(const void *sendbuf, int sendcount,
                                        MPI_Datatype sendtype, void *recvbuf,
                                        int recvcount, MPI_Datatype recvtype,
                                        int root, MPIR_Comm * comm, MPIR_Request ** request)
{
    int mpi_errno;

    mpi_errno = MPIR_Iscatter_impl(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcount, recvtype, root, comm, request);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_iscatterv(const void *sendbuf, const int *sendcounts,
                                         const int *displs, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcount,
                                         MPI_Datatype recvtype, int root,
                                         MPIR_Comm * comm, MPIR_Request ** request)
{
    int mpi_errno;

    mpi_errno = MPIR_Iscatterv_impl(sendbuf, sendcounts, displs, sendtype,
                                    recvbuf, recvcount, recvtype, root, comm, request);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ibarrier_sched(MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno;


    mpi_errno = MPIR_Ibarrier_sched_impl(comm, s);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ibcast_sched(void *buffer, int count, MPI_Datatype datatype,
                                            int root, MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno;


    mpi_errno = MPIR_Ibcast_sched_impl(buffer, count, datatype, root, comm, s);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_iallgather_sched(const void *sendbuf, int sendcount,
                                                MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                                MPI_Datatype recvtype, MPIR_Comm * comm,
                                                MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;


    mpi_errno = MPIR_Iallgather_sched_impl(sendbuf, sendcount, sendtype, recvbuf,
                                           recvcount, recvtype, comm, s);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_iallgatherv_sched(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 const int *recvcounts, const int *displs,
                                                 MPI_Datatype recvtype, MPIR_Comm * comm,
                                                 MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;


    mpi_errno = MPIR_Iallgatherv_sched_impl(sendbuf, sendcount, sendtype, recvbuf,
                                            recvcounts, displs, recvtype, comm, s);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_iallreduce_sched(const void *sendbuf, void *recvbuf, int count,
                                                MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                                MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;


    mpi_errno = MPIR_Iallreduce_sched_impl(sendbuf, recvbuf, count, datatype, op, comm, s);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ialltoall_sched(const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                               MPI_Datatype recvtype, MPIR_Comm * comm,
                                               MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;


    mpi_errno = MPIR_Ialltoall_sched_impl(sendbuf, sendcount, sendtype, recvbuf,
                                          recvcount, recvtype, comm, s);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ialltoallv_sched(const void *sendbuf, const int sendcounts[],
                                                const int sdispls[], MPI_Datatype sendtype,
                                                void *recvbuf, const int recvcounts[],
                                                const int rdispls[], MPI_Datatype recvtype,
                                                MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;


    mpi_errno = MPIR_Ialltoallv_sched_impl(sendbuf, sendcounts, sdispls, sendtype,
                                           recvbuf, recvcounts, rdispls, recvtype, comm, s);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ialltoallw_sched(const void *sendbuf, const int sendcounts[],
                                                const int sdispls[], const MPI_Datatype sendtypes[],
                                                void *recvbuf, const int recvcounts[],
                                                const int rdispls[], const MPI_Datatype recvtypes[],
                                                MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;


    mpi_errno = MPIR_Ialltoallw_sched_impl(sendbuf, sendcounts, sdispls, sendtypes,
                                           recvbuf, recvcounts, rdispls, recvtypes, comm, s);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_iexscan_sched(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                             MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;


    mpi_errno = MPIR_Iexscan_sched_impl(sendbuf, recvbuf, count, datatype, op, comm, s);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_igather_sched(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                             MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                             MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;


    mpi_errno = MPIR_Igather_sched_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                        recvtype, root, comm, s);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_igatherv_sched(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              const int *recvcounts, const int *displs,
                                              MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                              MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;


    mpi_errno = MPIR_Igatherv_sched_impl(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                         displs, recvtype, root, comm, s);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ireduce_scatter_block_sched(const void *sendbuf, void *recvbuf,
                                                           int recvcount, MPI_Datatype datatype,
                                                           MPI_Op op, MPIR_Comm * comm,
                                                           MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;


    mpi_errno = MPIR_Ireduce_scatter_block_sched_impl(sendbuf, recvbuf, recvcount,
                                                      datatype, op, comm, s);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ireduce_scatter_sched(const void *sendbuf, void *recvbuf,
                                                     const int recvcounts[], MPI_Datatype datatype,
                                                     MPI_Op op, MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;


    mpi_errno = MPIR_Ireduce_scatter_sched_impl(sendbuf, recvbuf, recvcounts, datatype,
                                                op, comm, s);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ireduce_sched(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op, int root,
                                             MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;


    mpi_errno = MPIR_Ireduce_sched_impl(sendbuf, recvbuf, count, datatype, op, root, comm, s);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_iscan_sched(const void *sendbuf, void *recvbuf, int count,
                                           MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                           MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;


    mpi_errno = MPIR_Iscan_sched_impl(sendbuf, recvbuf, count, datatype, op, comm, s);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_iscatter_sched(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                              MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                              MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;


    mpi_errno = MPIR_Iscatter_sched_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                         recvtype, root, comm, s);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_iscatterv_sched(const void *sendbuf, const int *sendcounts,
                                               const int *displs, MPI_Datatype sendtype,
                                               void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                               int root, MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;


    mpi_errno = MPIR_Iscatterv_sched_impl(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                          recvcount, recvtype, root, comm, s);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ineighbor_allgather_sched(const void *sendbuf, int sendcount,
                                                         MPI_Datatype sendtype, void *recvbuf,
                                                         int recvcount, MPI_Datatype recvtype,
                                                         MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;


    mpi_errno = MPIR_Ineighbor_allgather_sched_impl(sendbuf, sendcount, sendtype, recvbuf,
                                                    recvcount, recvtype, comm, s);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ineighbor_allgatherv_sched(const void *sendbuf, int sendcount,
                                                          MPI_Datatype sendtype, void *recvbuf,
                                                          const int recvcounts[],
                                                          const int displs[], MPI_Datatype recvtype,
                                                          MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;


    mpi_errno = MPIR_Ineighbor_allgatherv_sched_impl(sendbuf, sendcount, sendtype,
                                                     recvbuf, recvcounts, displs, recvtype, comm,
                                                     s);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ineighbor_alltoall_sched(const void *sendbuf, int sendcount,
                                                        MPI_Datatype sendtype, void *recvbuf,
                                                        int recvcount, MPI_Datatype recvtype,
                                                        MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;


    mpi_errno = MPIR_Ineighbor_alltoall_sched_impl(sendbuf, sendcount, sendtype, recvbuf,
                                                   recvcount, recvtype, comm, s);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ineighbor_alltoallv_sched(const void *sendbuf,
                                                         const int sendcounts[],
                                                         const int sdispls[], MPI_Datatype sendtype,
                                                         void *recvbuf, const int recvcounts[],
                                                         const int rdispls[], MPI_Datatype recvtype,
                                                         MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;


    mpi_errno = MPIR_Ineighbor_alltoallv_sched_impl(sendbuf, sendcounts, sdispls,
                                                    sendtype, recvbuf, recvcounts, rdispls,
                                                    recvtype, comm, s);

    return mpi_errno;
}

static inline int MPIDI_NM_mpi_ineighbor_alltoallw_sched(const void *sendbuf,
                                                         const int sendcounts[],
                                                         const MPI_Aint sdispls[],
                                                         const MPI_Datatype sendtypes[],
                                                         void *recvbuf, const int recvcounts[],
                                                         const MPI_Aint rdispls[],
                                                         const MPI_Datatype recvtypes[],
                                                         MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;


    mpi_errno = MPIR_Ineighbor_alltoallw_sched_impl(sendbuf, sendcounts, sdispls,
                                                    sendtypes, recvbuf, recvcounts, rdispls,
                                                    recvtypes, comm, s);

    return mpi_errno;
}

#endif /* OFI_COLL_H_INCLUDED */
