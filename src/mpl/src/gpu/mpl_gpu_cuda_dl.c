/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* MPL_gpu_ functions for a CUDA build with --enable-dlopen. The functions that
 * call CUDA are in a module that is loaded at runtime, see mpl_gpu_cuda_ops.h.
 * If the module cannot be loaded, e.g. on a node without the CUDA runtime,
 * the functions behave the same as a build without GPU support. */

#include "mpl.h"
#include "mpl_str.h"
#include "mpl_gpu_cuda_ops.h"
#include <dlfcn.h>
#include <assert.h>

#define MAX_GPU_STR_LEN 256
#define MAX_PATH_LEN 4096

static const MPL_gpu_cuda_ops_t *ops = NULL;
static bool load_attempted = false;
static char module_path[MAX_PATH_LEN] = { 0 };
static char load_error[MAX_GPU_STR_LEN] = { 0 };

static char affinity_env[MAX_GPU_STR_LEN] = { 0 };

/* The module is installed in the same directory as the library (or the
 * executable) that contains this function. It is the only place we look. */
static void load_module(void)
{
    load_attempted = true;

    Dl_info info;
    if (!dladdr((void *) &load_module, &info) || info.dli_fname == NULL) {
        MPL_strncpy(load_error, "unable to locate the calling library", MAX_GPU_STR_LEN);
        return;
    }

    const char *slash = strrchr(info.dli_fname, '/');
    int dir_len = slash ? (int) (slash - info.dli_fname) : 1;
    snprintf(module_path, MAX_PATH_LEN, "%.*s/%s", dir_len, slash ? info.dli_fname : ".",
             MPL_CUDA_MODULE_NAME);

    void *handle = dlopen(module_path, RTLD_NOW | RTLD_LOCAL);
    if (handle == NULL) {
        const char *err = dlerror();
        MPL_strncpy(load_error, err ? err : "dlopen failed", MAX_GPU_STR_LEN);
        return;
    }

    const MPL_gpu_cuda_ops_t *p;
    p = (const MPL_gpu_cuda_ops_t *) dlsym(handle, MPL_GPU_CUDA_OPS_SYMBOL);
    if (p == NULL) {
        MPL_strncpy(load_error, "symbol " MPL_GPU_CUDA_OPS_SYMBOL " not found", MAX_GPU_STR_LEN);
        dlclose(handle);
        return;
    }

    /* the module stays loaded for the lifetime of the process */
    p->set_gpu_info(&MPL_gpu_info);
    ops = p;
}

int MPL_gpu_init(int debug_summary)
{
    if (!load_attempted) {
        load_module();
        if (debug_summary) {
            if (ops) {
                printf("CUDA module loaded: %s\n", module_path);
            } else {
                printf("CUDA module not loaded, GPU support is disabled: %s\n", load_error);
            }
        }
    }

    return ops ? ops->init(debug_summary) : MPL_SUCCESS;
}

int MPL_gpu_finalize(void)
{
    return ops ? ops->finalize() : MPL_SUCCESS;
}

int MPL_gpu_get_dev_count(int *dev_cnt, int *dev_id, int *subdevice_id)
{
    if (!load_attempted) {
        load_module();
    }

    if (ops) {
        return ops->get_dev_count(dev_cnt, dev_id, subdevice_id);
    }

    *dev_cnt = *dev_id = *subdevice_id = -1;
    return MPL_SUCCESS;
}

int MPL_gpu_get_dev_list(int *dev_count, char ***dev_list, bool is_subdev)
{
    int device_count, max_dev_id, max_subdev_id;
    int ret = MPL_gpu_get_dev_count(&device_count, &max_dev_id, &max_subdev_id);

    if (device_count <= 0) {
        *dev_list = NULL;
        return ret;
    }

    char **device_list = (char **) MPL_malloc(device_count * sizeof(char *), MPL_MEM_OTHER);
    assert(device_list != NULL);

    for (int i = 0; i < device_count; ++i) {
        int str_len = snprintf(NULL, 0, "%d", i);
        device_list[i] = (char *) MPL_malloc((str_len + 1) * sizeof(char *), MPL_MEM_OTHER);
        snprintf(device_list[i], str_len + 1, "%d", i);
    }

    *dev_count = device_count;
    *dev_list = device_list;
    return ret;
}

int MPL_gpu_dev_affinity_to_env(int dev_count, char **dev_list, char **env)
{
    int ret = MPL_SUCCESS;

    if (!ops) {
        *env = NULL;
        return ret;
    }

    memset(affinity_env, 0, MAX_GPU_STR_LEN);
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

int MPL_gpu_query_pointer_attr(const void *ptr, MPL_pointer_attr_t * attr)
{
    if (ops) {
        return ops->query_pointer_attr(ptr, attr);
    }

    attr->type = MPL_GPU_POINTER_UNREGISTERED_HOST;
    attr->device = -1;
    return MPL_SUCCESS;
}

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
    return ops ? ops->ipc_handle_create(ptr, ptr_attr, ipc_handle) : MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_ipc_handle_destroy(MPL_gpu_ipc_mem_handle_t * mpl_ipc_handle)
{
    return MPL_SUCCESS;
}

int MPL_gpu_ipc_handle_map(MPL_gpu_ipc_mem_handle_t * ipc_handle, int dev_id,
                           MPL_gpu_map_t * map_out, bool is_mmap, size_t len)
{
    return ops ? ops->ipc_handle_map(ipc_handle, dev_id, map_out, is_mmap, len) :
        MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_ipc_handle_unmap(MPL_gpu_map_t * map_ptr)
{
    return ops ? ops->ipc_handle_unmap(map_ptr) : MPL_ERR_GPU_INTERNAL;
}

MPL_gpu_buffer_id_t MPL_gpu_get_buffer_id(void *ptr)
{
    return ops ? ops->get_buffer_id(ptr) : 0;
}

bool MPL_gpu_ipc_handle_is_valid(MPL_gpu_ipc_mem_handle_t * handle, void *ptr)
{
    return ops ? ops->ipc_handle_is_valid(handle, ptr) : false;
}

int MPL_gpu_malloc_host(void **ptr, size_t size)
{
    if (ops) {
        return ops->malloc_host(ptr, size);
    }

    *ptr = MPL_malloc(size, MPL_MEM_BUFFER);
    return MPL_SUCCESS;
}

int MPL_gpu_free_host(void *ptr)
{
    if (ops) {
        return ops->free_host(ptr);
    }

    MPL_free(ptr);
    return MPL_SUCCESS;
}

int MPL_gpu_register_host(const void *ptr, size_t size)
{
    return ops ? ops->register_host(ptr, size) : MPL_SUCCESS;
}

int MPL_gpu_unregister_host(const void *ptr)
{
    return ops ? ops->unregister_host(ptr) : MPL_SUCCESS;
}

int MPL_gpu_malloc(void **ptr, size_t size, MPL_gpu_device_handle_t h_device)
{
    return ops ? ops->malloc(ptr, size, h_device) : MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_free(void *ptr)
{
    return ops ? ops->free(ptr) : MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_global_to_local_dev_id(int global_dev_id)
{
    return ops ? ops->global_to_local_dev_id(global_dev_id) : -1;
}

int MPL_gpu_local_to_global_dev_id(int local_dev_id)
{
    return ops ? ops->local_to_global_dev_id(local_dev_id) : -1;
}

int MPL_gpu_get_dev_id_from_attr(MPL_pointer_attr_t * attr)
{
    return attr->device;
}

int MPL_gpu_get_root_device(int dev_id)
{
    return dev_id;
}

int MPL_gpu_get_current_dev_bdf(int *domain, int *bus, int *dev, int *func)
{
    return ops ? ops->get_current_dev_bdf(domain, bus, dev, func) : MPL_ERR_GPU_INTERNAL;
}

int MPL_gpu_get_buffer_bounds(const void *ptr, void **pbase, uintptr_t * len)
{
    return ops ? ops->get_buffer_bounds(ptr, pbase, len) : MPL_SUCCESS;
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

int MPL_gpu_launch_hostfn(MPL_gpu_stream_t stream, MPL_gpu_hostfn fn, void *data)
{
    return ops ? ops->launch_hostfn(stream, fn, data) : -1;
}

bool MPL_gpu_stream_is_valid(MPL_gpu_stream_t stream)
{
    return ops ? ops->stream_is_valid(stream) : false;
}

void MPL_gpu_enqueue_trigger(MPL_gpu_event_t * var, MPL_gpu_stream_t stream)
{
    assert(ops);
    ops->enqueue_trigger(var, stream);
}

void MPL_gpu_enqueue_wait(MPL_gpu_event_t * var, MPL_gpu_stream_t stream)
{
    assert(ops);
    ops->enqueue_wait(var, stream);
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
