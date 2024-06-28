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

/* handle_track_tree caches local MPL_gpu_ipc_mem_handle_t */

static void ipc_track_cache_free(void *handle_obj);
int MPIDI_GPUI_create_ipc_track_trees(void)
{
    int mpi_errno = MPI_SUCCESS;

    int num_ranks = MPIR_Process.local_size;
    int num_gdevs = MPIDI_GPUI_global.global_max_dev_id + 1;

#define TREES MPIDI_GPUI_global.ipc_handle_track_trees
    TREES = MPL_calloc(sizeof(MPL_gavl_tree_t *), num_ranks, MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!TREES, mpi_errno, MPI_ERR_OTHER, "**nomem");

    for (int i = 0; i < num_ranks; i++) {
        TREES[i] = MPL_calloc(sizeof(MPL_gavl_tree_t), num_gdevs, MPL_MEM_OTHER);
        MPIR_ERR_CHKANDJUMP(!TREES[i], mpi_errno, MPI_ERR_OTHER, "**nomem");

        for (int j = 0; j < num_gdevs; j++) {
            int mpl_err = MPL_gavl_tree_create(ipc_track_cache_free, &TREES[i][j]);
            MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                                "**mpl_gavl_create");
        }
    }
#undef TREES

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* handle_mapped_trees caches rempte mapped_base_addr */

static void ipc_mapped_cache_free(void *handle_obj);
int MPIDI_GPUI_create_ipc_mapped_trees(void)
{
    int mpi_errno = MPI_SUCCESS;

    int num_ranks = MPIR_Process.local_size;
    int num_gdevs = MPIDI_GPUI_global.global_max_dev_id + 1;
    int num_ldevs = MPIDI_GPUI_global.local_device_count;

#define TREES MPIDI_GPUI_global.ipc_handle_mapped_trees
    TREES = MPL_calloc(sizeof(MPL_gavl_tree_t **), num_ranks, MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!TREES, mpi_errno, MPI_ERR_OTHER, "**nomem");

    for (int i = 0; i < num_ranks; i++) {
        TREES[i] = MPL_calloc(sizeof(MPL_gavl_tree_t *), num_gdevs, MPL_MEM_OTHER);
        MPIR_ERR_CHKANDJUMP(!TREES[i], mpi_errno, MPI_ERR_OTHER, "**nomem");

        for (int j = 0; j < num_gdevs; j++) {
            TREES[i][j] = MPL_calloc(sizeof(MPL_gavl_tree_t), num_ldevs, MPL_MEM_OTHER);
            MPIR_ERR_CHKANDJUMP(!TREES[i][j], mpi_errno, MPI_ERR_OTHER, "**nomem");

            for (int k = 0; k < num_ldevs; k++) {
                int mpl_err = MPL_gavl_tree_create(ipc_mapped_cache_free, &TREES[i][j][k]);
                MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                                    "**mpl_gavl_create");
            }
        }
    }
#undef TREES

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* -- handle_track_tree -- */

static void ipc_track_cache_free(void *obj)
{
    MPL_free(obj);
}

static int ipc_track_cache_search(MPL_gavl_tree_t gavl_tree, const void *addr, uintptr_t len,
                                  MPL_gpu_ipc_mem_handle_t * handle_out, bool * found)
{
    int mpi_errno = MPI_SUCCESS;

    void *obj;
    int mpl_err = MPL_gavl_tree_search(gavl_tree, addr, len, &obj);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mpl_gavl_search");

    if (obj) {
        *handle_out = *((MPL_gpu_ipc_mem_handle_t *) obj);
        *found = true;
    } else {
        *found = false;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ipc_track_cache_insert(MPL_gavl_tree_t gavl_tree, const void *addr, uintptr_t len,
                                  MPL_gpu_ipc_mem_handle_t handle)
{
    int mpi_errno = MPI_SUCCESS;

    MPL_gpu_ipc_mem_handle_t *cache_obj = MPL_malloc(sizeof(handle), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!cache_obj, mpi_errno, MPI_ERR_OTHER, "**nomem");

    *cache_obj = handle;

    int mpl_err;
    mpl_err = MPL_gavl_tree_insert(gavl_tree, addr, len, cache_obj);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mpl_gavl_insert");

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* -- mapped_track_tree -- */

static void ipc_mapped_cache_free(void *obj)
{
    int mpl_err ATTRIBUTE((unused));

    void *mapped_base_addr = obj;
    mpl_err = MPL_gpu_ipc_handle_unmap(mapped_base_addr);
    MPIR_Assert(mpl_err == MPL_SUCCESS);
}

static int ipc_mapped_cache_search(MPL_gavl_tree_t gavl_tree, const void *addr, uintptr_t len,
                                   void **mapped_base_addr_out)
{
    int mpi_errno = MPI_SUCCESS;

    int mpl_err = MPL_gavl_tree_search(gavl_tree, addr, len, mapped_base_addr_out);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mpl_gavl_search");

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ipc_mapped_cache_insert(MPL_gavl_tree_t gavl_tree, const void *addr, uintptr_t len,
                                   const void *mapped_base_addr)
{
    int mpi_errno = MPI_SUCCESS;

    int mpl_err = MPL_gavl_tree_insert(gavl_tree, addr, len, mapped_base_addr);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mpl_gavl_insert");

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ipc_mapped_cache_delete(MPL_gavl_tree_t gavl_tree, const void *addr, uintptr_t len)
{
    int mpi_errno = MPI_SUCCESS;

    int mpl_err = MPL_gavl_tree_delete_range(gavl_tree, addr, len);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                        "**mpl_gavl_delete_range");

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_GPU_get_ipc_attr(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                           int remote_rank, MPIR_Comm * comm, MPIDI_IPCI_ipc_attr_t * ipc_attr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    ipc_attr->ipc_type = MPIDI_IPCI_TYPE__NONE;
    if (buf == MPI_BOTTOM) {
        goto fn_exit;
    }

    MPIR_Datatype *dt_ptr;
    bool dt_contig;
    MPI_Aint true_lb;
    uintptr_t data_sz ATTRIBUTE((unused));
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, true_lb);

    void *mem_addr;
    if (dt_contig) {
        mem_addr = MPIR_get_contig_ptr(buf, true_lb);
    } else {
        mem_addr = (char *) buf;
    }
    MPIR_GPU_query_pointer_attr(mem_addr, &ipc_attr->u.gpu.gpu_attr);
    if (ipc_attr->u.gpu.gpu_attr.type != MPL_GPU_POINTER_DEV) {
        goto fn_exit;
    }

    if (!dt_contig) {
        if (dt_ptr->contents) {
            /* skip HINDEXED and STRUCT */
            int combiner = dt_ptr->contents->combiner;
            MPIR_Datatype *tmp_dtp = dt_ptr;
            while (combiner == MPI_COMBINER_DUP || combiner == MPI_COMBINER_RESIZED) {
                int *ints;
                MPI_Aint *aints, *counts;
                MPI_Datatype *types;
                MPIR_Datatype_access_contents(tmp_dtp->contents, &ints, &aints, &counts, &types);
                MPIR_Datatype_get_ptr(types[0], tmp_dtp);
                if (!tmp_dtp || !tmp_dtp->contents) {
                    break;
                }
                combiner = tmp_dtp->contents->combiner;
            }
            switch (combiner) {
                case MPI_COMBINER_HINDEXED:
                case MPI_COMBINER_STRUCT:
                    goto fn_exit;
            }
        }

        /* skip negative lb and extent */
        if (dt_ptr->true_lb < 0 || dt_ptr->extent < 0) {
            goto fn_exit;
        }
    }

    void *bounds_base;
    uintptr_t bounds_len;
    int mpl_err = MPL_gpu_get_buffer_bounds(mem_addr, &bounds_base, &bounds_len);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gpu_get_buffer_info");

    /* Don't create or search for an ipc handle if the buffer doesn't meet the threshold
     * requirement */
    if (bounds_len < MPIR_CVAR_CH4_IPC_GPU_P2P_THRESHOLD && !MPIDI_IPCI_is_repeat_addr(bounds_base)) {
        goto fn_exit;
    }

    ipc_attr->ipc_type = MPIDI_IPCI_TYPE__GPU;
    if (remote_rank != MPI_PROC_NULL) {
        remote_rank = MPIDI_GPUI_global.local_ranks[MPIDIU_rank_to_lpid(remote_rank, comm)];
    }

    ipc_attr->u.gpu.remote_rank = remote_rank;
    /* ipc.attr->u.gpu.gpu_attr is already set */
    ipc_attr->u.gpu.vaddr = mem_addr;
    ipc_attr->u.gpu.bounds_base = bounds_base;
    ipc_attr->u.gpu.bounds_len = bounds_len;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_GPU_fill_ipc_handle(MPIDI_IPCI_ipc_attr_t * ipc_attr,
                              MPIDI_IPCI_ipc_handle_t * ipc_handle)
{
    int mpi_errno = MPI_SUCCESS;

    int local_dev_id, global_dev_id;
    local_dev_id = MPL_gpu_get_dev_id_from_attr(&ipc_attr->u.gpu.gpu_attr);
    global_dev_id = MPL_gpu_local_to_global_dev_id(local_dev_id);

    void *pbase = ipc_attr->u.gpu.bounds_base;
    MPI_Aint len = ipc_attr->u.gpu.bounds_len;
    int remote_rank = ipc_attr->u.gpu.remote_rank;

    int need_cache = (remote_rank != MPI_PROC_NULL) &&
        (MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE == MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE_generic);

    MPL_gpu_ipc_mem_handle_t handle;
    int handle_status;
    MPL_gavl_tree_t track_tree = NULL;
    if (need_cache) {
        bool found = false;
        track_tree = MPIDI_GPUI_global.ipc_handle_track_trees[remote_rank][local_dev_id];
        mpi_errno = ipc_track_cache_search(track_tree, pbase, len, &handle, &found);
        MPIR_ERR_CHECK(mpi_errno);

        if (found) {
            handle_status = MPIDI_GPU_IPC_HANDLE_VALID;
            goto fn_done;
        }
    }

    int mpl_err = MPL_gpu_ipc_handle_create(pbase, &ipc_attr->u.gpu.gpu_attr.device_attr, &handle);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                        "**gpu_ipc_handle_create");
    if (need_cache) {
        mpi_errno = ipc_track_cache_insert(track_tree, pbase, len, handle);
        MPIR_ERR_CHECK(mpi_errno);
    }
    handle_status = MPIDI_GPU_IPC_HANDLE_REMAP_REQUIRED;

  fn_done:
    /* MPIDI_GPU_get_ipc_attr will be called by sender to create an ipc handle.
     * remote_base_addr, len and node_rank attributes in ipc handle will be sent
     * to receiver and used to search cached ipc handle and/or insert new allocated
     * handle obj on receiver side. offset attribute is always needed no matter
     * whether we use caching or not in order to compute correct user addr. */
    ipc_handle->gpu.ipc_handle = handle;
    ipc_handle->gpu.global_dev_id = global_dev_id;
    ipc_handle->gpu.local_dev_id = local_dev_id;
    ipc_handle->gpu.remote_base_addr = (uintptr_t) pbase;
    ipc_handle->gpu.len = len;
    ipc_handle->gpu.node_rank = MPIR_Process.local_rank;
    ipc_handle->gpu.offset = (uintptr_t) ipc_attr->u.gpu.vaddr - (uintptr_t) pbase;
    ipc_handle->gpu.handle_status = handle_status;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_GPU_ipc_get_map_dev(int remote_global_dev_id, int local_dev_id, MPI_Datatype datatype)
{
    int map_to_dev_id = -1;

    MPIR_FUNC_ENTER;

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

    MPIR_FUNC_EXIT;
    return map_to_dev_id;
}

int MPIDI_GPU_ipc_handle_map(MPIDI_GPU_ipc_handle_t handle, int map_dev_id, void **vaddr,
                             bool do_mmap)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    bool need_cache;
    need_cache = (MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE == MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE_generic);
#define MAPPED_TREE(i) MPIDI_GPUI_global.ipc_handle_mapped_trees[handle.node_rank][handle.local_dev_id][i]
    if (need_cache && handle.handle_status == MPIDI_GPU_IPC_HANDLE_REMAP_REQUIRED) {
        for (int i = 0; i < MPIDI_GPUI_global.local_device_count; ++i) {
            mpi_errno = ipc_mapped_cache_delete(MAPPED_TREE(i),
                                                (void *) handle.remote_base_addr, handle.len);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    void *pbase = NULL;
    if (need_cache) {
        mpi_errno = ipc_mapped_cache_search(MAPPED_TREE(map_dev_id),
                                            (void *) handle.remote_base_addr, handle.len, &pbase);
        MPIR_ERR_CHECK(mpi_errno);

        if (pbase) {
            /* found in cache */
            *vaddr = (void *) ((char *) pbase + handle.offset);
            goto fn_exit;
        }
    }

    if (do_mmap) {
#ifdef MPL_HAVE_ZE
        mpl_err =
            MPL_ze_ipc_handle_mmap_host(&handle.ipc_handle, 1, map_dev_id, handle.len, &pbase);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**gpu_ipc_handle_map");
#else
        MPIR_Assert(0);
#endif
    } else {
        int mpl_err = MPL_gpu_ipc_handle_map(&handle.ipc_handle, map_dev_id, &pbase);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**gpu_ipc_handle_map");
    }

    if (need_cache) {
        mpi_errno = ipc_mapped_cache_insert(MAPPED_TREE(map_dev_id),
                                            (void *) handle.remote_base_addr, handle.len, pbase);
        MPIR_ERR_CHECK(mpi_errno);
    }

    *vaddr = (void *) ((uintptr_t) pbase + handle.offset);
#undef MAPPED_TREE

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_GPU_ipc_handle_unmap(void *vaddr, MPIDI_GPU_ipc_handle_t handle, int do_mmap)
{
    int mpi_errno = MPI_SUCCESS;

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
}

/* src pointer is a GPU pointer with received IPC handle
 * dest pointer is a local pointer
 * contig type only */
int MPIDI_GPU_ipc_fast_memcpy(MPIDI_IPCI_ipc_handle_t ipc_handle, void *dest_vaddr,
                              MPI_Aint src_data_sz, MPI_Datatype datatype)
{
    int mpi_errno = MPI_SUCCESS;

    int mpl_err = MPL_SUCCESS;
    void *src_buf_host;
    MPL_pointer_attr_t gpu_attr;
    MPI_Aint true_lb, true_extent;

    MPIR_FUNC_ENTER;

    /* find out local device id */
    MPIR_GPU_query_pointer_attr(dest_vaddr, &gpu_attr);
    int dev_id = MPL_gpu_get_dev_id_from_attr(&gpu_attr);
    int map_dev = MPIDI_GPU_ipc_get_map_dev(ipc_handle.gpu.global_dev_id, dev_id, datatype);

    /* mmap remote buffer */
    mpi_errno = MPIDI_GPU_ipc_handle_map(ipc_handle.gpu, map_dev, &src_buf_host, 1);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    dest_vaddr = (void *) ((char *) dest_vaddr + true_lb);

    mpl_err = MPL_gpu_fast_memcpy(src_buf_host, NULL, dest_vaddr, &gpu_attr, src_data_sz);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mpl_gpu_fast_memcpy");

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* nonblocking IPCI_copy_data via MPIX_Async */
struct gpu_ipc_async {
    MPIR_Request *rreq;
    /* async handle */
    MPIR_gpu_req yreq;
    /* for unmap */
    void *src_buf;
    MPIDI_GPU_ipc_handle_t gpu_handle;
};

static int gpu_ipc_async_poll(MPIX_Async_thing thing)
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
        return MPIX_ASYNC_DONE;
    }

    return MPIX_ASYNC_NOPROGRESS;
}

static int gpu_ipc_async_start(MPIR_Request * rreq, MPIR_gpu_req * req_p,
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

    mpi_errno = MPIR_Async_things_add(gpu_ipc_async_poll, p, NULL);

    return mpi_errno;
}

int MPIDI_GPU_copy_data_async(MPIDI_IPC_hdr * ipc_hdr, MPIR_Request * rreq, MPI_Aint src_data_sz)
{
    int mpi_errno = MPI_SUCCESS;

    void *src_buf = NULL;
#ifdef MPL_HAVE_ZE
    bool do_mmap = (src_data_sz <= MPIR_CVAR_CH4_IPC_GPU_FAST_COPY_MAX_SIZE);
#else
    bool do_mmap = false;
#endif
    MPL_pointer_attr_t attr;
    MPIR_GPU_query_pointer_attr(MPIDIG_REQUEST(rreq, buffer), &attr);
    int dev_id = MPL_gpu_get_dev_id_from_attr(&attr);
    int map_dev = MPIDI_GPU_ipc_get_map_dev(ipc_hdr->ipc_handle.gpu.global_dev_id, dev_id,
                                            MPIDIG_REQUEST(rreq, datatype));
    mpi_errno = MPIDI_GPU_ipc_handle_map(ipc_hdr->ipc_handle.gpu, map_dev, &src_buf, do_mmap);
    MPIR_ERR_CHECK(mpi_errno);

    /* copy */
    MPI_Aint src_count;
    MPI_Datatype src_dt;
    MPIR_Datatype *src_dt_ptr = NULL;
    if (ipc_hdr->is_contig) {
        src_count = src_data_sz;
        src_dt = MPI_BYTE;
    } else {
        /* TODO: get sender datatype and call MPIR_Typerep_op with mapped_device set to dev_id */
        void *flattened_type = ipc_hdr + 1;
        src_dt_ptr = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
        MPIR_Assert(src_dt_ptr);
        mpi_errno = MPIR_Typerep_unflatten(src_dt_ptr, flattened_type);
        MPIR_ERR_CHECK(mpi_errno);

        src_count = ipc_hdr->count;
        src_dt = src_dt_ptr->handle;
    }
    MPIDIG_REQUEST(rreq, req->rreq.u.ipc.src_dt_ptr) = src_dt_ptr;

    MPIR_gpu_req yreq;
    MPL_gpu_engine_type_t engine =
        MPIDI_IPCI_choose_engine(ipc_hdr->ipc_handle.gpu.global_dev_id, dev_id);
    mpi_errno = MPIR_Ilocalcopy_gpu(src_buf, src_count, src_dt, 0, NULL,
                                    MPIDIG_REQUEST(rreq, buffer), MPIDIG_REQUEST(rreq, count),
                                    MPIDIG_REQUEST(rreq, datatype), 0, &attr,
                                    MPL_GPU_COPY_DIRECTION_NONE, engine, true, &yreq);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = gpu_ipc_async_start(rreq, &yreq, src_buf, ipc_hdr->ipc_handle.gpu);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif /* MPIDI_CH4_SHM_ENABLE_GPU */
