/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "gpu_pre.h"
#include "gpu_types.h"

int MPIDI_GPU_get_ipc_attr(const void *vaddr, MPIDI_IPCI_ipc_attr_t * ipc_attr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GPU_GET_IPC_ATTR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GPU_GET_IPC_ATTR);

#ifdef MPIDI_CH4_SHM_ENABLE_GPU
    int local_dev_id;
    MPIDI_GPUI_dev_id_t *tmp;
    void *pbase;
    uintptr_t len;
    int mpl_err = MPL_SUCCESS;

    ipc_attr->ipc_type = MPIDI_IPCI_TYPE__GPU;
    mpl_err = MPL_gpu_get_buffer_bounds(vaddr, &pbase, &len);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gpu_get_buffer_info");

    mpl_err = MPL_gpu_ipc_handle_create(pbase, &ipc_attr->ipc_handle.gpu.ipc_handle);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                        "**gpu_ipc_handle_create");

    ipc_attr->ipc_handle.gpu.offset = (uintptr_t) vaddr - (uintptr_t) pbase;

    MPL_gpu_get_dev_id(ipc_attr->gpu_attr.device, &local_dev_id);
    HASH_FIND_INT(MPIDI_GPUI_global.local_to_global_map, &local_dev_id, tmp);
    ipc_attr->ipc_handle.gpu.global_dev_id = tmp->global_dev_id;
    ipc_attr->threshold.send_lmt_sz = MPIR_CVAR_CH4_IPC_GPU_P2P_THRESHOLD;
#else
    /* Do not support IPC data transfer */
    ipc_attr->ipc_type = MPIDI_IPCI_TYPE__NONE;
    ipc_attr->threshold.send_lmt_sz = MPIR_AINT_MAX;
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GPU_GET_IPC_ATTR);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_GPU_ipc_handle_map(MPIDI_GPU_ipc_handle_t handle,
                             MPL_gpu_device_handle_t dev_handle,
                             MPI_Datatype recv_type, void **vaddr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GPU_IPC_HANDLE_MAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GPU_IPC_HANDLE_MAP);

#ifdef MPIDI_CH4_SHM_ENABLE_GPU
    void *pbase;
    int recv_dt_contig, mpl_err = MPL_SUCCESS;
    MPIDI_GPUI_dev_id_t *avail_id = NULL;
    MPIDI_Datatype_check_contig(recv_type, recv_dt_contig);
    HASH_FIND_INT(MPIDI_GPUI_global.global_to_local_map, &handle.global_dev_id, avail_id);

    if (avail_id == NULL)
        mpl_err = MPL_gpu_ipc_handle_map(handle.ipc_handle, dev_handle, &pbase);
    else {
        MPL_gpu_device_handle_t remote_dev_handle;
        MPL_gpu_get_dev_handle(avail_id->local_dev_id, &remote_dev_handle);
        if (!recv_dt_contig)
            mpl_err = MPL_gpu_ipc_handle_map(handle.ipc_handle, dev_handle, &pbase);
        else
            mpl_err = MPL_gpu_ipc_handle_map(handle.ipc_handle, remote_dev_handle, &pbase);
    }
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gpu_ipc_handle_map");

    *vaddr = (void *) ((uintptr_t) pbase + handle.offset);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GPU_IPC_HANDLE_MAP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_GPU_ipc_handle_unmap(void *vaddr, MPIDI_GPU_ipc_handle_t handle)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GPU_IPC_HANDLE_UNMAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GPU_IPC_HANDLE_UNMAP);

#ifdef MPIDI_CH4_SHM_ENABLE_GPU
    int mpl_err = MPL_SUCCESS;
    mpl_err = MPL_gpu_ipc_handle_unmap((void *) ((uintptr_t) vaddr - handle.offset));
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gpu_ipc_handle_unmap");
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GPU_IPC_HANDLE_UNMAP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
