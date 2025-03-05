/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef GPU_TYPES_H_INCLUDED
#define GPU_TYPES_H_INCLUDED

#include "uthash.h"

struct MPIDI_GPUI_map_cache_entry {
    UT_hash_handle hh;
    struct map_key {
        int remote_rank;
        const void *remote_addr;
    } key;
    const void *mapped_addrs[]; /* array of base addresses indexed by device id */
};

typedef struct {
    int *local_ranks;
    int *local_procs;
    int local_device_count;
    int global_max_dev_id;
    int initialized;
    struct MPIDI_GPUI_map_cache_entry *ipc_map_cache;
    MPL_gavl_tree_t **ipc_handle_track_trees;
} MPIDI_GPUI_global_t;

extern MPIDI_GPUI_global_t MPIDI_GPUI_global;

#endif /* GPU_TYPES_H_INCLUDED */
