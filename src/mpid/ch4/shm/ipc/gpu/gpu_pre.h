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

/* memory handle definition */
typedef struct MPIDI_GPU_ipc_handle {
    MPL_gpu_ipc_mem_handle_t ipc_handle;
    int global_dev_id;
    int local_dev_id;
    uintptr_t remote_base_addr;
    uintptr_t len;
    int node_rank;
    uintptr_t offset;
    int handle_status;
} MPIDI_GPU_ipc_handle_t;

/* local struct used for query and preparing memory handle */
typedef struct MPIDI_GPU_ipc_attr {
    int remote_rank;            /* global rank or MPI_PROC_NULL */
    MPL_pointer_attr_t gpu_attr;
    const void *vaddr;
    void *bounds_base;
    MPI_Aint bounds_len;
} MPIDI_GPU_ipc_attr_t;

#endif /* GPU_PRE_H_INCLUDED */
