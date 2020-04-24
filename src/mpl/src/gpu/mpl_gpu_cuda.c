/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpl.h"

#define CUDA_ERR_CHECK(ret) if (unlikely((ret) != cudaSuccess)) goto fn_fail

int MPL_gpu_query_pointer_type(const void *ptr, MPL_pointer_type_t * attr)
{
    cudaError_t ret;
    struct cudaPointerAttributes ptr_attr = { 0 };
    ret = cudaPointerGetAttributes(&ptr_attr, ptr);
    if (ret == cudaSuccess) {
        switch (ptr_attr.type) {
            case cudaMemoryTypeUnregistered:
                *attr = MPL_GPU_POINTER_UNREGISTERED_HOST;
                break;
            case cudaMemoryTypeHost:
                *attr = MPL_GPU_POINTER_REGISTERED_HOST;
                break;
            case cudaMemoryTypeDevice:
                *attr = MPL_GPU_POINTER_DEV;
                break;
            case cudaMemoryTypeManaged:
                *attr = MPL_GPU_POINTER_MANAGED;
                break;
        }
    } else if (ret == cudaErrorInvalidValue) {
        *attr = MPL_GPU_POINTER_UNREGISTERED_HOST;
    } else {
        goto fn_fail;
    }

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_ipc_get_mem_handle(MPL_gpu_ipc_mem_handle_t * h_mem, void *ptr)
{
    cudaError_t ret;
    ret = cudaIpcGetMemHandle(h_mem, ptr);
    CUDA_ERR_CHECK(ret);

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_ipc_open_mem_handle(void **ptr, MPL_gpu_ipc_mem_handle_t h_mem)
{
    cudaError_t ret;
    ret = cudaIpcOpenMemHandle(ptr, h_mem, cudaIpcMemLazyEnablePeerAccess);
    CUDA_ERR_CHECK(ret);

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_ipc_close_mem_handle(void *ptr)
{
    cudaError_t ret;
    ret = cudaIpcCloseMemHandle(ptr);
    CUDA_ERR_CHECK(ret);

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_malloc(void **ptr, size_t size)
{
    cudaError_t ret;
    ret = cudaMalloc(ptr, size);
    CUDA_ERR_CHECK(ret);

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_free(void *ptr)
{
    cudaError_t ret;
    ret = cudaFree(ptr);
    CUDA_ERR_CHECK(ret);

  fn_exit:
    MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}
