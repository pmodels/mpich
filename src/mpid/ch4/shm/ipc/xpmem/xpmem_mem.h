/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */
#ifndef XPMEM_MEM_H_INCLUDED
#define XPMEM_MEM_H_INCLUDED

#include "ch4_impl.h"
#include "xpmem_pre.h"
#include "xpmem_seg.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_IPC_xpmem_get_mem_handle(void *vaddr, size_t size,
                                                            MPIDI_IPC_xpmem_mem_handle_t *
                                                            mem_handle)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPC_XPMEM_GET_MEM_HANDLE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPC_XPMEM_GET_MEM_HANDLE);

    mem_handle->src_offset = (uint64_t) vaddr;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPC_XPMEM_GET_MEM_HANDLE);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_IPC_xpmem_attach_mem(int node_rank,
                                                        MPIDI_IPC_xpmem_mem_handle_t handle,
                                                        size_t size,
                                                        MPIDI_IPC_xpmem_mem_seg_t * mem_seg,
                                                        void **vaddr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPC_XPMEM_ATTACH_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPC_XPMEM_ATTACH_MEM);

    mpi_errno = MPIDI_IPC_xpmem_seg_regist(node_rank, size, (void *) handle.src_offset,
                                           &mem_seg->seg_ptr, vaddr,
                                           &MPIDI_IPC_xpmem_global.
                                           segmaps[node_rank].segcache_ubuf);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPC_XPMEM_ATTACH_MEM);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_IPC_xpmem_close_mem(MPIDI_IPC_xpmem_mem_seg_t mem_seg,
                                                       void *vaddr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPC_XPMEM_CLOSE_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPC_XPMEM_CLOSE_MEM);

    mpi_errno = MPIDI_IPC_xpmem_seg_deregist(mem_seg.seg_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPC_XPMEM_CLOSE_MEM);
    return mpi_errno;
}

#endif /* XPMEM_MEM_H_INCLUDED */
