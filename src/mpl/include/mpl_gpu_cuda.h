/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPL_GPU_CUDA_H_INCLUDED
#define MPL_GPU_CUDA_H_INCLUDED

#include "cuda.h"
#include "cuda_runtime_api.h"

typedef unsigned long long MPL_gpu_buffer_id_t;
typedef struct {
    cudaIpcMemHandle_t handle;
    MPL_gpu_buffer_id_t id;
} MPL_gpu_ipc_mem_handle_t;
typedef int MPL_gpu_device_handle_t;
typedef struct cudaPointerAttributes MPL_gpu_device_attr;
typedef int MPL_gpu_map_attr;   /* dummy type */
typedef int MPL_gpu_request;
typedef cudaStream_t MPL_gpu_stream_t;

/* Note: event variable need be allocated on a gpu registered host buffer for it to work */
typedef volatile int MPL_gpu_event_t;

#define MPL_GPU_STREAM_DEFAULT 0
#define MPL_GPU_DEVICE_INVALID -1

#define MPL_GPU_DEV_AFFINITY_ENV "CUDA_VISIBLE_DEVICES"

/* Return the PCI bus/device/function (BDF) of the CUDA device this process is
 * currently using. If the process already has a current CUDA context, the BDF
 * of that context's device is returned. Otherwise it falls back to visible device
 * ordinal 0, which relies on CUDA_VISIBLE_DEVICES to identify the rank's GPU.
 * On success fills the out-params and returns MPL_SUCCESS; returns
 * MPL_ERR_GPU_INTERNAL if CUDA is unavailable or the BDF cannot be determined. */
int MPL_gpu_cuda_get_current_pci_bdf(int *domain, int *bus, int *dev, int *func);

#endif /* ifndef MPL_GPU_CUDA_H_INCLUDED */
