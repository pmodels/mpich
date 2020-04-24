/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpl.h"

int MPL_gpu_query_pointer_type(const void *ptr, MPL_pointer_type_t * attr)
{
    *attr = MPL_GPU_POINTER_UNREGISTERED_HOST;

    return MPL_SUCCESS;
}

int MPL_gpu_ipc_get_mem_handle(MPL_gpu_ipc_mem_handle_t * h_mem, void *ptr)
{
    abort();
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_ipc_open_mem_handle(void **ptr, MPL_gpu_ipc_mem_handle_t h_mem)
{
    abort();
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_ipc_close_mem_handle(void *ptr)
{
    abort();
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_malloc(void **ptr, size_t size)
{
    abort();
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_free(void *ptr)
{
    abort();
    return MPL_ERR_GPU_INTERNAL;
}
