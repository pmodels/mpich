/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */
#include "xpmem_seg.h"
#include "xpmem_post.h"

int MPIDI_XPMEM_attach_mem(MPIDI_XPMEM_mem_handle_t mem_handle, void **vaddr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_ATTACH_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_ATTACH_MEM);

    mpi_errno =
        MPIDI_XPMEMI_seg_regist(mem_handle.src_lrank, mem_handle.data_sz,
                                (void *) mem_handle.src_offset, vaddr,
                                &MPIDI_XPMEMI_global.segmaps[mem_handle.src_lrank].segcache_ubuf);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_ATTACH_MEM);
    return mpi_errno;
}
