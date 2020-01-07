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
#ifndef CH4_COLL_H_INCLUDED
#define CH4_COLL_H_INCLUDED

#include "ch4_impl.h"
#include "ch4r_proc.h"
#include "ch4_coll_select.h"

MPL_STATIC_INLINE_PREFIX int MPID_Barrier(MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;




    ch4_algo_parameters_container = MPIDI_Barrier_select(comm, errflag);

    switch (ch4_algo_parameters_container->id) {
        case MPIDI_Barrier_intra_composition_alpha_id:
            mpi_errno =
                MPIDI_Barrier_intra_composition_alpha(comm, errflag, ch4_algo_parameters_container);
            break;
        case MPIDI_Barrier_intra_composition_beta_id:
            mpi_errno =
                MPIDI_Barrier_intra_composition_beta(comm, errflag, ch4_algo_parameters_container);
            break;
        case MPIDI_Barrier_inter_composition_alpha_id:
            mpi_errno =
                MPIDI_Barrier_inter_composition_alpha(comm, errflag, ch4_algo_parameters_container);
            break;
        default:
            mpi_errno = MPIR_Barrier_impl(comm, errflag);
            break;
    }


    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPID_Bcast(void *buffer, int count, MPI_Datatype datatype,
                                        int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;




    ch4_algo_parameters_container =
        MPIDI_Bcast_select(buffer, count, datatype, root, comm, errflag);

    switch (ch4_algo_parameters_container->id) {
        case MPIDI_Bcast_intra_composition_alpha_id:
            mpi_errno =
                MPIDI_Bcast_intra_composition_alpha(buffer, count, datatype, root, comm,
                                                    errflag, ch4_algo_parameters_container);
            break;
        case MPIDI_Bcast_intra_composition_beta_id:
            mpi_errno =
                MPIDI_Bcast_intra_composition_beta(buffer, count, datatype, root, comm,
                                                   errflag, ch4_algo_parameters_container);
            break;
        case MPIDI_Bcast_intra_composition_gamma_id:
            mpi_errno =
                MPIDI_Bcast_intra_composition_gamma(buffer, count, datatype, root, comm,
                                                    errflag, ch4_algo_parameters_container);
            break;
        case MPIDI_Bcast_inter_composition_alpha_id:
            mpi_errno =
                MPIDI_Bcast_inter_composition_alpha(buffer, count, datatype, root, comm,
                                                    errflag, ch4_algo_parameters_container);
            break;
        default:
            mpi_errno = MPIR_Bcast_impl(buffer, count, datatype, root, comm, errflag);
            break;
    }


    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPID_Allreduce(const void *sendbuf, void *recvbuf, int count,
                                            MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                            MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;




    ch4_algo_parameters_container =
        MPIDI_Allreduce_select(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    switch (ch4_algo_parameters_container->id) {
        case MPIDI_Allreduce_intra_composition_alpha_id:
            mpi_errno =
                MPIDI_Allreduce_intra_composition_alpha(sendbuf, recvbuf, count, datatype, op,
                                                        comm, errflag,
                                                        ch4_algo_parameters_container);
            break;
        case MPIDI_Allreduce_intra_composition_beta_id:
            mpi_errno =
                MPIDI_Allreduce_intra_composition_beta(sendbuf, recvbuf, count, datatype, op,
                                                       comm, errflag,
                                                       ch4_algo_parameters_container);
            break;
        case MPIDI_Allreduce_intra_composition_gamma_id:
            mpi_errno =
                MPIDI_Allreduce_intra_composition_gamma(sendbuf, recvbuf, count, datatype, op,
                                                        comm, errflag,
                                                        ch4_algo_parameters_container);
            break;
        case MPIDI_Allreduce_inter_composition_alpha_id:
            mpi_errno =
                MPIDI_Allreduce_inter_composition_alpha(sendbuf, recvbuf, count, datatype, op,
                                                        comm, errflag,
                                                        ch4_algo_parameters_container);
            break;
        default:
            mpi_errno = MPIR_Allreduce_impl(sendbuf, recvbuf, count, datatype, op, comm, errflag);
            break;
    }


    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPID_Allgather(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                            MPI_Datatype recvtype, MPIR_Comm * comm,
                                            MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;




    ch4_algo_parameters_container =
        MPIDI_Allgather_select(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, comm, errflag);

    switch (ch4_algo_parameters_container->id) {
        case MPIDI_Allgather_intra_composition_alpha_id:
            mpi_errno =
                MPIDI_Allgather_intra_composition_alpha(sendbuf, sendcount, sendtype,
                                                        recvbuf, recvcount, recvtype,
                                                        comm, errflag,
                                                        ch4_algo_parameters_container);
            break;
        case MPIDI_Allgather_inter_composition_alpha_id:
            mpi_errno =
                MPIDI_Allgather_inter_composition_alpha(sendbuf, sendcount, sendtype,
                                                        recvbuf, recvcount, recvtype,
                                                        comm, errflag,
                                                        ch4_algo_parameters_container);
            break;
        default:
            mpi_errno = MPIR_Allgather_impl(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcount, recvtype, comm, errflag);
            break;
    }


    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPID_Allgatherv(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf,
                                             const int *recvcounts, const int *displs,
                                             MPI_Datatype recvtype, MPIR_Comm * comm,
                                             MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;




    ch4_algo_parameters_container =
        MPIDI_Allgatherv_select(sendbuf, sendcount, sendtype, recvbuf,
                                recvcounts, displs, recvtype, comm, errflag);

    switch (ch4_algo_parameters_container->id) {
        case MPIDI_Allgatherv_intra_composition_alpha_id:
            mpi_errno =
                MPIDI_Allgatherv_intra_composition_alpha(sendbuf, sendcount, sendtype,
                                                         recvbuf, recvcounts, displs,
                                                         recvtype, comm, errflag,
                                                         ch4_algo_parameters_container);
            break;
        case MPIDI_Allgatherv_inter_composition_alpha_id:
            mpi_errno =
                MPIDI_Allgatherv_inter_composition_alpha(sendbuf, sendcount, sendtype,
                                                         recvbuf, recvcounts, displs,
                                                         recvtype, comm, errflag,
                                                         ch4_algo_parameters_container);
            break;
        default:
            mpi_errno = MPIR_Allgatherv_impl(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcounts, displs, recvtype, comm, errflag);
            break;
    }


    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPID_Scatter(const void *sendbuf, int sendcount,
                                          MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                          MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                          MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;




    ch4_algo_parameters_container =
        MPIDI_Scatter_select(sendbuf, sendcount, sendtype, recvbuf,
                             recvcount, recvtype, root, comm, errflag);


    switch (ch4_algo_parameters_container->id) {
        case MPIDI_Scatter_intra_composition_alpha_id:
            mpi_errno =
                MPIDI_Scatter_intra_composition_alpha(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcount, recvtype, root, comm, errflag,
                                                      ch4_algo_parameters_container);
            break;
        case MPIDI_Scatter_inter_composition_alpha_id:
            mpi_errno =
                MPIDI_Scatter_inter_composition_alpha(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcount, recvtype, root, comm, errflag,
                                                      ch4_algo_parameters_container);
            break;
        default:
            MPIR_Scatter_impl(sendbuf, sendcount, sendtype, recvbuf,
                              recvcount, recvtype, root, comm, errflag);
            break;
    }


    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPID_Scatterv(const void *sendbuf, const int *sendcounts,
                                           const int *displs, MPI_Datatype sendtype,
                                           void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                           int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;




    ch4_algo_parameters_container =
        MPIDI_Scatterv_select(sendbuf, sendcounts, displs, sendtype,
                              recvbuf, recvcount, recvtype, root, comm, errflag);


    switch (ch4_algo_parameters_container->id) {
        case MPIDI_Scatterv_intra_composition_alpha_id:
            mpi_errno =
                MPIDI_Scatterv_intra_composition_alpha(sendbuf, sendcounts, displs, sendtype,
                                                       recvbuf, recvcount, recvtype, root,
                                                       comm, errflag,
                                                       ch4_algo_parameters_container);
            break;
        case MPIDI_Scatterv_inter_composition_alpha_id:
            mpi_errno =
                MPIDI_Scatterv_inter_composition_alpha(sendbuf, sendcounts, displs, sendtype,
                                                       recvbuf, recvcount, recvtype, root,
                                                       comm, errflag,
                                                       ch4_algo_parameters_container);
            break;
        default:
            MPIR_Scatterv_impl(sendbuf, sendcounts, displs, sendtype, recvbuf,
                               recvcount, recvtype, root, comm, errflag);
            break;
    }


    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPID_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                         int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;




    ch4_algo_parameters_container =
        MPIDI_Gather_select(sendbuf, sendcount, sendtype, recvbuf,
                            recvcount, recvtype, root, comm, errflag);

    switch (ch4_algo_parameters_container->id) {
        case MPIDI_Gather_intra_composition_alpha_id:
            mpi_errno =
                MPIDI_Gather_intra_composition_alpha(sendbuf, sendcount, sendtype, recvbuf,
                                                     recvcount, recvtype, root, comm, errflag,
                                                     ch4_algo_parameters_container);
            break;
        case MPIDI_Gather_inter_composition_alpha_id:
            mpi_errno =
                MPIDI_Gather_inter_composition_alpha(sendbuf, sendcount, sendtype, recvbuf,
                                                     recvcount, recvtype, root, comm, errflag,
                                                     ch4_algo_parameters_container);
            break;
        default:
            mpi_errno = MPIR_Gather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                         recvtype, root, comm, errflag);
            break;
    }


    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPID_Gatherv(const void *sendbuf, int sendcount,
                                          MPI_Datatype sendtype, void *recvbuf,
                                          const int *recvcounts, const int *displs,
                                          MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                          MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;




    ch4_algo_parameters_container =
        MPIDI_Gatherv_select(sendbuf, sendcount, sendtype, recvbuf,
                             recvcounts, displs, recvtype, root, comm, errflag);

    switch (ch4_algo_parameters_container->id) {
        case MPIDI_Gatherv_intra_composition_alpha_id:
            mpi_errno =
                MPIDI_Gatherv_intra_composition_alpha(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcounts, displs, recvtype, root,
                                                      comm, errflag, ch4_algo_parameters_container);
            break;
        case MPIDI_Gatherv_inter_composition_alpha_id:
            mpi_errno =
                MPIDI_Gatherv_inter_composition_alpha(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcounts, displs, recvtype, root,
                                                      comm, errflag, ch4_algo_parameters_container);
            break;
        default:
            mpi_errno = MPIR_Gatherv_impl(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                          displs, recvtype, root, comm, errflag);
            break;
    }


    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPID_Alltoall(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                           MPI_Datatype recvtype, MPIR_Comm * comm,
                                           MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;




    ch4_algo_parameters_container =
        MPIDI_Alltoall_select(sendbuf, sendcount, sendtype, recvbuf,
                              recvcount, recvtype, comm, errflag);

    switch (ch4_algo_parameters_container->id) {
        case MPIDI_Alltoall_intra_composition_alpha_id:
            mpi_errno =
                MPIDI_Alltoall_intra_composition_alpha(sendbuf, sendcount, sendtype,
                                                       recvbuf, recvcount, recvtype,
                                                       comm, errflag,
                                                       ch4_algo_parameters_container);
            break;
        case MPIDI_Alltoall_inter_composition_alpha_id:
            mpi_errno =
                MPIDI_Alltoall_inter_composition_alpha(sendbuf, sendcount, sendtype,
                                                       recvbuf, recvcount, recvtype,
                                                       comm, errflag,
                                                       ch4_algo_parameters_container);
            break;
        default:
            mpi_errno = MPIR_Alltoall_impl(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, comm, errflag);
            break;
    }


    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPID_Alltoallv(const void *sendbuf, const int *sendcounts,
                                            const int *sdispls, MPI_Datatype sendtype,
                                            void *recvbuf, const int *recvcounts,
                                            const int *rdispls, MPI_Datatype recvtype,
                                            MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;




    ch4_algo_parameters_container =
        MPIDI_Alltoallv_select(sendbuf, sendcounts, sdispls, sendtype,
                               recvbuf, recvcounts, rdispls, recvtype, comm, errflag);

    switch (ch4_algo_parameters_container->id) {
        case MPIDI_Alltoallv_intra_composition_alpha_id:
            mpi_errno =
                MPIDI_Alltoallv_intra_composition_alpha(sendbuf, sendcounts, sdispls,
                                                        sendtype, recvbuf, recvcounts,
                                                        rdispls, recvtype, comm, errflag,
                                                        ch4_algo_parameters_container);
            break;
        case MPIDI_Alltoallv_inter_composition_alpha_id:
            mpi_errno =
                MPIDI_Alltoallv_inter_composition_alpha(sendbuf, sendcounts, sdispls,
                                                        sendtype, recvbuf, recvcounts,
                                                        rdispls, recvtype, comm, errflag,
                                                        ch4_algo_parameters_container);
            break;
        default:
            mpi_errno = MPIR_Alltoallv_impl(sendbuf, sendcounts, sdispls,
                                            sendtype, recvbuf, recvcounts,
                                            rdispls, recvtype, comm, errflag);
            break;
    }


    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPID_Alltoallw(const void *sendbuf, const int sendcounts[],
                                            const int sdispls[], const MPI_Datatype sendtypes[],
                                            void *recvbuf, const int recvcounts[],
                                            const int rdispls[], const MPI_Datatype recvtypes[],
                                            MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;




    ch4_algo_parameters_container =
        MPIDI_Alltoallw_select(sendbuf, sendcounts, sdispls, sendtypes,
                               recvbuf, recvcounts, rdispls, recvtypes, comm, errflag);

    switch (ch4_algo_parameters_container->id) {
        case MPIDI_Alltoallw_intra_composition_alpha_id:
            mpi_errno =
                MPIDI_Alltoallw_intra_composition_alpha(sendbuf, sendcounts, sdispls,
                                                        sendtypes, recvbuf, recvcounts,
                                                        rdispls, recvtypes, comm, errflag,
                                                        ch4_algo_parameters_container);
            break;
        case MPIDI_Alltoallw_inter_composition_alpha_id:
            mpi_errno =
                MPIDI_Alltoallw_inter_composition_alpha(sendbuf, sendcounts, sdispls,
                                                        sendtypes, recvbuf, recvcounts,
                                                        rdispls, recvtypes, comm, errflag,
                                                        ch4_algo_parameters_container);
            break;
        default:
            mpi_errno = MPIR_Alltoallw_impl(sendbuf, sendcounts, sdispls,
                                            sendtypes, recvbuf, recvcounts,
                                            rdispls, recvtypes, comm, errflag);
            break;
    }


    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPID_Reduce(const void *sendbuf, void *recvbuf,
                                         int count, MPI_Datatype datatype, MPI_Op op,
                                         int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;




    ch4_algo_parameters_container =
        MPIDI_Reduce_select(sendbuf, recvbuf, count, datatype, op, root, comm, errflag);

    switch (ch4_algo_parameters_container->id) {
        case MPIDI_Reduce_intra_composition_alpha_id:
            mpi_errno =
                MPIDI_Reduce_intra_composition_alpha(sendbuf, recvbuf, count, datatype, op,
                                                     root, comm, errflag,
                                                     ch4_algo_parameters_container);
            break;
        case MPIDI_Reduce_intra_composition_beta_id:
            mpi_errno =
                MPIDI_Reduce_intra_composition_beta(sendbuf, recvbuf, count, datatype, op,
                                                    root, comm, errflag,
                                                    ch4_algo_parameters_container);
            break;
        case MPIDI_Reduce_intra_composition_gamma_id:
            mpi_errno =
                MPIDI_Reduce_intra_composition_gamma(sendbuf, recvbuf, count, datatype, op,
                                                     root, comm, errflag,
                                                     ch4_algo_parameters_container);
            break;
        case MPIDI_Reduce_inter_composition_alpha_id:
            mpi_errno =
                MPIDI_Reduce_inter_composition_alpha(sendbuf, recvbuf, count, datatype, op,
                                                     root, comm, errflag,
                                                     ch4_algo_parameters_container);
            break;
        default:
            mpi_errno = MPIR_Reduce_impl(sendbuf, recvbuf, count, datatype, op,
                                         root, comm, errflag);
            break;
    }


    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPID_Reduce_scatter(const void *sendbuf, void *recvbuf,
                                                 const int recvcounts[], MPI_Datatype datatype,
                                                 MPI_Op op, MPIR_Comm * comm,
                                                 MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;




    ch4_algo_parameters_container =
        MPIDI_Reduce_scatter_select(sendbuf, recvbuf, recvcounts, datatype, op, comm, errflag);

    switch (ch4_algo_parameters_container->id) {
        case MPIDI_Reduce_scatter_intra_composition_alpha_id:
            mpi_errno =
                MPIDI_Reduce_scatter_intra_composition_alpha(sendbuf, recvbuf, recvcounts,
                                                             datatype, op, comm, errflag,
                                                             ch4_algo_parameters_container);
            break;
        case MPIDI_Reduce_scatter_inter_composition_alpha_id:
            mpi_errno =
                MPIDI_Reduce_scatter_inter_composition_alpha(sendbuf, recvbuf, recvcounts,
                                                             datatype, op, comm, errflag,
                                                             ch4_algo_parameters_container);
            break;
        default:
            MPIR_Reduce_scatter_impl(sendbuf, recvbuf, recvcounts, datatype, op, comm, errflag);
            break;
    }


    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPID_Reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                       int recvcount, MPI_Datatype datatype,
                                                       MPI_Op op, MPIR_Comm * comm,
                                                       MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;




    ch4_algo_parameters_container =
        MPIDI_Reduce_scatter_block_select(sendbuf, recvbuf, recvcount, datatype, op, comm, errflag);

    switch (ch4_algo_parameters_container->id) {
        case MPIDI_Reduce_scatter_block_intra_composition_alpha_id:
            mpi_errno =
                MPIDI_Reduce_scatter_block_intra_composition_alpha(sendbuf, recvbuf, recvcount,
                                                                   datatype, op, comm, errflag,
                                                                   ch4_algo_parameters_container);
            break;
        case MPIDI_Reduce_scatter_block_inter_composition_alpha_id:
            mpi_errno =
                MPIDI_Reduce_scatter_block_inter_composition_alpha(sendbuf, recvbuf, recvcount,
                                                                   datatype, op, comm, errflag,
                                                                   ch4_algo_parameters_container);
            break;
        default:
            MPIR_Reduce_scatter_block_impl(sendbuf, recvbuf, recvcount, datatype, op, comm,
                                           errflag);
            break;
    }


    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPID_Scan(const void *sendbuf, void *recvbuf, int count,
                                       MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                       MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;




    ch4_algo_parameters_container =
        MPIDI_Scan_select(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    switch (ch4_algo_parameters_container->id) {
        case MPIDI_Scan_intra_composition_alpha_id:
            mpi_errno =
                MPIDI_Scan_intra_composition_alpha(sendbuf, recvbuf, count,
                                                   datatype, op, comm, errflag,
                                                   ch4_algo_parameters_container);
            break;
        case MPIDI_Scan_intra_composition_beta_id:
            mpi_errno =
                MPIDI_Scan_intra_composition_beta(sendbuf, recvbuf, count,
                                                  datatype, op, comm, errflag,
                                                  ch4_algo_parameters_container);
            break;
        default:
            MPIR_Scan_impl(sendbuf, recvbuf, count, datatype, op, comm, errflag);
            break;
    }


    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPID_Exscan(const void *sendbuf, void *recvbuf, int count,
                                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                         MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;




    ch4_algo_parameters_container =
        MPIDI_Exscan_select(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    switch (ch4_algo_parameters_container->id) {
        case MPIDI_Exscan_intra_composition_alpha_id:
            mpi_errno =
                MPIDI_Exscan_intra_composition_alpha(sendbuf, recvbuf, count,
                                                     datatype, op, comm, errflag,
                                                     ch4_algo_parameters_container);
            break;
        default:
            MPIR_Exscan_impl(sendbuf, recvbuf, count, datatype, op, comm, errflag);
            break;
    }


    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_allgather(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     int recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm)
{
    int ret;




    ret = MPIDI_NM_mpi_neighbor_allgather(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcount, recvtype, comm);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_allgatherv(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      const int *recvcounts, const int *displs,
                                                      MPI_Datatype recvtype, MPIR_Comm * comm)
{
    int ret;




    ret = MPIDI_NM_mpi_neighbor_allgatherv(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcounts, displs, recvtype, comm);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_alltoallv(const void *sendbuf, const int *sendcounts,
                                                     const int *sdispls, MPI_Datatype sendtype,
                                                     void *recvbuf, const int *recvcounts,
                                                     const int *rdispls, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm)
{
    int ret;




    ret = MPIDI_NM_mpi_neighbor_alltoallv(sendbuf, sendcounts, sdispls,
                                          sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_alltoallw(const void *sendbuf, const int *sendcounts,
                                                     const MPI_Aint * sdispls,
                                                     const MPI_Datatype * sendtypes, void *recvbuf,
                                                     const int *recvcounts,
                                                     const MPI_Aint * rdispls,
                                                     const MPI_Datatype * recvtypes,
                                                     MPIR_Comm * comm)
{
    int ret;




    ret = MPIDI_NM_mpi_neighbor_alltoallw(sendbuf, sendcounts, sdispls,
                                          sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_alltoall(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    int recvcount, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm)
{
    int ret;




    ret = MPIDI_NM_mpi_neighbor_alltoall(sendbuf, sendcount, sendtype,
                                         recvbuf, recvcount, recvtype, comm);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_allgather(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      int recvcount, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_mpi_ineighbor_allgather(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_allgatherv(const void *sendbuf, int sendcount,
                                                       MPI_Datatype sendtype, void *recvbuf,
                                                       const int *recvcounts, const int *displs,
                                                       MPI_Datatype recvtype, MPIR_Comm * comm,
                                                       MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_mpi_ineighbor_allgatherv(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcounts, displs, recvtype, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_alltoall(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     int recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_mpi_ineighbor_alltoall(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcount, recvtype, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_alltoallv(const void *sendbuf, const int *sendcounts,
                                                      const int *sdispls, MPI_Datatype sendtype,
                                                      void *recvbuf, const int *recvcounts,
                                                      const int *rdispls, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_mpi_ineighbor_alltoallv(sendbuf, sendcounts, sdispls,
                                           sendtype, recvbuf, recvcounts, rdispls, recvtype, comm,
                                           req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_alltoallw(const void *sendbuf, const int *sendcounts,
                                                      const MPI_Aint * sdispls,
                                                      const MPI_Datatype * sendtypes,
                                                      void *recvbuf, const int *recvcounts,
                                                      const MPI_Aint * rdispls,
                                                      const MPI_Datatype * recvtypes,
                                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_mpi_ineighbor_alltoallw(sendbuf, sendcounts, sdispls,
                                           sendtypes, recvbuf, recvcounts, rdispls, recvtypes,
                                           comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ibarrier(MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_mpi_ibarrier(comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ibcast(void *buffer, int count, MPI_Datatype datatype,
                                         int root, MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_mpi_ibcast(buffer, count, datatype, root, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iallgather(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                             MPI_Datatype recvtype, MPIR_Comm * comm,
                                             MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_mpi_iallgather(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcount, recvtype, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iallgatherv(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              const int *recvcounts, const int *displs,
                                              MPI_Datatype recvtype, MPIR_Comm * comm,
                                              MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_mpi_iallgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcounts, displs, recvtype, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iallreduce(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                             MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_mpi_iallreduce(sendbuf, recvbuf, count, datatype, op, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ialltoall(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                            MPI_Datatype recvtype, MPIR_Comm * comm,
                                            MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_mpi_ialltoall(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcount, recvtype, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ialltoallv(const void *sendbuf, const int *sendcounts,
                                             const int *sdispls, MPI_Datatype sendtype,
                                             void *recvbuf, const int *recvcounts,
                                             const int *rdispls, MPI_Datatype recvtype,
                                             MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_mpi_ialltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                  recvbuf, recvcounts, rdispls, recvtype, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ialltoallw(const void *sendbuf, const int *sendcounts,
                                             const int *sdispls, const MPI_Datatype * sendtypes,
                                             void *recvbuf, const int *recvcounts,
                                             const int *rdispls, const MPI_Datatype * recvtypes,
                                             MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_mpi_ialltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                  recvbuf, recvcounts, rdispls, recvtypes, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iexscan(const void *sendbuf, void *recvbuf, int count,
                                          MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                          MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_mpi_iexscan(sendbuf, recvbuf, count, datatype, op, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Igather(const void *sendbuf, int sendcount,
                                          MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                          MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                          MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_mpi_igather(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, root, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Igatherv(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           const int *recvcounts, const int *displs,
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                           MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_mpi_igatherv(sendbuf, sendcount, sendtype, recvbuf,
                                recvcounts, displs, recvtype, root, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                        int recvcount, MPI_Datatype datatype,
                                                        MPI_Op op, MPIR_Comm * comm,
                                                        MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_mpi_ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ireduce_scatter(const void *sendbuf, void *recvbuf,
                                                  const int *recvcounts, MPI_Datatype datatype,
                                                  MPI_Op op, MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_mpi_ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ireduce(const void *sendbuf, void *recvbuf, int count,
                                          MPI_Datatype datatype, MPI_Op op, int root,
                                          MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_mpi_ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iscan(const void *sendbuf, void *recvbuf, int count,
                                        MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                        MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_mpi_iscan(sendbuf, recvbuf, count, datatype, op, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iscatter(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                           MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_mpi_iscatter(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, root, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iscatterv(const void *sendbuf, const int *sendcounts,
                                            const int *displs, MPI_Datatype sendtype,
                                            void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                            int root, MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_mpi_iscatterv(sendbuf, sendcounts, displs, sendtype,
                                 recvbuf, recvcount, recvtype, root, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ibarrier_sched(MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret;




    ret = MPIDI_NM_mpi_ibarrier_sched(comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ibcast_sched(void *buffer, int count, MPI_Datatype datatype,
                                               int root, MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret;




    ret = MPIDI_NM_mpi_ibcast_sched(buffer, count, datatype, root, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iallgather_sched(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   int recvcount, MPI_Datatype recvtype,
                                                   MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret = MPI_SUCCESS;




    ret = MPIDI_NM_mpi_iallgather_sched(sendbuf, sendcount, sendtype, recvbuf,
                                        recvcount, recvtype, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iallgatherv_sched(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    const int *recvcounts, const int *displs,
                                                    MPI_Datatype recvtype, MPIR_Comm * comm,
                                                    MPIR_Sched_t s)
{
    int ret = MPI_SUCCESS;




    ret = MPIDI_NM_mpi_iallgatherv_sched(sendbuf, sendcount, sendtype, recvbuf,
                                         recvcounts, displs, recvtype, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iallreduce_sched(const void *sendbuf, void *recvbuf, int count,
                                                   MPI_Datatype datatype, MPI_Op op,
                                                   MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret = MPI_SUCCESS;




    ret = MPIDI_NM_mpi_iallreduce_sched(sendbuf, recvbuf, count, datatype, op, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ialltoall_sched(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  int recvcount, MPI_Datatype recvtype,
                                                  MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret = MPI_SUCCESS;




    ret = MPIDI_NM_mpi_ialltoall_sched(sendbuf, sendcount, sendtype, recvbuf,
                                       recvcount, recvtype, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ialltoallv_sched(const void *sendbuf, const int sendcounts[],
                                                   const int sdispls[], MPI_Datatype sendtype,
                                                   void *recvbuf, const int recvcounts[],
                                                   const int rdispls[], MPI_Datatype recvtype,
                                                   MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret = MPI_SUCCESS;




    ret = MPIDI_NM_mpi_ialltoallv_sched(sendbuf, sendcounts, sdispls, sendtype,
                                        recvbuf, recvcounts, rdispls, recvtype, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ialltoallw_sched(const void *sendbuf, const int sendcounts[],
                                                   const int sdispls[],
                                                   const MPI_Datatype sendtypes[], void *recvbuf,
                                                   const int recvcounts[], const int rdispls[],
                                                   const MPI_Datatype recvtypes[], MPIR_Comm * comm,
                                                   MPIR_Sched_t s)
{
    int ret = MPI_SUCCESS;




    ret = MPIDI_NM_mpi_ialltoallw_sched(sendbuf, sendcounts, sdispls, sendtypes,
                                        recvbuf, recvcounts, rdispls, recvtypes, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iexscan_sched(const void *sendbuf, void *recvbuf, int count,
                                                MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                                MPIR_Sched_t s)
{
    int ret = MPI_SUCCESS;




    ret = MPIDI_NM_mpi_iexscan_sched(sendbuf, recvbuf, count, datatype, op, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Igather_sched(const void *sendbuf, int sendcount,
                                                MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                                MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                                MPIR_Sched_t s)
{
    int ret = MPI_SUCCESS;




    ret = MPIDI_NM_mpi_igather_sched(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                     recvtype, root, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Igatherv_sched(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 const int *recvcounts, const int *displs,
                                                 MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                                 MPIR_Sched_t s)
{
    int ret = MPI_SUCCESS;




    ret = MPIDI_NM_mpi_igatherv_sched(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                      displs, recvtype, root, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ireduce_scatter_block_sched(const void *sendbuf, void *recvbuf,
                                                              int recvcount, MPI_Datatype datatype,
                                                              MPI_Op op, MPIR_Comm * comm,
                                                              MPIR_Sched_t s)
{
    int ret = MPI_SUCCESS;




    ret = MPIDI_NM_mpi_ireduce_scatter_block_sched(sendbuf, recvbuf, recvcount,
                                                   datatype, op, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ireduce_scatter_sched(const void *sendbuf, void *recvbuf,
                                                        const int recvcounts[],
                                                        MPI_Datatype datatype, MPI_Op op,
                                                        MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret = MPI_SUCCESS;




    ret = MPIDI_NM_mpi_ireduce_scatter_sched(sendbuf, recvbuf, recvcounts, datatype, op, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ireduce_sched(const void *sendbuf, void *recvbuf, int count,
                                                MPI_Datatype datatype, MPI_Op op, int root,
                                                MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret = MPI_SUCCESS;




    ret = MPIDI_NM_mpi_ireduce_sched(sendbuf, recvbuf, count, datatype, op, root, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iscan_sched(const void *sendbuf, void *recvbuf, int count,
                                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                              MPIR_Sched_t s)
{
    int ret = MPI_SUCCESS;




    ret = MPIDI_NM_mpi_iscan_sched(sendbuf, recvbuf, count, datatype, op, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iscatter_sched(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 int recvcount, MPI_Datatype recvtype, int root,
                                                 MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret = MPI_SUCCESS;




    ret = MPIDI_NM_mpi_iscatter_sched(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                      recvtype, root, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iscatterv_sched(const void *sendbuf, const int *sendcounts,
                                                  const int *displs, MPI_Datatype sendtype,
                                                  void *recvbuf, int recvcount,
                                                  MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                                  MPIR_Sched_t s)
{
    int ret = MPI_SUCCESS;




    ret = MPIDI_NM_mpi_iscatterv_sched(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                       recvcount, recvtype, root, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_allgather_sched(const void *sendbuf, int sendcount,
                                                            MPI_Datatype sendtype, void *recvbuf,
                                                            int recvcount, MPI_Datatype recvtype,
                                                            MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret = MPI_SUCCESS;




    ret = MPIDI_NM_mpi_ineighbor_allgather_sched(sendbuf, sendcount, sendtype, recvbuf,
                                                 recvcount, recvtype, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_allgatherv_sched(const void *sendbuf, int sendcount,
                                                             MPI_Datatype sendtype, void *recvbuf,
                                                             const int recvcounts[],
                                                             const int displs[],
                                                             MPI_Datatype recvtype,
                                                             MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret = MPI_SUCCESS;




    ret = MPIDI_NM_mpi_ineighbor_allgatherv_sched(sendbuf, sendcount, sendtype,
                                                  recvbuf, recvcounts, displs, recvtype, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_alltoall_sched(const void *sendbuf, int sendcount,
                                                           MPI_Datatype sendtype, void *recvbuf,
                                                           int recvcount, MPI_Datatype recvtype,
                                                           MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret = MPI_SUCCESS;




    ret = MPIDI_NM_mpi_ineighbor_alltoall_sched(sendbuf, sendcount, sendtype, recvbuf,
                                                recvcount, recvtype, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_alltoallv_sched(const void *sendbuf,
                                                            const int sendcounts[],
                                                            const int sdispls[],
                                                            MPI_Datatype sendtype, void *recvbuf,
                                                            const int recvcounts[],
                                                            const int rdispls[],
                                                            MPI_Datatype recvtype, MPIR_Comm * comm,
                                                            MPIR_Sched_t s)
{
    int ret = MPI_SUCCESS;




    ret = MPIDI_NM_mpi_ineighbor_alltoallv_sched(sendbuf, sendcounts, sdispls,
                                                 sendtype, recvbuf, recvcounts, rdispls, recvtype,
                                                 comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_alltoallw_sched(const void *sendbuf,
                                                            const int sendcounts[],
                                                            const MPI_Aint sdispls[],
                                                            const MPI_Datatype sendtypes[],
                                                            void *recvbuf, const int recvcounts[],
                                                            const MPI_Aint rdispls[],
                                                            const MPI_Datatype recvtypes[],
                                                            MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret = MPI_SUCCESS;




    ret = MPIDI_NM_mpi_ineighbor_alltoallw_sched(sendbuf, sendcounts, sdispls,
                                                 sendtypes, recvbuf, recvcounts, rdispls, recvtypes,
                                                 comm, s);


    return ret;
}

#endif /* CH4_COLL_H_INCLUDED */
