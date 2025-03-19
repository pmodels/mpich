/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "xpmem_post.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_XPMEM_SEG_CACHE_ENABLE
      category    : CH4
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Enable mapped segment cache on receiver side to avoid mapping overhead
        per operation.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

static MPIDI_XPMEMI_seg_t *seg_search(MPL_gavl_tree_t segcache, void *addr, uintptr_t size);
static void seg_insert(MPL_gavl_tree_t segcache, uintptr_t seg_low, uintptr_t seg_size,
                       void *att_vaddr);
static void seg_free(void *seg);

/* Maps region into the local process memory. Will check and use cached
 * region if available, or insert new entry into cache.
 * It internally rounds down the low address and rounds up the size to
 * ensure the cached segment is page aligned. Specific tree is given to
 * differentiate different cache tree (e.g. user buffer tree used to cache
 * user buffer, and XPMEM cooperative counter tree used to cache counter
 * obj)
 *
 * Input parameters:
 * - handle: handle for region to be mapped
 * Output parameters:
 * - vaddr: corresponding start address of the remote buffer in local
 *          virtual address space. */
int MPIDI_XPMEM_ipc_handle_map(MPIDI_XPMEM_ipc_handle_t * handle, void **vaddr)
{
    int mpi_errno = MPI_SUCCESS;
    /* map the true data range, assuming no data outside true_lb/true_ub */
    void *addr = MPIR_get_contig_ptr(handle->addr, handle->true_lb);
    void *addr_out;
    int node_rank = handle->src_lrank;
    uintptr_t size = handle->range;
    void *remote_vaddr = MPIR_get_contig_ptr(handle->addr, handle->true_lb);
    MPIDI_XPMEMI_segmap_t *segmap = &MPIDI_XPMEMI_global.segmaps[node_rank];
    MPL_gavl_tree_t segcache = segmap->segcache_ubuf;
    MPIDI_XPMEMI_seg_t *seg = NULL;
    uintptr_t seg_low;
    uintptr_t seg_size;
    void *att_vaddr;

    MPIR_FUNC_ENTER;

    /* Get apid if it is the first time registered on the local process. */
    if (segmap->apid == -1) {
        segmap->apid = xpmem_get(segmap->remote_segid, XPMEM_RDWR, XPMEM_PERMIT_MODE,
                                 MPIDI_XPMEMI_PERMIT_VALUE);
        /* 64-bit access permit ID or failure(-1) */
        MPIR_ERR_CHKANDJUMP(segmap->apid == -1, mpi_errno, MPI_ERR_OTHER, "**xpmem_get");
        XPMEM_TRACE("seg: register apid 0x%lx for node_rank %d, segid 0x%lx\n",
                    (uint64_t) segmap->apid, node_rank, (uint64_t) segmap->remote_segid);
    }

    /* Search a cached segment or create a new one. Both low and size must be page aligned. */
    seg_low = MPL_ROUND_DOWN_ALIGN((uint64_t) remote_vaddr,
                                   (uint64_t) MPIDI_XPMEMI_global.sys_page_sz);
    seg_size =
        MPL_ROUND_UP_ALIGN(size + ((uintptr_t) remote_vaddr - seg_low),
                           MPIDI_XPMEMI_global.sys_page_sz);

    seg = seg_search(segcache, remote_vaddr, size);
    if (seg == NULL) {
        struct xpmem_addr xpmem_addr;
        xpmem_addr.apid = segmap->apid;
        xpmem_addr.offset = seg_low;
        att_vaddr = xpmem_attach(xpmem_addr, seg_size, NULL);
        MPIR_ERR_CHKANDJUMP2(att_vaddr == (void *) -1, mpi_errno, MPI_ERR_OTHER, "**xpmem_attach",
                             "**xpmem_attach %p %d", remote_vaddr, (int) size);
        seg_insert(segcache, seg_low, seg_size, att_vaddr);
    } else {
        seg_low = seg->remote_align_addr;
        att_vaddr = (void *) seg->att_vaddr;
    }
    handle->att_vaddr = att_vaddr;

    /* return mapped vaddr without round down */
    addr_out = (void *) ((uintptr_t) remote_vaddr - seg_low + att_vaddr);
    XPMEM_TRACE("seg: mappped segment for node_rank %d, apid 0x%lx, "
                "size 0x%lx->0x%lx, seg->low %p->0x%lx, attached_vaddr %p, vaddr %p\n",
                node_rank, (uint64_t) segmap->apid, size, seg_size,
                remote_vaddr, seg_low, (void *) att_vaddr, addr_out);

    if (handle->is_contig) {
        /* We'll do MPIR_Typerep_unpack */
        *vaddr = addr_out;
    } else {
        /* We'll do MPIR_Localcopy */
        *vaddr = MPIR_get_contig_ptr(addr_out, -handle->true_lb);
    }

  fn_fail:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_XPMEM_ipc_handle_unmap(MPIDI_XPMEM_ipc_handle_t * handle)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;

    MPIR_FUNC_ENTER;

    /* skip unmap if cache enabled */
    if (MPIR_CVAR_CH4_XPMEM_SEG_CACHE_ENABLE) {
        goto fn_exit;
    }

    ret = xpmem_detach((void *) handle->att_vaddr);
    MPIR_ERR_CHKANDJUMP(ret != 0, mpi_errno, MPI_ERR_OTHER, "**xpmem_detach");

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*** segment cache routines ***/

/* Initialize an empty tree for segment cache.
 * It should be called only once for a AVL tree at MPI init.*/
int MPIDI_XPMEMI_segtree_init(MPL_gavl_tree_t * tree)
{
    int mpi_errno = MPI_SUCCESS, ret;
    MPIR_FUNC_ENTER;

    if (MPIR_CVAR_CH4_XPMEM_SEG_CACHE_ENABLE) {
        ret = MPL_gavl_tree_create(seg_free, tree);
        MPIR_ERR_CHKANDJUMP(ret != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**xpmem_segtree_init");
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_XPMEMI_segtree_finalize(MPL_gavl_tree_t tree)
{
    int mpi_errno = MPI_SUCCESS, ret;
    MPIR_FUNC_ENTER;

    if (MPIR_CVAR_CH4_XPMEM_SEG_CACHE_ENABLE) {
        ret = MPL_gavl_tree_destroy(tree);
        MPIR_ERR_CHKANDJUMP(ret != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**xpmem_segtree_finalize");
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static MPIDI_XPMEMI_seg_t *seg_search(MPL_gavl_tree_t segcache, void *addr, uintptr_t size)
{
    if (MPIR_CVAR_CH4_XPMEM_SEG_CACHE_ENABLE) {
        return MPL_gavl_tree_search(segcache, addr, size);
    }

    return NULL;
}

static void seg_insert(MPL_gavl_tree_t segcache, uintptr_t seg_low, uintptr_t seg_size,
                       void *att_vaddr)
{
    if (MPIR_CVAR_CH4_XPMEM_SEG_CACHE_ENABLE) {
        MPIDI_XPMEMI_seg_t *seg = MPL_malloc(sizeof(MPIDI_XPMEMI_seg_t), MPL_MEM_OTHER);
        MPIR_Assert(seg != NULL);
        seg->remote_align_addr = seg_low;
        seg->att_vaddr = (uintptr_t) att_vaddr;
        MPL_gavl_tree_insert(segcache, (void *) seg_low, seg_size, (void *) seg);
    }
}

static void seg_free(void *seg)
{
    MPIDI_XPMEMI_seg_t *seg_ptr = (MPIDI_XPMEMI_seg_t *) seg;
    MPIR_FUNC_ENTER;

    xpmem_detach((void *) seg_ptr->att_vaddr);
    MPL_free(seg);

    MPIR_FUNC_EXIT;
    return;
}
