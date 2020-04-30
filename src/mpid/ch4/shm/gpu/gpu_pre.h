/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef GPU_PRE_H_INCLUDED
#define GPU_PRE_H_INCLUDED

typedef struct {
    int dummy;
} MPIDI_GPU_global_t;

typedef struct {
    uint64_t data_sz;
    uint64_t sreq_ptr;
    MPL_gpu_ipc_mem_handle_t mem_handle;
} MPIDI_GPU_am_unexp_rreq_t;

typedef struct {
    MPIDI_GPU_am_unexp_rreq_t unexp_rreq;
} MPIDI_GPU_am_request_t;

#define MPIDI_GPU_IPC_REQUEST(req, field) ((req)->dev.ch4.am.shm_am.gpu.field)

#endif /* GPU_PRE_H_INCLUDED */
