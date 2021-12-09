/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef GPU_PRE_H_INCLUDED
#define GPU_PRE_H_INCLUDED

#include "mpl.h"

enum {
    MPIDI_GPU_IPC_HANDLE_VALID,
    MPIDI_GPU_IPC_HANDLE_REMAP_REQUIRED,
};

typedef struct MPIDI_GPU_ipc_handle {
    MPL_gpu_ipc_mem_handle_t ipc_handle;
    int global_dev_id;
    uintptr_t remote_base_addr;
    uintptr_t len;
    int node_rank;
    uintptr_t offset;
    int handle_status;
} MPIDI_GPU_ipc_handle_t;

#endif /* GPU_PRE_H_INCLUDED */
