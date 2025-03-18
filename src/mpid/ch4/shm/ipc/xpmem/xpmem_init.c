/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "xpmem_post.h"
#include "mpidu_init_shm.h"
#include "xpmem_seg.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_XPMEM_ENABLE
      category    : CH4
      type        : int
      default     : 1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        To manually disable XPMEM set to 0. The environment variable is valid only when the XPMEM
        submodule is enabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

int MPIDI_XPMEM_init_local(void)
{
    return MPI_SUCCESS;
}

int MPIDI_XPMEM_init_world(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;

    /* If the user has disabled XPMEM, don't even try to initialize it. */
    if (!MPIR_CVAR_CH4_XPMEM_ENABLE) {
        goto fn_exit;
    }

    if (MPIR_Process.local_size == 1) {
        goto fn_exit;
    }

    MPIR_FUNC_ENTER;
    MPIR_CHKPMEM_DECL();

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

    MPIDU_Init_shm_put(&MPIDI_XPMEMI_global.segid, sizeof(xpmem_segid_t));
    MPIDU_Init_shm_barrier();

    /* Initialize segmap for every local processes */
    MPIDI_XPMEMI_global.segmaps = NULL;
    MPIR_CHKPMEM_MALLOC(MPIDI_XPMEMI_global.segmaps,
                        sizeof(MPIDI_XPMEMI_segmap_t) * MPIR_Process.local_size, MPL_MEM_SHM);
    for (i = 0; i < MPIR_Process.local_size; i++) {
        MPIDU_Init_shm_get(i, sizeof(xpmem_segid_t), &MPIDI_XPMEMI_global.segmaps[i].remote_segid);
        MPIR_ERR_CHKANDJUMP(MPIDI_XPMEMI_global.segmaps[i].remote_segid == -1, mpi_errno,
                            MPI_ERR_OTHER, "**xpmem_segid");
        MPIDI_XPMEMI_global.segmaps[i].apid = -1;       /* get apid at the first communication  */
    }
    MPIDU_Init_shm_barrier();

    /* Initialize other global parameters */
    MPIDI_XPMEMI_global.sys_page_sz = (size_t) sysconf(_SC_PAGESIZE);

    for (i = 0; i < MPIR_Process.local_size; ++i) {
        /* Init AVL tree based segment cache */
        MPIDI_XPMEMI_segtree_init(&MPIDI_XPMEMI_global.segmaps[i].segcache_ubuf);       /* Initialize user buffer tree */
    }
    MPIDU_Init_shm_barrier();

    MPIDI_XPMEMI_global.initialized = true;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    if (MPIDI_XPMEMI_global.segid != -1) {
        xpmem_remove(MPIDI_XPMEMI_global.segid);
    }

    /* While this version of MPICH does require libxpmem to link, we don't necessarily require the
     * kernel module to be loaded at runtime. If XPMEM is not available, disable its use via the
     * special CVAR value. */
    XPMEM_TRACE("init: xpmem_make failed. Disabling XPMEM support");
    MPIDI_XPMEMI_global.initialized = false;

    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIDI_XPMEM_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i, ret = 0;
    MPIR_FUNC_ENTER;

    if (MPIDI_XPMEMI_global.segid == -1 || !MPIDI_XPMEMI_global.initialized) {
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

    MPIDI_XPMEMI_global.initialized = false;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
