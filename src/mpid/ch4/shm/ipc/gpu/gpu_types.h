/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef GPU_TYPES_H_INCLUDED
#define GPU_TYPES_H_INCLUDED

#include "uthash.h"

typedef struct {
    int *local_ranks;
    int *local_procs;
    int local_device_count;
    int global_max_dev_id;
    int initialized;
    MPL_gavl_tree_t ***ipc_handle_mapped_trees;
    MPL_gavl_tree_t **ipc_handle_track_trees;
} MPIDI_GPUI_global_t;

typedef struct {
    uintptr_t mapped_base_addr;
} MPIDI_GPUI_handle_obj_s;

extern MPIDI_GPUI_global_t MPIDI_GPUI_global;

#endif /* GPU_TYPES_H_INCLUDED */
