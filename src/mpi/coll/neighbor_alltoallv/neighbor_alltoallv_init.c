/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "mpiimpl.h"

int MPIR_Neighbor_alltoallv_init_impl(const void *sendbuf, const MPI_Aint sendcounts[],
                                      const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                      void *recvbuf, const MPI_Aint recvcounts[],
                                      const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                      MPIR_Comm * comm_ptr, MPIR_Info * info_ptr,
                                      MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    /* create a new request */
    MPIR_Request *req = MPIR_Request_create(MPIR_REQUEST_KIND__PREQUEST_COLL);
    MPIR_ERR_CHKANDJUMP(!req, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPIR_Comm_add_ref(comm_ptr);
    req->comm = comm_ptr;

    req->u.persist_coll.sched_type = MPIR_SCHED_INVALID;
    req->u.persist_coll.real_request = NULL;

    /* *INDENT-OFF* */
    mpi_errno = MPIR_Ineighbor_alltoallv_sched_impl(sendbuf, sendcounts, sdispls, sendtype,
                                                    recvbuf, recvcounts, rdispls, recvtype,
                                                    comm_ptr, true, /* is_persistent */
                                                    &req->u.persist_coll.sched,
                                                    &req->u.persist_coll.sched_type);
    /* *INDENT-ON* */
    MPIR_ERR_CHECK(mpi_errno);

    *request = req;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Neighbor_alltoallv_init(const void *sendbuf, const MPI_Aint sendcounts[],
                                 const MPI_Aint sdispls[], MPI_Datatype sendtype, void *recvbuf,
                                 const MPI_Aint recvcounts[], const MPI_Aint rdispls[],
                                 MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Info * info_ptr,
                                 MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_INEIGHBOR_ALLTOALLV_DEVICE_COLLECTIVE)) {
        mpi_errno = MPID_Neighbor_alltoallv_init(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                                 recvcounts, rdispls, recvtype, comm_ptr, info_ptr,
                                                 request);
    } else {
        mpi_errno =
            MPIR_Neighbor_alltoallv_init_impl(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                              recvcounts, rdispls, recvtype, comm_ptr, info_ptr,
                                              request);
    }

    return mpi_errno;
}
