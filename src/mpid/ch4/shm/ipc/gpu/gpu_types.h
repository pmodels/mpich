/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GPU_TYPES_H_INCLUDED
#define GPU_TYPES_H_INCLUDED

typedef struct {
    int local_device_count;
    int initialized;
} MPIDI_GPUI_global_t;

extern MPIDI_GPUI_global_t MPIDI_GPUI_global;

#endif /* GPU_TYPES_H_INCLUDED */
