/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */
#ifndef IPC_MEM_H_INCLUDED
#define IPC_MEM_H_INCLUDED

#include "ch4_impl.h"
#include "ipc_pre.h"
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
#include "../xpmem/xpmem_mem.h"
#endif

MPL_STATIC_INLINE_PREFIX int MPIDI_IPC_get_mem_handle(MPIDI_SHM_IPC_type_t ipc_type,
                                                      void *vaddr, size_t size,
                                                      MPIDI_IPC_mem_handle_t * mem_handle)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPC_GET_MEM_HANDLE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPC_GET_MEM_HANDLE);

    switch (ipc_type) {
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
        case MPIDI_SHM_IPC_TYPE__XPMEM:
            mpi_errno = MPIDI_IPC_xpmem_get_mem_handle(vaddr, size, &mem_handle->xpmem);
            break;
#endif
        default:
            /* Unknown IPC type */
            MPIR_Assert(0);
            break;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPC_GET_MEM_HANDLE);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_IPC_attach_mem(MPIDI_SHM_IPC_type_t ipc_type,
                                                  int node_rank, MPIDI_IPC_mem_handle_t mem_handle,
                                                  size_t size, MPIDI_IPC_mem_seg_t * mem_seg,
                                                  void **vaddr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPC_ATTACH_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPC_ATTACH_MEM);

    mem_seg->ipc_type = ipc_type;

    switch (ipc_type) {
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
        case MPIDI_SHM_IPC_TYPE__XPMEM:
            mpi_errno = MPIDI_IPC_xpmem_attach_mem(node_rank, mem_handle.xpmem, size,
                                                   &mem_seg->u.xpmem, vaddr);
            break;
#endif
        default:
            /* Unknown IPC type */
            MPIR_Assert(0);
            break;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPC_ATTACH_MEM);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_IPC_close_mem(MPIDI_IPC_mem_seg_t mem_seg, void *vaddr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPC_CLOSE_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPC_CLOSE_MEM);

    switch (mem_seg.ipc_type) {
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
        case MPIDI_SHM_IPC_TYPE__XPMEM:
            mpi_errno = MPIDI_IPC_xpmem_close_mem(mem_seg.u.xpmem, vaddr);
            break;
#endif
        default:
            /* Unknown IPC type */
            MPIR_Assert(0);
            break;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPC_CLOSE_MEM_HANDLE);
    return mpi_errno;
}
#endif /* IPC_MEM_H_INCLUDED */
