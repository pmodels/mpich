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

MPL_STATIC_INLINE_PREFIX int MPIDI_IPCI_handle_unmap(MPIDI_IPCI_type_t ipc_type, void *vaddr,
                                                     MPIDI_IPCI_ipc_handle_t ipc_handle)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPCI_HANDLE_UNMAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPCI_HANDLE_UNMAP);

    switch (ipc_type) {
        case MPIDI_IPCI_TYPE__XPMEM:
            break;
        case MPIDI_IPCI_TYPE__GPU:
            mpi_errno = MPIDI_GPU_ipc_handle_unmap(vaddr, ipc_handle.gpu);
            break;
        case MPIDI_IPCI_TYPE__NONE:
            /* noop */
            break;
        default:
            /* Unknown IPC type */
            MPIR_Assert(0);
            break;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPCI_HANDLE_UNMAP);
    return mpi_errno;
}

#endif /* IPC_MEM_H_INCLUDED */
