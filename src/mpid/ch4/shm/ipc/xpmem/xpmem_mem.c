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

    mpi_errno =
        MPIDI_XPMEMI_seg_regist(handle.src_lrank, handle.data_sz,
                                (void *) handle.src_offset, vaddr,
                                MPIDI_XPMEMI_global.segmaps[handle.src_lrank].segcache_ubuf);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}
