/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/* vim: set ft=c.mpich : */
/*
 *  (C) 2020 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpl.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#ifdef MPL_HAVE_ZE

ze_driver_handle_t global_ze_driver_handle;
int gpu_ze_init_driver();

#define ZE_ERR_CHECK(ret) if (unlikely((ret) != ZE_RESULT_SUCCESS)) goto fn_fail

#define ZE_MPL_ERR_CHECK(ret) if (unlikely((ret) != MPL_SUCCESS)) goto fn_fail

int MPL_gpu_init()
{
    ze_result_t ret;
    ret = gpu_ze_init_driver();
    ZE_MPL_ERR_CHECK(ret);

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_finalize()
{
    ze_result_t ret = ZE_RESULT_SUCCESS;
    ZE_ERR_CHECK(ret);

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

/* Loads a global ze driver */
int gpu_ze_init_driver()
{
    uint32_t driver_count = 0;
    ze_result_t ret;
    int i, d;
    ze_driver_handle_t *all_drivers;

    ret = zeDriverGet(&driver_count, NULL);
    ZE_ERR_CHECK(ret);
    if (driver_count == 0) {
        goto fn_fail;
    }

    all_drivers = MPL_malloc(driver_count * sizeof(ze_driver_handle_t), MPL_MEM_OTHER);
    ret = zeDriverGet(&driver_count, all_drivers);
    ZE_ERR_CHECK(ret);

    /* Find a driver instance with a GPU device */
    for (i = 0; i < driver_count; ++i) {
        uint32_t device_count = 0;
        ze_device_handle_t *all_devices;
        ret = zeDeviceGet(all_drivers[i], &device_count, NULL);
        ZE_ERR_CHECK(ret);
        all_devices = MPL_malloc(device_count * sizeof(ze_device_handle_t), MPL_MEM_OTHER);
        ret = zeDeviceGet(all_drivers[i], &device_count, all_devices);
        ZE_ERR_CHECK(ret);
        /* Check if the driver supports a gpu */
        for (d = 0; d < device_count; ++d) {
            ze_device_properties_t device_properties;
            ret = zeDeviceGetProperties(all_devices[d], &device_properties);
            ZE_ERR_CHECK(ret);

            if (ZE_DEVICE_TYPE_GPU == device_properties.type) {
                global_ze_driver_handle = all_drivers[i];
                break;
            }
        }

        MPL_free(all_devices);
        if (NULL != global_ze_driver_handle) {
            break;
        }
    }

    MPL_free(all_drivers);

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_query_pointer_type(const void *ptr, MPL_pointer_type_t * attr)
{
    ze_result_t ret;
    ze_memory_allocation_properties_t ptr_attr;;
    ret = zeDriverGetMemAllocProperties(global_ze_driver_handle, ptr, &ptr_attr, NULL);
    ZE_ERR_CHECK(ret);
    switch (ptr_attr.type) {
        case ZE_MEMORY_TYPE_UNKNOWN:
            *attr = MPL_GPU_POINTER_UNREGISTERED_HOST;
            break;
        case ZE_MEMORY_TYPE_HOST:
            *attr = MPL_GPU_POINTER_REGISTERED_HOST;
            break;
        case ZE_MEMORY_TYPE_DEVICE:
            *attr = MPL_GPU_POINTER_DEV;
            break;
        case ZE_MEMORY_TYPE_SHARED:
            *attr = MPL_GPU_POINTER_MANAGED;
            break;
    }

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_query_pointer_attr(const void *ptr, MPL_pointer_attr_t * attr)
{
    ze_result_t ret;
    ze_memory_allocation_properties_t ptr_attr;
    ze_device_handle_t device;
    ze_device_properties_t p_device_properties;
    ret = zeDriverGetMemAllocProperties(global_ze_driver_handle, ptr, &ptr_attr, &device);
    ZE_ERR_CHECK(ret);
    switch (ptr_attr.type) {
        case ZE_MEMORY_TYPE_UNKNOWN:
            attr->type = MPL_GPU_POINTER_UNREGISTERED_HOST;
            break;
        case ZE_MEMORY_TYPE_HOST:
            attr->type = MPL_GPU_POINTER_REGISTERED_HOST;
            break;
        case ZE_MEMORY_TYPE_DEVICE:
            attr->type = MPL_GPU_POINTER_DEV;
            break;
        case ZE_MEMORY_TYPE_SHARED:
            attr->type = MPL_GPU_POINTER_MANAGED;
            break;
    }
    ret = zeDeviceGetProperties(device, &p_device_properties);
    ZE_ERR_CHECK(ret);
    attr->device = p_device_properties.deviceId;;

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

/* Find device where a given memory address is allocated */
int get_ze_device_from_memhandle(ze_driver_handle_t driver_handle, const char *buff,
                                 ze_device_handle_t * device)
{
    ze_result_t ret;
    ze_memory_allocation_properties_t ptr_attr;
    ret = zeDriverGetMemAllocProperties(driver_handle, buff, &ptr_attr, device);
    ZE_ERR_CHECK(ret);

  fn_exit:
    return ret;
  fn_fail:
    goto fn_exit;
}

int MPL_gpu_ipc_get_mem_handle(MPL_gpu_ipc_mem_handle_t * h_mem, void *ptr)
{
    ze_result_t ret;
    ret = zeDriverGetMemIpcHandle(global_ze_driver_handle, ptr, h_mem);
    ZE_ERR_CHECK(ret);

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_get_device_handle(const void *buf, void *h_device)
{
    int ret;
    ret = get_ze_device_from_memhandle(global_ze_driver_handle, buf, h_device);
    ZE_ERR_CHECK(ret);
  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_ipc_open_mem_handle(void **ptr, MPL_gpu_ipc_mem_handle_t h_mem,
                                MPL_gpu_device_handle_t h_device)
{
    ze_result_t ret;
    ze_device_handle_t device;
    ret =
        zeDriverOpenMemIpcHandle(global_ze_driver_handle, h_device, h_mem,
                                 ZE_IPC_MEMORY_FLAG_NONE, ptr);
    ZE_ERR_CHECK(ret);

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_ipc_close_mem_handle(void *ptr)
{
    ze_result_t ret;
    ret = zeDriverCloseMemIpcHandle(global_ze_driver_handle, ptr);
    ZE_ERR_CHECK(ret);

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

/* Get device handle object for the given device id */
int get_ze_device_from_id(ze_driver_handle_t driver_handle, uint32_t device_id,
                          ze_device_handle_t * device)
{
    ze_result_t ret;
    int i;
    uint32_t device_count = 0;
    ze_device_handle_t *devices;
    ze_device_properties_t p_device_properties;
    ret = zeDeviceGet(driver_handle, &device_count, NULL);
    ZE_ERR_CHECK(ret);
    if (device_count == 0) {
        goto fn_fail;
    }

    /* Search a device that matches device_id */
    devices =
        (ze_device_handle_t *) MPL_malloc(sizeof(ze_device_handle_t) * device_count, MPL_MEM_OTHER);
    ret = zeDeviceGet(driver_handle, &device_count, devices);
    ZE_ERR_CHECK(ret);
    for (i = 0; i < device_count; i++) {
        ret = zeDeviceGetProperties(devices[i], &p_device_properties);
        ZE_ERR_CHECK(ret);
        if (p_device_properties.deviceId == device_id) {
            *device = devices[i];
            goto fn_exit;
        }
    }
    goto fn_fail;

  fn_exit:
    if (devices)
        MPL_free(devices);
    return MPL_SUCCESS;
  fn_fail:
    if (devices)
        MPL_free(devices);
    return MPL_ERR_GPU_INTERNAL;
}


int MPL_gpu_malloc(void **ptr, size_t size, int devid)
{
    int ret;
    ze_device_mem_alloc_desc_t device_desc;
    MPL_gpu_device_handle_t device;

    device_desc.flags = ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT;
    device_desc.ordinal = 0;    /* 0 is correct value for GPU */
    device_desc.version = ZE_DEVICE_MEM_ALLOC_DESC_VERSION_CURRENT;

    ret = get_ze_device_from_id(global_ze_driver_handle, devid, &device);

    /* FIXME: set different alignment? e.g., some ze tests use 4096u */
    ret = zeDriverAllocDeviceMem(global_ze_driver_handle, &device_desc,
                                 size, 1 /*alignment */ , device, ptr);

    ZE_ERR_CHECK(ret);
  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_malloc_host(void **ptr, size_t size)
{
    int ret;
    ze_device_mem_alloc_desc_t device_desc;
    device_desc.flags = ZE_HOST_MEM_ALLOC_FLAG_DEFAULT;
    device_desc.version = ZE_HOST_MEM_ALLOC_DESC_VERSION_CURRENT;

    /* FIXME: set different alignment? e.g., some ze tests use 4096u */
    ret = zeDriverAllocHostMem(global_ze_driver_handle, &device_desc, size, 1 /*alignment */ , ptr);
    ZE_ERR_CHECK(ret);
  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_free(void *ptr)
{
    int ret;
    ret = zeDriverFreeMem(global_ze_driver_handle, ptr);
    ZE_ERR_CHECK(ret);
  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_free_host(void *ptr)
{
    int ret;
    ret = zeDriverFreeMem(global_ze_driver_handle, ptr);
    ZE_ERR_CHECK(ret);
  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
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
