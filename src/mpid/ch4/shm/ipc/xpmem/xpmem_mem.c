/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */
#include "xpmem_seg.h"
#include "xpmem_post.h"

int MPIDI_XPMEM_ipc_handle_map(MPIDI_XPMEM_ipc_handle_t handle, void **vaddr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* map the true data range, assuming no data outside true_lb/true_ub */
    void *addr = MPIR_get_contig_ptr(handle.addr, handle.true_lb);
    void *addr_out;
    mpi_errno =
        MPIDI_XPMEMI_seg_regist(handle.src_lrank, handle.range, addr, &addr_out,
                                MPIDI_XPMEMI_global.segmaps[handle.src_lrank].segcache_ubuf);

    if (handle.is_contig) {
        /* We'll do MPIR_Typerep_unpack */
        *vaddr = addr_out;
    } else {
        /* We'll do MPIR_Localcopy */
        *vaddr = MPIR_get_contig_ptr(addr_out, -handle.true_lb);
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}
