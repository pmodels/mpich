/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpl.h"

int MPL_gpu_query_pointer_attr(const void *ptr, MPL_pointer_attr_t * attr)
{
    attr->type = MPL_GPU_POINTER_UNREGISTERED_HOST;
    attr->device = -1;

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

int MPL_gpu_malloc_host(void **ptr, size_t size)
{
    *ptr = MPL_malloc(size, MPL_MEM_BUFFER);
    return MPL_SUCCESS;
}

int MPL_gpu_free_host(void *ptr)
{
    MPL_free(ptr);
    return MPL_SUCCESS;
}

int MPL_gpu_register_host(const void *ptr, size_t size)
{
    return MPL_SUCCESS;
}

int MPL_gpu_unregister_host(const void *ptr)
{
    return MPL_SUCCESS;
}

int MPL_gpu_malloc(void **ptr, size_t size, int devid)
{
    abort();
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_free(void *ptr)
{
    abort();
    return MPL_ERR_GPU_INTERNAL;
}
