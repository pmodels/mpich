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

/* ZE specific function */
int MPL_ze_init_device_fds(int *num_fds, int *device_fds);
void MPL_ze_set_fds(int num_fds, int *fds);
void MPL_ze_ipc_remove_cache_handle(void *dptr);
int MPL_ze_ipc_handle_create(const void *ptr, MPL_gpu_device_attr * ptr_attr, int local_dev_id,
                             int use_shared_fd, MPL_gpu_ipc_mem_handle_t * ipc_handle);
int MPL_ze_ipc_handle_map(MPL_gpu_ipc_mem_handle_t ipc_handle, int is_shared_handle, int dev_id,
                          int is_mmap, size_t size, void **ptr);
int MPL_ze_ipc_handle_mmap_host(MPL_gpu_ipc_mem_handle_t ipc_handle, int shared_handle, int dev_id,
                                size_t size, void **ptr);
int MPL_ze_mmap_device_pointer(void *dptr, MPL_gpu_device_attr * attr,
                               MPL_gpu_device_handle_t device, void **mmaped_ptr);
int MPL_ze_mmap_handle_unmap(void *ptr, int dev_id);

#endif /* ifndef MPL_GPU_ZE_H_INCLUDED */
