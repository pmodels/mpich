/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpidimpl.h"
#include "mpidch4r.h"

/* This file includes all RMA callback routines on the packet issuing side.
 * All handler functions are named with suffix "_origin_cb". */

#undef FUNCNAME
#define FUNCNAME MPIDI_put_ack_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_put_ack_origin_cb(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PUT_ACK_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PUT_ACK_ORIGIN_CB);
    MPID_Request_complete(req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PUT_ACK_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_acc_ack_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_acc_ack_origin_cb(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ACC_ACK_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ACC_ACK_ORIGIN_CB);
    MPID_Request_complete(req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ACC_ACK_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_get_acc_ack_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_get_acc_ack_origin_cb(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GET_ACC_ACK_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GET_ACC_ACK_ORIGIN_CB);
    MPL_free(MPIDIG_REQUEST(req, req->areq.data));

    MPID_Request_complete(req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GET_ACC_ACK_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_cswap_ack_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_cswap_ack_origin_cb(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CSWAP_ACK_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CSWAP_ACK_ORIGIN_CB);

    MPL_free(MPIDIG_REQUEST(req, req->creq.data));
    MPID_Request_complete(req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CSWAP_ACK_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_get_ack_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_get_ack_origin_cb(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GET_ACK_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GET_ACK_ORIGIN_CB);

    if (MPIDIG_REQUEST(req, req->greq.dt_iov)) {
        MPL_free(MPIDIG_REQUEST(req, req->greq.dt_iov));
    }

    MPID_Request_complete(req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GET_ACK_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_put_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_put_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PUT_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PUT_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PUT_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_cswap_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_cswap_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CSWAP_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CSWAP_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CSWAP_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_acc_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_acc_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ACC_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ACC_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ACC_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_get_acc_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_get_acc_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GET_ACC_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GET_ACC_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GET_ACC_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_put_data_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_put_data_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PUT_DATA_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PUT_DATA_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PUT_DATA_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_acc_data_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_acc_data_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ACC_DATA_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ACC_DATA_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ACC_DATA_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_get_acc_data_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_get_acc_data_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GET_ACC_DATA_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GET_ACC_DATA_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GET_ACC_DATA_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_put_iov_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_put_iov_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PUT_IOV_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PUT_IOV_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PUT_IOV_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_acc_iov_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_acc_iov_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ACC_IOV_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ACC_IOV_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ACC_IOV_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_get_acc_iov_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_get_acc_iov_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GET_ACC_IOV_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GET_ACC_IOV_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GET_ACC_IOV_ORIGIN_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_get_origin_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_get_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GET_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GET_ORIGIN_CB);
    MPID_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GET_ORIGIN_CB);
    return mpi_errno;
}
