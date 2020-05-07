/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "xpmem_impl.h"
#include "xpmem_noinline.h"
#include "mpidu_init_shm.h"
#include "xpmem_seg.h"

int MPIDI_XPMEM_mpi_init_hook(int rank, int size, int *tag_bits)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    bool anyfail = false;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_MPI_INIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_MPI_INIT_HOOK);
    MPIR_CHKPMEM_DECL(3);

#ifdef MPL_USE_DBG_LOGGING
    MPIDI_CH4_SHM_XPMEM_GENERAL = MPL_dbg_class_alloc("SHM_XPMEM", "shm_xpmem");
#endif /* MPL_USE_DBG_LOGGING */

    /* Try to share entire address space */
    MPIDI_XPMEM_global.segid = xpmem_make(0, XPMEM_MAXADDR_SIZE, XPMEM_PERMIT_MODE,
                                          MPIDI_XPMEM_PERMIT_VALUE);
    /* 64-bit segment ID or failure(-1) */
    MPIR_ERR_CHKANDJUMP(MPIDI_XPMEM_global.segid == -1, mpi_errno, MPI_ERR_OTHER, "**xpmem_make");
    XPMEM_TRACE("init: make segid: 0x%lx\n", (uint64_t) MPIDI_XPMEM_global.segid);

    int num_local = MPIR_Process.local_size;
    MPIDI_XPMEM_global.num_local = num_local;
    MPIDI_XPMEM_global.local_rank = MPIR_Process.local_rank;
    MPIDI_XPMEM_global.node_group_ptr = NULL;

    MPIDU_Init_shm_put(&MPIDI_XPMEM_global.segid, sizeof(xpmem_segid_t));
    MPIDU_Init_shm_barrier();

    /* Initialize segmap for every local processes */
    MPIDI_XPMEM_global.segmaps = NULL;
    MPIR_CHKPMEM_MALLOC(MPIDI_XPMEM_global.segmaps, MPIDI_XPMEM_segmap_t *,
                        sizeof(MPIDI_XPMEM_segmap_t) * num_local,
                        mpi_errno, "xpmem segmaps", MPL_MEM_SHM);
    for (i = 0; i < num_local; i++) {
        MPIDU_Init_shm_get(i, sizeof(xpmem_segid_t), &MPIDI_XPMEM_global.segmaps[i].remote_segid);
        if (MPIDI_XPMEM_global.segmaps[i].remote_segid == -1) {
            anyfail = true;
        }
        MPIDI_XPMEM_global.segmaps[i].apid = -1;        /* get apid at the first communication  */
    }
    MPIDU_Init_shm_barrier();

    /* Check to make sure all processes initialized XPMEM correctly. */
    if (anyfail) {
        /* Not setting an mpi_errno value here because we can handle the failure of XPMEM
         * gracefully. */
        goto fn_fail;
    }

    /* Initialize other global parameters */
    MPIDI_XPMEM_global.sys_page_sz = (size_t) sysconf(_SC_PAGESIZE);

    /* Initialize coop counter
     * direct counter obj is attached here and will be used throughout the program.
     * Once direct obj is used up, indirect counter obj is dynamically allocated in
     * p2p and attached per counter. We pay one-time attachment overhead and the
     * attached indirect objs can be reused in later communication. */
    MPIR_cc_set(&MPIDI_XPMEM_global.num_pending_cnt, 0);

    MPIR_CHKPMEM_MALLOC(MPIDI_XPMEM_global.coop_counter_direct, MPIDI_XPMEM_cnt_t **,
                        sizeof(MPIDI_XPMEM_cnt_t *) * num_local,
                        mpi_errno, "xpmem direct counter array", MPL_MEM_SHM);
    MPIR_CHKPMEM_MALLOC(MPIDI_XPMEM_global.coop_counter_seg_direct, MPIDI_XPMEM_seg_t **,
                        sizeof(MPIDI_XPMEM_seg_t *) * num_local,
                        mpi_errno, "xpmem direct counter segs", MPL_MEM_SHM);

    uint64_t cnt_mem_addr = (uint64_t) (&MPIDI_XPMEM_cnt_mem_direct);
    uint64_t remote_cnt_mem_addr;

    MPIDU_Init_shm_put(&cnt_mem_addr, sizeof(uint64_t));
    MPIDU_Init_shm_barrier();
    for (i = 0; i < num_local; ++i) {
        /* Init AVL tree based segment cache */
        MPIDI_XPMEM_segtree_init(&MPIDI_XPMEM_global.segmaps[i].segcache_ubuf); /* Initialize user buffer tree */
        MPIDI_XPMEM_segtree_init(&MPIDI_XPMEM_global.segmaps[i].segcache_cnt);
        if (i != MPIDI_XPMEM_global.local_rank) {
            MPIDU_Init_shm_get(i, sizeof(uint64_t), &remote_cnt_mem_addr);
            mpi_errno =
                MPIDI_XPMEM_seg_regist(i, sizeof(MPIDI_XPMEM_cnt_t) * MPIDI_XPMEM_CNT_PREALLOC,
                                       (void *) remote_cnt_mem_addr,
                                       &MPIDI_XPMEM_global.coop_counter_seg_direct[i],
                                       (void **) &MPIDI_XPMEM_global.coop_counter_direct[i],
                                       &MPIDI_XPMEM_global.segmaps[i].segcache_cnt);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }
    MPIDU_Init_shm_barrier();

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_MPI_INIT_HOOK);
    return mpi_errno;
  fn_fail:
    if (MPIDI_XPMEM_global.segid != -1) {
        xpmem_remove(MPIDI_XPMEM_global.segid);
    }

    /* While this version of MPICH does require libxpmem to link, we don't necessarily require the
     * kernel module to be loaded at runtime. If XPMEM is not available, disable its use via the
     * special CVAR value. */
    XPMEM_TRACE("init: xpmem_make failed. Disabling XPMEM support");
    MPIR_CVAR_CH4_XPMEM_LMT_MSG_SIZE = -1;

    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIDI_XPMEM_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i, ret = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_MPI_FINALIZE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_MPI_FINALIZE_HOOK);

    /* Ensure all counter objs are freed at MPIDI_XPMEM_ctrl_send_lmt_cnt_free_cb */
    while (MPIR_cc_get(MPIDI_XPMEM_global.num_pending_cnt)) {
        /* Since it is non-critical in finalize, we call global progress
         * instead of shm/posix progress for simplicity */
        MPID_Progress_test();
    }

    /* Free pre-attached direct coop counter */
    for (i = 0; i < MPIDI_XPMEM_global.num_local; ++i) {
        if (i != MPIDI_XPMEM_global.local_rank)
            MPIDI_XPMEM_seg_deregist(MPIDI_XPMEM_global.coop_counter_seg_direct[i]);
    }
    MPL_free(MPIDI_XPMEM_global.coop_counter_direct);
    MPL_free(MPIDI_XPMEM_global.coop_counter_seg_direct);

    for (i = 0; i < MPIDI_XPMEM_global.num_local; i++) {
        /* should be called before xpmem_release
         * MPIDI_XPMEM_segtree_tree_delete_all will call xpmem_detach */
        MPIDI_XPMEM_segtree_delete_all(&MPIDI_XPMEM_global.segmaps[i].segcache_ubuf);
        MPIDI_XPMEM_segtree_delete_all(&MPIDI_XPMEM_global.segmaps[i].segcache_cnt);
        if (MPIDI_XPMEM_global.segmaps[i].apid != -1) {
            XPMEM_TRACE("finalize: release apid: node_rank %d, 0x%lx\n",
                        i, (uint64_t) MPIDI_XPMEM_global.segmaps[i].apid);
            ret = xpmem_release(MPIDI_XPMEM_global.segmaps[i].apid);
            /* success(0) or failure(-1) */
            MPIR_ERR_CHKANDJUMP(ret == -1, mpi_errno, MPI_ERR_OTHER, "**xpmem_release");
        }
    }

    MPL_free(MPIDI_XPMEM_global.segmaps);

    if (MPIDI_XPMEM_global.segid != -1) {
        XPMEM_TRACE("finalize: remove segid: 0x%lx\n", (uint64_t) MPIDI_XPMEM_global.segid);
        ret = xpmem_remove(MPIDI_XPMEM_global.segid);
        /* success(0) or failure(-1) */
        MPIR_ERR_CHKANDJUMP(ret == -1, mpi_errno, MPI_ERR_OTHER, "**xpmem_remove");
    }

    if (MPIDI_XPMEM_global.node_group_ptr) {
        mpi_errno = MPIR_Group_free_impl(MPIDI_XPMEM_global.node_group_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_MPI_FINALIZE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
