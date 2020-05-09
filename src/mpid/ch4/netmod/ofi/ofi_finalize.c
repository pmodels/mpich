/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_noinline.h"

int MPIDI_OFI_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i = 0;
    int barrier[2] = { 0 };
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

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

    /* Barrier over allreduce, but force non-immediate send */
    MPIDI_OFI_global.max_buffered_send = 0;
    mpi_errno = MPIR_Allreduce_allcomm_auto(&barrier[0], &barrier[1], 1, MPI_INT, MPI_SUM,
                                            MPIR_Process.comm_world, &errflag);
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
