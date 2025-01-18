/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===
cvars:
    - name        : MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE
      category    : CH4
      type        : enum
      default     : specialized
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        By default, we will cache ipc handles using the specialized cache mechanism. If the
        gpu-specific backend does not implement a specialized cache, then we will fallback to
        the generic cache mechanism. Users can optionally force the generic cache mechanism or
        disable ipc caching entirely.
        generic - use the cache mechanism in the generic layer
        specialized - use the cache mechanism in a gpu-specific mpl layer (if applicable)
        disabled - disable caching completely

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

    - name        : MPIR_CVAR_CH4_IPC_GPU_FAST_COPY_MAX_SIZE
      category    : CH4
      type        : int
      default     : 1024
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If a send message size is less than or equal to MPIR_CVAR_CH4_IPC_GPU_FAST_COPY_MAX_SIZE (in
        bytes), then enable GPU-basedfast memcpy. The environment variable is valid only when then
        GPU IPC shmmod is enabled.

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

    - name        : MPIR_CVAR_CH4_IPC_GPU_ENGINE_TYPE
      category    : CH4
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        By default, select engine type automatically
        auto - select automatically
        compute - use compute engine
        copy_high_bandwidth - use high-bandwidth copy engine
        copy_low_latency - use low-latency copy engine

    - name        : MPIR_CVAR_CH4_IPC_GPU_READ_WRITE_PROTOCOL
      category    : CH4
      type        : enum
      default     : read
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        By default, use read protocol.
        auto - select automatically
        read - use read protocol
        write - use write protocol if remote device is visible

    - name        : MPIR_CVAR_CH4_IPC_GPU_RMA_ENGINE_TYPE
      category    : CH4
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        By default, select engine type automatically
        yaksa - don't select, use yaksa
        auto - select automatically
        compute - use compute engine
        copy_high_bandwidth - use high-bandwidth copy engine
        copy_low_latency - use low-latency copy engine

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#include "mpidimpl.h"
#include "gpu_pre.h"
#include "gpu_types.h"
#include "mpir_async_things.h"

#ifdef MPIDI_CH4_SHM_ENABLE_GPU

static int ipc_handle_cache_search(MPL_gavl_tree_t gavl_tree, const void *addr, uintptr_t len,
                                   void **handle_obj)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    *handle_obj = NULL;
    if (MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE == MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE_generic) {
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
    if (MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE == MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE_generic) {
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
    if (MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE == MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE_generic) {
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
        mpl_err =
            MPL_gpu_ipc_handle_create(pbase, &ipc_attr->gpu_attr.device_attr,
                                      &ipc_attr->ipc_handle.gpu.ipc_handle);
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
    int remote_local_dev_id;

    if (MPIR_CVAR_CH4_IPC_GPU_READ_WRITE_PROTOCOL == MPIR_CVAR_CH4_IPC_GPU_READ_WRITE_PROTOCOL_read) {
        map_to_dev_id = local_dev_id;
    } else {
        remote_local_dev_id = MPL_gpu_global_to_local_dev_id(remote_global_dev_id);
        if (MPIR_CVAR_CH4_IPC_GPU_READ_WRITE_PROTOCOL ==
            MPIR_CVAR_CH4_IPC_GPU_READ_WRITE_PROTOCOL_write) {
            map_to_dev_id = remote_local_dev_id;
            if (map_to_dev_id < 0)
                map_to_dev_id = local_dev_id;
        } else {
            /* auto select */
            MPIDI_Datatype_check_contig(datatype, dt_contig);
            /* TODO: more analyses on the non-contig cases */
            if (remote_local_dev_id == -1 || !dt_contig) {
                map_to_dev_id = local_dev_id;
            } else {
                map_to_dev_id = remote_local_dev_id;
            }
        }
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

int MPIDI_GPU_ipc_handle_map(MPIDI_GPU_ipc_handle_t handle, int map_dev_id, void **vaddr,
                             bool do_mmap)
{
    int mpi_errno = MPI_SUCCESS;

#ifdef MPIDI_CH4_SHM_ENABLE_GPU
    MPIR_FUNC_ENTER;

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
        if (do_mmap) {
#ifdef MPL_HAVE_ZE
            mpl_err =
                MPL_ze_ipc_handle_mmap_host(&handle.ipc_handle, 1, map_dev_id, handle.len, &pbase);
            MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                                "**gpu_ipc_handle_map");
            *vaddr = (void *) ((uintptr_t) pbase + handle.offset);
#else
            mpi_errno = MPI_ERR_OTHER;
            goto fn_fail;
#endif
        } else {
            mpl_err = MPL_gpu_ipc_handle_map(&handle.ipc_handle, map_dev_id, &pbase);
            MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                                "**gpu_ipc_handle_map");

            *vaddr = (void *) ((uintptr_t) pbase + handle.offset);
        }

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

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
#else
    return mpi_errno;
#endif
}

int MPIDI_GPU_ipc_handle_unmap(void *vaddr, MPIDI_GPU_ipc_handle_t handle, int do_mmap)
{
    int mpi_errno = MPI_SUCCESS;

#ifdef MPIDI_CH4_SHM_ENABLE_GPU
    MPIR_FUNC_ENTER;

    if (MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE == MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE_disabled) {
        int mpl_err = MPL_SUCCESS;
        mpl_err = MPL_gpu_ipc_handle_unmap((void *) ((uintptr_t) vaddr - handle.offset));
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**gpu_ipc_handle_unmap");
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
#else
    return mpi_errno;
#endif
}

/* src pointer is a GPU pointer with received IPC handle
 * dest pointer is a local pointer
 * contig type only */
int MPIDI_GPU_ipc_fast_memcpy(MPIDI_IPCI_ipc_handle_t ipc_handle, void *dest_vaddr,
                              MPI_Aint src_data_sz, MPI_Datatype datatype)
{
    int mpi_errno = MPI_SUCCESS;

#ifdef MPIDI_CH4_SHM_ENABLE_GPU
    int mpl_err = MPL_SUCCESS;
    void *src_buf_host;
    MPIDI_IPCI_ipc_attr_t ipc_attr;
    MPI_Aint true_lb, true_extent;

    MPIR_FUNC_ENTER;

    /* find out local device id */
    MPIR_GPU_query_pointer_attr(dest_vaddr, &ipc_attr.gpu_attr);
    int dev_id = MPL_gpu_get_dev_id_from_attr(&ipc_attr.gpu_attr);
    int map_dev = MPIDI_GPU_ipc_get_map_dev(ipc_handle.gpu.global_dev_id, dev_id, datatype);

    /* mmap remote buffer */
    mpi_errno = MPIDI_GPU_ipc_handle_map(ipc_handle.gpu, map_dev, &src_buf_host, 1);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    dest_vaddr = (void *) ((char *) dest_vaddr + true_lb);

    mpl_err = MPL_gpu_fast_memcpy(src_buf_host, NULL, dest_vaddr, &ipc_attr.gpu_attr, src_data_sz);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mpl_gpu_fast_memcpy");

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
#else
    return mpi_errno;
#endif
}

/* nonblocking IPCI_copy_data via MPIR_Async_things */
struct gpu_ipc_async {
    MPIR_Request *rreq;
    /* async handle */
    MPIR_gpu_req yreq;
    /* for unmap */
    void *src_buf;
    MPIDI_GPU_ipc_handle_t gpu_handle;
};

static int gpu_ipc_async_poll(MPIR_Async_thing * thing)
{
    int err;
    int is_done = 0;

    struct gpu_ipc_async *p = MPIR_Async_thing_get_state(thing);
    switch (p->yreq.type) {
        case MPIR_NULL_REQUEST:
            /* a dummy, immediately complete */
            is_done = 1;
            break;
        case MPIR_TYPEREP_REQUEST:
            MPIR_Typerep_test(p->yreq.u.y_req, &is_done);
            break;
        case MPIR_GPU_REQUEST:
            err = MPL_gpu_test(&p->yreq.u.gpu_req, &is_done);
            MPIR_Assertp(err == MPL_SUCCESS);
            break;
        default:
            MPIR_Assert(0);
    }

    if (is_done) {
        int vci = MPIDIG_REQUEST(p->rreq, req->local_vci);

        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
        err = MPIDI_GPU_ipc_handle_unmap(p->src_buf, p->gpu_handle, 0);
        MPIR_Assertp(err == MPI_SUCCESS);
        err = MPIDI_IPC_complete(p->rreq, MPIDI_IPCI_TYPE__GPU);
        MPIR_Assertp(err == MPI_SUCCESS);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);

        MPL_free(p);
        return MPIR_ASYNC_THING_DONE;
    }

    return MPIR_ASYNC_THING_NOPROGRESS;
}

int MPIDI_GPU_ipc_async_start(MPIR_Request * rreq, MPIR_gpu_req * req_p,
                              void *src_buf, MPIDI_GPU_ipc_handle_t gpu_handle)
{
    int mpi_errno = MPI_SUCCESS;

    struct gpu_ipc_async *p;
    p = MPL_malloc(sizeof(*p), MPL_MEM_OTHER);
    p->rreq = rreq;
    p->src_buf = src_buf;
    p->gpu_handle = gpu_handle;
    if (req_p) {
        p->yreq = *req_p;
    } else {
        p->yreq.type = MPIR_NULL_REQUEST;
    }

    mpi_errno = MPIR_Async_things_add(gpu_ipc_async_poll, p);

    return mpi_errno;
}
