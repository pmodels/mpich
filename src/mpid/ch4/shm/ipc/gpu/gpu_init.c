/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "gpu_post.h"
#include "gpu_types.h"

static void ipc_handle_cache_free(void *handle_obj)
{
    int mpl_err;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_IPC_HANDLE_FREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_IPC_HANDLE_FREE);

    MPIDI_GPUI_handle_obj_s *handle_obj_ptr = (MPIDI_GPUI_handle_obj_s *) handle_obj;
    mpl_err = MPL_gpu_ipc_handle_unmap((void *) handle_obj_ptr->mapped_base_addr);
    MPIR_Assert(mpl_err == MPL_SUCCESS);
    MPL_free(handle_obj);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_IPC_HANDLE_FREE);
    return;
}

static void ipc_handle_status_free(void *handle_obj)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_IPC_HANDLE_STATUS_FREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_IPC_HANDLE_STATUS_FREE);

    MPL_free(handle_obj);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_IPC_HANDLE_STATUS_FREE);
    return;
}

static void ipc_handle_free_hook(void *dptr)
{
    void *pbase;
    uintptr_t len;
    int mpl_err, local_dev_id;
    MPL_pointer_attr_t gpu_attr;
    MPIDI_GPUI_dev_id_t *tmp;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_IPC_HANDLE_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_IPC_HANDLE_FREE_HOOK);

    if (MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE) {
        mpl_err = MPL_gpu_get_buffer_bounds(dptr, &pbase, &len);
        MPIR_Assert(mpl_err == MPL_SUCCESS);

        MPIR_GPU_query_pointer_attr(pbase, &gpu_attr);
        MPL_gpu_get_dev_id(gpu_attr.device, &local_dev_id);
        HASH_FIND_INT(MPIDI_GPUI_global.local_to_global_map, &local_dev_id, tmp);

        for (int i = 0; i < MPIR_Process.local_size; ++i) {
            mpl_err = MPL_gavl_tree_delete_range(MPIDI_GPUI_global.ipc_handle_track_trees[i]
                                                 [tmp->global_dev_id], pbase, len);
            MPIR_Assert(mpl_err == MPL_SUCCESS);
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_IPC_HANDLE_FREE_HOOK);
    return;
}

int MPIDI_GPU_mpi_init_hook(int rank, int size, int *tag_bits)
{
    int mpl_err, mpi_errno = MPI_SUCCESS;
    int device_count;
    int my_max_dev_id, node_max_dev_id = -1;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GPU_MPI_INIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GPU_MPI_INIT_HOOK);
    MPIR_CHKPMEM_DECL(1);

    MPIDI_GPUI_global.initialized = 0;
    mpl_err = MPL_gpu_get_dev_count(&device_count, &my_max_dev_id);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gpu_get_dev_count");
    if (device_count < 0)
        goto fn_exit;

    int *global_ids = MPL_malloc(sizeof(int) * device_count, MPL_MEM_OTHER);
    MPIR_Assert(global_ids);

    mpl_err = MPL_gpu_get_global_dev_ids(global_ids, device_count);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                        "**gpu_get_global_dev_ids");

    MPIDI_GPUI_global.local_to_global_map = NULL;
    MPIDI_GPUI_global.global_to_local_map = NULL;
    for (int i = 0; i < device_count; ++i) {
        MPIDI_GPUI_dev_id_t *id_obj =
            (MPIDI_GPUI_dev_id_t *) MPL_malloc(sizeof(MPIDI_GPUI_dev_id_t), MPL_MEM_OTHER);
        MPIR_Assert(id_obj);
        id_obj->local_dev_id = i;
        id_obj->global_dev_id = global_ids[i];
        HASH_ADD_INT(MPIDI_GPUI_global.local_to_global_map, local_dev_id, id_obj, MPL_MEM_OTHER);

        id_obj = (MPIDI_GPUI_dev_id_t *) MPL_malloc(sizeof(MPIDI_GPUI_dev_id_t), MPL_MEM_OTHER);
        MPIR_Assert(id_obj);
        id_obj->local_dev_id = i;
        id_obj->global_dev_id = global_ids[i];
        HASH_ADD_INT(MPIDI_GPUI_global.global_to_local_map, global_dev_id, id_obj, MPL_MEM_OTHER);
    }

    MPL_free(global_ids);
    MPIDU_Init_shm_put(&my_max_dev_id, sizeof(int));
    MPIDU_Init_shm_barrier();

    /* get node max device id */
    for (int i = 0; i < MPIR_Process.local_size; i++) {
        MPIDU_Init_shm_get(i, sizeof(int), &my_max_dev_id);
        if (my_max_dev_id > node_max_dev_id)
            node_max_dev_id = my_max_dev_id;
    }
    MPIDU_Init_shm_barrier();

    MPIDI_GPUI_global.global_max_dev_id = node_max_dev_id;

    MPIR_CHKPMEM_MALLOC(MPIDI_GPUI_global.visible_dev_global_id, int **,
                        sizeof(int *) * MPIR_Process.local_size, mpi_errno, "gpu devmaps",
                        MPL_MEM_SHM);
    for (int i = 0; i < MPIR_Process.local_size; ++i) {
        MPIDI_GPUI_global.visible_dev_global_id[i] =
            (int *) MPL_malloc(sizeof(int) * (MPIDI_GPUI_global.global_max_dev_id + 1),
                               MPL_MEM_OTHER);
        MPIR_Assert(MPIDI_GPUI_global.visible_dev_global_id[i]);

        if (i == MPIR_Process.local_rank) {
            for (int j = 0; j < (MPIDI_GPUI_global.global_max_dev_id + 1); ++j) {
                MPIDI_GPUI_dev_id_t *tmp = NULL;
                HASH_FIND_INT(MPIDI_GPUI_global.global_to_local_map, &j, tmp);
                if (tmp)
                    MPIDI_GPUI_global.visible_dev_global_id[i][j] = 1;
                else
                    MPIDI_GPUI_global.visible_dev_global_id[i][j] = 0;
            }
            MPIDU_Init_shm_put(MPIDI_GPUI_global.visible_dev_global_id[i],
                               sizeof(int) * (MPIDI_GPUI_global.global_max_dev_id + 1));
        }
    }
    MPIDU_Init_shm_barrier();

    /* FIXME: current implementation uses MPIDU_Init_shm_get to exchange visible id.
     * shm buffer size is defined as 64 bytes by default. Therefore, if number of
     * gpu device is larger than 16, the MPIDU_Init_shm_get would fail. */
    MPIR_Assert((MPIDI_GPUI_global.global_max_dev_id + 1) <=
                MPIDU_INIT_SHM_BLOCK_SIZE / sizeof(int));
    for (int i = 0; i < MPIR_Process.local_size; ++i)
        MPIDU_Init_shm_get(i, sizeof(int) * (MPIDI_GPUI_global.global_max_dev_id + 1),
                           MPIDI_GPUI_global.visible_dev_global_id[i]);
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
           sizeof(MPL_gavl_tree_t) * MPIR_Process.local_size);

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
               sizeof(MPL_gavl_tree_t *) * (MPIDI_GPUI_global.global_max_dev_id + 1));

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

    MPIDI_GPUI_global.initialized = 1;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GPU_MPI_INIT_HOOK);
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIDI_GPU_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_GPUI_dev_id_t *current, *tmp;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GPU_MPI_FINALIZE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GPU_MPI_FINALIZE_HOOK);

    if (MPIDI_GPUI_global.initialized) {
        HASH_ITER(hh, MPIDI_GPUI_global.local_to_global_map, current, tmp) {
            HASH_DEL(MPIDI_GPUI_global.local_to_global_map, current);
            MPL_free(current);
        }

        HASH_ITER(hh, MPIDI_GPUI_global.global_to_local_map, current, tmp) {
            HASH_DEL(MPIDI_GPUI_global.global_to_local_map, current);
            MPL_free(current);
        }
        MPL_free(MPIDI_GPUI_global.local_ranks);
        for (int i = 0; i < MPIR_Process.local_size; ++i)
            MPL_free(MPIDI_GPUI_global.visible_dev_global_id[i]);
        MPL_free(MPIDI_GPUI_global.visible_dev_global_id);
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

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GPU_MPI_FINALIZE_HOOK);
    return mpi_errno;
}
