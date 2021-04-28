/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "xpmem_pre.h"
#include "xpmem_seg.h"

/****************************************/
/* Segment cache public routines        */
/****************************************/
/* Initialize an empty tree for segment cache.
 * It should be called only once for a AVL tree at MPI init.*/
int MPIDI_XPMEMI_segtree_init(MPL_gavl_tree_t * tree)
{
    int mpi_errno = MPI_SUCCESS, ret;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEMI_SEGTREE_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEMI_SEGTREE_INIT);

    ret = MPL_gavl_tree_create(MPIDI_XPMEM_seg_free, tree);
    MPIR_ERR_CHKANDJUMP(ret != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**xpmem_segtree_init");

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEMI_SEGTREE_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Registers a segment into cache for the specified remote buffer.
 * It internally rounds down the low address and rounds up the size to
 * ensure the cached segment is page aligned. Specific tree is given to
 * differentiate different cache tree (e.g. user buffer tree used to cache
 * user buffer, and XPMEM cooperative counter tree used to cache counter
 * obj)
 *
 * Input parameters:
 * - node_rank:    rank of remote process on local node.
 * - size:         size in bytes of the remote buffer.
 * - remote_vaddr: start virtual address of the remote buffer
 * - segcache: specific tree we want to insert segment into
 * Output parameters:
 * - seg_ptr: registered segment. It can be a matched existing segment
 *            or a newly created one.
 * - vaddr:   corresponding start address of the remote buffer in local
 *            virtual address space. */
int MPIDI_XPMEMI_seg_regist(int node_rank, uintptr_t size,
                            void *remote_vaddr, void **vaddr, MPL_gavl_tree_t segcache)
{
    int mpi_errno = MPI_SUCCESS, mpl_err;
    MPIDI_XPMEMI_segmap_t *segmap = &MPIDI_XPMEMI_global.segmaps[node_rank];
    MPIDI_XPMEMI_seg_t *seg = NULL;
    uintptr_t seg_low;
    uintptr_t seg_size;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEMI_SEG_REGIST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEMI_SEG_REGIST);
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

    mpl_err = MPL_gavl_tree_search(segcache, remote_vaddr, size, (void **) &seg);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mpl_gavl_search");

    if (seg == NULL) {
        struct xpmem_addr xpmem_addr;
        void *att_vaddr;

        seg = (MPIDI_XPMEMI_seg_t *) MPL_malloc(sizeof(MPIDI_XPMEMI_seg_t), MPL_MEM_OTHER);
        MPIR_Assert(seg != NULL);

        xpmem_addr.apid = segmap->apid;
        xpmem_addr.offset = seg_low;
        att_vaddr = xpmem_attach(xpmem_addr, seg_size, NULL);
        MPIR_ERR_CHKANDJUMP(att_vaddr == (void *) -1, mpi_errno, MPI_ERR_OTHER, "**xpmem_attach");
        seg->remote_align_addr = seg_low;
        seg->att_vaddr = (uintptr_t) att_vaddr;
        MPL_gavl_tree_insert(segcache, (void *) seg_low, seg_size, (void *) seg);
    }

    /* return mapped vaddr without round down */
    *vaddr = (void *) ((uintptr_t) remote_vaddr - seg->remote_align_addr + seg->att_vaddr);
    XPMEM_TRACE("seg: register segment %p for node_rank %d, apid 0x%lx, "
                "size 0x%lx->0x%lx, seg->low %p->0x%lx, attached_vaddr %p, vaddr %p\n", seg,
                node_rank, (uint64_t) segmap->apid, size, seg_size,
                remote_vaddr, seg_low, (void *) seg->att_vaddr, *vaddr);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEMI_SEG_REGIST);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

void MPIDI_XPMEM_seg_free(void *seg)
{
    MPIDI_XPMEMI_seg_t *seg_ptr = (MPIDI_XPMEMI_seg_t *) seg;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_FREE_SEG);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_FREE_SEG);

    xpmem_detach((void *) seg_ptr->att_vaddr);
    MPL_free(seg);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_FREE_SEG);
    return;
}
