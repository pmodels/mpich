/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpl.h"
#include "mpl_str.h"
#include <dlfcn.h>
#include <assert.h>
#ifdef MPL_HAVE_HIP
#define HIP_ERR_CHECK(ret) if (unlikely((ret) != hipSuccess)) goto fn_fail
#define HI_ERR_CHECK(ret) if (unlikely((ret) != HIP_SUCCESS)) goto fn_fail

static int gpu_initialized = 0;
static int device_count = -1;
static int max_dev_id = -1;
static char **device_list = NULL;
#define MAX_GPU_STR_LEN 256
static char affinity_env[MAX_GPU_STR_LEN] = { 0 };

static int *local_to_global_map;        /* [device_count] */
static int *global_to_local_map;        /* [max_dev_id + 1]   */

int MPL_gpu_get_dev_count(int *dev_cnt, int *dev_id, int *subdevice_id)
{
    int ret = MPL_SUCCESS;
    if (!gpu_initialized) {
        ret = MPL_gpu_init(0);
    }

    *dev_cnt = device_count;
    *dev_id = max_dev_id;
    *subdevice_id = 0;
    return ret;
}

int MPL_gpu_get_dev_list(int *dev_count, char ***dev_list, bool is_subdev)
{
    int ret = MPL_SUCCESS;
    if (!gpu_initialized) {
        ret = MPL_gpu_init(0);
    }

    device_list = (char **) MPL_malloc(device_count * sizeof(char *), MPL_MEM_OTHER);
    assert(device_list);

    for (int i = 0; i < device_count; ++i) {
        int str_len = snprintf(NULL, 0, "%d", i);
        device_list[i] = (char *) MPL_malloc((str_len + 1) * sizeof(char *), MPL_MEM_OTHER);
        snprintf(device_list[i], str_len + 1, "%d", i);
    }

    *dev_list = device_list;
    return ret;
}

int MPL_gpu_dev_affinity_to_env(int dev_count, char **dev_list, char **env)
{
    int ret = MPL_SUCCESS;
    if (dev_count == 0) {
        snprintf(affinity_env, 3, "-1");
    } else {
        int str_offset = 0;
        for (int i = 0; i < dev_count; ++i) {
            if (i) {
                MPL_strncpy(affinity_env + str_offset, ",", MAX_GPU_STR_LEN - str_offset);
                str_offset++;
            }
            MPL_strncpy(affinity_env + str_offset, dev_list[i], MAX_GPU_STR_LEN - str_offset);
            str_offset += strlen(dev_list[i]);
        }
    }
    *env = affinity_env;
    return ret;
}

#ifdef MPL_HIP_USE_MEMORYTYPE
/* pre-ROCm 6.0 */
#define DEVICE_ATTR_TYPE attr->device_attr.memoryType
int MPL_gpu_query_pointer_attr(const void *ptr, MPL_pointer_attr_t * attr)
{
    int mpl_err = MPL_SUCCESS;
    hipError_t ret;
    ret = hipPointerGetAttributes(&attr->device_attr, ptr);
    if (ret == hipSuccess) {
        switch (DEVICE_ATTR_TYPE) {
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
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}
#else
/* post-ROCm 6.0 */
#define DEVICE_ATTR_TYPE attr->device_attr.type
int MPL_gpu_query_pointer_attr(const void *ptr, MPL_pointer_attr_t * attr)
{
    int mpl_err = MPL_SUCCESS;
    hipError_t ret;
    ret = hipPointerGetAttributes(&attr->device_attr, ptr);
    if (ret == hipSuccess) {
        switch (DEVICE_ATTR_TYPE) {
            case hipMemoryTypeHost:
                attr->type = MPL_GPU_POINTER_REGISTERED_HOST;
                attr->device = attr->device_attr.device;
                break;
            case hipMemoryTypeDevice:
                attr->type = MPL_GPU_POINTER_DEV;
                attr->device = attr->device_attr.device;
                break;
            case hipMemoryTypeUnregistered:
                attr->type = MPL_GPU_POINTER_UNREGISTERED_HOST;
                attr->device = -1;
                break;
            case hipMemoryTypeManaged:
                attr->type = MPL_GPU_POINTER_MANAGED;
                attr->device = attr->device_attr.device;
                break;
        }
    } else {
        goto fn_fail;
    }

  fn_exit:
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}
#endif


int MPL_gpu_attr_is_dev(MPL_pointer_attr_t * attr)
{
    return attr->type == MPL_GPU_POINTER_DEV;
}

int MPL_gpu_attr_is_strict_dev(MPL_pointer_attr_t * attr)
{
    return attr->type == MPL_GPU_POINTER_DEV;
}

int MPL_gpu_query_is_same_dev(int dev1, int dev2)
{
    return dev1 == dev2;
}

int MPL_gpu_ipc_handle_create(const void *ptr, MPL_gpu_device_attr * ptr_attr,
                              MPL_gpu_ipc_mem_handle_t * ipc_handle)
{
    int mpl_err = MPL_SUCCESS;
    hipError_t ret;

    ret = hipIpcGetMemHandle(&ipc_handle->handle, (void *) ptr);
    HIP_ERR_CHECK(ret);

    ret = hipPointerGetAttribute(&ipc_handle->id, HIP_POINTER_ATTRIBUTE_BUFFER_ID,
                                 (hipDeviceptr_t) ptr);
    HIP_ERR_CHECK(ret);

  fn_exit:
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_ipc_handle_destroy(const void *ptr, MPL_pointer_attr_t * gpu_attr)
{
    return MPL_SUCCESS;
}

int MPL_gpu_ipc_handle_map(MPL_gpu_ipc_mem_handle_t * ipc_handle, int dev_id, void **ptr)
{
    int mpl_err = MPL_SUCCESS;
    hipError_t ret;
    int prev_devid;

    hipGetDevice(&prev_devid);
    hipSetDevice(dev_id);
    ret = hipIpcOpenMemHandle(ptr, ipc_handle->handle, hipIpcMemLazyEnablePeerAccess);
    HIP_ERR_CHECK(ret);

  fn_exit:
    hipSetDevice(prev_devid);
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_ipc_handle_unmap(void *ptr)
{
    int mpl_err = MPL_SUCCESS;
    hipError_t ret;
    ret = hipIpcCloseMemHandle(ptr);
    HIP_ERR_CHECK(ret);

  fn_exit:
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

bool MPL_gpu_ipc_handle_is_valid(MPL_gpu_ipc_mem_handle_t * handle, void *ptr)
{
    hipError_t ret;
    uint32_t buffer_id;

    ret = hipPointerGetAttribute(&buffer_id, HIP_POINTER_ATTRIBUTE_BUFFER_ID, (hipDeviceptr_t) ptr);
    assert(ret == hipSuccess);

    return buffer_id == handle->id;
}

int MPL_gpu_malloc_host(void **ptr, size_t size)
{
    int mpl_err = MPL_SUCCESS;
    hipError_t ret;
    ret = hipHostMalloc(ptr, size, hipHostMallocDefault);
    HIP_ERR_CHECK(ret);

  fn_exit:
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_free_host(void *ptr)
{
    int mpl_err = MPL_SUCCESS;
    hipError_t ret;
    ret = hipHostFree(ptr);
    HIP_ERR_CHECK(ret);

  fn_exit:
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_register_host(const void *ptr, size_t size)
{
    int mpl_err = MPL_SUCCESS;
    hipError_t ret;
    ret = hipHostRegister((void *) ptr, size, hipHostRegisterDefault);
    HIP_ERR_CHECK(ret);

  fn_exit:
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_unregister_host(const void *ptr)
{
    int mpl_err = MPL_SUCCESS;
    hipError_t ret;
    ret = hipHostUnregister((void *) ptr);
    HIP_ERR_CHECK(ret);

  fn_exit:
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_malloc(void **ptr, size_t size, MPL_gpu_device_handle_t h_device)
{
    int mpl_err = MPL_SUCCESS;
    int prev_devid;
    hipError_t ret;
    hipGetDevice(&prev_devid);
    hipSetDevice(h_device);
    ret = hipMalloc(ptr, size);
    HIP_ERR_CHECK(ret);

  fn_exit:
    hipSetDevice(prev_devid);
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_free(void *ptr)
{
    int mpl_err = MPL_SUCCESS;
    hipError_t ret;
    ret = hipFree(ptr);
    HIP_ERR_CHECK(ret);

  fn_exit:
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_init(int debug_summary)
{
    int mpl_err = MPL_SUCCESS;
    hipError_t ret = hipGetDeviceCount(&device_count);
    if (ret == hipErrorNoDevice) {
        goto fn_exit;
    }
    HIP_ERR_CHECK(ret);

    MPL_gpu_info.debug_summary = debug_summary;
    MPL_gpu_info.enable_ipc = true;
    MPL_gpu_info.ipc_handle_type = MPL_GPU_IPC_HANDLE_SHAREABLE;
    MPL_gpu_info.specialized_cache = false;

    char *visible_devices = getenv("HIP_VISIBLE_DEVICES");
    if (visible_devices) {
        local_to_global_map = MPL_malloc(device_count * sizeof(int), MPL_MEM_OTHER);

        uintptr_t len = strlen(visible_devices);
        char *devices = MPL_malloc(len + 1, MPL_MEM_OTHER);
        char *free_ptr = devices;
        memcpy(devices, visible_devices, len + 1);
        for (int i = 0; i < device_count; i++) {
            int global_dev_id;
            char *tmp = strtok(devices, ",");
            assert(tmp);
            local_to_global_map[i] = atoi(tmp);
            if (max_dev_id < local_to_global_map[i]) {
                max_dev_id = local_to_global_map[i];
            }
            devices = NULL;
        }
        MPL_free(free_ptr);

        global_to_local_map = MPL_malloc((max_dev_id + 1) * sizeof(int), MPL_MEM_OTHER);
        for (int i = 0; i < max_dev_id + 1; i++) {
            global_to_local_map[i] = -1;
        }
        for (int i = 0; i < device_count; i++) {
            global_to_local_map[local_to_global_map[i]] = i;
        }
    } else {
        local_to_global_map = MPL_malloc(device_count * sizeof(int), MPL_MEM_OTHER);
        global_to_local_map = MPL_malloc(device_count * sizeof(int), MPL_MEM_OTHER);
        for (int i = 0; i < device_count; i++) {
            local_to_global_map[i] = i;
            global_to_local_map[i] = i;
        }
        max_dev_id = device_count - 1;
    }

    gpu_initialized = 1;

    if (MPL_gpu_info.debug_summary) {
        printf("==== GPU Init (HIP) ====\n");
        printf("device_count: %d\n", device_count);
        if (visible_devices) {
            printf("HIP_VISIBLE_DEVICES: %s\n", visible_devices);
        }
        printf("=========================\n");
    }

  fn_exit:
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_finalize(void)
{
    if (device_count <= 0) {
        goto fn_exit;
    }

    MPL_free(local_to_global_map);
    MPL_free(global_to_local_map);

    /* Reset initialization state */
    gpu_initialized = 0;

  fn_exit:
    return MPL_SUCCESS;
}

int MPL_gpu_global_to_local_dev_id(int global_dev_id)
{
    assert(global_dev_id <= max_dev_id);
    return global_to_local_map[global_dev_id];
}

int MPL_gpu_local_to_global_dev_id(int local_dev_id)
{
    assert(local_dev_id < device_count);
    return local_to_global_map[local_dev_id];
}

int MPL_gpu_get_dev_id_from_attr(MPL_pointer_attr_t * attr)
{
    return attr->device;
}

int MPL_gpu_get_root_device(int dev_id)
{
    return dev_id;
}

int MPL_gpu_get_buffer_bounds(const void *ptr, void **pbase, uintptr_t * len)
{
    int mpl_err = MPL_SUCCESS;
    hipError_t hiret;

    hiret = hipMemGetAddressRange((hipDeviceptr_t *) pbase, (size_t *) len, (hipDeviceptr_t) ptr);
    HI_ERR_CHECK(hiret);

  fn_exit:
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_free_hook_register(void (*free_hook) (void *dptr))
{
    /* free hooks not used */
    return MPL_SUCCESS;
}

int MPL_gpu_fast_memcpy(void *src, MPL_pointer_attr_t * src_attr, void *dest,
                        MPL_pointer_attr_t * dest_attr, size_t size)
{
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_imemcpy(void *dest_ptr, void *src_ptr, size_t size, int dev,
                    MPL_gpu_copy_direction_t dir, MPL_gpu_engine_type_t engine_type,
                    MPL_gpu_request * req, bool commit)
{
    return MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_test(MPL_gpu_request * req, int *completed)
{
    return MPL_ERR_GPU_INTERNAL;
}

/* ---- */
struct stream_callback_wrapper {
    MPL_gpu_hostfn fn;
    void *data;
};

static void stream_callback(hipStream_t stream, hipError_t status, void *wrapper_data)
{
    struct stream_callback_wrapper *p = wrapper_data;
    p->fn(p->data);
    MPL_free(p);
}

int MPL_gpu_launch_hostfn(hipStream_t stream, MPL_gpu_hostfn fn, void *data)
{
    hipError_t result;
    struct stream_callback_wrapper *p =
        MPL_malloc(sizeof(struct stream_callback_wrapper), MPL_MEM_OTHER);
    p->fn = fn;
    p->data = data;
    result = hipStreamAddCallback(stream, stream_callback, p, 0);
    return result;
}

/* ---- */
bool MPL_gpu_stream_is_valid(MPL_gpu_stream_t stream)
{
    return true;
}

void MPL_gpu_enqueue_trigger(MPL_gpu_event_t * var, MPL_gpu_stream_t stream)
{
    assert(0);
}

void MPL_gpu_enqueue_wait(MPL_gpu_event_t * var, MPL_gpu_stream_t stream)
{
    assert(0);
}

void MPL_gpu_event_init_count(MPL_gpu_event_t * var, int count)
{
    *var = count;
}

void MPL_gpu_event_complete(MPL_gpu_event_t * var)
{
    *var -= 1;
}

bool MPL_gpu_event_is_complete(MPL_gpu_event_t * var)
{
    return (*var) <= 0;
}

#endif /* MPL_HAVE_HIP */
