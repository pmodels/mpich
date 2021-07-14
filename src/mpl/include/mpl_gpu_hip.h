/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPL_GPU_HIP_H_INCLUDED
#define MPL_GPU_HIP_H_INCLUDED

#include "hip/hip_runtime.h"
#include "hip/hip_runtime_api.h"

typedef hipIpcMemHandle_t MPL_gpu_ipc_mem_handle_t;
typedef int MPL_gpu_device_handle_t;
typedef struct hipPointerAttribute_t MPL_gpu_device_attr;
#define MPL_GPU_DEVICE_INVALID -1

#endif /* ifndef MPL_GPU_HIP_H_INCLUDED */
