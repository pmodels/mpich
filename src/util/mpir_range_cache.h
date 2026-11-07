/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MPIR_RANGE_CACHE_H_INCLUDED
#define MPIR_RANGE_CACHE_H_INCLUDED

/* A small LRU cache of resources bound to a memory range, such as the IPC handle
 * of a GPU buffer.
 *
 * An entry is keyed by its range [base, base + len) and a tag that identifies the
 * allocation behind the range (e.g. the GPU buffer id), so that a new allocation at
 * a reused address does not hit the old entry. The cache knows nothing about the
 * resource: the owner embeds struct MPIR_range_cache_entry as the first member of
 * its own entry, creates the resource, and releases it (and the entry) in the
 * free_fn it passes to MPIR_range_cache_init. free_fn gets the ctx argument of the
 * call that drops the entry.
 *
 *   * search: a hit is an entry with the same base and tag. Entries that overlap
 *     the range without matching are stale and are dropped.
 *   * insert: when the cache holds `limit` entries, the least recently used entry
 *     that is not in use is dropped first. A forced insert may go beyond `limit`, up
 *     to `max_entries`. make_room does the same eviction without inserting, so that
 *     the owner can skip creating a resource that would not be cached.
 *   * in_use: the owner counts the users of an entry; an entry in use is never
 *     evicted.
 *
 * The cache does no locking; the owner serializes access.
 */

struct MPIR_range_cache_entry {
    const void *base;
    MPI_Aint len;
    uint64_t tag;

    int in_use;
    unsigned long long usage_stamp;
};

typedef int (*MPIR_range_cache_free_fn) (struct MPIR_range_cache_entry * entry, void *ctx);

struct MPIR_range_cache {
    struct MPIR_range_cache_entry **entries;
    int count;
    int limit;
    int max_entries;
    unsigned long long usage_counter;   /* for tracking LRU (Least Recently Used) */
    MPIR_range_cache_free_fn free_fn;
};

int MPIR_range_cache_init(struct MPIR_range_cache *cache, int limit, int max_entries,
                          MPIR_range_cache_free_fn free_fn);
int MPIR_range_cache_finalize(struct MPIR_range_cache *cache, void *ctx);
int MPIR_range_cache_search(struct MPIR_range_cache *cache, const void *base, MPI_Aint len,
                            uint64_t tag, void *ctx, struct MPIR_range_cache_entry **entry_out);
struct MPIR_range_cache_entry *MPIR_range_cache_find(struct MPIR_range_cache *cache,
                                                     const void *base);
int MPIR_range_cache_make_room(struct MPIR_range_cache *cache, bool force, void *ctx,
                               bool * has_room_out);
int MPIR_range_cache_insert(struct MPIR_range_cache *cache, struct MPIR_range_cache_entry *entry,
                            bool force, void *ctx, bool * inserted_out);
int MPIR_range_cache_remove(struct MPIR_range_cache *cache, struct MPIR_range_cache_entry *entry,
                            void *ctx);

#endif /* MPIR_RANGE_CACHE_H_INCLUDED */
