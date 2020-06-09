/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef GPU_PRE_H_INCLUDED
#define GPU_PRE_H_INCLUDED

typedef struct MPIDI_GPU_mem_handle {
    MPL_gpu_ipc_mem_handle_t ipc_handle;
} MPIDI_GPU_mem_handle_t;

typedef struct MPIDI_GPU_mem_seg {
    MPL_gpu_ipc_mem_handle_t ipc_handle;
    void *vaddr;
} MPIDI_GPU_mem_seg_t;

#endif /* GPU_PRE_H_INCLUDED */
