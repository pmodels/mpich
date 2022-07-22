/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidch4r.h"
#include "mpidu_genq.h"

MPIDIG_global_t MPIDIG_global;

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

int MPIDIG_am_check_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    size_t buf_size_limit ATTRIBUTE((unused)) = 0;
#ifdef MPIDI_CH4_DIRECT_NETMOD
    buf_size_limit = MPIDI_NM_am_eager_buf_limit();
#else
    buf_size_limit = MPL_MAX(MPIDI_SHM_am_eager_buf_limit(), MPIDI_NM_am_eager_buf_limit());
#endif
    MPIR_Assert(MPIR_CVAR_CH4_PACK_BUFFER_SIZE >= buf_size_limit);
    return mpi_errno;
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
    MPIR_FUNC_ENTER;

    MPIDIG_global.target_msg_cbs[handler_id] = target_msg_cb;
    MPIDIG_global.origin_cbs[handler_id] = origin_cb;

    MPIR_FUNC_EXIT;
}

void MPIDIG_am_rndv_reg_cb(int rndv_id, MPIDIG_am_rndv_cb rndv_cb)
{
    MPIR_FUNC_ENTER;

    MPIR_Assert(rndv_id < MPIDIG_RNDV_STATIC_MAX);
    MPIDIG_global.rndv_cbs[rndv_id] = rndv_cb;

    MPIR_FUNC_EXIT;
}


int MPIDIG_am_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    for (int vci = 0; vci < MPIDI_global.n_total_vcis; vci++) {
        MPIDI_global.per_vci[vci].posted_list = NULL;
        MPIDI_global.per_vci[vci].unexp_list = NULL;

        mpi_errno = MPIDU_genq_private_pool_create(MPIDIU_REQUEST_POOL_CELL_SIZE,
                                                   MPIDIU_REQUEST_POOL_NUM_CELLS_PER_CHUNK,
                                                   0 /* unlimited */ ,
                                                   host_alloc, host_free,
                                                   &MPIDI_global.per_vci[vci].request_pool);
        MPIR_ERR_CHECK(mpi_errno);

        MPIDI_global.per_vci[vci].cmpl_list = NULL;
        MPL_atomic_store_uint64(&MPIDI_global.per_vci[vci].exp_seq_no, 0);
        MPL_atomic_store_uint64(&MPIDI_global.per_vci[vci].nxt_seq_no, 0);
    }

    MPIDI_global.part_posted_list = NULL;
    MPIDI_global.part_unexp_list = NULL;

    MPL_atomic_store_int(&MPIDIG_global.rma_am_flag, 0);
    MPIR_cc_set(&MPIDIG_global.rma_am_poll_cntr, 0);

    MPIR_Assert(MPIDIG_HANDLER_STATIC_MAX <= MPIDI_AM_HANDLERS_MAX);

    MPIDIG_am_reg_cb(MPIDIG_SEND, &MPIDIG_send_origin_cb, &MPIDIG_send_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_SEND_CTS, NULL, &MPIDIG_send_cts_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_SEND_DATA,
                     &MPIDIG_send_data_origin_cb, &MPIDIG_send_data_target_msg_cb);

    MPIDIG_am_reg_cb(MPIDIG_SSEND_ACK, NULL, &MPIDIG_ssend_ack_target_msg_cb);

    MPIDIG_am_reg_cb(MPIDIG_PART_SEND_INIT, NULL, &MPIDIG_part_send_init_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_PART_CTS, NULL, &MPIDIG_part_cts_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_PART_SEND_DATA, &MPIDIG_part_send_data_origin_cb,
                     &MPIDIG_part_send_data_target_msg_cb);

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
    MPIDIG_am_reg_cb(MPIDIG_WIN_COMPLETE, NULL, &MPIDIG_win_complete_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_WIN_POST, NULL, &MPIDIG_win_post_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_WIN_LOCK, NULL, &MPIDIG_win_lock_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_WIN_LOCK_ACK, NULL, &MPIDIG_win_lock_ack_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_WIN_UNLOCK, NULL, &MPIDIG_win_unlock_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_WIN_UNLOCK_ACK, NULL, &MPIDIG_win_unlock_ack_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_WIN_LOCKALL, NULL, &MPIDIG_win_lockall_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_WIN_LOCKALL_ACK, NULL, &MPIDIG_win_lockall_ack_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_WIN_UNLOCKALL, NULL, &MPIDIG_win_unlockall_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDIG_WIN_UNLOCKALL_ACK, NULL, &MPIDIG_win_unlockall_ack_target_msg_cb);
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

    MPIDIG_am_rndv_reg_cb(MPIDIG_RNDV_GENERIC, &MPIDIG_do_cts);

    MPIDIG_am_comm_abort_init();

    mpi_errno = MPIDIG_RMA_Init_sync_pvars();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_RMA_Init_targetcb_pvars();
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_FUNC_EXIT;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

void MPIDIG_am_finalize(void)
{
    MPIR_FUNC_ENTER;

    MPIDIU_map_destroy(MPIDI_global.win_map);
    for (int vci = 0; vci < MPIDI_global.n_total_vcis; vci++) {
        MPIDU_genq_private_pool_destroy(MPIDI_global.per_vci[vci].request_pool);
    }

    MPIR_FUNC_EXIT;
}

int MPIDIG_init_comm(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    if (MPIR_CONTEXT_READ_FIELD(DYNAMIC_PROC, comm->recvcontext_id))
        goto fn_exit;

    MPIDIG_COMM(comm, window_instance) = 0;
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIG_destroy_comm(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (MPIR_CONTEXT_READ_FIELD(DYNAMIC_PROC, comm->recvcontext_id))
        goto fn_exit;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

void *MPIDIG_mpi_alloc_mem(MPI_Aint size, MPIR_Info * info_ptr)
{
    MPIR_FUNC_ENTER;
    void *p;

    p = MPL_malloc(size, MPL_MEM_USER);

    MPIR_FUNC_EXIT;
    return p;
}

int MPIDIG_mpi_free_mem(void *ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPL_free(ptr);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}
