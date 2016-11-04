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
#ifndef CH4R_RMA_ORIGIN_CALLBACKS_H_INCLUDED
#define CH4R_RMA_ORIGIN_CALLBACKS_H_INCLUDED

#include "ch4r_request.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_put_ack_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_put_ack_origin_cmpl_handler(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_PUT_ACK_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_PUT_ACK_TX_HANDLER);
    MPIDI_Request_complete(req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_PUT_ACK_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_acc_ack_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_acc_ack_origin_cmpl_handler(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_ACC_ACK_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_ACC_ACK_TX_HANDLER);
    MPIDI_Request_complete(req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_ACC_ACK_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_get_acc_ack_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_get_acc_ack_origin_cmpl_handler(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_GET_ACC_ACK_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_GET_ACC_ACK_TX_HANDLER);
    MPL_free(MPIDI_CH4U_REQUEST(req, req->areq.data));

    win = MPIDI_CH4U_REQUEST(req, req->areq.win_ptr);
    /* MPIDI_CS_ENTER(); */
    OPA_decr_int(&MPIDI_CH4U_WIN(win, outstanding_ops));
    /* MPIDI_CS_EXIT(); */

    MPIDI_Request_complete(req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_GET_ACC_ACK_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_cswap_ack_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_cswap_ack_origin_cmpl_handler(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_CSWAP_ACK_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_CSWAP_ACK_TX_HANDLER);

    MPL_free(MPIDI_CH4U_REQUEST(req, req->creq.data));
    win = MPIDI_CH4U_REQUEST(req, req->creq.win_ptr);
    /* MPIDI_CS_ENTER(); */
    OPA_decr_int(&MPIDI_CH4U_WIN(win, outstanding_ops));
    /* MPIDI_CS_EXIT(); */

    MPIDI_Request_complete(req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_CSWAP_ACK_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_get_ack_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_get_ack_origin_cmpl_handler(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_GET_ACK_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_GET_ACK_TX_HANDLER);

    if (MPIDI_CH4U_REQUEST(req, req->greq.dt_iov)) {
        MPL_free(MPIDI_CH4U_REQUEST(req, req->greq.dt_iov));
    }

    win = MPIDI_CH4U_REQUEST(req, req->greq.win_ptr);
    /* MPIDI_CS_ENTER(); */
    OPA_decr_int(&MPIDI_CH4U_WIN(win, outstanding_ops));
    /* MPIDI_CS_EXIT(); */

    MPIDI_Request_complete(req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_GET_ACK_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_put_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_put_origin_cmpl_handler(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_PUT_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_PUT_TX_HANDLER);
    MPIDI_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_PUT_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_cswap_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_cswap_origin_cmpl_handler(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_CSWAP_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_CSWAP_TX_HANDLER);
    MPIDI_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_CSWAP_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_acc_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_acc_origin_cmpl_handler(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_ACC_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_ACC_TX_HANDLER);
    MPIDI_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_ACC_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_get_acc_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_get_acc_origin_cmpl_handler(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_GET_ACC_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_GET_ACC_TX_HANDLER);
    MPIDI_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_GET_ACC_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_put_data_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_put_data_origin_cmpl_handler(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_PUT_DATA_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_PUT_DATA_TX_HANDLER);
    MPIDI_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_PUT_DATA_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_acc_data_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_acc_data_origin_cmpl_handler(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_ACC_DATA_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_ACC_DATA_TX_HANDLER);
    MPIDI_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_ACC_DATA_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_get_acc_data_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_get_acc_data_origin_cmpl_handler(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_GET_ACC_DATA_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_GET_ACC_DATA_TX_HANDLER);
    MPIDI_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_GET_ACC_DATA_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_put_iov_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_put_iov_origin_cmpl_handler(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_PUT_IOV_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_PUT_IOV_TX_HANDLER);
    MPIDI_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_PUT_IOV_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_acc_iov_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_acc_iov_origin_cmpl_handler(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_ACC_IOV_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_ACC_IOV_TX_HANDLER);
    MPIDI_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_ACC_IOV_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_get_acc_iov_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_get_acc_iov_origin_cmpl_handler(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_GET_ACC_IOV_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_GET_ACC_IOV_TX_HANDLER);
    MPIDI_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_GET_ACC_IOV_TX_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_get_origin_cmpl_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4U_get_origin_cmpl_handler(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_GET_TX_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_GET_TX_HANDLER);
    MPIDI_Request_complete(sreq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_GET_TX_HANDLER);
    return mpi_errno;
}

#endif /* CH4R_RMA_ORIGIN_CALLBACKS_H_INCLUDED */
