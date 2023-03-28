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

    - name        : MPIR_CVAR_CH4_IPC_GPU_P2P_THRESHOLD
      category    : CH4
      type        : int
      default     : 1048576
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If a send message size is greater than or equal to MPIR_CVAR_CH4_IPC_GPU_P2P_THRESHOLD (in
        bytes), then enable GPU-based single copy protocol for intranode communication. The
        environment variable is valid only when then GPU IPC shmmod is enabled.

    - name        : MPIR_CVAR_CH4_IPC_ZE_SHAREABLE_HANDLE
      category    : CH4
      type        : enum
      default     : drmfd
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select implementation for ZE shareable IPC handle
        pidfd - use pidfd_getfd syscall to implement shareable IPC handle
        drmfd - force to use device fd-based shareable IPC handle
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

    MPIR_FUNC_ENTER;

    *handle_obj = NULL;
    if (MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE) {
        int mpl_err = MPL_SUCCESS;
        mpl_err = MPL_gavl_tree_search(gavl_tree, addr, len, handle_obj);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mpl_gavl_search");
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ipc_handle_cache_insert(MPL_gavl_tree_t gavl_tree, const void *addr, uintptr_t len,
                                   const void *handle_obj, bool * insert_successful)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    *insert_successful = false;
    if (MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE) {
        int mpl_err = MPL_SUCCESS;
        mpl_err = MPL_gavl_tree_insert(gavl_tree, addr, len, handle_obj);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mpl_gavl_insert");
        *insert_successful = true;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ipc_handle_cache_delete(MPL_gavl_tree_t gavl_tree, const void *addr, uintptr_t len)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    int mpl_err = MPL_SUCCESS;
    if (MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE) {
        mpl_err = MPL_gavl_tree_delete_range(gavl_tree, addr, len);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**mpl_gavl_delete_range");
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif

int MPIDI_GPU_ipc_handle_cache_insert(int rank, MPIR_Comm * comm, MPIDI_GPU_ipc_handle_t handle)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

#ifdef MPIDI_CH4_SHM_ENABLE_GPU
    if (handle.handle_status == MPIDI_GPU_IPC_HANDLE_REMAP_REQUIRED) {
        bool insert_successful = false;
        int recv_lrank = MPIDI_GPUI_global.local_ranks[MPIDIU_rank_to_lpid(rank, comm)];

        MPIDI_GPU_ipc_handle_t *handle_obj =
            MPL_malloc(sizeof(MPIDI_GPU_ipc_handle_t), MPL_MEM_OTHER);
        *handle_obj = handle;
        handle_obj->handle_status = MPIDI_GPU_IPC_HANDLE_VALID;

        mpi_errno = ipc_handle_cache_insert(MPIDI_GPUI_global.ipc_handle_track_trees[recv_lrank]
                                            [handle.local_dev_id],
                                            (void *) handle.remote_base_addr, handle.len,
                                            handle_obj, &insert_successful);
        MPIR_ERR_CHECK(mpi_errno);

        if (insert_successful == false) {
            MPL_free(handle_obj);
        }
    }
  fn_fail:
#endif

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_GPU_get_ipc_attr(const void *vaddr, int rank, MPIR_Comm * comm,
                           MPIDI_IPCI_ipc_attr_t * ipc_attr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

#ifdef MPIDI_CH4_SHM_ENABLE_GPU
    int local_dev_id;
    void *pbase;
    uintptr_t len;
    int mpl_err = MPL_SUCCESS;
    MPIDI_GPU_ipc_handle_t *handle_obj = NULL;
    int recv_lrank;

    recv_lrank = MPIDI_GPUI_global.local_ranks[MPIDIU_rank_to_lpid(rank, comm)];
    ipc_attr->ipc_type = MPIDI_IPCI_TYPE__GPU;
    ipc_attr->threshold.send_lmt_sz = MPIR_CVAR_CH4_IPC_GPU_P2P_THRESHOLD;

    local_dev_id = MPL_gpu_get_dev_id_from_attr(&ipc_attr->gpu_attr);
    int global_dev_id = MPL_gpu_local_to_global_dev_id(local_dev_id);

    mpl_err = MPL_gpu_get_buffer_bounds(vaddr, &pbase, &len);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gpu_get_buffer_info");

    /* Don't create or search for an ipc handle if the buffer doesn't meet the threshold
     * requirement */
    if (len < ipc_attr->threshold.send_lmt_sz && !MPIDI_IPCI_is_repeat_addr(pbase)) {
        ipc_attr->ipc_type = MPIDI_IPCI_TYPE__NONE;
        goto fn_fail;
    }

    MPL_gavl_tree_t track_tree = MPIDI_GPUI_global.ipc_handle_track_trees[recv_lrank][local_dev_id];
    mpi_errno = ipc_handle_cache_search(track_tree, pbase, len, (void **) &handle_obj);
    MPIR_ERR_CHECK(mpi_errno);

    if (handle_obj == NULL) {
        mpl_err = MPL_gpu_ipc_handle_create(pbase, &ipc_attr->ipc_handle.gpu.ipc_handle);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**gpu_ipc_handle_create");
        ipc_attr->ipc_handle.gpu.handle_status = MPIDI_GPU_IPC_HANDLE_REMAP_REQUIRED;
    } else {
        ipc_attr->ipc_handle.gpu.ipc_handle = handle_obj->ipc_handle;
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

    ipc_attr->ipc_handle.gpu.global_dev_id = global_dev_id;
    ipc_attr->ipc_handle.gpu.local_dev_id = local_dev_id;

    if (handle_obj == NULL) {
        mpi_errno = MPIDI_GPU_ipc_handle_cache_insert(rank, comm, ipc_attr->ipc_handle.gpu);
        MPIR_ERR_CHECK(mpi_errno);
    }
  fn_fail:
#else
    /* Do not support IPC data transfer */
    ipc_attr->ipc_type = MPIDI_IPCI_TYPE__NONE;
    ipc_attr->threshold.send_lmt_sz = -1;
#endif

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_GPU_ipc_get_map_dev(int remote_global_dev_id, int local_dev_id, MPI_Datatype datatype)
{
    int map_to_dev_id = -1;

    MPIR_FUNC_ENTER;

#ifdef MPIDI_CH4_SHM_ENABLE_GPU
    int dt_contig;
    MPIDI_Datatype_check_contig(datatype, dt_contig);

    int remote_local_dev_id = MPL_gpu_global_to_local_dev_id(remote_global_dev_id);

    /* TODO: more analyses on the non-contig cases */
    if (remote_local_dev_id == -1 || !dt_contig) {
        map_to_dev_id = local_dev_id;
    } else {
        map_to_dev_id = remote_local_dev_id;
    }

    if (map_to_dev_id < 0) {
        /* This is the case for local host memory. We need a valid device id
         * to map on. Assume at least device 0 is always available. */
        map_to_dev_id = 0;
    }
#endif

    MPIR_FUNC_EXIT;
    return map_to_dev_id;
}

int MPIDI_GPU_ipc_handle_map(MPIDI_GPU_ipc_handle_t handle, int map_dev_id, void **vaddr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

#ifdef MPIDI_CH4_SHM_ENABLE_GPU
    void *pbase;
    int mpl_err = MPL_SUCCESS;
    MPIDI_GPUI_handle_obj_s *handle_obj = NULL;

#define MAPPED_TREE(i) MPIDI_GPUI_global.ipc_handle_mapped_trees[handle.node_rank][handle.local_dev_id][i]
    if (handle.handle_status == MPIDI_GPU_IPC_HANDLE_REMAP_REQUIRED) {
        for (int i = 0; i < MPIDI_GPUI_global.local_device_count; ++i) {
            mpi_errno = ipc_handle_cache_delete(MAPPED_TREE(i),
                                                (void *) handle.remote_base_addr, handle.len);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    mpi_errno = ipc_handle_cache_search(MAPPED_TREE(map_dev_id),
                                        (void *) handle.remote_base_addr, handle.len,
                                        (void **) &handle_obj);
    MPIR_ERR_CHECK(mpi_errno);

    if (handle_obj) {
        /* found in cache */
        *vaddr = (void *) (handle_obj->mapped_base_addr + handle.offset);
    } else {
        mpl_err = MPL_gpu_ipc_handle_map(handle.ipc_handle, map_dev_id, &pbase);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**gpu_ipc_handle_map");

        *vaddr = (void *) ((uintptr_t) pbase + handle.offset);

        /* insert to cache */
        bool insert_successful = false;
        handle_obj = MPL_malloc(sizeof(MPIDI_GPUI_handle_obj_s), MPL_MEM_OTHER);
        MPIR_Assert(handle_obj != NULL);
        handle_obj->mapped_base_addr = (uintptr_t) pbase;
        mpi_errno = ipc_handle_cache_insert(MAPPED_TREE(map_dev_id),
                                            (void *) handle.remote_base_addr, handle.len,
                                            handle_obj, &insert_successful);
        MPIR_ERR_CHECK(mpi_errno);
        if (!insert_successful) {
            MPL_free(handle_obj);
        }
    }
#undef MAPPED_TREE
  fn_fail:
#endif

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_GPU_ipc_handle_unmap(void *vaddr, MPIDI_GPU_ipc_handle_t handle)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

#ifdef MPIDI_CH4_SHM_ENABLE_GPU
    if (!MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE) {
        int mpl_err = MPL_SUCCESS;
        mpl_err = MPL_gpu_ipc_handle_unmap((void *) ((uintptr_t) vaddr - handle.offset));
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**gpu_ipc_handle_unmap");
    }
  fn_fail:
#endif

    MPIR_FUNC_EXIT;
    return mpi_errno;
}
