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
    int mpi_errno = MPI_SUCCESS;

    int device_count;
    int my_max_dev_id;
    int my_max_subdev_id;

    MPIDI_GPUI_global.initialized = 0;

    if (MPIR_CVAR_ENABLE_GPU) {
        int mpl_err = MPL_gpu_get_dev_count(&device_count, &my_max_dev_id, &my_max_subdev_id);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**gpu_get_dev_count");

        MPIDI_GPUI_global.local_device_count = device_count;
        MPL_gpu_free_hook_register(ipc_handle_free_hook);

        MPIDI_GPUI_global.initialized = 1;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_GPU_comm_bootstrap(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_Process.local_size == 1) {
        goto fn_exit;
    }

    /* This hook is needed when using the drmfd shareable ipc handle implementation in ze backend */
    mpi_errno = MPIDI_FD_comm_bootstrap(comm);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_GPU_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

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
