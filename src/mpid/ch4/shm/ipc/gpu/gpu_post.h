/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */
#ifndef GPU_POST_H_INCLUDED
#define GPU_POST_H_INCLUDED

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_GPU_LMT_MSG_SIZE
      category    : CH4
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If a send message size is greater than or equal to MPIR_CVAR_CH4_GPU_LMT_MSG_SIZE (in
        bytes), then enable GPU-based single copy protocol for intranode communication. The
        environment variable is valid only when then GPU IPC shmmod is enabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* TODO: We set MPIR_CVAR_CH4_GPU_LMT_MSG_SIZE to 0 for now to always use
 * IPC for GPU send/receive because POSIX does not support GPU buffers.
 * However, once we add support in POSIX, the recommended value should be 32768 */

#include "ch4_impl.h"
#include "gpu_pre.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_GPU_get_mem_attr(const void *vaddr, MPIDI_IPCI_mem_attr_t * attr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GPU_GET_MEM_ATTR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPC_GET_MEM_ATTR);

#ifdef MPIDI_CH4_SHM_ENABLE_GPU
    attr->ipc_type = MPIDI_IPCI_TYPE__GPU;
    MPL_gpu_ipc_get_mem_handle(&attr->mem_handle.gpu.ipc_handle, (void *) vaddr);
    attr->threshold.send_lmt_sz = MPIR_CVAR_CH4_GPU_LMT_MSG_SIZE;
#else
    /* Do not support IPC data transfer */
    attr->ipc_type = MPIDI_IPCI_TYPE__NONE;
    attr->threshold.send_lmt_sz = MPIR_AINT_MAX;
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GPU_GET_MEM_ATTR);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_GPU_attach_mem(MPIDI_GPU_mem_handle_t mem_handle,
                                                  MPL_gpu_device_handle_t dev_handle,
                                                  MPIDI_GPU_mem_seg_t * mem_seg, void **vaddr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GPU_ATTACH_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GPU_ATTACH_MEM);

    MPL_gpu_ipc_open_mem_handle(vaddr, mem_handle.ipc_handle, dev_handle);

    mem_seg->vaddr = *vaddr;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GPU_ATTACH_MEM);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_GPU_close_mem(MPIDI_GPU_mem_seg_t mem_seg)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GPU_CLOSE_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GPU_CLOSE_MEM);

    MPL_gpu_ipc_close_mem_handle(mem_seg.vaddr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GPU_CLOSE_MEM);
    return mpi_errno;
}

#endif /* GPU_POST_H_INCLUDED */
