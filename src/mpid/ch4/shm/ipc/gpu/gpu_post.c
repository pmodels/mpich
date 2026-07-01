/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===
cvars:
    - name        : MPIR_CVAR_CH4_IPC_GPU_CACHE_SIZE
      category    : CH4
      type        : int
      default     : 16
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        The maximum number of entries to hold in the cache containing IPC mapped buffers.
        When an entry is evicted, the corresponding IPC handle is closed. If MPIR_CVAR_CH4_IPC_GPU_CACHE_SIZE
        is 0, IPC handle caching is disabled.

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

/* IPC GPU handle/map caching:
 *   * NVIDIA CUDA sender side handle creation is cheap, but receive side mapping is expensive.
         * if we don't unmap freed handles, new handle may overlap the address, and new map will fail.
     * Intel ZE handle is actually file handles. They are bound to the allocation.
         * there won't be stale handle or map issue, since the memory won't be freed if one is active.
         * But unlimited active entries will cause apparent memory leak to the users and eventually
           exhaust the memory.

    * Having two independent caching for both sender-side handles and receiver-side mapping is a mess.
      When both side cache drift, we either create stale address issue or inefficient memory horading.
    * Solution: only cache on the sender side, but the cache need store the remote mapped address as well.
        * Cache miss: IPC send handle, receiver map, tell sender its mapped address via AM.
        * Cache hit: IPC send mapped address directly.
        * Cache invalidate: send AM to each mapped rank to unmap.
 */

#define IPC_HANDLE_CACHE_MAX 1024

struct map_entry {
    int remote_rank;
    void *mapped_addr;
};

struct handle_cache_entry {
    const void *base_addr;
    MPI_Aint len;

    bool in_use;
    unsigned long long usage_stamp;

    MPL_gpu_ipc_mem_handle_t handle;
    /* a dynamic array for remote mapped addrs */
    int num_maps;
    int max_maps;
    struct map_entry *maps;
    struct map_entry static_maps[1];
};

/* We may need send AM messages as we evict cache entries. Wrap the message context in a struct to
   keep the interface clean */
struct am_context {
    MPIR_Comm *comm;
    int local_vci;
    int remote_vci;
};

static struct am_context default_am_ctx = { NULL, 0, 0 };

static struct handle_cache_entry ipc_handle_cache[IPC_HANDLE_CACHE_MAX];
static int ipc_handle_cache_count = 0;

static unsigned long long ipc_handle_cache_usage_counter;       /* for tracking LRU (Least Recently Used) */

/* ipc handle cache utilities */
static int ipc_track_cache_free(int idx, struct am_context am_ctx);
static int ipc_track_cache_delete(int idx, struct am_context am_ctx);
static struct handle_cache_entry *ipc_track_cache_search(const void *addr, MPI_Aint len,
                                                         struct am_context am_ctx);
static int ipc_track_cache_check_limit(struct am_context am_ctx);
static int ipc_track_cache_insert(const void *addr, MPI_Aint len,
                                  MPL_gpu_ipc_mem_handle_t handle,
                                  struct am_context am_ctx, bool * cache_inserted);
static int ipc_track_cache_map_addr(const void *addr, const void *map_addr, int rank);

/* free the cache entry without updating ipc_handle_cache array */
static int ipc_track_cache_free(int idx, struct am_context am_ctx)
{
    int mpi_errno = MPI_SUCCESS;

    struct handle_cache_entry *entry = &ipc_handle_cache[idx];

    MPL_gpu_ipc_handle_destroy(entry->base_addr);

    if (entry->num_maps > 0) {
        for (int i = 0; i < entry->num_maps; i++) {
            mpi_errno = MPIDI_IPC_send_unmap(MPIR_Process.comm_world, entry->maps[i].remote_rank,
                                             am_ctx.local_vci, am_ctx.remote_vci,
                                             MPIDI_IPCI_TYPE__GPU, entry->maps[i].mapped_addr);
            if (am_ctx.comm == NULL) {
                /* ignore the error in case the remote process already exit */
                mpi_errno = MPI_SUCCESS;
            } else {
                MPIR_ERR_CHECK(mpi_errno);
            }
        }
    }

    if (entry->num_maps > 1) {
        MPL_free(entry->maps);
    }

    /* The caller will take care of shifting ipc_handle_cache and updating ipc_handle_cache_count */

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* free the cache entry and shift ipc_handle_cache array */
static int ipc_track_cache_delete(int idx, struct am_context am_ctx)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(idx < ipc_handle_cache_count);

    mpi_errno = ipc_track_cache_free(idx, am_ctx);
    MPIR_ERR_CHECK(mpi_errno);
    if (ipc_handle_cache_count - 1 > idx) {
        memmove(ipc_handle_cache + idx, ipc_handle_cache + idx + 1,
                (ipc_handle_cache_count - idx - 1) * sizeof(ipc_handle_cache[0]));
    }
    ipc_handle_cache_count -= 1;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static struct handle_cache_entry *ipc_track_cache_search(const void *addr, MPI_Aint len,
                                                         struct am_context am_ctx)
{
    struct handle_cache_entry *found = NULL;

    for (int i = 0; i < ipc_handle_cache_count; i++) {
        struct handle_cache_entry *entry = &ipc_handle_cache[i];
        if (entry->base_addr == addr && MPL_gpu_ipc_handle_is_valid(&entry->handle, (void *) addr)) {
            found = entry;
            goto fn_exit;
        }
        /* check potential stale entry.
         * Overlap condition between [a1, b1) and [a2, b2) is a1<b2 && b1>a2 */
        if ((uintptr_t) entry->base_addr < (uintptr_t) addr + len &&
            (uintptr_t) entry->base_addr + entry->len > (uintptr_t) addr) {
            ipc_track_cache_delete(i, am_ctx);
            i--;        /* the cache array has been shifted up */
            continue;
        }
    }

  fn_exit:
    if (found) {
        /* update usage_stamp for LRU */
        found->usage_stamp = ++ipc_handle_cache_usage_counter;
    }
    return found;
}

static int ipc_track_cache_map_addr(const void *addr, const void *map_addr, int rank)
{
    for (int i = 0; i < ipc_handle_cache_count; i++) {
        struct handle_cache_entry *entry = &ipc_handle_cache[i];
        if (entry->base_addr == addr) {
            if (entry->num_maps == 0) {
                entry->maps = entry->static_maps;
                entry->maps[0].remote_rank = rank;
                entry->maps[0].mapped_addr = (void *) map_addr;
                entry->num_maps = 1;
            } else {
                if (entry->num_maps == 1) {
                    entry->max_maps = 10;
                    entry->maps = MPL_malloc(sizeof(entry->maps[0]) * entry->max_maps,
                                             MPL_MEM_OTHER);
                    entry->maps[0] = entry->static_maps[0];
                } else if (entry->num_maps == entry->max_maps) {
                    entry->max_maps *= 2;
                    entry->maps = MPL_realloc(entry->maps,
                                              sizeof(entry->maps[0]) * entry->max_maps,
                                              MPL_MEM_OTHER);
                }
                entry->maps[entry->num_maps].remote_rank = rank;
                entry->maps[entry->num_maps].mapped_addr = (void *) map_addr;
                entry->num_maps++;
            }
            return MPI_SUCCESS;
        }
    }
    /* entry must exist when we receive a mapaddr AM */
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static int ipc_track_cache_check_limit(struct am_context am_ctx)
{
    int mpi_errno = MPI_SUCCESS;

    int cache_limit = MPL_MIN(MPIR_CVAR_CH4_IPC_GPU_CACHE_SIZE, IPC_HANDLE_CACHE_MAX);
    if (ipc_handle_cache_count >= cache_limit) {
        /* find LRU slot */
        int min_idx = -1;
        unsigned long long min_stamp;
        for (int i = 0; i < ipc_handle_cache_count; i++) {
            if (ipc_handle_cache[i].in_use) {
                continue;
            }
            if (min_idx == -1 || ipc_handle_cache[i].usage_stamp < min_stamp) {
                min_stamp = ipc_handle_cache[i].usage_stamp;
                min_idx = i;
            }
        }

        if (min_idx != -1) {
            mpi_errno = ipc_track_cache_delete(min_idx, am_ctx);
        }
    }

    return mpi_errno;
}

static int ipc_track_cache_insert(const void *addr, MPI_Aint len,
                                  MPL_gpu_ipc_mem_handle_t handle,
                                  struct am_context am_ctx, bool * cache_inserted)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = ipc_track_cache_check_limit(am_ctx);
    MPIR_ERR_CHECK(mpi_errno);

    int cache_limit = MPL_MIN(MPIR_CVAR_CH4_IPC_GPU_CACHE_SIZE, IPC_HANDLE_CACHE_MAX);
    if (ipc_handle_cache_count < cache_limit) {
        struct handle_cache_entry *entry = &ipc_handle_cache[ipc_handle_cache_count];
        memset(entry, 0, sizeof(*entry));
        entry->base_addr = addr;
        entry->len = len;
        entry->handle = handle;
        entry->usage_stamp = ++ipc_handle_cache_usage_counter;
        ipc_handle_cache_count++;

        *cache_inserted = true;
    } else {
        *cache_inserted = false;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Optionally we may install a free_hook to more efficiently invalidate entries */
void MPIDI_GPU_handle_free_hook(void *dptr)
{
    for (int i = 0; i < ipc_handle_cache_count; i++) {
        struct handle_cache_entry *entry = &ipc_handle_cache[i];
        if (entry->base_addr == dptr) {
            MPIR_Assert(!entry->in_use);
            ipc_track_cache_delete(i, default_am_ctx);
            break;
        }
    }
}

int MPIDI_GPU_ipc_cache_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    for (int i = 0; i < ipc_handle_cache_count; i++) {
        MPIR_Assert(!ipc_handle_cache[i].in_use);
        mpi_errno = ipc_track_cache_free(i, default_am_ctx);
        MPIR_ERR_CHECK(mpi_errno);
    }
    ipc_handle_cache_count = 0;

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
        remote_rank = MPIDI_SHM_global.local_ranks[MPIDIU_get_grank(remote_rank, comm)];
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

/* Non-cached version, used by ipc_win.c and posix_coll_gpu_ipc.h */
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

    MPL_gpu_ipc_mem_handle_t handle;
    mpl_err = MPL_gpu_ipc_handle_create(pbase, &ipc_attr->u.gpu.gpu_attr.device_attr, &handle);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                        "**gpu_ipc_handle_create");

    ipc_handle->gpu.ipc_handle = handle;
    ipc_handle->gpu.global_dev_id = global_dev_id;
    ipc_handle->gpu.local_dev_id = local_dev_id;
    ipc_handle->gpu.remote_base_addr = (uintptr_t) pbase;
    ipc_handle->gpu.len = len;
    ipc_handle->gpu.node_rank = MPIR_Process.local_rank;
    ipc_handle->gpu.offset = (uintptr_t) ipc_attr->u.gpu.vaddr - (uintptr_t) pbase;
    ipc_handle->gpu.handle_is_cached = false;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Cached version, used by p2p path. If a mapped address is cached for the remote rank,
 * ipc_attr->ipc_type is overwritten to MPIDI_IPCI_TYPE__DIRECT and ipc_handle is set
 * to the mapped address. */
int MPIDI_GPU_fill_ipc_handle_cache(MPIDI_IPCI_ipc_attr_t * ipc_attr,
                                    MPIDI_IPCI_ipc_handle_t * ipc_handle, MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    bool is_cached = true;

    void *pbase = ipc_attr->u.gpu.bounds_base;
    MPI_Aint len = ipc_attr->u.gpu.bounds_len;
    int remote_rank = ipc_attr->u.gpu.remote_rank;
    struct am_context ctx =
        { req->comm, MPIDIG_REQUEST(req, req->local_vci), MPIDIG_REQUEST(req, req->remote_vci) };

    struct handle_cache_entry *entry = ipc_track_cache_search(pbase, len, ctx);
    if (entry) {
        /* check if we have a cached mapped address for the remote rank */
        for (int i = 0; i < entry->num_maps; i++) {
            if (entry->maps[i].remote_rank == remote_rank) {
                uintptr_t offset = (uintptr_t) ipc_attr->u.gpu.vaddr - (uintptr_t) pbase;
                ipc_attr->ipc_type = MPIDI_IPCI_TYPE__DIRECT;
                ipc_handle->direct = (void *) ((uintptr_t) entry->maps[i].mapped_addr + offset);
                goto fn_done;
            }
        }

        /* cache hit but no mapped addr yet, fill handle from cache */
        mpi_errno = MPIDI_GPU_fill_ipc_handle(ipc_attr, ipc_handle);
        MPIR_ERR_CHECK(mpi_errno);
        ipc_handle->gpu.ipc_handle = entry->handle;
        goto fn_done;
    }

    /* cache miss, create handle and insert into cache */
    mpi_errno = MPIDI_GPU_fill_ipc_handle(ipc_attr, ipc_handle);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = ipc_track_cache_insert(pbase, len, ipc_handle->gpu.ipc_handle, ctx, &is_cached);
    MPIR_ERR_CHECK(mpi_errno);


  fn_done:
    if (req) {
        /* store base_addr in case we need free the handle at completion */
        MPIDI_SHM_REQUEST(req, ipc.base_addr) = is_cached ? NULL : pbase;
    }
    if (ipc_attr->ipc_type != MPIDI_IPCI_TYPE__DIRECT) {
        ipc_handle->gpu.handle_is_cached = is_cached;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_GPU_ipc_cache_map_addr(void *base_addr, void *map_addr, int rank)
{
    return ipc_track_cache_map_addr(base_addr, map_addr, rank);
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

    void *pbase = NULL;
    if (do_mmap) {
#ifdef MPL_HAVE_ZE
        int mpl_err = MPL_ze_ipc_handle_mmap_host(&handle.ipc_handle, 1, map_dev_id, handle.len,
                                                  &pbase);
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

    *vaddr = (void *) ((uintptr_t) pbase + handle.offset);
#undef MAPPED_TREE

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* this is used by noncached paths, e.g. window */
int MPIDI_GPU_ipc_handle_unmap(void *vaddr, MPIDI_GPU_ipc_handle_t handle, int do_mmap)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int mpl_err = MPL_SUCCESS;
    mpl_err = MPL_gpu_ipc_handle_unmap((void *) ((uintptr_t) vaddr - handle.offset));
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gpu_ipc_handle_unmap");

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* origin process (sender) tell us via AM to unmap, we just unmap */
int MPIDI_GPU_ipc_cache_unmap(void *mapped_addr)
{
    int mpi_errno = MPI_SUCCESS;

    int mpl_err = MPL_SUCCESS;
    mpl_err = MPL_gpu_ipc_handle_unmap(mapped_addr);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gpu_ipc_handle_unmap");

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* nonblocking IPCI_copy_data via MPIX_Async */
struct gpu_ipc_async {
    MPIR_Request *req;
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
    MPIR_async_test(&(p->yreq), &is_done);

    if (is_done) {
        int vci = MPIDIG_REQUEST(p->req, req->local_vci);

        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI_LOCK(vci));
        err = MPIDI_IPC_complete(p->req, MPIDI_IPCI_TYPE__GPU);
        MPIR_Assertp(err == MPI_SUCCESS);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI_LOCK(vci));

        MPL_free(p);
        return MPIX_ASYNC_DONE;
    }

    return MPIX_ASYNC_NOPROGRESS;
}

static int gpu_ipc_async_start(MPIR_Request * req, MPIR_gpu_req * req_p,
                               void *src_buf, MPIDI_GPU_ipc_handle_t gpu_handle)
{
    int mpi_errno = MPI_SUCCESS;

    struct gpu_ipc_async *p;
    p = MPL_malloc(sizeof(*p), MPL_MEM_OTHER);
    p->req = req;
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

int MPIDI_GPU_copy_data_async(MPIDI_IPC_hdr * ipc_hdr, MPIR_Request * req, MPI_Aint src_data_sz)
{
    int mpi_errno = MPI_SUCCESS;

    void *src_buf = NULL;
    MPL_pointer_attr_t attr;
    int dev_id;
    MPIR_GPU_query_pointer_attr(MPIDIG_REQUEST(req, buffer), &attr);
    dev_id = MPL_gpu_get_dev_id_from_attr(&attr);

    if (ipc_hdr->ipc_type == MPIDI_IPCI_TYPE__DIRECT) {
        src_buf = ipc_hdr->ipc_handle.direct;
    } else {
#ifdef MPL_HAVE_ZE
        bool do_mmap = (src_data_sz <= MPIR_CVAR_GPU_FAST_COPY_MAX_SIZE);
#else
        bool do_mmap = false;
#endif
        int map_dev = MPIDI_GPU_ipc_get_map_dev(ipc_hdr->ipc_handle.gpu.global_dev_id, dev_id,
                                                MPIDIG_REQUEST(req, datatype));
        mpi_errno = MPIDI_GPU_ipc_handle_map(ipc_hdr->ipc_handle.gpu, map_dev, &src_buf, do_mmap);
        MPIR_ERR_CHECK(mpi_errno);

        void *mapped_base = (void *) ((uintptr_t) src_buf - ipc_hdr->ipc_handle.gpu.offset);
        if (ipc_hdr->ipc_handle.gpu.handle_is_cached) {
            /* notify sender of mapped address so it can use DIRECT path next time */
            mpi_errno = MPIDI_IPC_send_mapaddr(req->comm, MPIDIG_REQUEST(req, u.ipc.peer_rank),
                                               MPIDIG_REQUEST(req, req->local_vci),
                                               MPIDIG_REQUEST(req, req->remote_vci),
                                               MPIDI_IPCI_TYPE__GPU,
                                               (void *) ipc_hdr->ipc_handle.gpu.remote_base_addr,
                                               mapped_base);
            MPIR_ERR_CHECK(mpi_errno);
            MPIDI_SHM_REQUEST(req, ipc.base_addr) = NULL;
        } else {
            MPIDI_SHM_REQUEST(req, ipc.base_addr) = mapped_base;
        }
    }

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
        mpi_errno = MPIR_Typerep_unflatten(&src_dt_ptr, flattened_type);
        MPIR_ERR_CHECK(mpi_errno);

        src_count = ipc_hdr->count;
        src_dt = src_dt_ptr->handle;
    }
    MPIDIG_REQUEST(req, u.ipc.src_dt_ptr) = src_dt_ptr;

    MPIR_gpu_req yreq;
    MPL_gpu_engine_type_t engine =
        MPIDI_IPCI_choose_engine(ipc_hdr->ipc_handle.gpu.global_dev_id, dev_id);
    mpi_errno = MPIR_Ilocalcopy_gpu(src_buf, src_count, src_dt, 0, NULL,
                                    MPIDIG_REQUEST(req, buffer), MPIDIG_REQUEST(req, count),
                                    MPIDIG_REQUEST(req, datatype), 0, &attr, engine, true, &yreq);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = gpu_ipc_async_start(req, &yreq, src_buf, ipc_hdr->ipc_handle.gpu);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_GPU_write_data_async(MPIDI_IPC_hdr * ipc_hdr, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    void *src_buf = MPIDIG_REQUEST(sreq, buffer);
    MPI_Aint src_count = MPIDIG_REQUEST(sreq, count);
    MPI_Datatype src_datatype = MPIDIG_REQUEST(sreq, datatype);

    MPI_Aint src_data_sz;
    MPIR_Datatype_get_size_macro(src_datatype, src_data_sz);
    src_data_sz *= src_count;

    /* map remote ipc buffer */
    void *dst_buf;
    MPL_pointer_attr_t attr;
    int dev_id = -1;
    if (ipc_hdr->ipc_type == MPIDI_IPCI_TYPE__DIRECT) {
        dst_buf = ipc_hdr->ipc_handle.direct;
    } else {
#ifdef MPL_HAVE_ZE
        bool do_mmap = (src_data_sz <= MPIR_CVAR_GPU_FAST_COPY_MAX_SIZE);
#else
        bool do_mmap = false;
#endif
        MPIR_GPU_query_pointer_attr(src_buf, &attr);
        dev_id = MPL_gpu_get_dev_id_from_attr(&attr);
        int map_dev = MPIDI_GPU_ipc_get_map_dev(ipc_hdr->ipc_handle.gpu.global_dev_id, dev_id,
                                                src_datatype);
        mpi_errno = MPIDI_GPU_ipc_handle_map(ipc_hdr->ipc_handle.gpu, map_dev, &dst_buf, do_mmap);
        MPIR_ERR_CHECK(mpi_errno);
        /* notify sender of mapped address so it can use DIRECT path next time */
        void *mapped_base = (void *) ((uintptr_t) dst_buf - ipc_hdr->ipc_handle.gpu.offset);
        mpi_errno = MPIDI_IPC_send_mapaddr(sreq->comm, MPIDIG_REQUEST(sreq, u.ipc.peer_rank),
                                           MPIDIG_REQUEST(sreq, req->local_vci),
                                           MPIDIG_REQUEST(sreq, req->remote_vci),
                                           MPIDI_IPCI_TYPE__GPU,
                                           (void *) ipc_hdr->ipc_handle.gpu.remote_base_addr,
                                           mapped_base);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* retrieve remote count and datatype  */
    MPI_Aint dst_count;
    MPI_Datatype dst_datatype;
    if (ipc_hdr->is_contig) {
        dst_count = ipc_hdr->count;
        dst_datatype = MPIR_BYTE_INTERNAL;
    } else {
        /* TODO: get sender datatype and call MPIR_Typerep_op with mapped_device set to dev_id */
        void *flattened_type = ipc_hdr + 1;
        MPIR_Datatype *dt_ptr;
        mpi_errno = MPIR_Typerep_unflatten(&dt_ptr, flattened_type);
        MPIR_ERR_CHECK(mpi_errno);

        dst_count = ipc_hdr->count;
        dst_datatype = dt_ptr->handle;
        /* remember the flattened type so we can free it later */
        MPIDIG_REQUEST(sreq, u.ipc.src_dt_ptr) = dt_ptr;
    }

    /* copy */
    MPIR_gpu_req yreq;
    MPL_gpu_engine_type_t engine =
        MPIDI_IPCI_choose_engine(ipc_hdr->ipc_handle.gpu.global_dev_id, dev_id);
    mpi_errno = MPIR_Ilocalcopy_gpu(src_buf, src_count, src_datatype, 0, &attr,
                                    dst_buf, dst_count, dst_datatype, 0, NULL, engine, true, &yreq);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = gpu_ipc_async_start(sreq, &yreq, dst_buf, ipc_hdr->ipc_handle.gpu);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* The following two are hooks at IPC completions, at handle creation side and mapping side, respectively.
 * The clean up only happens if the handle is not cached, e.g. cache size is 0.
 */

int MPIDI_GPU_ipc_handle_complete(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    void *pbase = MPIDI_SHM_REQUEST(req, ipc.base_addr);
    if (pbase) {
        int mpl_err = MPL_gpu_ipc_handle_destroy(pbase);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**gpu_ipc_handle_destroy");
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_GPU_ipc_map_complete(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    void *pbase = MPIDI_SHM_REQUEST(req, ipc.base_addr);
    if (pbase) {
        int mpl_err = MPL_gpu_ipc_handle_unmap(pbase);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**gpu_ipc_handle_unmap");
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* MPIDI_CH4_SHM_ENABLE_GPU */
