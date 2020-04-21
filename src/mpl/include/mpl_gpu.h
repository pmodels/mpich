/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPL_GPU_H_INCLUDED
#define MPL_GPU_H_INCLUDED

#include "mplconfig.h"

#include "mpl_gpu_fallback.h"

typedef enum {
    MPL_GPU_POINTER_UNREGISTERED_HOST = 0,
    MPL_GPU_POINTER_REGISTERED_HOST,
    MPL_GPU_POINTER_DEV,
    MPL_GPU_POINTER_MANAGED
} MPL_pointer_type_t;

int MPL_gpu_query_pointer_type(const void *ptr, MPL_pointer_type_t * attr);

int MPL_gpu_ipc_get_mem_handle(MPL_gpu_ipc_mem_handle_t * h_mem, void *ptr);
int MPL_gpu_ipc_open_mem_handle(void **ptr, MPL_gpu_ipc_mem_handle_t h_mem);
int MPL_gpu_ipc_close_mem_handle(void *ptr);

#endif /* ifndef MPL_GPU_H_INCLUDED */
