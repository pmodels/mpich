/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpidimpl.h"
#include "mpidig.h"
#include "mpidch4r.h"

#undef FUNCNAME
#define FUNCNAME MPIDIG_am_reg_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_am_reg_cb(int handler_id,
                     MPIDIG_am_origin_cb origin_cb, MPIDIG_am_target_msg_cb target_msg_cb)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_AM_REG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_AM_REG_CB);

    MPIDIG_global.target_msg_cbs[handler_id] = target_msg_cb;
    MPIDIG_global.origin_cbs[handler_id] = origin_cb;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_AM_REG_CB);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_init(MPIR_Comm * comm_world, MPIR_Comm * comm_self, int n_vnis)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_INIT);

    MPIDI_CH4_Global.is_ch4u_initialized = 0;

    MPIDI_CH4_Global.comm_req_lists = (MPIDI_CH4U_comm_req_list_t *)
        MPL_calloc(MPIR_MAX_CONTEXT_MASK * MPIR_CONTEXT_INT_BITS,
                   sizeof(MPIDI_CH4U_comm_req_list_t));
#ifndef MPIDI_CH4U_USE_PER_COMM_QUEUE
    MPIDI_CH4_Global.posted_list = NULL;
    MPIDI_CH4_Global.unexp_list = NULL;
#endif

    MPIDI_CH4_Global.cmpl_list = NULL;
    OPA_store_int(&MPIDI_CH4_Global.exp_seq_no, 0);
    OPA_store_int(&MPIDI_CH4_Global.nxt_seq_no, 0);

    MPIDI_CH4_Global.buf_pool = MPIDI_CH4U_create_buf_pool(MPIDI_CH4I_BUF_POOL_NUM,
                                                           MPIDI_CH4I_BUF_POOL_SZ);
    MPIR_Assert(MPIDI_CH4_Global.buf_pool);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_SEND, &MPIDI_send_origin_cb, &MPIDI_send_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_SEND_LONG_REQ, NULL /* Injection only */ ,
                                 &MPIDI_send_long_req_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_SEND_LONG_ACK, NULL /* Injection only */ ,
                                 &MPIDI_send_long_ack_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_SEND_LONG_LMT,
                                 &MPIDI_send_long_lmt_origin_cb,
                                 &MPIDI_send_long_lmt_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_SSEND_REQ,
                                 &MPIDI_send_origin_cb, &MPIDI_ssend_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_SSEND_ACK,
                                 &MPIDI_ssend_ack_origin_cb, &MPIDI_ssend_ack_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno =
        MPIDIG_am_reg_cb(MPIDI_CH4U_PUT_REQ, &MPIDI_put_origin_cb, &MPIDI_put_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_PUT_ACK, NULL, &MPIDI_put_ack_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno =
        MPIDIG_am_reg_cb(MPIDI_CH4U_GET_REQ, &MPIDI_get_origin_cb, &MPIDI_get_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_GET_ACK,
                                 &MPIDI_get_ack_origin_cb, &MPIDI_get_ack_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_CSWAP_REQ,
                                 &MPIDI_cswap_origin_cb, &MPIDI_cswap_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_CSWAP_ACK,
                                 &MPIDI_cswap_ack_origin_cb, &MPIDI_cswap_ack_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno =
        MPIDIG_am_reg_cb(MPIDI_CH4U_ACC_REQ, &MPIDI_acc_origin_cb, &MPIDI_acc_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_GET_ACC_REQ,
                                 &MPIDI_get_acc_origin_cb, &MPIDI_get_acc_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_ACC_ACK, NULL, &MPIDI_acc_ack_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_GET_ACC_ACK,
                                 &MPIDI_get_acc_ack_origin_cb, &MPIDI_get_acc_ack_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_WIN_COMPLETE, NULL, &MPIDI_win_ctrl_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_WIN_POST, NULL, &MPIDI_win_ctrl_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_WIN_LOCK, NULL, &MPIDI_win_ctrl_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_WIN_LOCK_ACK, NULL, &MPIDI_win_ctrl_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_WIN_UNLOCK, NULL, &MPIDI_win_ctrl_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_WIN_UNLOCK_ACK, NULL, &MPIDI_win_ctrl_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_WIN_LOCKALL, NULL, &MPIDI_win_ctrl_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_WIN_LOCKALL_ACK, NULL, &MPIDI_win_ctrl_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_WIN_UNLOCKALL, NULL, &MPIDI_win_ctrl_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_WIN_UNLOCKALL_ACK, NULL, &MPIDI_win_ctrl_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_PUT_IOV_REQ,
                                 &MPIDI_put_iov_origin_cb, &MPIDI_put_iov_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_PUT_IOV_ACK, NULL, &MPIDI_put_iov_ack_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_PUT_DAT_REQ,
                                 &MPIDI_put_data_origin_cb, &MPIDI_put_data_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_ACC_IOV_REQ,
                                 &MPIDI_acc_iov_origin_cb, &MPIDI_acc_iov_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_GET_ACC_IOV_REQ,
                                 &MPIDI_get_acc_iov_origin_cb, &MPIDI_get_acc_iov_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_ACC_IOV_ACK, NULL, &MPIDI_acc_iov_ack_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_GET_ACC_IOV_ACK,
                                 NULL, &MPIDI_get_acc_iov_ack_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_ACC_DAT_REQ,
                                 &MPIDI_acc_data_origin_cb, &MPIDI_acc_data_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDI_CH4U_GET_ACC_DAT_REQ,
                                 &MPIDI_get_acc_data_origin_cb, &MPIDI_get_acc_data_target_msg_cb);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDI_CH4U_init_comm(comm_world);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIDI_CH4U_init_comm(comm_self);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIDI_CH4_Global.win_hash = NULL;

    MPIDI_CH4_Global.is_ch4u_initialized = 1;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_INIT);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIDIG_finalize(void)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_FINALIZE);

    MPIDI_CH4_Global.is_ch4u_initialized = 0;
    MPL_HASH_CLEAR(dev.ch4u.hash_handle, MPIDI_CH4_Global.win_hash);
    MPIDI_CH4R_destroy_buf_pool(MPIDI_CH4_Global.buf_pool);
    MPL_free(MPIDI_CH4_Global.comm_req_lists);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_FINALIZE);
}
