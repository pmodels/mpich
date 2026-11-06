/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===
cvars:
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
#include "ipc_p2p.h"
#include "mpir_async_things.h"
#include "mpidu_gpu_cache.h"

#ifdef MPIDI_CH4_SHM_ENABLE_GPU

/* IPC GPU handle/map caching:
 *   * The IPC handles of local buffers are cached in MPIDU_gpu_cache (see mpidu_gpu_cache.c).

    * Having two independent caching for both sender-side handles and receiver-side mapping is a mess.
      When both side cache drift, we either create stale address issue or inefficient memory horading.
    * Solution: only cache on the sender side, but the cache need store the remote mapped address as well.
        * Cache miss: IPC send handle, receiver map, tell sender its mapped address via AM.
        * Cache hit: IPC send mapped address directly.
        * Cache invalidate: send AM to each mapped rank to unmap.
      The remote mapped addresses are kept in the MPIDU_gpu_cache entry (entry->ext), and are
      unmapped by ipc_remote_maps_free when the entry is dropped.
 */

#define IPC_STATIC_MAPS_SIZE 1

struct map_entry {
    int remote_lrank;
    MPL_gpu_map_t map;
};

/* entry->ext of a MPIDU_gpu_cache entry */
struct ipc_remote_maps {
    /* a dynamic array for remote mapped addrs */
    int num_maps;
    int max_maps;
    struct map_entry *maps;
    struct map_entry static_maps[IPC_STATIC_MAPS_SIZE];
};

/* Cache for remote IPC mappings. Stores the IPC handle so we can efficiently
 * mmap to host when needed for fast_memcpy, without re-exporting through the driver. */
#ifdef MPL_HAVE_ZE
#include "uthash.h"

struct ipc_map_cache {
    void *dev_addr;             /* device base address (hash key) */
    MPL_gpu_ipc_mem_handle_t ipc_handle;        /* saved IPC handle for mmap */
    size_t len;                 /* allocation length */
    int map_dev;                /* device id for mapping */
    MPL_gpu_map_t mmap;         /* mmap result, mapped_addr is NULL until needed */
    UT_hash_handle hh;
};

struct ipc_map_cache *ipc_map_cache_table = NULL;

/* Insert a cache entry after the first (non-DIRECT) IPC mapping */
static void ipc_map_cache_insert(void *dev_addr, MPL_gpu_ipc_mem_handle_t * ipc_handle,
                                 size_t len, int map_dev)
{
    struct ipc_map_cache *entry;
    HASH_FIND_PTR(ipc_map_cache_table, &dev_addr, entry);
    if (!entry) {
        entry = MPL_malloc(sizeof(struct ipc_map_cache), MPL_MEM_OTHER);
        entry->dev_addr = dev_addr;
        entry->ipc_handle = *ipc_handle;
        entry->len = len;
        entry->map_dev = map_dev;
        memset(&entry->mmap, 0, sizeof(MPL_gpu_map_t));
        HASH_ADD_PTR(ipc_map_cache_table, dev_addr, entry, MPL_MEM_OTHER);
    }
}

static int ipc_map_cache_remove(void *dev_addr)
{
    struct ipc_map_cache *entry;
    HASH_FIND_PTR(ipc_map_cache_table, &dev_addr, entry);
    if (entry) {
        if (entry->mmap.mapped_addr) {
            int mpl_err = MPL_gpu_ipc_handle_unmap(&entry->mmap);
            MPIR_Assertp(mpl_err == MPL_SUCCESS);
        }
        HASH_DEL(ipc_map_cache_table, entry);
        MPL_free(entry);
    }
    return MPI_SUCCESS;
}

static void *ipc_map_cache_get_mmap(void *dev_addr)
{
    struct ipc_map_cache *entry;
    HASH_FIND_PTR(ipc_map_cache_table, &dev_addr, entry);
    MPIR_Assert(entry);

    if (!entry->mmap.mapped_addr) {
        int mpl_err = MPL_gpu_ipc_handle_map(&entry->ipc_handle, entry->map_dev,
                                             &entry->mmap, true, entry->len);
        MPIR_Assertp(mpl_err == MPL_SUCCESS);
    }
    return entry->mmap.mapped_addr;
}

#else /* !MPL_HAVE_ZE */
static void ipc_map_cache_insert(void *dev_addr, MPL_gpu_ipc_mem_handle_t * ipc_handle,
                                 size_t len, int map_dev)
{
}

static int ipc_map_cache_remove(void *dev_addr)
{
    return MPI_SUCCESS;
}

static void *ipc_map_cache_get_mmap(void *dev_addr)
{
    return NULL;
}
#endif /* MPL_HAVE_ZE */

/* We may need send AM messages as we evict cache entries. Wrap the message context in a struct to
   keep the interface clean */
struct am_context {
    MPIR_Comm *comm;
    int local_vci;
    int remote_vci;
};

static struct am_context default_am_ctx = { NULL, 0, 0 };

static struct map_entry *ipc_remote_maps_get(struct MPIDU_gpu_cache_entry *entry,
                                             int *num_maps_out);
static int ipc_remote_maps_free(struct MPIDU_gpu_cache_entry *entry, void *ctx);
static int ipc_track_cache_map_addr(const void *addr, MPL_gpu_map_t map, int lrank);

int MPIDI_GPU_ipc_cache_init(void)
{
    MPIDU_gpu_cache_set_ext_free(ipc_remote_maps_free);
    return MPI_SUCCESS;
}

/* the mapped addresses of local peers for a cached buffer */
static struct map_entry *ipc_remote_maps_get(struct MPIDU_gpu_cache_entry *entry, int *num_maps_out)
{
    struct ipc_remote_maps *rmaps = entry->ext;
    if (!rmaps) {
        *num_maps_out = 0;
        return NULL;
    }
    *num_maps_out = rmaps->num_maps;
    return (rmaps->num_maps > IPC_STATIC_MAPS_SIZE) ? rmaps->maps : rmaps->static_maps;
}

/* called when MPIDU_gpu_cache drops an entry: tell each mapped rank to unmap.
 * ctx is the struct am_context of the caller, or NULL for default_am_ctx. */
static int ipc_remote_maps_free(struct MPIDU_gpu_cache_entry *entry, void *ctx)
{
    int mpi_errno = MPI_SUCCESS;

    struct ipc_remote_maps *rmaps = entry->ext;
    if (!rmaps) {
        goto fn_exit;
    }

    struct am_context am_ctx = ctx ? *(struct am_context *) ctx : default_am_ctx;
    int num_maps;
    struct map_entry *maps = ipc_remote_maps_get(entry, &num_maps);
    for (int i = 0; i < num_maps; i++) {
        int grank = MPIR_Process.node_local_map[maps[i].remote_lrank];
        mpi_errno = MPIDI_IPC_send_unmap(NULL, grank, am_ctx.local_vci, am_ctx.remote_vci,
                                         MPIDI_IPCI_TYPE__GPU, maps[i].map);
        if (am_ctx.comm == NULL) {
            /* ignore the error in case the remote process already exit */
            mpi_errno = MPI_SUCCESS;
        } else if (mpi_errno) {
            break;
        }
    }

    if (rmaps->num_maps > IPC_STATIC_MAPS_SIZE) {
        MPL_free(rmaps->maps);
    }
    MPL_free(rmaps);
    entry->ext = NULL;

  fn_exit:
    return mpi_errno;
}

static int ipc_track_cache_map_addr(const void *addr, MPL_gpu_map_t map, int lrank)
{
    struct MPIDU_gpu_cache_entry *entry = MPIDU_gpu_cache_find(addr);
    /* entry must exist when we receive a mapaddr AM */
    MPIR_Assert(entry);

    struct ipc_remote_maps *rmaps = entry->ext;
    if (!rmaps) {
        rmaps = MPL_calloc(1, sizeof(*rmaps), MPL_MEM_OTHER);
        MPIR_Assert(rmaps);
        entry->ext = rmaps;
    }

    struct map_entry *maps;
    if (rmaps->num_maps < IPC_STATIC_MAPS_SIZE) {
        maps = rmaps->static_maps;
    } else {
        if (rmaps->num_maps == IPC_STATIC_MAPS_SIZE) {
            /* switch from static maps to dynamic maps */
            rmaps->max_maps = IPC_STATIC_MAPS_SIZE + 10;
            rmaps->maps = MPL_malloc(sizeof(rmaps->maps[0]) * rmaps->max_maps, MPL_MEM_OTHER);
            for (int j = 0; j < rmaps->num_maps; j++) {
                rmaps->maps[j] = rmaps->static_maps[j];
            }
        } else if (rmaps->num_maps == rmaps->max_maps) {
            /* need grow the dynamic maps */
            rmaps->max_maps *= 2;
            rmaps->maps = MPL_realloc(rmaps->maps, sizeof(rmaps->maps[0]) * rmaps->max_maps,
                                      MPL_MEM_OTHER);
        }
        MPIR_Assert(rmaps->maps);
        maps = rmaps->maps;
    }
    maps[rmaps->num_maps].remote_lrank = lrank;
    maps[rmaps->num_maps].map = map;
    rmaps->num_maps++;

    return MPI_SUCCESS;
}

int MPIDI_GPU_get_ipc_attr(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                           MPIDI_IPCI_ipc_attr_t * ipc_attr)
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
    if (!MPL_gpu_attr_is_host(&ipc_attr->u.gpu.gpu_attr)) {
        /* if it's not host memory, we cannot do XPMEM or CMA IPC, so set default to SKIP */
        ipc_attr->ipc_type = MPIDI_IPCI_TYPE__SKIP;
    }
    if (!MPL_gpu_attr_is_dev(&ipc_attr->u.gpu.gpu_attr)) {
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

static int fill_ipc_handle(MPIDI_IPCI_ipc_attr_t * ipc_attr,
                           MPL_gpu_ipc_mem_handle_t * handle_ptr,
                           MPIDI_IPCI_ipc_handle_t * ipc_handle)
{
    int mpi_errno = MPI_SUCCESS;

    int local_dev_id, global_dev_id;
    local_dev_id = MPL_gpu_get_dev_id_from_attr(&ipc_attr->u.gpu.gpu_attr);
    global_dev_id = MPL_gpu_local_to_global_dev_id(local_dev_id);

    void *pbase = ipc_attr->u.gpu.bounds_base;
    MPI_Aint len = ipc_attr->u.gpu.bounds_len;

    ipc_handle->gpu.ipc_handle = *handle_ptr;
    ipc_handle->gpu.global_dev_id = global_dev_id;
    ipc_handle->gpu.local_dev_id = local_dev_id;
    ipc_handle->gpu.remote_base_addr = (uintptr_t) pbase;
    ipc_handle->gpu.len = len;
    ipc_handle->gpu.node_rank = MPIR_Process.local_rank;
    ipc_handle->gpu.offset = (uintptr_t) ipc_attr->u.gpu.vaddr - (uintptr_t) pbase;
    ipc_handle->gpu.handle_is_cached = false;

    return mpi_errno;
}

/* NOTE: currently only ZE drmfd path require handle-destroy. All other paths are effectively noop */
int MPIDI_GPU_handle_destroy(void *handle)
{
    int mpi_errno = MPI_SUCCESS;

    if (handle) {
        int mpl_err = MPL_gpu_ipc_handle_destroy((MPL_gpu_ipc_mem_handle_t *) handle);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**gpu_ipc_handle_destroy");

        MPL_free(handle);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

/* Non-cached version, used by ipc_win.c and posix_coll_gpu_ipc.h */
int MPIDI_GPU_fill_ipc_handle(MPIDI_IPCI_ipc_attr_t * ipc_attr,
                              MPIDI_IPCI_ipc_handle_t * ipc_handle, void **local_handle_out)
{
    int mpi_errno = MPI_SUCCESS;

    MPL_gpu_ipc_mem_handle_t *handle_ptr = NULL;
    handle_ptr = MPL_malloc(sizeof(MPL_gpu_ipc_mem_handle_t), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!handle_ptr, mpi_errno, MPI_ERR_OTHER, "**nomem");

    int mpl_err;
    mpl_err = MPL_gpu_ipc_handle_create(ipc_attr->u.gpu.bounds_base,
                                        &ipc_attr->u.gpu.gpu_attr.device_attr, handle_ptr);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                        "**gpu_ipc_handle_create");

    mpi_errno = fill_ipc_handle(ipc_attr, handle_ptr, ipc_handle);
    MPIR_ERR_CHECK(mpi_errno);

    if (MPL_gpu_info.ipc_handle_need_destroy) {
        *local_handle_out = (void *) handle_ptr;
    } else {
        *local_handle_out = NULL;
        MPL_free(handle_ptr);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    if (handle_ptr) {
        MPL_free(handle_ptr);
    }
    goto fn_exit;
}

/* Cached version, used by p2p path. If a mapped address is cached for the remote rank,
 * ipc_attr->ipc_type is overwritten to MPIDI_IPCI_TYPE__DIRECT and ipc_handle is set
 * to the mapped address. */
int MPIDI_GPU_fill_ipc_handle_cache(MPIDI_IPCI_ipc_attr_t * ipc_attr,
                                    MPIDI_IPCI_ipc_handle_t * ipc_handle,
                                    MPIR_Request * req, int remote_lrank)
{
    int mpi_errno = MPI_SUCCESS;
    MPL_gpu_ipc_mem_handle_t *handle_ptr = NULL;

    void *pbase = ipc_attr->u.gpu.bounds_base;
    MPI_Aint len = ipc_attr->u.gpu.bounds_len;
    struct am_context ctx =
        { req->comm, MPIDIG_REQUEST(req, req->local_vci), MPIDIG_REQUEST(req, req->remote_vci) };

    /* on a cache miss, the handle is created and inserted into the cache if there is room */
    struct MPIDU_gpu_cache_entry *entry = NULL;
    mpi_errno = MPIDU_gpu_cache_get_handle(pbase, len, &ipc_attr->u.gpu.gpu_attr.device_attr,
                                           &ctx, &entry, &handle_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    bool is_cached = (entry != NULL);
    if (is_cached) {
        /* check if we have a cached mapped address for the remote rank */
        int num_maps;
        struct map_entry *maps = ipc_remote_maps_get(entry, &num_maps);
        for (int i = 0; i < num_maps; i++) {
            if (maps[i].remote_lrank == remote_lrank) {
                ipc_attr->ipc_type = MPIDI_IPCI_TYPE__DIRECT;
                ipc_handle->direct.base_addr = maps[i].map.mapped_addr;
                ipc_handle->direct.offset = (uintptr_t) ipc_attr->u.gpu.vaddr - (uintptr_t) pbase;

                int local_dev_id = MPL_gpu_get_dev_id_from_attr(&ipc_attr->u.gpu.gpu_attr);
                ipc_handle->direct.global_dev_id = MPL_gpu_local_to_global_dev_id(local_dev_id);
                entry->range.in_use++;
                goto fn_done;
            }
        }
    }

    /* no mapped addr yet, fill the handle */
    mpi_errno = fill_ipc_handle(ipc_attr, handle_ptr, ipc_handle);
    MPIR_ERR_CHECK(mpi_errno);
    if (is_cached) {
        entry->range.in_use++;
    }

  fn_done:
    if (req) {
        MPIDI_SHM_REQUEST(req, ipc.u.handle.is_cached) = is_cached;
        MPIDI_SHM_REQUEST(req, ipc.u.handle.handle_ptr) = handle_ptr;
    }
    if (ipc_attr->ipc_type != MPIDI_IPCI_TYPE__DIRECT) {
        ipc_handle->gpu.handle_is_cached = is_cached;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    if (handle_ptr && !entry) {
        MPIDI_GPU_handle_destroy(handle_ptr);
    }
    goto fn_exit;
}

int MPIDI_GPU_ipc_cache_map_addr(void *base_addr, MPL_gpu_map_t map, int lrank)
{
    return ipc_track_cache_map_addr(base_addr, map, lrank);
}

/* Use the cached host mmap of a local device buffer for a copy that is small enough
 * for fast_memcpy. See MPIDU_gpu_cache_local_mmap. */
int MPIDI_GPU_ipc_local_mmap(void *dev_ptr, MPL_pointer_attr_t * attr,
                             MPI_Aint data_sz, void **host_ptr_out)
{
    if (data_sz > MPIR_CVAR_GPU_FAST_COPY_MAX_SIZE) {
        *host_ptr_out = dev_ptr;
        return MPI_SUCCESS;
    }
    return MPIDU_gpu_cache_local_mmap(dev_ptr, attr, host_ptr_out);
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

int MPIDI_GPU_ipc_handle_map_base(MPIDI_GPU_ipc_handle_t handle, int map_dev_id,
                                  MPL_gpu_map_t * map_out, bool do_mmap)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int mpl_err = MPL_gpu_ipc_handle_map(&handle.ipc_handle, map_dev_id,
                                         map_out, do_mmap, handle.len);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gpu_ipc_handle_map");

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* this is used by noncached paths, e.g. window */
int MPIDI_GPU_ipc_handle_unmap(MPL_gpu_map_t * map_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int mpl_err = MPL_gpu_ipc_handle_unmap(map_ptr);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gpu_ipc_handle_unmap");

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* origin process (sender) tell us via AM to unmap, we just unmap */
int MPIDI_GPU_ipc_handle_unmap_base(MPL_gpu_map_t * map_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    /* remove cached mmap (if any) before unmapping the device mapping */
    mpi_errno = ipc_map_cache_remove(map_ptr->mapped_addr);
    MPIR_ERR_CHECK(mpi_errno);

    int mpl_err = MPL_gpu_ipc_handle_unmap(map_ptr);
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

static int ipc_map_addr(MPIDI_IPC_hdr * ipc_hdr, MPIR_Request * req, MPI_Aint data_sz,
                        int dev_id, void **addr_out, MPL_pointer_attr_t * attr_out)
{
    int mpi_errno = MPI_SUCCESS;

    memset(&MPIDI_SHM_REQUEST(req, ipc.u.map), 0, sizeof(MPL_gpu_map_t));

    void *mapped_base;
    MPI_Aint offset;
    bool use_cache = true;
    if (ipc_hdr->ipc_type == MPIDI_IPCI_TYPE__DIRECT) {
        mapped_base = ipc_hdr->ipc_handle.direct.base_addr;
        offset = ipc_hdr->ipc_handle.direct.offset;
    } else {
        int map_dev = MPIDI_GPU_ipc_get_map_dev(ipc_hdr->ipc_handle.gpu.global_dev_id, dev_id,
                                                MPIDIG_REQUEST(req, datatype));
        MPL_gpu_map_t map;
        /* Always map to device address space. If we need fast_memcpy, we'll mmap separately. */
        mpi_errno = MPIDI_GPU_ipc_handle_map_base(ipc_hdr->ipc_handle.gpu, map_dev, &map, false);
        MPIR_ERR_CHECK(mpi_errno);

        if (ipc_hdr->ipc_handle.gpu.handle_is_cached) {
            /* Cache the IPC handle so DIRECT path can mmap from it later */
            ipc_map_cache_insert(map.mapped_addr, &ipc_hdr->ipc_handle.gpu.ipc_handle,
                                 ipc_hdr->ipc_handle.gpu.len, map_dev);

            /* notify sender of mapped address so it can use DIRECT path next time */
            mpi_errno = MPIDI_IPC_send_mapaddr(req->comm, MPIDIG_REQUEST(req, u.ipc.peer_rank),
                                               MPIDIG_REQUEST(req, req->local_vci),
                                               MPIDIG_REQUEST(req, req->remote_vci),
                                               MPIDI_IPCI_TYPE__GPU,
                                               (void *) ipc_hdr->ipc_handle.gpu.remote_base_addr,
                                               map);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            use_cache = false;
            MPIDI_SHM_REQUEST(req, ipc.u.map) = map;
        }

        mapped_base = map.mapped_addr;
        offset = ipc_hdr->ipc_handle.gpu.offset;
    }

    bool do_mmap;
#ifdef MPL_HAVE_ZE
    do_mmap = (data_sz <= MPIR_CVAR_GPU_FAST_COPY_MAX_SIZE);
#else
    do_mmap = false;
#endif

    if (do_mmap && use_cache) {
        void *mmap_base = ipc_map_cache_get_mmap(mapped_base);
        *addr_out = (void *) ((uintptr_t) mmap_base + offset);
        attr_out->type = MPL_GPU_POINTER_DEV_MMAP;
    } else {
        *addr_out = (void *) ((uintptr_t) mapped_base + offset);
        MPIR_GPU_query_pointer_attr(*addr_out, attr_out);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_GPU_copy_data_async(MPIDI_IPC_hdr * ipc_hdr, MPIR_Request * req, MPI_Aint src_data_sz)
{
    int mpi_errno = MPI_SUCCESS;

    void *src_buf = NULL;
    MPL_pointer_attr_t attr;
    int dev_id;
    MPIR_GPU_query_pointer_attr(MPIDIG_REQUEST(req, buffer), &attr);
    dev_id = MPL_gpu_get_dev_id_from_attr(&attr);

    MPL_pointer_attr_t src_attr = { 0 };
    mpi_errno = ipc_map_addr(ipc_hdr, req, src_data_sz, dev_id, &src_buf, &src_attr);
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
        mpi_errno = MPIR_Typerep_unflatten(&src_dt_ptr, flattened_type);
        MPIR_ERR_CHECK(mpi_errno);

        src_count = ipc_hdr->count;
        src_dt = src_dt_ptr->handle;
    }
    MPIDIG_REQUEST(req, u.ipc.src_dt_ptr) = src_dt_ptr;

    /* try to use cached local mmap for fast_memcpy */
    void *local_buf = MPIDIG_REQUEST(req, buffer);
    mpi_errno = MPIDI_GPU_ipc_local_mmap(local_buf, &attr, src_data_sz, &local_buf);
    MPIR_ERR_CHECK(mpi_errno);

    int remote_global_dev_id = (ipc_hdr->ipc_type == MPIDI_IPCI_TYPE__DIRECT) ?
        ipc_hdr->ipc_handle.direct.global_dev_id : ipc_hdr->ipc_handle.gpu.global_dev_id;

    MPIR_gpu_req yreq;
    MPL_gpu_engine_type_t engine = MPIDI_IPCI_choose_engine(remote_global_dev_id, dev_id);
    mpi_errno = MPIR_Ilocalcopy_gpu(src_buf, src_count, src_dt, 0, &src_attr,
                                    local_buf, MPIDIG_REQUEST(req, count),
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
    MPIR_GPU_query_pointer_attr(src_buf, &attr);
    dev_id = MPL_gpu_get_dev_id_from_attr(&attr);

    MPL_pointer_attr_t dst_attr = { 0 };
    mpi_errno = ipc_map_addr(ipc_hdr, sreq, src_data_sz, dev_id, &dst_buf, &dst_attr);
    MPIR_ERR_CHECK(mpi_errno);

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

    /* try to use cached local mmap for fast_memcpy */
    mpi_errno = MPIDI_GPU_ipc_local_mmap(src_buf, &attr, src_data_sz, &src_buf);
    MPIR_ERR_CHECK(mpi_errno);

    /* copy */
    int remote_global_dev_id = (ipc_hdr->ipc_type == MPIDI_IPCI_TYPE__DIRECT) ?
        ipc_hdr->ipc_handle.direct.global_dev_id : ipc_hdr->ipc_handle.gpu.global_dev_id;

    MPIR_gpu_req yreq;
    MPL_gpu_engine_type_t engine = MPIDI_IPCI_choose_engine(remote_global_dev_id, dev_id);
    mpi_errno = MPIR_Ilocalcopy_gpu(src_buf, src_count, src_datatype, 0, &attr,
                                    dst_buf, dst_count, dst_datatype, 0, &dst_attr, engine, true,
                                    &yreq);
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

    MPL_gpu_ipc_mem_handle_t *handle_ptr = MPIDI_SHM_REQUEST(req, ipc.u.handle.handle_ptr);
    if (MPIDI_SHM_REQUEST(req, ipc.u.handle.is_cached)) {
        struct MPIDU_gpu_cache_entry *entry = MPIDU_gpu_cache_find_handle(handle_ptr);
        MPIR_Assert(entry);
        entry->range.in_use--;
        MPIR_Assert(entry->range.in_use >= 0);
    } else {
        mpi_errno = MPIDI_GPU_handle_destroy(handle_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_GPU_ipc_map_complete(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPL_gpu_map_t *map = &MPIDI_SHM_REQUEST(req, ipc.u.map);
    if (map->mapped_addr) {
        int mpl_err = MPL_gpu_ipc_handle_unmap(map);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**gpu_ipc_handle_unmap");
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* MPIDI_CH4_SHM_ENABLE_GPU */
