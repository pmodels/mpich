/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef GPU_TYPES_H_INCLUDED
#define GPU_TYPES_H_INCLUDED

#include "uthash.h"

typedef struct MPIDI_GPUI_dev_id {
    int local_dev_id;
    int global_dev_id;
    UT_hash_handle hh;
} MPIDI_GPUI_dev_id_t;

typedef struct {
    MPIDI_GPUI_dev_id_t *local_to_global_map;
    MPIDI_GPUI_dev_id_t *global_to_local_map;
    int **visible_dev_global_id;
    int *local_ranks;
    int *local_procs;
    int global_max_dev_id;
    int initialized;
} MPIDI_GPUI_global_t;

extern MPIDI_GPUI_global_t MPIDI_GPUI_global;

#endif /* GPU_TYPES_H_INCLUDED */
