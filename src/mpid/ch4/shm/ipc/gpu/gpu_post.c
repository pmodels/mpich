/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===
cvars:
    - name        : MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE
      category    : CH4
      type        : int
      default     : 1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        By default, we will cache ipc handle. To manually disable ipc
        handle cache, user can set this variable to 0.
=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#include "mpidimpl.h"
#include "gpu_pre.h"
#include "gpu_types.h"

#ifdef MPIDI_CH4_SHM_ENABLE_GPU

static int ipc_handle_cache_search(MPL_gavl_tree_t gavl_tree, const void *addr, uintptr_t len,
                                   void **handle_obj)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_IPC_HANDLE_CACHE_SEARCH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_IPC_HANDLE_CACHE_SEARCH);

    *handle_obj = NULL;
    if (MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE) {
        int mpl_err = MPL_SUCCESS;
        mpl_err = MPL_gavl_tree_search(gavl_tree, addr, len, handle_obj);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mpl_gavl_search");
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_IPC_HANDLE_CACHE_SEARCH);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ipc_handle_cache_insert(MPL_gavl_tree_t gavl_tree, const void *addr, uintptr_t len,
                                   const void *handle_obj, bool * insert_successful)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_IPC_HANDLE_CACHE_INSERT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_IPC_HANDLE_CACHE_INSERT);

    *insert_successful = false;
    if (MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE) {
        int mpl_err = MPL_SUCCESS;
        mpl_err = MPL_gavl_tree_insert(gavl_tree, addr, len, handle_obj);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mpl_gavl_insert");
        *insert_successful = true;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_IPC_HANDLE_CACHE_INSERT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int get_map_device(int remote_global_dev_id,
                          MPL_gpu_device_handle_t local_dev_handle,
                          MPI_Datatype recv_type, int *dev_id)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_GET_MAP_DEVICE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_GET_MAP_DEVICE);

    int recv_dev_id;
    int recv_dt_contig;
    MPIDI_GPUI_dev_id_t *avail_id = NULL;

    MPIDI_Datatype_check_contig(recv_type, recv_dt_contig);

    HASH_FIND_INT(MPIDI_GPUI_global.global_to_local_map, &remote_global_dev_id, avail_id);
    MPL_gpu_get_dev_id(local_dev_handle, &recv_dev_id);
    if (recv_dev_id < 0) {
        /* when receiver's buffer is on host memory, recv_dev_id will be less than 0.
         * however, when we decide to map buffer onto receiver's device, this mapping
         * will be invalid, so we need to assign a default gpu instead; for now, we
         * assume process can at least access one GPU, so device id 0 is set. */
        recv_dev_id = 0;
    }

    if (avail_id == NULL) {
        *dev_id = recv_dev_id;
    } else {
        if (!recv_dt_contig)
            *dev_id = recv_dev_id;
        else
            *dev_id = avail_id->local_dev_id;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_GET_MAP_DEVICE);
    return mpi_errno;
}

static int ipc_handle_cache_delete(MPL_gavl_tree_t gavl_tree, const void *addr, uintptr_t len)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_IPC_HANDLE_DELETE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_IPC_HANDLE_DELETE);

    int mpl_err = MPL_SUCCESS;
    if (MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE) {
        mpl_err = MPL_gavl_tree_delete_range(gavl_tree, addr, len);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**mpl_gavl_delete_range");
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_IPC_HANDLE_DELETE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif

int MPIDI_GPU_ipc_handle_cache_insert(int rank, MPIR_Comm * comm, MPIDI_GPU_ipc_handle_t handle)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GPU_IPC_HANDLE_CACHE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GPU_IPC_HANDLE_CACHE);

#ifdef MPIDI_CH4_SHM_ENABLE_GPU
    if (handle.handle_status == MPIDI_GPU_IPC_HANDLE_REMAP_REQUIRED) {
        bool insert_successful = false;
        int recv_lrank = MPIDI_GPUI_global.local_ranks[MPIDIU_rank_to_lpid(rank, comm)];

        MPIDI_GPU_ipc_handle_t *handle_obj =
            MPL_malloc(sizeof(MPIDI_GPU_ipc_handle_t), MPL_MEM_OTHER);
        *handle_obj = handle;
        handle_obj->handle_status = MPIDI_GPU_IPC_HANDLE_VALID;

        mpi_errno = ipc_handle_cache_insert(MPIDI_GPUI_global.ipc_handle_track_trees[recv_lrank]
                                            [handle.global_dev_id],
                                            (void *) handle.remote_base_addr, handle.len,
                                            handle_obj, &insert_successful);
        MPIR_ERR_CHECK(mpi_errno);

        if (insert_successful == false) {
            MPL_free(handle_obj);
        }
    }
  fn_fail:
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GPU_IPC_HANDLE_CACHE);
    return mpi_errno;
}

int MPIDI_GPU_get_ipc_attr(const void *vaddr, int rank, MPIR_Comm * comm,
                           MPIDI_IPCI_ipc_attr_t * ipc_attr)
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
    MPIDI_GPU_ipc_handle_t *handle_obj = NULL;
    int recv_lrank;

    recv_lrank = MPIDI_GPUI_global.local_ranks[MPIDIU_rank_to_lpid(rank, comm)];
    ipc_attr->ipc_type = MPIDI_IPCI_TYPE__GPU;

    MPL_gpu_get_dev_id(ipc_attr->gpu_attr.device, &local_dev_id);
    HASH_FIND_INT(MPIDI_GPUI_global.local_to_global_map, &local_dev_id, tmp);

    mpl_err = MPL_gpu_get_buffer_bounds(vaddr, &pbase, &len);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gpu_get_buffer_info");

    mpi_errno = ipc_handle_cache_search(MPIDI_GPUI_global.ipc_handle_track_trees[recv_lrank]
                                        [tmp->global_dev_id], pbase, len, (void **) &handle_obj);
    MPIR_ERR_CHECK(mpi_errno);

    if (handle_obj == NULL) {
        mpl_err = MPL_gpu_ipc_handle_create(pbase, &ipc_attr->ipc_handle.gpu.ipc_handle);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**gpu_ipc_handle_create");
        ipc_attr->ipc_handle.gpu.handle_status = MPIDI_GPU_IPC_HANDLE_REMAP_REQUIRED;
    } else {
        ipc_attr->ipc_handle.gpu.handle_status = MPIDI_GPU_IPC_HANDLE_VALID;
    }

    /* MPIDI_GPU_get_ipc_attr will be called by sender to create an ipc handle.
     * remote_base_addr, len and node_rank attributes in ipc handle will be sent
     * to receiver and used to search cached ipc handle and/or insert new allocated
     * handle obj on receiver side. offset attribute is always needed no matter
     * whether we use caching or not in order to compute correct user addr. */
    ipc_attr->ipc_handle.gpu.remote_base_addr = (uintptr_t) pbase;
    ipc_attr->ipc_handle.gpu.len = len;
    ipc_attr->ipc_handle.gpu.node_rank = MPIR_Process.local_rank;
    ipc_attr->ipc_handle.gpu.offset = (uintptr_t) vaddr - (uintptr_t) pbase;

    ipc_attr->ipc_handle.gpu.global_dev_id = tmp->global_dev_id;
    ipc_attr->threshold.send_lmt_sz = MPIR_CVAR_CH4_IPC_GPU_P2P_THRESHOLD;
  fn_fail:
#else
    /* Do not support IPC data transfer */
    ipc_attr->ipc_type = MPIDI_IPCI_TYPE__NONE;
    ipc_attr->threshold.send_lmt_sz = MPIR_AINT_MAX;
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GPU_GET_IPC_ATTR);
    return mpi_errno;
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
    int mpl_err = MPL_SUCCESS;
    int dev_id;
    MPIDI_GPUI_handle_obj_s *handle_obj = NULL;

    if (handle.handle_status == MPIDI_GPU_IPC_HANDLE_REMAP_REQUIRED) {
        for (int i = 0; i < MPIDI_GPUI_global.local_device_count; ++i) {
            mpi_errno =
                ipc_handle_cache_delete(MPIDI_GPUI_global.ipc_handle_mapped_trees[handle.node_rank]
                                        [handle.global_dev_id][i], (void *) handle.remote_base_addr,
                                        handle.len);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    mpi_errno = get_map_device(handle.global_dev_id, dev_handle, recv_type, &dev_id);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = ipc_handle_cache_search(MPIDI_GPUI_global.ipc_handle_mapped_trees[handle.node_rank]
                                        [handle.global_dev_id][dev_id],
                                        (void *) handle.remote_base_addr, handle.len,
                                        (void **) &handle_obj);
    MPIR_ERR_CHECK(mpi_errno);

    if (handle_obj == NULL) {
        bool insert_successful = false;
        MPL_gpu_get_dev_handle(dev_id, &dev_handle);
        mpl_err = MPL_gpu_ipc_handle_map(handle.ipc_handle, dev_handle, &pbase);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**gpu_ipc_handle_map");

        *vaddr = (void *) ((uintptr_t) pbase + handle.offset);

        handle_obj =
            (MPIDI_GPUI_handle_obj_s *) MPL_malloc(sizeof(MPIDI_GPUI_handle_obj_s), MPL_MEM_OTHER);
        MPIR_Assert(handle_obj != NULL);
        handle_obj->mapped_base_addr = (uintptr_t) pbase;
        mpi_errno =
            ipc_handle_cache_insert(MPIDI_GPUI_global.ipc_handle_mapped_trees[handle.node_rank]
                                    [handle.global_dev_id][dev_id],
                                    (void *) handle.remote_base_addr, handle.len, handle_obj,
                                    &insert_successful);
        MPIR_ERR_CHECK(mpi_errno);
        if (insert_successful == false)
            MPL_free(handle_obj);
    } else {
        *vaddr = (void *) (handle_obj->mapped_base_addr + handle.offset);
    }
  fn_fail:
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GPU_IPC_HANDLE_MAP);
    return mpi_errno;
}

int MPIDI_GPU_ipc_handle_unmap(void *vaddr, MPIDI_GPU_ipc_handle_t handle)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GPU_IPC_HANDLE_UNMAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GPU_IPC_HANDLE_UNMAP);

#ifdef MPIDI_CH4_SHM_ENABLE_GPU
    if (!MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE) {
        int mpl_err = MPL_SUCCESS;
        mpl_err = MPL_gpu_ipc_handle_unmap((void *) ((uintptr_t) vaddr - handle.offset));
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**gpu_ipc_handle_unmap");
    }
  fn_fail:
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GPU_IPC_HANDLE_UNMAP);
    return mpi_errno;
}
