/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "gpu_post.h"
#include "gpu_types.h"

static void ipc_handle_cache_free(void *handle_obj)
{
    int mpl_err ATTRIBUTE((unused));

    MPIR_FUNC_ENTER;

    MPIDI_GPUI_handle_obj_s *handle_obj_ptr = (MPIDI_GPUI_handle_obj_s *) handle_obj;
    mpl_err = MPL_gpu_ipc_handle_unmap((void *) handle_obj_ptr->mapped_base_addr);
    MPIR_Assert(mpl_err == MPL_SUCCESS);
    MPL_free(handle_obj);

    MPIR_FUNC_EXIT;
    return;
}

static void ipc_handle_status_free(void *handle_obj)
{
    MPIR_FUNC_ENTER;

    MPL_free(handle_obj);

    MPIR_FUNC_EXIT;
    return;
}

static void ipc_handle_free_hook(void *dptr)
{
    void *pbase;
    uintptr_t len;
    int mpl_err ATTRIBUTE((unused));
    int local_dev_id;
    MPL_pointer_attr_t gpu_attr;

    MPIR_FUNC_ENTER;

    if (MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE) {
        mpl_err = MPL_gpu_get_buffer_bounds(dptr, &pbase, &len);
        MPIR_Assert(mpl_err == MPL_SUCCESS);

        MPIR_GPU_query_pointer_attr(pbase, &gpu_attr);
        if (gpu_attr.type == MPL_GPU_POINTER_DEV) {
            local_dev_id = MPL_gpu_get_dev_id_from_attr(&gpu_attr);

            for (int i = 0; i < MPIR_Process.local_size; ++i) {
                MPL_gavl_tree_t track_tree =
                    MPIDI_GPUI_global.ipc_handle_track_trees[i][local_dev_id];
                mpl_err = MPL_gavl_tree_delete_range(track_tree, pbase, len);
                MPIR_Assert(mpl_err == MPL_SUCCESS);
            }

            mpl_err = MPL_gpu_ipc_handle_destroy(pbase);
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

int MPIDI_GPU_init_world(void)
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

    /* Initialize the local and global device mappings */
    MPL_gpu_init_device_mappings(node_max_dev_id, node_max_subdev_id);

    if (node_max_subdev_id > 0) {
        /* global_device_count = device_count * (subdevice_count + 1)
         * global_max_dev_id = global_devices - 1 */
        MPIDI_GPUI_global.global_max_dev_id = (node_max_dev_id + 1) * (node_max_subdev_id + 2) - 1;
    } else {
        MPIDI_GPUI_global.global_max_dev_id = node_max_dev_id;
    }

    MPIDI_GPUI_global.local_procs = MPIR_Process.node_local_map;
    MPIDI_GPUI_global.local_ranks =
        (int *) MPL_malloc(MPIR_Process.size * sizeof(int), MPL_MEM_SHM);
    for (int i = 0; i < MPIR_Process.size; ++i) {
        MPIDI_GPUI_global.local_ranks[i] = -1;
    }
    for (int i = 0; i < MPIR_Process.local_size; i++) {
        MPIDI_GPUI_global.local_ranks[MPIDI_GPUI_global.local_procs[i]] = i;
    }

    MPIDI_GPUI_global.ipc_handle_mapped_trees =
        (MPL_gavl_tree_t ***) MPL_malloc(sizeof(MPL_gavl_tree_t **) * MPIR_Process.local_size,
                                         MPL_MEM_OTHER);
    MPIR_Assert(MPIDI_GPUI_global.ipc_handle_mapped_trees != NULL);
    memset(MPIDI_GPUI_global.ipc_handle_mapped_trees, 0,
           sizeof(MPL_gavl_tree_t *) * MPIR_Process.local_size);

    MPIDI_GPUI_global.ipc_handle_track_trees =
        (MPL_gavl_tree_t **) MPL_malloc(sizeof(MPL_gavl_tree_t *) * MPIR_Process.local_size,
                                        MPL_MEM_OTHER);
    MPIR_Assert(MPIDI_GPUI_global.ipc_handle_track_trees != NULL);
    memset(MPIDI_GPUI_global.ipc_handle_track_trees, 0,
           sizeof(MPL_gavl_tree_t *) * MPIR_Process.local_size);

    for (int i = 0; i < MPIR_Process.local_size; ++i) {
        MPIDI_GPUI_global.ipc_handle_mapped_trees[i] =
            (MPL_gavl_tree_t **) MPL_malloc(sizeof(MPL_gavl_tree_t *) *
                                            (MPIDI_GPUI_global.global_max_dev_id + 1),
                                            MPL_MEM_OTHER);
        MPIR_Assert(MPIDI_GPUI_global.ipc_handle_mapped_trees[i]);
        memset(MPIDI_GPUI_global.ipc_handle_mapped_trees[i], 0,
               sizeof(MPL_gavl_tree_t *) * (MPIDI_GPUI_global.global_max_dev_id + 1));

        MPIDI_GPUI_global.ipc_handle_track_trees[i] =
            (MPL_gavl_tree_t *) MPL_malloc(sizeof(MPL_gavl_tree_t) *
                                           (MPIDI_GPUI_global.global_max_dev_id + 1),
                                           MPL_MEM_OTHER);
        MPIR_Assert(MPIDI_GPUI_global.ipc_handle_track_trees[i]);
        memset(MPIDI_GPUI_global.ipc_handle_track_trees[i], 0,
               sizeof(MPL_gavl_tree_t) * (MPIDI_GPUI_global.global_max_dev_id + 1));

        for (int j = 0; j < (MPIDI_GPUI_global.global_max_dev_id + 1); ++j) {
            MPIDI_GPUI_global.ipc_handle_mapped_trees[i][j] =
                (MPL_gavl_tree_t *) MPL_malloc(sizeof(MPL_gavl_tree_t) *
                                               device_count, MPL_MEM_OTHER);
            MPIR_Assert(MPIDI_GPUI_global.ipc_handle_mapped_trees[i][j]);
            memset(MPIDI_GPUI_global.ipc_handle_mapped_trees[i][j], 0,
                   sizeof(MPL_gavl_tree_t) * device_count);

            for (int k = 0; k < device_count; ++k) {
                mpl_err =
                    MPL_gavl_tree_create(ipc_handle_cache_free,
                                         &MPIDI_GPUI_global.ipc_handle_mapped_trees[i][j][k]);
                MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                                    "**mpl_gavl_create");
            }

            mpl_err =
                MPL_gavl_tree_create(ipc_handle_status_free,
                                     &MPIDI_GPUI_global.ipc_handle_track_trees[i][j]);
            MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                                "**mpl_gavl_create");
        }
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

    if (MPIDI_GPUI_global.ipc_handle_mapped_trees) {
        for (int i = 0; i < MPIR_Process.local_size; ++i) {
            if (MPIDI_GPUI_global.ipc_handle_mapped_trees[i]) {
                for (int j = 0; j < (MPIDI_GPUI_global.global_max_dev_id + 1); ++j) {
                    if (MPIDI_GPUI_global.ipc_handle_mapped_trees[i][j]) {
                        for (int k = 0; k < MPIDI_GPUI_global.local_device_count; ++k) {
                            if (MPIDI_GPUI_global.ipc_handle_mapped_trees[i][j][k])
                                MPL_gavl_tree_destory(MPIDI_GPUI_global.ipc_handle_mapped_trees[i]
                                                      [j]
                                                      [k]);
                        }
                    }
                    MPL_free(MPIDI_GPUI_global.ipc_handle_mapped_trees[i][j]);
                }
            }
            MPL_free(MPIDI_GPUI_global.ipc_handle_mapped_trees[i]);
        }
    }
    MPL_free(MPIDI_GPUI_global.ipc_handle_mapped_trees);

    if (MPIDI_GPUI_global.ipc_handle_track_trees) {
        for (int i = 0; i < MPIR_Process.local_size; ++i) {
            if (MPIDI_GPUI_global.ipc_handle_track_trees[i]) {
                for (int j = 0; j < (MPIDI_GPUI_global.global_max_dev_id + 1); ++j) {
                    if (MPIDI_GPUI_global.ipc_handle_track_trees[i][j])
                        MPL_gavl_tree_destory(MPIDI_GPUI_global.ipc_handle_track_trees[i][j]);
                }
            }
            MPL_free(MPIDI_GPUI_global.ipc_handle_track_trees[i]);
        }
    }
    MPL_free(MPIDI_GPUI_global.ipc_handle_track_trees);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
