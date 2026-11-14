/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MPIDU_GPU_CACHE_H_INCLUDED
#define MPIDU_GPU_CACHE_H_INCLUDED

#include "mpir_range_cache.h"

/* Cache of the IPC handles of local GPU buffers and of their host mmaps.
 *
 * A buffer has at most one cached IPC handle, and every module that needs the
 * handle or the host mmap of a local device buffer gets it here (the ZE drmfd path
 * cannot create a second handle for a buffer that has one):
 *   * the shm IPC module exports the handle to local peers and keeps their mappings
 *     in entry->ext, which it releases in the ext_free callback;
 *   * small (CPU) copies use the host mmap from MPIDU_gpu_cache_local_mmap instead
 *     of mapping and unmapping the buffer on every copy (ZE only).
 *
 * Entries are keyed by the allocation (base, length, buffer id). An entry is dropped
 * when the cache is full (LRU among the entries not in use), when a lookup finds it
 * stale, when the buffer is freed (free hook), and at finalize. The ctx argument is
 * passed through to ext_free.
 *
 * There is no locking inside; the callers serialize access as before.
 */

struct MPIDU_gpu_cache_entry {
    struct MPIR_range_cache_entry range;        /* must be first */
    MPL_gpu_ipc_mem_handle_t *handle_ptr;
    MPL_gpu_map_t local_map;    /* host mmap; mapped_addr is NULL until it is needed */
    void *ext;                  /* owned by the module that set ext_free */
};

typedef int (*MPIDU_gpu_cache_ext_free_fn) (struct MPIDU_gpu_cache_entry * entry, void *ctx);

int MPIDU_gpu_cache_init(bool force_insert);
int MPIDU_gpu_cache_finalize(void);
void MPIDU_gpu_cache_set_ext_free(MPIDU_gpu_cache_ext_free_fn ext_free);

int MPIDU_gpu_cache_get_handle(void *base, uintptr_t len, MPL_gpu_device_attr * dev_attr,
                               void *ctx, struct MPIDU_gpu_cache_entry **entry_out,
                               MPL_gpu_ipc_mem_handle_t ** handle_out);
struct MPIDU_gpu_cache_entry *MPIDU_gpu_cache_find(const void *base);
struct MPIDU_gpu_cache_entry *MPIDU_gpu_cache_find_handle(MPL_gpu_ipc_mem_handle_t * handle_ptr);
int MPIDU_gpu_cache_local_mmap(void *dev_ptr, MPL_pointer_attr_t * attr, void **host_ptr_out);

#endif /* MPIDU_GPU_CACHE_H_INCLUDED */
