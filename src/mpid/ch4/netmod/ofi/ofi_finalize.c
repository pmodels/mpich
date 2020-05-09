/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_noinline.h"

#define MPIDI_OFI_FLUSH_CONTEXT_ID 0xF000
#define MPIDI_OFI_FLUSH_TAG        1

/* send a dummy message to flush the send queue */
static int flush_send(int dst)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Comm *comm = MPIR_Process.comm_world;
    int data = 0;
    uint64_t match_bits = MPIDI_OFI_init_sendtag(MPIDI_OFI_FLUSH_CONTEXT_ID,
                                                 MPIDI_OFI_FLUSH_TAG, MPIDI_OFI_DYNPROC_SEND);

    /* Use the same direct send method as used in establishing dynamic processes */
    MPIDI_OFI_dynamic_process_request_t req;
    req.done = 0;
    req.event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;

    MPIDI_OFI_CALL_RETRY(fi_tsenddata(MPIDI_OFI_global.ctx[0].tx, &data, 4, NULL, 0,
                                      MPIDI_OFI_av_to_phys(MPIDIU_comm_rank_to_av(comm, dst)),
                                      match_bits, &req.context), tsenddata, FALSE);
    MPIDI_OFI_PROGRESS_WHILE(!req.done);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* recv the dummy message the other process sent for the purpose flushing send queue */
static int flush_recv(int src)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Comm *comm = MPIR_Process.comm_world;
    int data;
    uint64_t mask_bits = 0;
    uint64_t match_bits = MPIDI_OFI_init_sendtag(MPIDI_OFI_FLUSH_CONTEXT_ID,
                                                 MPIDI_OFI_FLUSH_TAG, MPIDI_OFI_DYNPROC_SEND);

    /* Use the same direct recv method as used in establishing dynamic processes */
    MPIDI_OFI_dynamic_process_request_t req;
    req.done = 0;
    req.event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;

    MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_global.ctx[0].rx, &data, 4, NULL,
                                  MPIDI_OFI_av_to_phys(MPIDIU_comm_rank_to_av(comm, src)),
                                  match_bits, mask_bits, &req.context), trecv, FALSE);
    MPIDI_OFI_PROGRESS_WHILE(!req.done);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* every process send and recv a dummy message to clear out the send queue */
static int flush_send_queue(void)
{
    int mpi_errno = MPI_SUCCESS;

    int n = MPIR_Process.size;
    int i = MPIR_Process.rank;
    if (n > 1) {
        int dst = (i + 1) % n;
        int src = (i - 1 + n) % n;
        if (i % 2 == 0) {
            mpi_errno = flush_send(dst);
            MPIR_ERR_CHECK(mpi_errno);
            mpi_errno = flush_recv(src);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            mpi_errno = flush_recv(src);
            MPIR_ERR_CHECK(mpi_errno);
            mpi_errno = flush_send(dst);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_FINALIZE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_FINALIZE_HOOK);

    /* clean dynamic process connections */
    mpi_errno = MPIDI_OFI_conn_manager_destroy();
    MPIR_ERR_CHECK(mpi_errno);

    /* Progress until we drain all inflight RMA send long buffers */
    while (MPL_atomic_load_int(&MPIDI_OFI_global.am_inflight_rma_send_mrs) > 0)
        MPIDI_OFI_PROGRESS();

    /* Destroy RMA key allocator */
    MPIDI_OFI_mr_key_allocator_destroy();

    /* Flush any last lightweight send */
    mpi_errno = flush_send_queue();
    MPIR_ERR_CHECK(mpi_errno);

    /* Progress until we drain all inflight injection emulation requests */
    while (MPL_atomic_load_int(&MPIDI_OFI_global.am_inflight_inject_emus) > 0)
        MPIDI_OFI_PROGRESS();
    MPIR_Assert(MPL_atomic_load_int(&MPIDI_OFI_global.am_inflight_inject_emus) == 0);

    mpi_errno = MPIDI_OFI_vni_finalize();
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_global.fabric->fid), fabricclose);

    fi_freeinfo(MPIDI_OFI_global.prov_use);

    MPIDIU_map_destroy(MPIDI_OFI_global.win_map);

    if (MPIDI_OFI_ENABLE_AM) {
        while (MPIDI_OFI_global.am_unordered_msgs) {
            MPIDI_OFI_am_unordered_msg_t *uo_msg = MPIDI_OFI_global.am_unordered_msgs;
            DL_DELETE(MPIDI_OFI_global.am_unordered_msgs, uo_msg);
        }
        MPIDIU_map_destroy(MPIDI_OFI_global.am_send_seq_tracker);
        MPIDIU_map_destroy(MPIDI_OFI_global.am_recv_seq_tracker);

        for (i = 0; i < MPIDI_OFI_NUM_AM_BUFFERS; i++)
            MPL_free(MPIDI_OFI_global.am_bufs[i]);

        MPIDIU_destroy_buf_pool(MPIDI_OFI_global.am_buf_pool);

        MPIR_Assert(MPIDI_OFI_global.cq_buffered_static_head ==
                    MPIDI_OFI_global.cq_buffered_static_tail);
        MPIR_Assert(NULL == MPIDI_OFI_global.cq_buffered_dynamic_head);
    }

    int err;
    MPID_Thread_mutex_destroy(&MPIDI_OFI_THREAD_UTIL_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_destroy(&MPIDI_OFI_THREAD_PROGRESS_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_destroy(&MPIDI_OFI_THREAD_FI_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPID_Thread_mutex_destroy(&MPIDI_OFI_THREAD_SPAWN_MUTEX, &err);
    MPIR_Assert(err == 0);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_FINALIZE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
