/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpidimpl.h"
#include "ch4r_rma_origin_callbacks.h"

/* This file includes all RMA callback routines on the packet issuing side.
 * All handler functions are named with suffix "_origin_cb". */

#undef FUNCNAME
#define FUNCNAME MPIDIG_put_ack_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_put_ack_origin_cb(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PUT_ACK_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PUT_ACK_ORIGIN_CB);
    MPID_Request_complete(req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_ACK_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_acc_ack_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_acc_ack_origin_cb(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ACC_ACK_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ACC_ACK_ORIGIN_CB);
    MPID_Request_complete(req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ACC_ACK_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_get_acc_ack_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_get_acc_ack_origin_cb(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_ACK_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_ACK_ORIGIN_CB);
    MPL_free(MPIDIG_REQUEST(req, req->areq.data));

    MPID_Request_complete(req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_ACK_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_cswap_ack_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_cswap_ack_origin_cb(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_CSWAP_ACK_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_CSWAP_ACK_ORIGIN_CB);

    MPL_free(MPIDIG_REQUEST(req, req->creq.data));
    MPID_Request_complete(req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_CSWAP_ACK_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_get_ack_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_get_ack_origin_cb(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACK_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACK_ORIGIN_CB);

    MPL_free(MPIDIG_REQUEST(req, req->greq.dt_iov));

    MPID_Request_complete(req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACK_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_put_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_put_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PUT_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PUT_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_cswap_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_cswap_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_CSWAP_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_CSWAP_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_CSWAP_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_acc_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_acc_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ACC_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ACC_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ACC_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_get_acc_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_get_acc_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_put_data_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_put_data_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PUT_DATA_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PUT_DATA_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_DATA_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_acc_data_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_acc_data_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ACC_DATA_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ACC_DATA_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ACC_DATA_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_get_acc_data_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_get_acc_data_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_DATA_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_DATA_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_DATA_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_put_iov_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_put_iov_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PUT_IOV_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PUT_IOV_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PUT_IOV_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_acc_iov_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_acc_iov_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ACC_IOV_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ACC_IOV_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ACC_IOV_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_get_acc_iov_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_get_acc_iov_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ACC_IOV_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ACC_IOV_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ACC_IOV_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_get_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_get_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_ORIGIN_CB);
    return mpi_errno;
}
