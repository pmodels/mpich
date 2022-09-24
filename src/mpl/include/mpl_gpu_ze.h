/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPL_GPU_ZE_H_INCLUDED
#define MPL_GPU_ZE_H_INCLUDED

#include "level_zero/ze_api.h"

typedef struct {
    ze_memory_allocation_properties_t prop;
    ze_device_handle_t device;
} ze_alloc_attr_t;

typedef ze_ipc_mem_handle_t MPL_gpu_ipc_mem_handle_t;
typedef ze_device_handle_t MPL_gpu_device_handle_t;
typedef ze_alloc_attr_t MPL_gpu_device_attr;
/* FIXME: implement ze stream */
typedef int MPL_gpu_stream_t;

typedef volatile int MPL_gpu_event_t;

#define MPL_GPU_STREAM_DEFAULT 0
#define MPL_GPU_DEVICE_INVALID NULL

#define MPL_GPU_DEV_AFFINITY_ENV "ZE_AFFINITY_MASK"

#endif /* ifndef MPL_GPU_ZE_H_INCLUDED */
