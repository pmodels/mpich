/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef GPU_PRE_H_INCLUDED
#define GPU_PRE_H_INCLUDED

#include "mpl.h"

typedef struct MPIDI_GPU_ipc_handle {
    MPL_gpu_ipc_mem_handle_t ipc_handle;
    int global_dev_id;
    uintptr_t offset;
} MPIDI_GPU_ipc_handle_t;

#endif /* GPU_PRE_H_INCLUDED */
