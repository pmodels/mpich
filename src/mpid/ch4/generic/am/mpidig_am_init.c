/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidig_am.h"
#include "mpidch4r.h"
#include "mpidu_genq.h"

static int dynamic_am_handler_id = MPIDIG_HANDLER_STATIC_MAX;

static void *host_alloc(uintptr_t size);
static void host_free(void *ptr);

static void *host_alloc(uintptr_t size)
{
    return MPL_malloc(size, MPL_MEM_BUFFER);
}

static void host_free(void *ptr)
{
    MPL_free(ptr);
}

int MPIDIG_am_reg_cb_dynamic(MPIDIG_am_origin_cb origin_cb, MPIDIG_am_target_msg_cb target_msg_cb)
{
    if (dynamic_am_handler_id < MPIDI_AM_HANDLERS_MAX) {
        MPIDIG_am_reg_cb(dynamic_am_handler_id, origin_cb, target_msg_cb);
        dynamic_am_handler_id++;
        return dynamic_am_handler_id - 1;
    } else {
        return -1;
    }
}

void MPIDIG_am_reg_cb(int handler_id,
                      MPIDIG_am_origin_cb origin_cb, MPIDIG_am_target_msg_cb target_msg_cb)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_AM_REG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_AM_REG_CB);

    MPIDIG_global.target_msg_cbs[handler_id] = target_msg_cb;
    MPIDIG_global.origin_cbs[handler_id] = origin_cb;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_AM_REG_CB);
}

int MPIDIG_am_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_AM_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_AM_INIT);

    MPIDI_global.comm_req_lists = (MPIDIG_comm_req_list_t *)
        MPL_calloc(MPIR_MAX_CONTEXT_MASK * MPIR_CONTEXT_INT_BITS,
                   sizeof(MPIDIG_comm_req_list_t), MPL_MEM_OTHER);
#ifndef MPIDI_CH4U_USE_PER_COMM_QUEUE
    MPIDI_global.posted_list = NULL;
    MPIDI_global.unexp_list = NULL;
#endif

    MPIDI_global.cmpl_list = NULL;
    MPL_atomic_store_uint64(&MPIDI_global.exp_seq_no, 0);
    MPL_atomic_store_uint64(&MPIDI_global.nxt_seq_no, 0);

    mpi_errno =
        MPIDU_genq_private_pool_create_unsafe(MPIDIU_REQUEST_POOL_CELL_SIZE,
                                              MPIDIU_REQUEST_POOL_NUM_CELLS_PER_CHUNK,
                                              MPIDIU_REQUEST_POOL_MAX_NUM_CELLS, host_alloc,
                                              host_free, &MPIDI_global.request_pool);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Assert(MPIDIG_HANDLER_STATIC_MAX <= MPIDI_AM_HANDLERS_MAX);

    MPIDIG_am_reg_cb(MPIDIG_SEND, &MPIDIG_send_origin_cb, &MPIDIG_send_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_SEND_LONG_REQ, NULL, &MPIDIG_send_long_req_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_SEND_LONG_ACK, NULL, &MPIDIG_send_long_ack_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_SEND_LONG_LMT,
                     &MPIDIG_send_long_lmt_origin_cb, &MPIDIG_send_long_lmt_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_SSEND_REQ, &MPIDIG_send_origin_cb, &MPIDIG_ssend_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_SSEND_ACK,
                     &MPIDIG_ssend_ack_origin_cb, &MPIDIG_ssend_ack_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_PUT_REQ, &MPIDIG_put_origin_cb, &MPIDIG_put_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_PUT_ACK, NULL, &MPIDIG_put_ack_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_GET_REQ, &MPIDIG_get_origin_cb, &MPIDIG_get_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_GET_ACK, &MPIDIG_get_ack_origin_cb, &MPIDIG_get_ack_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_CSWAP_REQ, &MPIDIG_cswap_origin_cb, &MPIDIG_cswap_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_CSWAP_ACK,
                     &MPIDIG_cswap_ack_origin_cb, &MPIDIG_cswap_ack_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_ACC_REQ, &MPIDIG_acc_origin_cb, &MPIDIG_acc_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_GET_ACC_REQ, &MPIDIG_get_acc_origin_cb, &MPIDIG_get_acc_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_ACC_ACK, NULL, &MPIDIG_acc_ack_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_GET_ACC_ACK,
                     &MPIDIG_get_acc_ack_origin_cb, &MPIDIG_get_acc_ack_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_WIN_COMPLETE, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_WIN_POST, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_WIN_LOCK, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_WIN_LOCK_ACK, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_WIN_UNLOCK, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_WIN_UNLOCK_ACK, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_WIN_LOCKALL, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_WIN_LOCKALL_ACK, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_WIN_UNLOCKALL, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_WIN_UNLOCKALL_ACK, NULL, &MPIDIG_win_ctrl_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_PUT_DT_REQ, &MPIDIG_put_dt_origin_cb, &MPIDIG_put_dt_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_PUT_DT_ACK, NULL, &MPIDIG_put_dt_ack_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_PUT_DAT_REQ,
                     &MPIDIG_put_data_origin_cb, &MPIDIG_put_data_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_ACC_DT_REQ, &MPIDIG_acc_dt_origin_cb, &MPIDIG_acc_dt_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_GET_ACC_DT_REQ,
                     &MPIDIG_get_acc_dt_origin_cb, &MPIDIG_get_acc_dt_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_ACC_DT_ACK, NULL, &MPIDIG_acc_dt_ack_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_GET_ACC_DT_ACK, NULL, &MPIDIG_get_acc_dt_ack_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_ACC_DAT_REQ,
                     &MPIDIG_acc_data_origin_cb, &MPIDIG_acc_data_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_GET_ACC_DAT_REQ,
                     &MPIDIG_get_acc_data_origin_cb, &MPIDIG_get_acc_data_target_msg_cb);

    MPIDIG_am_comm_abort_init();

    mpi_errno = MPIDIG_RMA_Init_sync_pvars();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_RMA_Init_targetcb_pvars();
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_AM_INIT);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

void MPIDIG_am_finalize(void)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_AM_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_AM_FINALIZE);

    MPIDIU_map_destroy(MPIDI_global.win_map);
    MPIDU_genq_private_pool_destroy_unsafe(MPIDI_global.request_pool);
    MPL_free(MPIDI_global.comm_req_lists);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_AM_FINALIZE);
}
