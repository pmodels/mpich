/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_range_cache.h"

/* The entries are kept in an array of pointers, so an entry stays at the same
 * address while other entries are dropped. The array is short (it is bounded by
 * max_entries and usually holds a few dozen entries), so a linear scan is used. */

static int cache_delete(struct MPIR_range_cache *cache, int idx, void *ctx);
static int cache_check_limit(struct MPIR_range_cache *cache, void *ctx);

int MPIR_range_cache_init(struct MPIR_range_cache *cache, int limit, int max_entries,
                          MPIR_range_cache_free_fn free_fn)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(max_entries > 0);
    cache->entries = MPL_malloc(max_entries * sizeof(cache->entries[0]), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!cache->entries, mpi_errno, MPI_ERR_OTHER, "**nomem");

    cache->count = 0;
    cache->limit = MPL_MAX(MPL_MIN(limit, max_entries), 0);
    cache->max_entries = max_entries;
    cache->usage_counter = 0;
    cache->free_fn = free_fn;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_range_cache_finalize(struct MPIR_range_cache *cache, void *ctx)
{
    int mpi_errno = MPI_SUCCESS;

    while (cache->count > 0) {
        MPIR_Assert(!cache->entries[cache->count - 1]->in_use);
        mpi_errno = cache_delete(cache, cache->count - 1, ctx);
        MPIR_ERR_CHECK(mpi_errno);
    }
    MPL_free(cache->entries);
    cache->entries = NULL;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_range_cache_search(struct MPIR_range_cache *cache, const void *base, MPI_Aint len,
                            uint64_t tag, void *ctx, struct MPIR_range_cache_entry **entry_out)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIR_range_cache_entry *found = NULL;

    for (int i = 0; i < cache->count; i++) {
        struct MPIR_range_cache_entry *entry = cache->entries[i];
        if (entry->base == base && entry->tag == tag) {
            found = entry;
            break;
        }
        /* drop stale entries.
         * Overlap condition between [a1, b1) and [a2, b2) is a1<b2 && b1>a2 */
        if ((uintptr_t) entry->base < (uintptr_t) base + len &&
            (uintptr_t) entry->base + entry->len > (uintptr_t) base) {
            mpi_errno = cache_delete(cache, i, ctx);
            MPIR_ERR_CHECK(mpi_errno);
            i--;        /* the array has been shifted up */
        }
    }

    if (found) {
        /* update usage_stamp for LRU */
        found->usage_stamp = ++cache->usage_counter;
    }
    *entry_out = found;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

struct MPIR_range_cache_entry *MPIR_range_cache_find(struct MPIR_range_cache *cache,
                                                     const void *base)
{
    for (int i = 0; i < cache->count; i++) {
        if (cache->entries[i]->base == base) {
            return cache->entries[i];
        }
    }
    return NULL;
}

bool MPIR_range_cache_can_insert(struct MPIR_range_cache * cache)
{
    return cache->count < cache->limit;
}

int MPIR_range_cache_insert(struct MPIR_range_cache *cache, struct MPIR_range_cache_entry *entry,
                            bool force, void *ctx, bool * inserted_out)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = cache_check_limit(cache, ctx);
    MPIR_ERR_CHECK(mpi_errno);

    int limit = force ? cache->max_entries : cache->limit;
    if (cache->count < limit) {
        entry->in_use = 0;
        entry->usage_stamp = ++cache->usage_counter;
        cache->entries[cache->count] = entry;
        cache->count++;
        *inserted_out = true;
    } else {
        *inserted_out = false;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_range_cache_remove(struct MPIR_range_cache *cache, struct MPIR_range_cache_entry *entry,
                            void *ctx)
{
    for (int i = 0; i < cache->count; i++) {
        if (cache->entries[i] == entry) {
            return cache_delete(cache, i, ctx);
        }
    }
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

/* ---- static functions ---- */

/* remove the entry from the array, then release it */
static int cache_delete(struct MPIR_range_cache *cache, int idx, void *ctx)
{
    MPIR_Assert(idx < cache->count);
    struct MPIR_range_cache_entry *entry = cache->entries[idx];

    cache->count--;
    if (cache->count > idx) {
        /* shift the entries up to fill the gap */
        memmove(cache->entries + idx, cache->entries + idx + 1,
                (cache->count - idx) * sizeof(cache->entries[0]));
    }

    return cache->free_fn(entry, ctx);
}

/* at the limit, drop the least recently used entry that is not in use */
static int cache_check_limit(struct MPIR_range_cache *cache, void *ctx)
{
    int mpi_errno = MPI_SUCCESS;

    if (cache->count >= cache->limit) {
        int min_idx = -1;
        unsigned long long min_stamp = 0;
        for (int i = 0; i < cache->count; i++) {
            if (cache->entries[i]->in_use) {
                continue;
            }
            if (min_idx == -1 || cache->entries[i]->usage_stamp < min_stamp) {
                min_stamp = cache->entries[i]->usage_stamp;
                min_idx = i;
            }
        }

        if (min_idx != -1) {
            mpi_errno = cache_delete(cache, min_idx, ctx);
        }
    }

    return mpi_errno;
}
