/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "gpu_post.h"
#include "gpu_types.h"

static void ipc_handle_free_hook(void *dptr)
{
    int mpl_err ATTRIBUTE((unused));
    MPL_pointer_attr_t gpu_attr;

    MPIR_FUNC_ENTER;

    if (MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE == MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE_generic) {
        struct MPIDI_GPUI_handle_cache_entry *entry;

        HASH_FIND_PTR(MPIDI_GPUI_global.ipc_handle_cache, &dptr, entry);
        if (entry) {
            HASH_DEL(MPIDI_GPUI_global.ipc_handle_cache, entry);
            MPL_free(entry);

            MPIR_GPU_query_pointer_attr(dptr, &gpu_attr);
            mpl_err = MPL_gpu_ipc_handle_destroy(dptr, &gpu_attr);
            MPIR_Assert(mpl_err == MPL_SUCCESS);
        }
    }

    MPIR_FUNC_EXIT;
    return;
}

int MPIDI_GPU_init_local(void)
{
    return MPI_SUCCESS;
}

int MPIDI_GPU_comm_bootstrap(comm)
{
    int mpl_err, mpi_errno = MPI_SUCCESS;
    int device_count;
    /* my_max_* represents the max value local to the process. node_max_* represents the max value
     * between all node-local processes. */
    int my_max_dev_id, node_max_dev_id = -1;
    int my_max_subdev_id, node_max_subdev_id = -1;
    MPIDI_GPU_device_info_t local_gpu_info, remote_gpu_info;

    MPIDI_GPUI_global.initialized = 0;
    mpl_err = MPL_gpu_get_dev_count(&device_count, &my_max_dev_id, &my_max_subdev_id);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gpu_get_dev_count");
    if (device_count < 0)
        goto fn_exit;

    if (MPIR_Process.local_size == 1) {
        goto fn_exit;
    }

    local_gpu_info.max_dev_id = remote_gpu_info.max_dev_id = my_max_dev_id;
    local_gpu_info.max_subdev_id = remote_gpu_info.max_subdev_id = my_max_subdev_id;

    MPIDU_Init_shm_put(&local_gpu_info, sizeof(MPIDI_GPU_device_info_t));
    MPIDU_Init_shm_barrier();

    /* get node max device id */
    for (int i = 0; i < MPIR_Process.local_size; i++) {
        MPIDU_Init_shm_get(i, sizeof(MPIDI_GPU_device_info_t), &remote_gpu_info);
        if (remote_gpu_info.max_dev_id > node_max_dev_id)
            node_max_dev_id = remote_gpu_info.max_dev_id;
        if (remote_gpu_info.max_subdev_id > node_max_subdev_id)
            node_max_subdev_id = remote_gpu_info.max_subdev_id;
    }
    MPIDU_Init_shm_barrier();

    MPIDI_GPUI_global.local_procs = MPIR_Process.node_local_map;
    MPIDI_GPUI_global.local_ranks =
        (int *) MPL_malloc(MPIR_Process.size * sizeof(int), MPL_MEM_SHM);
    for (int i = 0; i < MPIR_Process.size; ++i) {
        MPIDI_GPUI_global.local_ranks[i] = -1;
    }
    for (int i = 0; i < MPIR_Process.local_size; i++) {
        MPIDI_GPUI_global.local_ranks[MPIDI_GPUI_global.local_procs[i]] = i;
    }

    MPIDI_GPUI_global.local_device_count = device_count;
    MPL_gpu_free_hook_register(ipc_handle_free_hook);

    /* This hook is needed when using the drmfd shareable ipc handle implementation in ze backend */
    mpi_errno = MPIDI_FD_mpi_init_hook();
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_GPUI_global.initialized = 1;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_GPU_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    if (MPIDI_GPUI_global.initialized) {
        mpi_errno = MPIDI_FD_mpi_finalize_hook();
        MPIR_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gpu_finalize");

        MPL_free(MPIDI_GPUI_global.local_ranks);
    }

    {
        struct MPIDI_GPUI_map_cache_entry *entry, *tmp;
        HASH_ITER(hh, MPIDI_GPUI_global.ipc_map_cache, entry, tmp) {
            HASH_DEL(MPIDI_GPUI_global.ipc_map_cache, entry);
            for (int i = 0; i < MPIDI_GPUI_global.local_device_count; i++) {
                if (entry->mapped_addrs[i]) {
                    int mpl_err = MPL_gpu_ipc_handle_unmap((void *) entry->mapped_addrs[i]);
                    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                                        "**gpu_ipc_handle_unmap");
                }
            }
            MPL_free(entry);
        }
    }

    {
        struct MPIDI_GPUI_handle_cache_entry *entry, *tmp;
        HASH_ITER(hh, MPIDI_GPUI_global.ipc_handle_cache, entry, tmp) {
            MPL_pointer_attr_t gpu_attr;
            int mpl_err;

            MPIR_GPU_query_pointer_attr(entry->base_addr, &gpu_attr);
            mpl_err = MPL_gpu_ipc_handle_destroy(entry->base_addr, &gpu_attr);
            MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                                "**gpu_ipc_handle_destroy");

            HASH_DEL(MPIDI_GPUI_global.ipc_handle_cache, entry);
            MPL_free(entry);
        }
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
