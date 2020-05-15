/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpl.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#ifdef MPL_HAVE_ZE

ze_driver_handle_t global_ze_driver_handle;
int gpu_ze_init_driver();

#define ZE_ERR_CHECK(ret) \
    do { \
        if (unlikely((ret) != ZE_RESULT_SUCCESS)) \
            goto fn_fail; \
    } while (0)

int MPL_gpu_init()
{
    int ret_error;
    ret_error = gpu_ze_init_driver();
    if (ret_error != MPL_SUCCESS)
        goto fn_fail;

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return ret_error;
}

/* Loads a global ze driver */
int gpu_ze_init_driver()
{
    uint32_t driver_count = 0;
    ze_result_t ret;
    int ret_error = MPL_SUCCESS;
    ze_driver_handle_t *all_drivers;

    ret = zeDriverGet(&driver_count, NULL);
    ZE_ERR_CHECK(ret);
    if (driver_count == 0) {
        goto fn_fail;
    }

    all_drivers = MPL_malloc(driver_count * sizeof(ze_driver_handle_t), MPL_MEM_OTHER);
    if (all_drivers == NULL) {
        ret_error = MPL_ERR_GPU_NOMEM;
        goto fn_fail;
    }
    ret = zeDriverGet(&driver_count, all_drivers);
    ZE_ERR_CHECK(ret);

    int i, d;
    ze_device_handle_t *all_devices = NULL;
    /* Find a driver instance with a GPU device */
    for (i = 0; i < driver_count; ++i) {
        uint32_t device_count = 0;
        ret = zeDeviceGet(all_drivers[i], &device_count, NULL);
        ZE_ERR_CHECK(ret);
        all_devices = MPL_malloc(device_count * sizeof(ze_device_handle_t), MPL_MEM_OTHER);
        if (all_devices == NULL) {
            ret_error = MPL_ERR_GPU_NOMEM;
            goto fn_fail;
        }
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
        all_devices = NULL;
        if (NULL != global_ze_driver_handle) {
            break;
        }
    }

  fn_exit:
    MPL_free(all_drivers);
    return ret_error;
  fn_fail:
    MPL_free(all_devices);
    /* If error code is already set, preserve it */
    if (ret_error == MPL_SUCCESS)
        ret_error = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_finalize()
{
    return MPL_SUCCESS;
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
