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

int MPIDIG_init(MPIR_Comm * comm_world, MPIR_Comm * comm_self, int n_vcis)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_INIT);

    MPIDI_global.is_ch4u_initialized = 0;

    MPIDI_global.comm_req_lists = (MPIDIG_comm_req_list_t *)
        MPL_calloc(MPIR_MAX_CONTEXT_MASK * MPIR_CONTEXT_INT_BITS,
                   sizeof(MPIDIG_comm_req_list_t), MPL_MEM_OTHER);
#ifndef MPIDI_CH4U_USE_PER_COMM_QUEUE
    MPIDI_global.posted_list = NULL;
    MPIDI_global.unexp_list = NULL;
#endif

    MPIDI_global.cmpl_list = NULL;
    OPA_store_int(&MPIDI_global.exp_seq_no, 0);
    OPA_store_int(&MPIDI_global.nxt_seq_no, 0);

    MPIDI_global.buf_pool = MPIDIU_create_buf_pool(MPIDIU_BUF_POOL_NUM, MPIDIU_BUF_POOL_SZ);
    MPIR_Assert(MPIDI_global.buf_pool);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_SEND, &MPIDIG_send_origin_cb, &MPIDIG_send_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_SEND_LONG_REQ, NULL /* Injection only */ ,
                                 &MPIDIG_send_long_req_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_SEND_LONG_ACK, NULL /* Injection only */ ,
                                 &MPIDIG_send_long_ack_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_SEND_LONG_LMT,
                                 &MPIDIG_send_long_lmt_origin_cb,
                                 &MPIDIG_send_long_lmt_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_SSEND_REQ,
                                 &MPIDIG_send_origin_cb, &MPIDIG_ssend_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_SSEND_ACK,
                                 &MPIDIG_ssend_ack_origin_cb, &MPIDIG_ssend_ack_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_PUT_REQ, &MPIDIG_put_origin_cb, &MPIDIG_put_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_PUT_ACK, NULL, &MPIDIG_put_ack_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_GET_REQ, &MPIDIG_get_origin_cb, &MPIDIG_get_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_GET_ACK,
                                 &MPIDIG_get_ack_origin_cb, &MPIDIG_get_ack_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_CSWAP_REQ,
                                 &MPIDIG_cswap_origin_cb, &MPIDIG_cswap_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_CSWAP_ACK,
                                 &MPIDIG_cswap_ack_origin_cb, &MPIDIG_cswap_ack_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_ACC_REQ, &MPIDIG_acc_origin_cb, &MPIDIG_acc_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_GET_ACC_REQ,
                                 &MPIDIG_get_acc_origin_cb, &MPIDIG_get_acc_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_ACC_ACK, NULL, &MPIDIG_acc_ack_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_GET_ACC_ACK,
                                 &MPIDIG_get_acc_ack_origin_cb, &MPIDIG_get_acc_ack_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_WIN_COMPLETE, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_WIN_POST, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_WIN_LOCK, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_WIN_LOCK_ACK, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_WIN_UNLOCK, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_WIN_UNLOCK_ACK, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_WIN_LOCKALL, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_WIN_LOCKALL_ACK, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_WIN_UNLOCKALL, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_WIN_UNLOCKALL_ACK, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_PUT_IOV_REQ,
                                 &MPIDIG_put_iov_origin_cb, &MPIDIG_put_iov_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_PUT_IOV_ACK, NULL, &MPIDIG_put_iov_ack_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_PUT_DAT_REQ,
                                 &MPIDIG_put_data_origin_cb, &MPIDIG_put_data_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_ACC_IOV_REQ,
                                 &MPIDIG_acc_iov_origin_cb, &MPIDIG_acc_iov_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_GET_ACC_IOV_REQ,
                                 &MPIDIG_get_acc_iov_origin_cb, &MPIDIG_get_acc_iov_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_ACC_IOV_ACK, NULL, &MPIDIG_acc_iov_ack_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_GET_ACC_IOV_ACK,
                                 NULL, &MPIDIG_get_acc_iov_ack_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_ACC_DAT_REQ,
                                 &MPIDIG_acc_data_origin_cb, &MPIDIG_acc_data_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_GET_ACC_DAT_REQ,
                                 &MPIDIG_get_acc_data_origin_cb,
                                 &MPIDIG_get_acc_data_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_COMM_ABORT,
                                 &MPIDIG_comm_abort_origin_cb, &MPIDIG_comm_abort_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);


    mpi_errno = MPIDIG_init_comm(comm_world);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_init_comm(comm_self);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDIU_map_create((void **) &(MPIDI_global.win_map), MPL_MEM_RMA);

    mpi_errno = MPIDIG_RMA_Init_sync_pvars();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_RMA_Init_targetcb_pvars();
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_global.is_ch4u_initialized = 1;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_INIT);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

void MPIDIG_finalize(void)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_FINALIZE);

    MPIDI_global.is_ch4u_initialized = 0;
    MPIDIU_map_destroy(MPIDI_global.win_map);
    MPIDIU_destroy_buf_pool(MPIDI_global.buf_pool);
    MPL_free(MPIDI_global.comm_req_lists);
    MPIDI_global.comm_req_lists = NULL;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_FINALIZE);
}
