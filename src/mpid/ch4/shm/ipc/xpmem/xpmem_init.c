/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "xpmem_noinline.h"
#include "mpidu_init_shm.h"
#include "xpmem_seg.h"
#include "shm_control.h"

static void regist_shm_ctrl_cb(void)
{
    MPIDI_SHM_ctrl_reg_cb(MPIDI_SHM_XPMEM_SEND_LMT_COOP_CTS, &MPIDI_IPC_xpmem_send_lmt_cts_cb);
    MPIDI_SHM_ctrl_reg_cb(MPIDI_SHM_XPMEM_SEND_LMT_COOP_SEND_FIN,
                          &MPIDI_IPC_xpmem_send_lmt_send_fin_cb);
    MPIDI_SHM_ctrl_reg_cb(MPIDI_SHM_XPMEM_SEND_LMT_COOP_RECV_FIN,
                          &MPIDI_IPC_xpmem_send_lmt_recv_fin_cb);
    MPIDI_SHM_ctrl_reg_cb(MPIDI_SHM_XPMEM_SEND_LMT_COOP_CNT_FREE,
                          &MPIDI_IPC_xpmem_send_lmt_cnt_free_cb);
}

int MPIDI_IPC_xpmem_mpi_init_hook(int rank, int size, int *tag_bits)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    bool anyfail = false;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPC_XPMEM_MPI_INIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPC_XPMEM_MPI_INIT_HOOK);
    MPIR_CHKPMEM_DECL(3);

#ifdef MPL_USE_DBG_LOGGING
    MPIDI_CH4_SHM_IPC_XPMEM_GENERAL = MPL_dbg_class_alloc("SHM_IPC_XPMEM", "shm_ipc_xpmem");
#endif /* MPL_USE_DBG_LOGGING */

    /* Try to share entire address space */
    MPIDI_IPC_xpmem_global.segid = xpmem_make(0, XPMEM_MAXADDR_SIZE, XPMEM_PERMIT_MODE,
                                              MPIDI_IPC_XPMEM_PERMIT_VALUE);
    /* 64-bit segment ID or failure(-1) */
    MPIR_ERR_CHKANDJUMP(MPIDI_IPC_xpmem_global.segid == -1, mpi_errno, MPI_ERR_OTHER,
                        "**xpmem_make");
    XPMEM_TRACE("init: make segid: 0x%lx\n", (uint64_t) MPIDI_IPC_xpmem_global.segid);

    MPIDI_IPC_xpmem_global.node_group_ptr = NULL;

    MPIDU_Init_shm_put(&MPIDI_IPC_xpmem_global.segid, sizeof(xpmem_segid_t));
    MPIDU_Init_shm_barrier();

    /* Initialize segmap for every local processes */
    MPIDI_IPC_xpmem_global.segmaps = NULL;
    MPIR_CHKPMEM_MALLOC(MPIDI_IPC_xpmem_global.segmaps, MPIDI_IPC_xpmem_segmap_t *,
                        sizeof(MPIDI_IPC_xpmem_segmap_t) * MPIR_Process.local_size,
                        mpi_errno, "xpmem segmaps", MPL_MEM_SHM);
    for (i = 0; i < MPIR_Process.local_size; i++) {
        MPIDU_Init_shm_get(i, sizeof(xpmem_segid_t),
                           &MPIDI_IPC_xpmem_global.segmaps[i].remote_segid);
        if (MPIDI_IPC_xpmem_global.segmaps[i].remote_segid == -1) {
            anyfail = true;
        }
        MPIDI_IPC_xpmem_global.segmaps[i].apid = -1;    /* get apid at the first communication  */
    }
    MPIDU_Init_shm_barrier();

    /* Check to make sure all processes initialized XPMEM correctly. */
    if (anyfail) {
        /* Not setting an mpi_errno value here because we can handle the failure of XPMEM
         * gracefully. */
        goto fn_fail;
    }

    /* Initialize other global parameters */
    MPIDI_IPC_xpmem_global.sys_page_sz = (size_t) sysconf(_SC_PAGESIZE);

    /* Initialize coop counter
     * direct counter obj is attached here and will be used throughout the program.
     * Once direct obj is used up, indirect counter obj is dynamically allocated in
     * p2p and attached per counter. We pay one-time attachment overhead and the
     * attached indirect objs can be reused in later communication. */
    MPIR_cc_set(&MPIDI_IPC_xpmem_global.num_pending_cnt, 0);

    MPIR_CHKPMEM_MALLOC(MPIDI_IPC_xpmem_global.coop_counter_direct, MPIDI_IPC_xpmem_cnt_t **,
                        sizeof(MPIDI_IPC_xpmem_cnt_t *) * MPIR_Process.local_size,
                        mpi_errno, "xpmem direct counter array", MPL_MEM_SHM);
    MPIR_CHKPMEM_MALLOC(MPIDI_IPC_xpmem_global.coop_counter_seg_direct, MPIDI_IPC_xpmem_seg_t **,
                        sizeof(MPIDI_IPC_xpmem_seg_t *) * MPIR_Process.local_size,
                        mpi_errno, "xpmem direct counter segs", MPL_MEM_SHM);

    uint64_t cnt_mem_addr = (uint64_t) (&MPIDI_IPC_xpmem_cnt_mem_direct);
    uint64_t remote_cnt_mem_addr;

    MPIDU_Init_shm_put(&cnt_mem_addr, sizeof(uint64_t));
    MPIDU_Init_shm_barrier();
    for (i = 0; i < MPIR_Process.local_size; ++i) {
        /* Init AVL tree based segment cache */
        MPIDI_IPC_xpmem_segtree_init(&MPIDI_IPC_xpmem_global.segmaps[i].segcache_ubuf); /* Initialize user buffer tree */
        MPIDI_IPC_xpmem_segtree_init(&MPIDI_IPC_xpmem_global.segmaps[i].segcache_cnt);
        if (i != MPIR_Process.local_rank) {
            MPIDU_Init_shm_get(i, sizeof(uint64_t), &remote_cnt_mem_addr);
            mpi_errno =
                MPIDI_IPC_xpmem_seg_regist(i,
                                           sizeof(MPIDI_IPC_xpmem_cnt_t) *
                                           MPIDI_IPC_XPMEM_CNT_PREALLOC,
                                           (void *) remote_cnt_mem_addr,
                                           &MPIDI_IPC_xpmem_global.coop_counter_seg_direct[i],
                                           (void **) &MPIDI_IPC_xpmem_global.coop_counter_direct[i],
                                           &MPIDI_IPC_xpmem_global.segmaps[i].segcache_cnt);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }
    MPIDU_Init_shm_barrier();

    regist_shm_ctrl_cb();

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPC_XPMEM_MPI_INIT_HOOK);
    return mpi_errno;
  fn_fail:
    if (MPIDI_IPC_xpmem_global.segid != -1) {
        xpmem_remove(MPIDI_IPC_xpmem_global.segid);
    }

    /* While this version of MPICH does require libxpmem to link, we don't necessarily require the
     * kernel module to be loaded at runtime. If XPMEM is not available, disable its use via the
     * special CVAR value. */
    XPMEM_TRACE("init: xpmem_make failed. Disabling XPMEM support");
    MPIR_CVAR_CH4_XPMEM_LMT_MSG_SIZE = -1;

    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIDI_IPC_xpmem_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i, ret = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPC_XPMEM_MPI_FINALIZE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPC_XPMEM_MPI_FINALIZE_HOOK);

    /* Ensure all counter objs are freed at MPIDI_IPC_xpmem_ctrl_send_lmt_cnt_free_cb */
    while (MPIR_cc_get(MPIDI_IPC_xpmem_global.num_pending_cnt)) {
        /* Since it is non-critical in finalize, we call global progress
         * instead of shm/posix progress for simplicity */
        MPID_Progress_test();
    }

    /* Free pre-attached direct coop counter */
    for (i = 0; i < MPIR_Process.local_size; ++i) {
        if (i != MPIR_Process.local_rank)
            MPIDI_IPC_xpmem_seg_deregist(MPIDI_IPC_xpmem_global.coop_counter_seg_direct[i]);
    }
    MPL_free(MPIDI_IPC_xpmem_global.coop_counter_direct);
    MPL_free(MPIDI_IPC_xpmem_global.coop_counter_seg_direct);

    for (i = 0; i < MPIR_Process.local_size; i++) {
        /* should be called before xpmem_release
         * MPIDI_IPC_xpmem_segtree_tree_delete_all will call xpmem_detach */
        MPIDI_IPC_xpmem_segtree_delete_all(&MPIDI_IPC_xpmem_global.segmaps[i].segcache_ubuf);
        MPIDI_IPC_xpmem_segtree_delete_all(&MPIDI_IPC_xpmem_global.segmaps[i].segcache_cnt);
        if (MPIDI_IPC_xpmem_global.segmaps[i].apid != -1) {
            XPMEM_TRACE("finalize: release apid: node_rank %d, 0x%lx\n",
                        i, (uint64_t) MPIDI_IPC_xpmem_global.segmaps[i].apid);
            ret = xpmem_release(MPIDI_IPC_xpmem_global.segmaps[i].apid);
            /* success(0) or failure(-1) */
            MPIR_ERR_CHKANDJUMP(ret == -1, mpi_errno, MPI_ERR_OTHER, "**xpmem_release");
        }
    }

    MPL_free(MPIDI_IPC_xpmem_global.segmaps);

    if (MPIDI_IPC_xpmem_global.segid != -1) {
        XPMEM_TRACE("finalize: remove segid: 0x%lx\n", (uint64_t) MPIDI_IPC_xpmem_global.segid);
        ret = xpmem_remove(MPIDI_IPC_xpmem_global.segid);
        /* success(0) or failure(-1) */
        MPIR_ERR_CHKANDJUMP(ret == -1, mpi_errno, MPI_ERR_OTHER, "**xpmem_remove");
    }

    if (MPIDI_IPC_xpmem_global.node_group_ptr) {
        mpi_errno = MPIR_Group_free_impl(MPIDI_IPC_xpmem_global.node_group_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPC_XPMEM_MPI_FINALIZE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
