/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===
cvars:
    - name        : MPIR_CVAR_GPU_HANDLE_CACHE_SIZE
      category    : GPU
      alt-env     : MPIR_CVAR_CH4_IPC_GPU_CACHE_SIZE
      type        : int
      default     : 16
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        The maximum number of entries to hold in the cache of the IPC handles and host
        mmaps of local GPU buffers, which is shared by the GPU IPC shmmod and the netmod.
        When an entry is evicted, the corresponding IPC handle is closed. If
        MPIR_CVAR_GPU_HANDLE_CACHE_SIZE is 0, the handles are not cached, except where
        the ZE drmfd shareable handle requires it.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#include "mpiimpl.h"
#include "mpidu_gpu_cache.h"

/* GPU IPC handle caching:
 *   * NVIDIA CUDA sender side handle creation is cheap, but receive side mapping is expensive.
 *       * if we don't unmap freed handles, new handle may overlap the address, and new map will fail.
 *   * Intel ZE handle is actually file handles. They are bound to the allocation.
 *       * there won't be stale handle or map issue, since the memory won't be freed if one is active.
 *       * But unlimited active entries will cause apparent memory leak to the users and eventually
 *         exhaust the memory.
 */

#define GPU_CACHE_MAX 1024

static struct {
    bool initialized;
    bool force_insert;          /* insert beyond the limit, up to GPU_CACHE_MAX */
    struct MPIR_range_cache cache;
    MPIDU_gpu_cache_ext_free_fn ext_free;
} gpu_cache;

static int cache_search(void *base, uintptr_t len, uint64_t tag, void *ctx,
                        struct MPIDU_gpu_cache_entry **entry_out);
static int cache_insert(void *base, uintptr_t len, uint64_t tag, MPL_gpu_device_attr * dev_attr,
                        void *ctx,
                        struct MPIDU_gpu_cache_entry **entry_out,
                        MPL_gpu_ipc_mem_handle_t ** handle_out);
static int entry_free(struct MPIR_range_cache_entry *range, void *ctx);
static void gpu_free_hook(void *dptr);

int MPIDU_gpu_cache_init(bool force_insert)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_range_cache_init(&gpu_cache.cache, MPIR_CVAR_GPU_HANDLE_CACHE_SIZE,
                                      GPU_CACHE_MAX, entry_free);
    MPIR_ERR_CHECK(mpi_errno);

    gpu_cache.force_insert = force_insert;
    gpu_cache.ext_free = NULL;
    MPL_gpu_free_hook_register(gpu_free_hook);

    gpu_cache.initialized = true;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDU_gpu_cache_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    if (gpu_cache.initialized) {
        gpu_cache.initialized = false;
        mpi_errno = MPIR_range_cache_finalize(&gpu_cache.cache, NULL);
    }

    return mpi_errno;
}

void MPIDU_gpu_cache_set_ext_free(MPIDU_gpu_cache_ext_free_fn ext_free)
{
    gpu_cache.ext_free = ext_free;
}

/* Get the IPC handle of the buffer [base, base + len), a whole allocation.
 * On a hit, or when the new handle is cached, *entry_out is the cache entry and
 * *handle_out is its handle. When the cache has no room, *entry_out is NULL and
 * *handle_out is a new handle that the caller destroys and frees. */
int MPIDU_gpu_cache_get_handle(void *base, uintptr_t len, MPL_gpu_device_attr * dev_attr,
                               void *ctx, struct MPIDU_gpu_cache_entry **entry_out,
                               MPL_gpu_ipc_mem_handle_t ** handle_out)
{
    int mpi_errno = MPI_SUCCESS;

    uint64_t tag = (uint64_t) MPL_gpu_get_buffer_id(base);
    struct MPIDU_gpu_cache_entry *entry;
    mpi_errno = cache_search(base, len, tag, ctx, &entry);
    MPIR_ERR_CHECK(mpi_errno);

    if (entry) {
        *entry_out = entry;
        *handle_out = entry->handle_ptr;
    } else {
        mpi_errno = cache_insert(base, len, tag, dev_attr, ctx, entry_out, handle_out);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* find the entry by its base address, without the stale check */
struct MPIDU_gpu_cache_entry *MPIDU_gpu_cache_find(const void *base)
{
    return (struct MPIDU_gpu_cache_entry *) MPIR_range_cache_find(&gpu_cache.cache, base);
}

struct MPIDU_gpu_cache_entry *MPIDU_gpu_cache_find_handle(MPL_gpu_ipc_mem_handle_t * handle_ptr)
{
    for (int i = 0; i < gpu_cache.cache.count; i++) {
        struct MPIR_range_cache_entry *range = gpu_cache.cache.entries[i];
        struct MPIDU_gpu_cache_entry *entry = (struct MPIDU_gpu_cache_entry *) range;
        if (entry->handle_ptr == handle_ptr) {
            return entry;
        }
    }
    return NULL;
}

/* Cache the host mmap of a local device buffer. On success *host_ptr_out is the host
 * address of dev_ptr and attr->type is MPL_GPU_POINTER_DEV_MMAP, so that
 * MPIR_Localcopy_gpu copies directly. A new buffer evicts the least recently used
 * entry that is not in use when the cache is full. Otherwise (not ZE device memory,
 * every entry in use, or caching disabled) *host_ptr_out is dev_ptr and attr is
 * unchanged. */
int MPIDU_gpu_cache_local_mmap(void *dev_ptr, MPL_pointer_attr_t * attr, void **host_ptr_out)
{
    int mpi_errno = MPI_SUCCESS;

    *host_ptr_out = dev_ptr;    /* default: unchanged */

#ifdef MPL_HAVE_ZE
    if (!gpu_cache.initialized || !MPL_gpu_attr_is_dev(attr)) {
        goto fn_exit;
    }

    void *pbase;
    uintptr_t len;
    int mpl_err = MPL_gpu_get_buffer_bounds(dev_ptr, &pbase, &len);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gpu_get_buffer_info");

    uint64_t tag = (uint64_t) MPL_gpu_get_buffer_id(pbase);
    struct MPIDU_gpu_cache_entry *entry;
    mpi_errno = cache_search(pbase, len, tag, NULL, &entry);
    MPIR_ERR_CHECK(mpi_errno);

    if (!entry) {
        /* evict the least recently used entry if the cache is full */
        bool has_room;
        mpi_errno = MPIR_range_cache_make_room(&gpu_cache.cache, gpu_cache.force_insert, NULL,
                                               &has_room);
        MPIR_ERR_CHECK(mpi_errno);
        if (!has_room) {
            /* every entry is in use, or caching is disabled */
            goto fn_exit;
        }
        MPL_gpu_ipc_mem_handle_t *handle_ptr;
        mpi_errno = cache_insert(pbase, len, tag, &attr->device_attr, NULL, &entry, &handle_ptr);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(entry);
    }

    if (!entry->local_map.mapped_addr) {
        /* map the local device buffer to host via mmap */
        int dev_id = MPL_gpu_get_dev_id_from_attr(attr);
        MPL_gpu_ipc_mem_handle_t ipc_handle = *entry->handle_ptr;
        mpl_err = MPL_gpu_ipc_handle_map(&ipc_handle, dev_id, &entry->local_map, true,
                                         entry->range.len);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**gpu_ipc_handle_map");
    }

    uintptr_t offset = (uintptr_t) dev_ptr - (uintptr_t) pbase;
    *host_ptr_out = (void *) ((uintptr_t) entry->local_map.mapped_addr + offset);
    attr->type = MPL_GPU_POINTER_DEV_MMAP;

  fn_exit:
#endif
    return mpi_errno;
#ifdef MPL_HAVE_ZE
  fn_fail:
    goto fn_exit;
#endif
}

/* ---- static functions ---- */

static int cache_search(void *base, uintptr_t len, uint64_t tag, void *ctx,
                        struct MPIDU_gpu_cache_entry **entry_out)
{
    struct MPIR_range_cache_entry *range;
    int mpi_errno = MPIR_range_cache_search(&gpu_cache.cache, base, len, tag, ctx, &range);

    *entry_out = (struct MPIDU_gpu_cache_entry *) range;
    return mpi_errno;
}

/* create the IPC handle and try to cache it; *entry_out is NULL if it is not cached */
static int cache_insert(void *base, uintptr_t len, uint64_t tag, MPL_gpu_device_attr * dev_attr,
                        void *ctx, struct MPIDU_gpu_cache_entry **entry_out,
                        MPL_gpu_ipc_mem_handle_t ** handle_out)
{
    int mpi_errno = MPI_SUCCESS;
    MPL_gpu_ipc_mem_handle_t *handle_ptr = NULL;
    struct MPIDU_gpu_cache_entry *entry = NULL;

    handle_ptr = MPL_malloc(sizeof(*handle_ptr), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!handle_ptr, mpi_errno, MPI_ERR_OTHER, "**nomem");

    int mpl_err = MPL_gpu_ipc_handle_create(base, dev_attr, handle_ptr);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                        "**gpu_ipc_handle_create");

    entry = MPL_calloc(1, sizeof(*entry), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!entry, mpi_errno, MPI_ERR_OTHER, "**nomem");
    entry->range.base = base;
    entry->range.len = len;
    entry->range.tag = tag;
    entry->handle_ptr = handle_ptr;

    bool inserted;
    mpi_errno = MPIR_range_cache_insert(&gpu_cache.cache, &entry->range, gpu_cache.force_insert,
                                        ctx, &inserted);
    MPIR_ERR_CHECK(mpi_errno);

    if (!inserted) {
        MPL_free(entry);
        entry = NULL;
    }
    *entry_out = entry;
    *handle_out = handle_ptr;

  fn_exit:
    return mpi_errno;
  fn_fail:
    MPL_free(entry);
    MPL_free(handle_ptr);
    goto fn_exit;
}

/* release an entry the cache has dropped */
static int entry_free(struct MPIR_range_cache_entry *range, void *ctx)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDU_gpu_cache_entry *entry = (struct MPIDU_gpu_cache_entry *) range;

    if (gpu_cache.ext_free) {
        mpi_errno = gpu_cache.ext_free(entry, ctx);
    }

    if (entry->local_map.mapped_addr) {
        int mpl_err = MPL_gpu_ipc_handle_unmap(&entry->local_map);
        if (mpl_err != MPL_SUCCESS && mpi_errno == MPI_SUCCESS) {
            MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**gpu_ipc_handle_unmap");
        }
    }

    MPL_gpu_ipc_handle_destroy(entry->handle_ptr);
    MPL_free(entry->handle_ptr);
    MPL_free(entry);

    return mpi_errno;
}

/* invalidate the entry of a buffer when it is freed */
static void gpu_free_hook(void *dptr)
{
    if (!gpu_cache.initialized) {
        return;
    }

    struct MPIR_range_cache_entry *range = MPIR_range_cache_find(&gpu_cache.cache, dptr);
    if (range) {
        MPIR_Assert(!range->in_use);
        MPIR_range_cache_remove(&gpu_cache.cache, range, NULL);
    }
}
