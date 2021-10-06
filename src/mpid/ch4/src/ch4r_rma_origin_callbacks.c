/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ch4r_rma_origin_callbacks.h"

/* This file includes all RMA callback routines on the packet issuing side.
 * All handler functions are named with suffix "_origin_cb". */

int MPIDIG_put_ack_origin_cb(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    MPID_Request_complete(req);
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIG_acc_ack_origin_cb(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    MPID_Request_complete(req);
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIG_get_acc_ack_origin_cb(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;
    MPL_free(MPIDIG_REQUEST(req, req->areq.data));

    MPID_Request_complete(req);
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIG_cswap_ack_origin_cb(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    MPL_free(MPIDIG_REQUEST(req, req->creq.data));
    MPID_Request_complete(req);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIG_get_ack_origin_cb(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    MPL_free(MPIDIG_REQUEST(req, req->greq.flattened_dt));
    if (MPIDIG_REQUEST(req, req->greq.dt))
        MPIR_Datatype_ptr_release(MPIDIG_REQUEST(req, req->greq.dt));

    MPID_Request_complete(req);
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIG_put_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    MPID_Request_complete(sreq);
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIG_cswap_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    MPID_Request_complete(sreq);
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIG_acc_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    MPID_Request_complete(sreq);
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIG_get_acc_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    MPID_Request_complete(sreq);
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIG_put_data_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    MPID_Request_complete(sreq);
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIG_acc_data_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    MPID_Request_complete(sreq);
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIG_get_acc_data_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    MPID_Request_complete(sreq);
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIG_put_dt_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    MPID_Request_complete(sreq);
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIG_acc_dt_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    MPID_Request_complete(sreq);
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIG_get_acc_dt_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    MPID_Request_complete(sreq);
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIG_get_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    MPID_Request_complete(sreq);
    MPIR_FUNC_EXIT;
    return mpi_errno;
}
