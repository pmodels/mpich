/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef GPU_TYPES_H_INCLUDED
#define GPU_TYPES_H_INCLUDED

#include "uthash.h"

#define MPIDI_CH4_IPC_GPU_MAX_CACHE_ENTRIES 100

struct MPIDI_GPUI_map_cache_entry {
    int remote_rank;
    const void *remote_addr;
    MPL_gpu_buffer_id_t remote_buffer_id;
    uintptr_t len;
    int device_id;
    unsigned long long usage;
    const void *mapped_addr;
};

typedef struct {
    int local_device_count;
    int initialized;
    unsigned long long ipc_map_cache_usage_counter;
    struct MPIDI_GPUI_map_cache_entry ipc_map_cache[MPIDI_CH4_IPC_GPU_MAX_CACHE_ENTRIES];
} MPIDI_GPUI_global_t;

extern MPIDI_GPUI_global_t MPIDI_GPUI_global;

#endif /* GPU_TYPES_H_INCLUDED */
