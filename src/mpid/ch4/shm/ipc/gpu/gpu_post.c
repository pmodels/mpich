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

    - name        : MPIR_CVAR_CH4_IPC_GPU_MAX_CACHE_ENTRIES
      category    : CH4
      type        : int
      default     : 16
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        The maximum number of entries to hold per device in the cache containing IPC mapped buffers.
        When an entry is evicted, the corresponding IPC handle is closed. This value is relevant
        only when MPIR_CVAR_CH4_IPC_GPU_CACHE_SIZE=limited.

    - name        : MPIR_CVAR_CH4_IPC_GPU_CACHE_SIZE
      category    : CH4
      type        : enum
      default     : limited
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        The behavior of the cache containing IPC mapped buffers.
        unlimited - don't restrict the cache size
        limited - limit the cache size based on MPIR_CVAR_CH4_IPC_GPU_MAX_CACHE_ENTRIES
        disabled - don't cache mapped IPC buffers

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

static int ipc_track_cache_search(const void *addr, MPL_gpu_ipc_mem_handle_t * handle_out,
                                  bool * found)
{
    struct MPIDI_GPUI_handle_cache_entry *entry;

    HASH_FIND_PTR(MPIDI_GPUI_global.ipc_handle_cache, &addr, entry);
    if (entry) {
        MPL_DBG_MSG_P(MPIDI_CH4_DBG_IPC, VERBOSE, "cached gpu ipc handle HIT for %p", addr);
        *handle_out = entry->handle;
        *found = true;
    } else {
        MPL_DBG_MSG_P(MPIDI_CH4_DBG_IPC, VERBOSE, "cached gpu ipc handle MISS for %p", addr);
        *found = false;
    }

    return MPI_SUCCESS;
}

static int ipc_track_cache_insert(const void *addr, MPL_gpu_ipc_mem_handle_t handle)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDI_GPUI_handle_cache_entry *entry;

    MPL_DBG_MSG_P(MPIDI_CH4_DBG_IPC, VERBOSE, "caching NEW gpu ipc handle for %p", addr);

    entry = MPL_malloc(sizeof(struct MPIDI_GPUI_handle_cache_entry), MPL_MEM_SHM);
    MPIR_ERR_CHKANDJUMP(!entry, mpi_errno, MPI_ERR_OTHER, "**nomem");
    entry->base_addr = addr;
    entry->handle = handle;
    HASH_ADD_PTR(MPIDI_GPUI_global.ipc_handle_cache, base_addr, entry, MPL_MEM_SHM);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ipc_track_cache_remove(const void *addr)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDI_GPUI_handle_cache_entry *entry;

    MPL_DBG_MSG_P(MPIDI_CH4_DBG_IPC, VERBOSE, "removing STALE gpu ipc handle for %p", addr);

    HASH_FIND_PTR(MPIDI_GPUI_global.ipc_handle_cache, &addr, entry);
    if (entry) {
        HASH_DEL(MPIDI_GPUI_global.ipc_handle_cache, entry);
        MPL_free(entry);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* -- mapped_track_tree -- */

static int ipc_mapped_cache_search(const void *remote_addr, int remote_rank, int device_id,
                                   void **mapped_base_addr_out)
{
    struct MPIDI_GPUI_map_cache_entry *entry;
    struct map_key key;

    memset(&key, 0, sizeof(key));
    key.remote_rank = remote_rank;
    key.remote_addr = remote_addr;
    HASH_FIND(hh, MPIDI_GPUI_global.ipc_map_cache, &key, sizeof(struct map_key), entry);

    if (entry) {
        MPL_DBG_MSG_P(MPIDI_CH4_DBG_IPC, VERBOSE, "mapped gpu ipc handle cache HIT for %p",
                      remote_addr);
        *mapped_base_addr_out = (void *) entry->mapped_addrs[device_id];
    } else {
        MPL_DBG_MSG_P(MPIDI_CH4_DBG_IPC, VERBOSE, "mapped gpu ipc handle MISS for %p", remote_addr);
        *mapped_base_addr_out = NULL;
    }

    return MPI_SUCCESS;
}

static int ipc_mapped_cache_insert(const void *remote_addr, int remote_rank, int device_id,
                                   const void *mapped_base_addr)
{
    int mpi_errno = MPI_SUCCESS;

    struct MPIDI_GPUI_map_cache_entry *entry;
    struct map_key key;

    MPL_DBG_MSG_P(MPIDI_CH4_DBG_IPC, VERBOSE, "caching NEW mapped ipc handle for %p", remote_addr);

    memset(&key, 0, sizeof(key));
    key.remote_rank = remote_rank;
    key.remote_addr = remote_addr;
    HASH_FIND(hh, MPIDI_GPUI_global.ipc_map_cache, &key, sizeof(struct map_key), entry);

    if (entry) {
        entry->mapped_addrs[device_id] = mapped_base_addr;
    } else {
        /* create and add new entry */
        size_t entry_size = sizeof(struct MPIDI_GPUI_map_cache_entry) +
            (MPIDI_GPUI_global.local_device_count * sizeof(void *));
        entry = MPL_malloc(entry_size, MPL_MEM_OTHER);
        memset(entry, 0, entry_size);
        entry->key.remote_rank = remote_rank;
        entry->key.remote_addr = remote_addr;
        entry->mapped_addrs[device_id] = mapped_base_addr;
        HASH_ADD(hh, MPIDI_GPUI_global.ipc_map_cache, key, sizeof(struct map_key), entry,
                 MPL_MEM_SHM);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ipc_mapped_cache_delete(const void *remote_addr, int remote_rank)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDI_GPUI_map_cache_entry *entry;
    struct map_key key;

    MPL_DBG_MSG_P(MPIDI_CH4_DBG_IPC, VERBOSE, "removing STALE mapped gpu ipc handle for %p",
                  remote_addr);

    memset(&key, 0, sizeof(key));
    key.remote_rank = remote_rank;
    key.remote_addr = remote_addr;
    HASH_FIND(hh, MPIDI_GPUI_global.ipc_map_cache, &key, sizeof(struct map_key), entry);

    if (entry) {
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
    if (MPL_gpu_attr_is_dev(&ipc_attr->u.gpu.gpu_attr)) {
        /* if it's a device buffer, we cannot do XPMEM or CMA IPC, so set default to SKIP */
        ipc_attr->ipc_type = MPIDI_IPCI_TYPE__SKIP;
    }
    if (!MPL_gpu_attr_is_strict_dev(&ipc_attr->u.gpu.gpu_attr)) {
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
     * requirement or it isn't a repeat address */
    if (bounds_len < MPIR_CVAR_CH4_IPC_GPU_P2P_THRESHOLD &&
        (MPIDI_IPCI_is_repeat_addr(bounds_base) < 1)) {
        goto fn_exit;
    }

    ipc_attr->ipc_type = MPIDI_IPCI_TYPE__GPU;
    if (remote_rank != MPI_PROC_NULL) {
        remote_rank = MPIDI_GPUI_global.local_ranks[MPIDIU_get_grank(remote_rank, comm)];
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
    int mpl_err;

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
    if (need_cache) {
        bool found = false;
        mpi_errno = ipc_track_cache_search(pbase, &handle, &found);
        MPIR_ERR_CHECK(mpi_errno);

        if (found) {
            if (MPL_gpu_ipc_handle_is_valid(&handle, pbase)) {
                handle_status = MPIDI_GPU_IPC_HANDLE_VALID;
                goto fn_done;
            } else {
                /* remove and destroy invalid handle */
                mpi_errno = ipc_track_cache_remove(pbase);
                MPIR_ERR_CHECK(mpi_errno);

                mpl_err = MPL_gpu_ipc_handle_destroy(pbase, &ipc_attr->u.gpu.gpu_attr);
                MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                                    "**gpu_ipc_handle_destroy");
            }
        }
    }

    mpl_err = MPL_gpu_ipc_handle_create(pbase, &ipc_attr->u.gpu.gpu_attr.device_attr, &handle);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                        "**gpu_ipc_handle_create");
    if (need_cache) {
        mpi_errno = ipc_track_cache_insert(pbase, handle);
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
    if (need_cache && handle.handle_status == MPIDI_GPU_IPC_HANDLE_REMAP_REQUIRED) {
        mpi_errno = ipc_mapped_cache_delete((void *) handle.remote_base_addr, handle.node_rank);
        MPIR_ERR_CHECK(mpi_errno);
    }

    void *pbase = NULL;
    if (need_cache) {
        mpi_errno =
            ipc_mapped_cache_search((void *) handle.remote_base_addr, handle.node_rank, map_dev_id,
                                    &pbase);
        MPIR_ERR_CHECK(mpi_errno);

        if (pbase) {
            /* found in cache */
            *vaddr = (void *) ((char *) pbase + handle.offset);
            goto fn_exit;
        }
    }

    if (do_mmap) {
#ifdef MPL_HAVE_ZE
        int mpl_err =
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
        mpi_errno =
            ipc_mapped_cache_insert((void *) handle.remote_base_addr, handle.node_rank, map_dev_id,
                                    pbase);
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

    if (MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE == MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE_disabled ||
        (MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE == MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE_specialized &&
         MPIR_CVAR_CH4_IPC_GPU_MAX_CACHE_ENTRIES == 0)) {
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

        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI_LOCK(vci));
        err = MPIDI_GPU_ipc_handle_unmap(p->src_buf, p->gpu_handle, 0);
        MPIR_Assertp(err == MPI_SUCCESS);
        err = MPIDI_IPC_complete(p->rreq, MPIDI_IPCI_TYPE__GPU);
        MPIR_Assertp(err == MPI_SUCCESS);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI_LOCK(vci));

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
    bool do_mmap = (src_data_sz <= MPIR_CVAR_GPU_FAST_COPY_MAX_SIZE);
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
        src_dt = MPIR_BYTE_INTERNAL;
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

int MPIDI_GPU_send_complete(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE == MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE_disabled ||
        (MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE == MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE_specialized &&
         MPIR_CVAR_CH4_IPC_GPU_MAX_CACHE_ENTRIES == 0)) {
        void *pbase = MPIDI_SHM_REQUEST(sreq, ipc.gpu_attr.bounds_base);
        MPL_pointer_attr_t *gpu_attr = &MPIDI_SHM_REQUEST(sreq, ipc.gpu_attr.gpu_attr);
        int mpl_err = MPL_gpu_ipc_handle_destroy(pbase, gpu_attr);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**gpu_ipc_handle_destroy");
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif /* MPIDI_CH4_SHM_ENABLE_GPU */
