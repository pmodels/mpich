/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "xpmem_post.h"
#include "mpidu_init_shm.h"
#include "xpmem_seg.h"

int MPIDI_XPMEM_init_local(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    MPIR_CHKPMEM_DECL();

    MPIDI_XPMEMI_global.segid = -1;

    /* If the user has disabled XPMEM, don't even try to initialize it. */
    if (!MPIR_CVAR_CH4_XPMEM_ENABLE) {
        goto fn_exit;
    }

    if (MPIR_Process.local_size == 1) {
        goto fn_exit;
    }
#ifdef MPL_USE_DBG_LOGGING
    MPIDI_XPMEMI_DBG_GENERAL = MPL_dbg_class_alloc("SHM_IPC_XPMEM", "shm_ipc_xpmem");
#endif /* MPL_USE_DBG_LOGGING */

    /* Try to share entire address space */
    MPIDI_XPMEMI_global.segid = xpmem_make(0, XPMEM_MAXADDR_SIZE, XPMEM_PERMIT_MODE,
                                           MPIDI_XPMEMI_PERMIT_VALUE);
#ifdef MPIDI_CH4_SHM_XPMEM_ALLOW_SILENT_FALLBACK
    if (MPIDI_XPMEMI_global.segid == -1) {
        /* do not throw an error; instead gracefully disable XPMEM */
        goto fn_fail;
    }
#else
    MPIR_ERR_CHKANDJUMP(MPIDI_XPMEMI_global.segid == -1, mpi_errno, MPI_ERR_OTHER, "**xpmem_make");
#endif

    /* 64-bit segment ID */
    XPMEM_TRACE("init: make segid: 0x%lx\n", (uint64_t) MPIDI_XPMEMI_global.segid);

    /* Initialize segmap for every local processes */
    MPIDI_XPMEMI_global.segmaps = NULL;
    MPIR_CHKPMEM_MALLOC(MPIDI_XPMEMI_global.segmaps,
                        sizeof(MPIDI_XPMEMI_segmap_t) * MPIR_Process.local_size, MPL_MEM_SHM);
    for (int i = 0; i < MPIR_Process.local_size; i++) {
        MPIDI_XPMEMI_global.segmaps[i].remote_segid = -1;
        MPIDI_XPMEMI_global.segmaps[i].apid = -1;       /* get apid at the first communication  */
    }

    /* Initialize other global parameters */
    MPIDI_XPMEMI_global.sys_page_sz = (size_t) sysconf(_SC_PAGESIZE);

    for (int i = 0; i < MPIR_Process.local_size; ++i) {
        /* Init AVL tree based segment cache */
        MPIDI_XPMEMI_segtree_init(&MPIDI_XPMEMI_global.segmaps[i].segcache_ubuf);       /* Initialize user buffer tree */
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
  fn_fail:
    if (MPIDI_XPMEMI_global.segid != -1) {
        xpmem_remove(MPIDI_XPMEMI_global.segid);
    }

    /* While this version of MPICH does require libxpmem to link, we don't necessarily require the
     * kernel module to be loaded at runtime. If XPMEM is not available, disable its use via the
     * special CVAR value. */
    XPMEM_TRACE("init: xpmem_make failed. Disabling XPMEM support");
    MPIR_CVAR_CH4_XPMEM_ENABLE = 0;

    goto fn_exit;
}

int MPIDI_XPMEM_comm_bootstrap(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    MPIR_CHKLMEM_DECL();

    int local_size = comm->num_local;
    if (local_size > 1) {
        /* NOTE: always run Allgather in case some process disable XPMEM inconsistently */
        MPIR_Comm *node_comm = MPIR_Comm_get_node_comm(comm);
        MPIR_Assert(node_comm);

        xpmem_segid_t *all_segids;
        MPIR_CHKLMEM_MALLOC(all_segids, local_size * sizeof(xpmem_segid_t));

        mpi_errno = MPIR_Allgather_impl(&MPIDI_XPMEMI_global.segid, sizeof(xpmem_segid_t),
                                        MPIR_BYTE_INTERNAL, all_segids, sizeof(xpmem_segid_t),
                                        MPIR_BYTE_INTERNAL, node_comm, MPIR_ERR_NONE);
        MPIR_ERR_CHECK(mpi_errno);

        int num_disabled = 0;
        for (int i = 0; i < local_size; i++) {
            int grank = MPIDIU_get_grank(i, node_comm);
            int local_id = MPIDI_SHM_global.local_ranks[grank];;
            MPIDI_XPMEMI_global.segmaps[local_id].remote_segid = all_segids[i];
            if (all_segids[i] == -1) {
                num_disabled++;
            }
        }
        MPIR_Assertp(num_disabled == 0 || num_disabled == local_size);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_XPMEM_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i, ret = 0;
    MPIR_FUNC_ENTER;

    if (MPIDI_XPMEMI_global.segid == -1) {
        /* if XPMEM was disabled at runtime, return */
        goto fn_exit;
    }

    for (i = 0; i < MPIR_Process.local_size; i++) {
        /* should be called before xpmem_release
         * MPIDI_XPMEMI_segtree_delete_all will call xpmem_detach */
        MPL_gavl_tree_destroy(MPIDI_XPMEMI_global.segmaps[i].segcache_ubuf);
        if (MPIDI_XPMEMI_global.segmaps[i].apid != -1) {
            XPMEM_TRACE("finalize: release apid: node_rank %d, 0x%lx\n",
                        i, (uint64_t) MPIDI_XPMEMI_global.segmaps[i].apid);
            ret = xpmem_release(MPIDI_XPMEMI_global.segmaps[i].apid);
            /* success(0) or failure(-1) */
            MPIR_ERR_CHKANDJUMP(ret == -1, mpi_errno, MPI_ERR_OTHER, "**xpmem_release");
        }
    }

    MPL_free(MPIDI_XPMEMI_global.segmaps);

    XPMEM_TRACE("finalize: remove segid: 0x%lx\n", (uint64_t) MPIDI_XPMEMI_global.segid);
    ret = xpmem_remove(MPIDI_XPMEMI_global.segid);
    /* success(0) or failure(-1) */
    MPIR_ERR_CHKANDJUMP(ret == -1, mpi_errno, MPI_ERR_OTHER, "**xpmem_remove");

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
