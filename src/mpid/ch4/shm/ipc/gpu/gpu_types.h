/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef GPU_TYPES_H_INCLUDED
#define GPU_TYPES_H_INCLUDED

#include "uthash.h"

typedef struct {
    int remote_rank;
    void *mapped_addr;
} MPIDI_GPUI_map_entry;

struct MPIDI_GPUI_handle_cache_entry {
    UT_hash_handle hh;
    const void *base_addr;
    MPL_gpu_ipc_mem_handle_t handle;
    int num_maps;
    MPIDI_GPUI_map_entry *maps;
};

typedef struct {
    int local_device_count;
    int initialized;
    struct MPIDI_GPUI_handle_cache_entry *ipc_handle_cache;
} MPIDI_GPUI_global_t;

extern MPIDI_GPUI_global_t MPIDI_GPUI_global;

#endif /* GPU_TYPES_H_INCLUDED */
