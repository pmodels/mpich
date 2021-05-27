/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpl.h"
#include <dlfcn.h>
#include <assert.h>
#ifdef MPL_HAVE_HIP
#define HIP_ERR_CHECK(ret) if (unlikely((ret) != hipSuccess)) goto fn_fail
#define HI_ERR_CHECK(ret) if (unlikely((ret) != HIP_SUCCESS)) goto fn_fail

typedef struct gpu_free_hook {
    void (*free_hook) (void *dptr);
    struct gpu_free_hook *next;
} gpu_free_hook_s;

static int gpu_initialized = 0;
static int device_count = -1;
static int max_dev_id = -1;

static gpu_free_hook_s *free_hook_chain = NULL;

static hipError_t(*sys_hipFree) (void *dptr);

static int gpu_mem_hook_init();

int MPL_gpu_get_dev_count(int *dev_cnt, int *dev_id)
{
    int ret = MPL_SUCCESS;
    if (!gpu_initialized) {
        ret = MPL_gpu_init();
    }

    *dev_cnt = device_count;
    *dev_id = max_dev_id;
    return ret;
}

int MPL_gpu_query_pointer_attr(const void *ptr, MPL_pointer_attr_t * attr)
{
    hipError_t ret;
    ret = hipPointerGetAttributes(&attr->device_attr, ptr);
    if (ret == hipSuccess) {
        switch (attr->device_attr.memoryType) {
            case hipMemoryTypeHost:
                attr->type = MPL_GPU_POINTER_REGISTERED_HOST;
                attr->device = attr->device_attr.device;
                break;
            case hipMemoryTypeDevice:
                attr->type = MPL_GPU_POINTER_DEV;
                attr->device = attr->device_attr.device;
        }
        if (attr->device_attr.isManaged) {
            attr->type = MPL_GPU_POINTER_MANAGED;
            attr->device = attr->device_attr.device;
        }
    } else if (ret == hipErrorInvalidValue) {
        attr->type = MPL_GPU_POINTER_UNREGISTERED_HOST;
        attr->device = -1;
    } else {
        goto fn_fail;
    }

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_ipc_handle_create(const void *ptr, MPL_gpu_ipc_mem_handle_t * ipc_handle)
{
    hipError_t ret;

    ret = hipIpcGetMemHandle(ipc_handle, (void *) ptr);
    HIP_ERR_CHECK(ret);

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_ipc_handle_map(MPL_gpu_ipc_mem_handle_t ipc_handle, MPL_gpu_device_handle_t dev_handle,
                           void **ptr)
{
    hipError_t ret;
    int prev_devid;

    hipGetDevice(&prev_devid);
    hipSetDevice(dev_handle);
    ret = hipIpcOpenMemHandle(ptr, ipc_handle, hipIpcMemLazyEnablePeerAccess);
    HIP_ERR_CHECK(ret);

  fn_exit:
    hipSetDevice(prev_devid);
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_ipc_handle_unmap(void *ptr)
{
    hipError_t ret;
    ret = hipIpcCloseMemHandle(ptr);
    HIP_ERR_CHECK(ret);

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_malloc_host(void **ptr, size_t size)
{
    hipError_t ret;
    ret = hipHostMalloc(ptr, size, hipHostMallocDefault);
    HIP_ERR_CHECK(ret);

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_free_host(void *ptr)
{
    hipError_t ret;
    ret = hipHostFree(ptr);
    HIP_ERR_CHECK(ret);

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_register_host(const void *ptr, size_t size)
{
    hipError_t ret;
    ret = hipHostRegister((void *) ptr, size, hipHostRegisterDefault);
    HIP_ERR_CHECK(ret);

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_unregister_host(const void *ptr)
{
    hipError_t ret;
    ret = hipHostUnregister((void *) ptr);
    HIP_ERR_CHECK(ret);

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_malloc(void **ptr, size_t size, MPL_gpu_device_handle_t h_device)
{
    int mpl_errno = MPL_SUCCESS;
    int prev_devid;
    hipError_t ret;
    hipGetDevice(&prev_devid);
    hipSetDevice(h_device);
    ret = hipMalloc(ptr, size);
    HIP_ERR_CHECK(ret);

  fn_exit:
    hipSetDevice(prev_devid);
    return mpl_errno;
  fn_fail:
    mpl_errno = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_free(void *ptr)
{
    hipError_t ret;
    ret = hipFree(ptr);
    HIP_ERR_CHECK(ret);

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_init()
{
    hipError_t ret = hipGetDeviceCount(&device_count);
    HIP_ERR_CHECK(ret);

    char *visible_devices = getenv("HIP_VISIBLE_DEVICES");
    if (visible_devices) {
        uintptr_t len = strlen(visible_devices);
        char *devices = MPL_malloc(len + 1, MPL_MEM_OTHER);
        char *free_ptr = devices;
        memcpy(devices, visible_devices, len + 1);
        for (int i = 0; i < device_count; i++) {
            int global_dev_id;
            char *tmp = strtok(devices, ",");
            assert(tmp);
            global_dev_id = atoi(tmp);
            if (global_dev_id > max_dev_id)
                max_dev_id = global_dev_id;
            devices = NULL;
        }
        MPL_free(free_ptr);
    } else {
        max_dev_id = device_count - 1;
    }

    /* gpu shm module would cache gpu handle to accelerate intra-node
     * communication; we must register hooks for memory-related functions
     * in hip, such as hipFree, to track user behaviors on
     * the memory buffer and invalidate cached handle/buffer respectively
     * for result correctness. */
    gpu_mem_hook_init();
    gpu_initialized = 1;

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_finalize()
{
    gpu_free_hook_s *prev;
    while (free_hook_chain) {
        prev = free_hook_chain;
        free_hook_chain = free_hook_chain->next;
        MPL_free(prev);
    }
    return MPL_SUCCESS;
}

int MPL_gpu_get_dev_id(MPL_gpu_device_handle_t dev_handle, int *dev_id)
{
    *dev_id = dev_handle;
    return MPL_SUCCESS;
}

int MPL_gpu_get_dev_handle(int dev_id, MPL_gpu_device_handle_t * dev_handle)
{
    *dev_handle = dev_id;
    return MPL_SUCCESS;
}

int MPL_gpu_get_global_dev_ids(int *global_ids, int count)
{
    char *visible_devices = getenv("HIP_VISIBLE_DEVICES");

    if (visible_devices) {
        uintptr_t len = strlen(visible_devices);
        char *devices = MPL_malloc(len + 1, MPL_MEM_OTHER);
        char *free_ptr = devices;
        memcpy(devices, visible_devices, len + 1);
        for (int i = 0; i < count; i++) {
            char *tmp = strtok(devices, ",");
            assert(tmp);
            global_ids[i] = atoi(tmp);
            devices = NULL;
        }
        MPL_free(free_ptr);
    } else {
        for (int i = 0; i < count; i++) {
            global_ids[i] = i;
        }
    }

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_get_buffer_bounds(const void *ptr, void **pbase, uintptr_t * len)
{
    hipError_t hiret;

    hiret = hipMemGetAddressRange((hipDeviceptr_t *) pbase, (size_t *) len, (hipDeviceptr_t) ptr);
    HI_ERR_CHECK(hiret);

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

static void gpu_free_hooks_cb(void *dptr)
{
    gpu_free_hook_s *current = free_hook_chain;
    if (dptr != NULL) {
        /* we call gpu hook only when dptr != NULL */
        while (current) {
            current->free_hook(dptr);
            current = current->next;
        }
    }
    return;
}

static int gpu_mem_hook_init()
{
    void *libhip_handle;
    void *libhiprt_handle;

    libhip_handle = dlopen("libamdhip64.so", RTLD_LAZY | RTLD_GLOBAL);
    assert(libhip_handle);
    sys_hipFree = (void *) dlsym(libhip_handle, "hipFree");
    assert(sys_hipFree);
    return MPL_SUCCESS;
}

int MPL_gpu_free_hook_register(void (*free_hook) (void *dptr))
{
    gpu_free_hook_s *hook_obj = MPL_malloc(sizeof(gpu_free_hook_s), MPL_MEM_OTHER);
    assert(hook_obj);
    hook_obj->free_hook = free_hook;
    hook_obj->next = NULL;
    if (!free_hook_chain)
        free_hook_chain = hook_obj;
    else {
        hook_obj->next = free_hook_chain;
        free_hook_chain = hook_obj;
    }

    return MPL_SUCCESS;
}

hipError_t hipFree(void *dptr)
{
    hipError_t result;
    gpu_free_hooks_cb(dptr);
    result = sys_hipFree(dptr);
    return result;
}
#endif /* MPL_HAVE_HIP */
