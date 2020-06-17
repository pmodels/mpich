/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "gpu_pre.h"
#include "gpu_types.h"

int MPIDI_GPU_get_mem_attr(const void *vaddr, MPIDI_IPCI_mem_attr_t * attr)
{
    int mpi_errno = MPI_SUCCESS, mpl_err = MPL_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GPU_GET_MEM_ATTR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPC_GET_MEM_ATTR);

#ifdef MPIDI_CH4_SHM_ENABLE_GPU
    int local_dev_id;
    MPIDI_GPUI_dev_id_t *tmp;

    attr->ipc_type = MPIDI_IPCI_TYPE__GPU;
    mpl_err = MPL_gpu_ipc_handle_create(vaddr, &attr->mem_handle.gpu.ipc_handle);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                        "**gpu_ipc_handle_create");

    MPL_gpu_get_dev_id(attr->gpu_attr.device, &local_dev_id);
    HASH_FIND_INT(MPIDI_GPUI_global.local_to_global_map, &local_dev_id, tmp);
    attr->mem_handle.gpu.global_dev_id = tmp->global_dev_id;
    attr->threshold.send_lmt_sz = MPIR_CVAR_CH4_IPC_GPU_P2P_THRESHOLD;
#else
    /* Do not support IPC data transfer */
    attr->ipc_type = MPIDI_IPCI_TYPE__NONE;
    attr->threshold.send_lmt_sz = MPIR_AINT_MAX;
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GPU_GET_MEM_ATTR);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_GPU_attach_mem(MPIDI_GPU_mem_handle_t mem_handle,
                         MPL_gpu_device_handle_t dev_handle, void **vaddr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GPU_ATTACH_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GPU_ATTACH_MEM);

#ifdef MPIDI_CH4_SHM_ENABLE_GPU
    int mpl_err = MPL_SUCCESS;
    if (mem_handle.global_dev_id == -1)
        mpl_err = MPL_gpu_ipc_handle_map(mem_handle.ipc_handle, dev_handle, vaddr);
    else {
        MPL_gpu_device_handle_t remote_dev_handle;
        MPIDI_GPUI_dev_id_t *tmp;
        HASH_FIND_INT(MPIDI_GPUI_global.global_to_local_map, &mem_handle.global_dev_id, tmp);

        MPL_gpu_get_dev_handle(tmp->local_dev_id, &remote_dev_handle);
        mpl_err = MPL_gpu_ipc_handle_map(mem_handle.ipc_handle, remote_dev_handle, vaddr);
    }
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gpu_ipc_handle_map");
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GPU_ATTACH_MEM);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_GPU_close_mem(void *vaddr, MPIDI_GPU_mem_handle_t handle)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GPU_CLOSE_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GPU_CLOSE_MEM);

#ifdef MPIDI_CH4_SHM_ENABLE_GPU
    int mpl_err = MPL_SUCCESS;
    mpl_err = MPL_gpu_ipc_handle_unmap(vaddr, handle.ipc_handle);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gpu_ipc_handle_unmap");
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GPU_CLOSE_MEM);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
