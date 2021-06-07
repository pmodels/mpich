/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2019 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpiimpl.h"

int MPIR_Alltoall_init_impl(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                            void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                            MPIR_Comm * comm_ptr, MPIR_Info * info_ptr, MPIR_Request ** request)
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
    mpi_errno = MPIR_Ialltoall_sched_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                          recvtype, comm_ptr, true, /* is_persistent */
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

int MPIR_Alltoall_init(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                       void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                       MPIR_Comm * comm_ptr, MPIR_Info * info_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_IALLTOALL_DEVICE_COLLECTIVE)) {
        mpi_errno = MPID_Alltoall_init(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                       comm_ptr, info_ptr, request);
    } else {
        mpi_errno = MPIR_Alltoall_init_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                            recvtype, comm_ptr, info_ptr, request);
    }

    return mpi_errno;
}
