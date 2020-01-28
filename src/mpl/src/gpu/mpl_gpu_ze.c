/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpl.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#ifdef MPL_HAVE_ZE

int MPL_gpu_init()
{
    return MPL_SUCCESS;
}

int MPL_gpu_finalize()
{
    return MPL_SUCCESS;
}

int MPL_gpu_ipc_get_mem_handle(MPL_gpu_ipc_mem_handle_t * h_mem, void *ptr)
{
    return MPL_SUCCESS;
}

int MPL_gpu_ipc_open_mem_handle(void **ptr, MPL_gpu_ipc_mem_handle_t h_mem,
                                MPL_gpu_device_handle_t h_device)
{
    return MPL_SUCCESS;
}

int MPL_gpu_ipc_close_mem_handle(void *ptr)
{
    return MPL_SUCCESS;
}

int MPL_gpu_query_pointer_attr(const void *ptr, MPL_pointer_attr_t * attr)
{
    return MPL_SUCCESS;
}

int MPL_gpu_get_device_handle(const void *buf, MPL_gpu_device_handle_t * h_device)
{
    return MPL_SUCCESS;
}

int MPL_gpu_malloc(void **ptr, size_t size, MPL_gpu_device_handle_t h_device)
{
    return MPL_SUCCESS;
}

int MPL_gpu_malloc_host(void **ptr, size_t size)
{
    return MPL_SUCCESS;
}

int MPL_gpu_free(void *ptr)
{
    return MPL_SUCCESS;
}

int MPL_gpu_free_host(void *ptr)
{
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

#endif /* MPL_HAVE_ZE */
