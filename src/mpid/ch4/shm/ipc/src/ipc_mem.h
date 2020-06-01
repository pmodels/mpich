/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */
#ifndef IPC_MEM_H_INCLUDED
#define IPC_MEM_H_INCLUDED

#include "ch4_impl.h"
#include "ipc_pre.h"
#include "ipc_types.h"
#include "../xpmem/xpmem_post.h"
#include "../gpu/gpu_post.h"

/* Get local memory handle. No-op if the IPC type is NONE. */
MPL_STATIC_INLINE_PREFIX int MPIDI_IPCI_get_mem_attr(const void *vaddr,
                                                     MPIDI_IPCI_mem_attr_t * attr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPCI_GET_MEM_ATTR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPCI_GET_MEM_ATTR);

    MPIR_GPU_query_pointer_attr(vaddr, &attr->gpu_attr);

    if (attr->gpu_attr.type == MPL_GPU_POINTER_DEV) {
        mpi_errno = MPIDI_GPU_get_mem_attr(vaddr, attr);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        mpi_errno = MPIDI_XPMEM_get_mem_attr(vaddr, attr);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPCI_GET_MEM_ATTR);
    return mpi_errno;
  fn_fail:
    attr->ipc_type = MPIDI_IPCI_TYPE__NONE;
    memset(&attr->mem_handle, 0, sizeof(MPIDI_IPCI_mem_handle_t));
    goto fn_exit;
}

/* Attach remote memory handle. Return the local memory segment handle and
 * the mapped virtual address. No-op if the IPC type is NONE. */
MPL_STATIC_INLINE_PREFIX int MPIDI_IPCI_attach_mem(MPIDI_IPCI_type_t ipc_type,
                                                   int node_rank,
                                                   MPIDI_IPCI_mem_handle_t mem_handle,
                                                   MPL_gpu_device_handle_t dev_handle, size_t size,
                                                   MPIDI_IPCI_mem_seg_t * mem_seg, void **vaddr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPCI_ATTACH_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPCI_ATTACH_MEM);

    mem_seg->ipc_type = ipc_type;

    switch (ipc_type) {
        case MPIDI_IPCI_TYPE__XPMEM:
            mpi_errno = MPIDI_XPMEM_attach_mem(node_rank, mem_handle.xpmem, size,
                                               &mem_seg->u.xpmem, vaddr);
            break;
        case MPIDI_IPCI_TYPE__GPU:
            mpi_errno = MPIDI_GPU_attach_mem(mem_handle.gpu, dev_handle, &mem_seg->u.gpu, vaddr);
            break;
        case MPIDI_IPCI_TYPE__NONE:
            /* no-op */
            break;
        default:
            /* Unknown IPC type */
            MPIR_Assert(0);
            break;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPCI_ATTACH_MEM);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_IPCI_close_mem(MPIDI_IPCI_mem_seg_t mem_seg)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPCI_CLOSE_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPCI_CLOSE_MEM);

    switch (mem_seg.ipc_type) {
        case MPIDI_IPCI_TYPE__XPMEM:
            mpi_errno = MPIDI_XPMEM_close_mem(mem_seg.u.xpmem);
            break;
        case MPIDI_IPCI_TYPE__GPU:
            mpi_errno = MPIDI_GPU_close_mem(mem_seg.u.gpu);
            break;
        case MPIDI_IPCI_TYPE__NONE:
            /* noop */
            break;
        default:
            /* Unknown IPC type */
            MPIR_Assert(0);
            break;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPCI_CLOSE_MEM_HANDLE);
    return mpi_errno;
}
#endif /* IPC_MEM_H_INCLUDED */
