/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "xpmem_impl.h"
#include "xpmem_noinline.h"
#include "mpir_nodemap.h"
#include "mpidu_init_shm.h"
#include "xpmem_seg.h"

int MPIDI_XPMEM_mpi_init_hook(int rank, int size, int *n_vsis_provided, int *tag_bits)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    bool anyfail = false;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_INIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_INIT_HOOK);
    MPIR_CHKPMEM_DECL(1);

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

        /* Init AVL tree based segment cache */
        MPIDI_XPMEM_segtree_init(&MPIDI_XPMEM_global.segmaps[i].segcache);
    }

    /* Check to make sure all processes initialized XPMEM correctly. */
    if (anyfail) {
        for (i = 0; i < num_local; i++) {
            MPIDI_XPMEM_segtree_delete_all(&MPIDI_XPMEM_global.segmaps[i].segcache);
        }
        /* Not setting an mpi_errno value here because we can handle the failure of XPMEM
         * gracefully. */
        goto fn_fail;
    }

    /* Initialize other global parameters */
    MPIDI_XPMEM_global.sys_page_sz = (size_t) sysconf(_SC_PAGESIZE);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_INIT_HOOK);
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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_FINALIZE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_FINALIZE_HOOK);

    for (i = 0; i < MPIDI_XPMEM_global.num_local; i++) {
        /* should be called before xpmem_release
         * MPIDI_XPMEM_segtree_tree_delete_all will call xpmem_detach */
        MPIDI_XPMEM_segtree_delete_all(&MPIDI_XPMEM_global.segmaps[i].segcache);
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_FINALIZE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
