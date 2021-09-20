/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpl.h"
#include <assert.h>
#include <dlfcn.h>

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#ifdef MPL_HAVE_ZE
#include <dirent.h>
#if defined(MPL_HAVE_DRM_I915_DRM_H)
#include "drm/i915_drm.h"
#define MPL_ENABLE_DRMFD
#elif defined(MPL_HAVE_LIBDRM_I915_DRM_H)
#include "libdrm/i915_drm.h"
#define MPL_ENABLE_DRMFD
#endif
#include <sys/ioctl.h>
#include <sys/syscall.h>        /* Definition of SYS_* constants */
#include "uthash.h"

static int gpu_initialized = 0;
static uint32_t device_count;
static uint32_t max_dev_id;
static char **device_list = NULL;
#define MAX_GPU_STR_LEN 256
static char affinity_env[MAX_GPU_STR_LEN] = { 0 };

static int *local_to_global_map;        /* [global_ze_device_count] */
static int *global_to_local_map;        /* [max_dev_id + 1]   */

/* Maps a subdevice id to the upper device id, specifically for indexing into shared_device_fds */
static int *subdevice_map = NULL;
/* Keeps the subdevice count for all locally visible devices */
static uint32_t *subdevice_count = NULL;

static int shared_device_fd_count = 0;
static int *shared_device_fds = NULL;

/* pidfd */
#ifndef __NR_pidfd_open
#define __NR_pidfd_open 434     /* System call # on most architectures */
#endif
#ifndef __NR_pidfd_getfd
#define __NR_pidfd_getfd 438    /* System call # on most architectures */
#endif

typedef struct {
    const void *ptr;
    int dev_id;
    int handle;
    UT_hash_handle hh;
} MPL_ze_gem_hash_entry_t;

static MPL_ze_gem_hash_entry_t *gem_hash = NULL;

typedef struct {
    int fd;
    pid_t pid;
    int dev_id;
} fd_pid_t;

typedef struct gpu_free_hook {
    void (*free_hook) (void *dptr);
    struct gpu_free_hook *next;
} gpu_free_hook_s;

pid_t mypid;

/* Level-zero API v1.0:
 * http://spec.oneapi.com/level-zero/latest/index.html
 */
ze_driver_handle_t global_ze_driver_handle;
/* global_ze_devices_handle contains all devices and subdevices. Indices [0, device_count) are
 * devices while the rest are subdevices. Keeping them all in the same array allows for easy
 * comparison of device handle when a device id is passed from the upper layer. The only time it
 * matters if we have a subdevice vs a device is when with creating or mapping an ipc handle. In
 * situations, we use the subdevice_map to find the upper device id for indexing into
 * shared_device_fds, since these are only opened on the upper devices. */
ze_device_handle_t *global_ze_devices_handle = NULL;
ze_context_handle_t global_ze_context;
uint32_t global_ze_device_count;        /* This counts both devices and subdevices */
static int gpu_ze_init_driver(void);
static int fd_to_handle(int dev_fd, int fd, int *handle);
static int handle_to_fd(int dev_fd, int handle, int *fd);
static int close_handle(int dev_fd, int handle);

static gpu_free_hook_s *free_hook_chain = NULL;

static ze_result_t ZE_APICALL(*sys_zeMemFree) (ze_context_handle_t hContext, void *dptr) = NULL;

static int gpu_mem_hook_init(void);

#define ZE_ERR_CHECK(ret) \
    do { \
        if (unlikely((ret) != ZE_RESULT_SUCCESS)) \
            goto fn_fail; \
    } while (0)

int MPL_gpu_get_dev_count(int *dev_cnt, int *dev_id)
{
    int ret = MPL_SUCCESS;
    if (!gpu_initialized) {
        ret = MPL_gpu_init(0);
    }

    *dev_cnt = global_ze_device_count;
    *dev_id = max_dev_id;
    return ret;
}

int MPL_gpu_get_dev_list(int *dev_count, char ***dev_list, bool is_subdev)
{
    int ret = MPL_SUCCESS;
    if (!gpu_initialized) {
        ret = MPL_gpu_init(0);
    }

    if (!is_subdev) {
        device_list = (char **) MPL_malloc(device_count * sizeof(char *), MPL_MEM_OTHER);
        assert(device_list != NULL);

        for (int i = 0; i < device_count; ++i) {
            int str_len = snprintf(NULL, 0, "%d", i);
            device_list[i] = (char *) MPL_malloc((str_len + 1) * sizeof(char *), MPL_MEM_OTHER);
            sprintf(device_list[i], "%d", i);
        }

        *dev_count = device_count;
        *dev_list = device_list;
    } else {
        int driver_count = 0;
        int *subdev_counts = NULL;
        int total_subdev_count = 0;
        ze_driver_handle_t *all_drivers = NULL;

        ret = zeDriverGet(&driver_count, NULL);
        assert(ret == ZE_RESULT_SUCCESS);
        assert(device_count);

        all_drivers = MPL_malloc(driver_count * sizeof(ze_driver_handle_t), MPL_MEM_OTHER);
        assert(all_drivers);
        ret = zeDriverGet(&driver_count, all_drivers);
        assert(ret == ZE_RESULT_SUCCESS);

        /* Find a driver instance with a GPU device */
        for (int i = 0; i < driver_count; ++i) {
            global_ze_device_count = 0;
            ret = zeDeviceGet(all_drivers[i], &global_ze_device_count, NULL);
            global_ze_devices_handle =
                MPL_malloc(global_ze_device_count * sizeof(ze_device_handle_t), MPL_MEM_OTHER);
            assert(global_ze_devices_handle);
            subdev_counts = MPL_malloc(global_ze_device_count * sizeof(int), MPL_MEM_OTHER);
            memset(subdev_counts, 0, global_ze_device_count * sizeof(int));
            assert(subdev_counts);

            ret = zeDeviceGet(all_drivers[i], &global_ze_device_count, global_ze_devices_handle);
            assert(ret == ZE_RESULT_SUCCESS);

            /* Check if the driver supports a gpu */
            for (int d = 0; d < global_ze_device_count; ++d) {
                ze_device_properties_t device_properties;
                ret = zeDeviceGetProperties(global_ze_devices_handle[d], &device_properties);
                assert(ret == ZE_RESULT_SUCCESS);

                if (!(device_properties.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE)) {
                    zeDeviceGetSubDevices(global_ze_devices_handle[d], &subdev_counts[d], NULL);
                    if (subdev_counts[d] == 0) {
                        /* ZE reports no subdevice when there is only one subdevice */
                        subdev_counts[d] = 1;
                    }
                    total_subdev_count += subdev_counts[d];

                }
            }
            MPL_free(global_ze_devices_handle);
        }

        device_list = (char **) MPL_malloc(total_subdev_count * sizeof(char *), MPL_MEM_OTHER);
        assert(device_list != NULL);

        int idx = 0;
        for (int i = 0; i < device_count; ++i) {
            for (int j = 0; j < subdev_counts[i]; j++) {
                int str_len = snprintf(NULL, 0, "%d.%d", i, j);
                device_list[idx] =
                    (char *) MPL_malloc((str_len + 1) * sizeof(char *), MPL_MEM_OTHER);
                sprintf(device_list[idx], "%d.%d", i, j);
                device_list[idx][str_len] = 0;
                idx++;
            }
        }

        MPL_free(subdev_counts);

        *dev_count = total_subdev_count;
        *dev_list = device_list;
    }
    return ret;
}

int MPL_gpu_dev_affinity_to_env(int dev_count, char **dev_list, char **env)
{
    int ret = MPL_SUCCESS;
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

int MPL_gpu_init(int debug_summary)
{
    int mpl_err = MPL_SUCCESS;
    if (gpu_initialized) {
        goto fn_exit;
    }

    mpl_err = gpu_ze_init_driver();
    if (mpl_err != MPL_SUCCESS)
        goto fn_fail;

    MPL_gpu_info.debug_summary = debug_summary;
    MPL_gpu_info.enable_ipc = true;
    MPL_gpu_info.ipc_handle_type = MPL_GPU_IPC_HANDLE_SHAREABLE_FD;

    max_dev_id = global_ze_device_count - 1;

    if (global_ze_device_count <= 0) {
        gpu_initialized = 1;
        goto fn_exit;
    }

    local_to_global_map = MPL_malloc(global_ze_device_count * sizeof(int), MPL_MEM_OTHER);
    if (local_to_global_map == NULL) {
        mpl_err = MPL_ERR_GPU_NOMEM;
        goto fn_fail;
    }

    global_to_local_map = MPL_malloc(global_ze_device_count * sizeof(int), MPL_MEM_OTHER);
    if (global_to_local_map == NULL) {
        mpl_err = MPL_ERR_GPU_NOMEM;
        goto fn_fail;
    }

    for (int i = 0; i < global_ze_device_count; i++) {
        local_to_global_map[i] = i;
        global_to_local_map[i] = i;
    }

    mypid = getpid();

    gpu_mem_hook_init();
    gpu_initialized = 1;

    if (MPL_gpu_info.debug_summary) {
        printf("==== GPU Init (ZE) ====\n");
        printf("device_count: %d\n", device_count);
        printf("subdevice_count: %d\n", global_ze_device_count - device_count);
        printf("=========================\n");
    }

  fn_exit:
    return mpl_err;
  fn_fail:
    goto fn_exit;
}

/* Get dev_id for shared_device_fds from regular dev_id */
int MPL_gpu_get_root_device(int dev_id)
{
    return subdevice_map[dev_id];
}

/* Get dev_id from device handle */
MPL_STATIC_INLINE_PREFIX int device_to_dev_id(MPL_gpu_device_handle_t device)
{
    int dev_id = -1;
    for (int d = 0; d < global_ze_device_count; d++) {
        if (global_ze_devices_handle[d] == device) {
            dev_id = d;
            break;
        }
    }

    return dev_id;
}

/* Get device from dev_id */
MPL_STATIC_INLINE_PREFIX int dev_id_to_device(int dev_id, MPL_gpu_device_handle_t * device)
{
    int mpl_err = MPL_SUCCESS;

    if (dev_id > max_dev_id) {
        goto fn_fail;
    }

    *device = global_ze_devices_handle[dev_id];

  fn_exit:
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

/* Functions for managing shareable IPC handles */
static int fd_to_handle(int dev_fd, int fd, int *handle)
{
#ifdef MPL_ENABLE_DRMFD
    struct drm_prime_handle open_fd = { 0, 0, 0 };
    open_fd.fd = fd;

    int ret = ioctl(dev_fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &open_fd);
    assert(ret != -1);
    *handle = open_fd.handle;
    return ret;
#else
    return -1;
#endif
}

static int handle_to_fd(int dev_fd, int handle, int *fd)
{
#ifdef MPL_ENABLE_DRMFD
    struct drm_prime_handle open_fd = { 0, 0, 0 };
    open_fd.flags = DRM_CLOEXEC | DRM_RDWR;
    open_fd.handle = handle;

    int ret = ioctl(dev_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &open_fd);
    assert(ret != -1);
    *fd = open_fd.fd;
    return ret;
#else
    return -1;
#endif
}

static int close_handle(int dev_fd, int handle)
{
#ifdef MPL_ENABLE_DRMFD
    struct drm_gem_close close = { 0, 0 };
    close.handle = handle;

    int ret = ioctl(dev_fd, DRM_IOCTL_GEM_CLOSE, &close);
    assert(ret != -1);
    return ret;
#else
    return -1;
#endif
}

/* Loads a global ze driver */
static int gpu_ze_init_driver(void)
{
    uint32_t driver_count = 0;
    ze_result_t ret;
    int ret_error = MPL_SUCCESS;
    ze_driver_handle_t *all_drivers = NULL;

    ze_init_flag_t flags = ZE_INIT_FLAG_GPU_ONLY;
    ret = zeInit(flags);
    ZE_ERR_CHECK(ret);

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
    /* Find a driver instance with a GPU device */
    for (i = 0; i < driver_count; ++i) {
        device_count = 0;
        ret = zeDeviceGet(all_drivers[i], &device_count, NULL);
        ZE_ERR_CHECK(ret);
        global_ze_devices_handle =
            MPL_malloc(device_count * sizeof(ze_device_handle_t), MPL_MEM_OTHER);
        if (global_ze_devices_handle == NULL) {
            ret_error = MPL_ERR_GPU_NOMEM;
            goto fn_fail;
        }
        ret = zeDeviceGet(all_drivers[i], &device_count, global_ze_devices_handle);
        ZE_ERR_CHECK(ret);
        /* Check if the driver supports a gpu */
        for (d = 0; d < device_count; ++d) {
            ze_device_properties_t device_properties;
            device_properties.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
            device_properties.pNext = NULL;
            ret = zeDeviceGetProperties(global_ze_devices_handle[d], &device_properties);
            ZE_ERR_CHECK(ret);

            if (ZE_DEVICE_TYPE_GPU == device_properties.type) {
                global_ze_driver_handle = all_drivers[i];
                break;
            }
        }

        if (NULL != global_ze_driver_handle) {
            break;
        } else {
            MPL_free(global_ze_devices_handle);
            global_ze_devices_handle = NULL;
        }
    }

    /* Setup subdevices */
    global_ze_device_count = device_count;
    if (global_ze_devices_handle != NULL) {
        /* Count the subdevices */
        subdevice_count = MPL_malloc(device_count * sizeof(uint32_t), MPL_MEM_OTHER);
        if (subdevice_count == NULL) {
            ret_error = MPL_ERR_GPU_NOMEM;
            goto fn_fail;
        }

        for (d = 0; d < device_count; ++d) {
            subdevice_count[d] = 0;
            ret = zeDeviceGetSubDevices(global_ze_devices_handle[d], &subdevice_count[d], NULL);
            ZE_ERR_CHECK(ret);

            global_ze_device_count += subdevice_count[d];
        }

        subdevice_map = MPL_malloc(global_ze_device_count * sizeof(int), MPL_MEM_OTHER);
        if (subdevice_map == NULL) {
            ret_error = MPL_ERR_GPU_NOMEM;
            goto fn_fail;
        }

        /* Add the subdevices to the device array */
        global_ze_devices_handle =
            MPL_realloc(global_ze_devices_handle,
                        global_ze_device_count * sizeof(ze_device_handle_t), MPL_MEM_OTHER);
        if (global_ze_devices_handle == NULL) {
            ret_error = MPL_ERR_GPU_NOMEM;
            goto fn_fail;
        }

        int dev_id = device_count;
        for (d = 0; d < device_count; ++d) {
            ret =
                zeDeviceGetSubDevices(global_ze_devices_handle[d], &subdevice_count[d],
                                      &global_ze_devices_handle[dev_id]);
            ZE_ERR_CHECK(ret);

            /* Setup the subdevice map for shared_device_fds */
            subdevice_map[d] = d;
            for (i = 0; i < subdevice_count[d]; ++i) {
                subdevice_map[dev_id + i] = d;
            }

            dev_id += subdevice_count[d];
        }
    }

    ze_context_desc_t contextDesc = {
        .stype = ZE_STRUCTURE_TYPE_CONTEXT_DESC,
        .pNext = NULL,
        .flags = 0,
    };
    ret = zeContextCreate(global_ze_driver_handle, &contextDesc, &global_ze_context);
    ZE_ERR_CHECK(ret);

  fn_exit:
    MPL_free(all_drivers);
    return ret_error;
  fn_fail:
    MPL_free(global_ze_devices_handle);
    /* If error code is already set, preserve it */
    if (ret_error == MPL_SUCCESS)
        ret_error = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_finalize(void)
{
    int i;

    MPL_free(local_to_global_map);
    MPL_free(global_to_local_map);
    MPL_free(global_ze_devices_handle);
    MPL_free(subdevice_map);
    MPL_free(subdevice_count);

    MPL_ze_gem_hash_entry_t *entry = NULL, *tmp = NULL;
    HASH_ITER(hh, gem_hash, entry, tmp) {
        HASH_DELETE(hh, gem_hash, entry);
        MPL_free(entry);
    }

    for (i = 0; i < shared_device_fd_count; ++i) {
        close(shared_device_fds[i]);
    }

    MPL_free(shared_device_fds);

    gpu_free_hook_s *prev;
    while (free_hook_chain) {
        prev = free_hook_chain;
        free_hook_chain = free_hook_chain->next;
        MPL_free(prev);
    }

    return MPL_SUCCESS;
}

int MPL_gpu_global_to_local_dev_id(int global_dev_id)
{
    assert(global_dev_id <= max_dev_id);
    return global_to_local_map[global_dev_id];
}

int MPL_gpu_local_to_global_dev_id(int local_dev_id)
{
    assert(local_dev_id < global_ze_device_count);
    return local_to_global_map[local_dev_id];
}

int MPL_gpu_ipc_handle_create(const void *ptr, MPL_gpu_ipc_mem_handle_t * ipc_handle)
{
    int mpl_err = MPL_SUCCESS;
    ze_result_t ret;
    unsigned long shared_handle;
    int fd, handle, status, dev_id = -1;
    ze_device_handle_t device;
    ze_ipc_mem_handle_t ze_ipc_handle;

    ze_memory_allocation_properties_t ptr_attr = {
        .stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES,
        .pNext = NULL,
        .type = 0,
        .id = 0,
        .pageSize = 0,
    };

    ret = zeMemGetIpcHandle(global_ze_context, ptr, &ze_ipc_handle);
    ZE_ERR_CHECK(ret);

    ret = zeMemGetAllocProperties(global_ze_context, ptr, &ptr_attr, &device);
    ZE_ERR_CHECK(ret);

    dev_id = device_to_dev_id(device);
    if (dev_id == -1) {
        goto fn_fail;
    }

    if (shared_device_fds != NULL) {
        int shared_dev_id = MPL_gpu_get_root_device(dev_id);
        /* convert dma_buf fd to GEM handle */
        memcpy(&fd, &ze_ipc_handle, sizeof(fd));
        status = fd_to_handle(shared_device_fds[shared_dev_id], fd, &handle);
        if (status) {
            goto fn_fail;
        }

        shared_handle = shared_dev_id;
        shared_handle = shared_handle << 32 | handle;
        memcpy(ipc_handle, &shared_handle, sizeof(shared_handle));

        /* Hash (ptr, dev_id, handle) to close later */
        MPL_ze_gem_hash_entry_t *entry = NULL;
        HASH_FIND_PTR(gem_hash, &ptr, entry);

        if (entry == NULL) {
            entry =
                (MPL_ze_gem_hash_entry_t *) MPL_malloc(sizeof(MPL_ze_gem_hash_entry_t),
                                                       MPL_MEM_OTHER);
            if (entry == NULL) {
                goto fn_fail;
            }

            entry->ptr = ptr;
            entry->dev_id = shared_dev_id;
            entry->handle = handle;
            HASH_ADD_PTR(gem_hash, ptr, entry, MPL_MEM_OTHER);
        }
    } else {
        fd_pid_t h;
        memcpy(&h.fd, &ze_ipc_handle, sizeof(fd));
        h.pid = mypid;
        h.dev_id = dev_id;
        memcpy(ipc_handle, &h, sizeof(fd_pid_t));
    }

  fn_exit:
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_ipc_handle_destroy(const void *ptr)
{
    int status, mpl_err = MPL_SUCCESS;

    if (shared_device_fds != NULL) {
        MPL_ze_gem_hash_entry_t *entry = NULL;
        HASH_FIND_PTR(gem_hash, &ptr, entry);
        if (entry == NULL) {
            /* This might get called for pointers that didn't have IPC handles created */
            goto fn_exit;
        }

        HASH_DEL(gem_hash, entry);
        MPL_free(entry);

        /* close GEM handle */
        status = close_handle(shared_device_fds[entry->dev_id], entry->handle);
        if (status) {
            goto fn_fail;
        }
    }

  fn_exit:
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_ipc_handle_map(MPL_gpu_ipc_mem_handle_t ipc_handle, int dev_id, void **ptr)
{
    int mpl_err = MPL_SUCCESS;
    ze_result_t ret;
    unsigned long shared_handle;
    int fd, handle = 0, status;
    MPL_gpu_device_handle_t dev_handle;
    ze_ipc_mem_handle_t ze_ipc_handle;

    memset(&ze_ipc_handle, 0, sizeof(ze_ipc_mem_handle_t));

    if (shared_device_fds != NULL) {
        /* convert GEM handle to fd */
        memcpy(&shared_handle, &ipc_handle, sizeof(shared_handle));
        int shared_dev_id = shared_handle >> 32;
        handle = shared_handle << 32 >> 32;

        status = handle_to_fd(shared_device_fds[shared_dev_id], handle, &fd);
        if (status) {
            goto fn_fail;
        }
    } else {
        /* pidfd_getfd */
        fd_pid_t h;
        memcpy(&h, &ipc_handle, sizeof(fd_pid_t));
        if (h.pid != mypid) {
            int pid_fd = syscall(__NR_pidfd_open, h.pid, 0);
            if (pid_fd == -1) {
                printf("pidfd_open error: %s (%d %d %d)\n", strerror(errno), h.pid, h.fd, h.dev_id);
            }
            assert(pid_fd != -1);
            fd = syscall(__NR_pidfd_getfd, pid_fd, h.fd, 0);
            if (fd == -1) {
                printf("Error> pidfd_getfd is not implemented!");
                mpl_err = MPL_ERR_GPU_INTERNAL;
                goto fn_fail;
            }
            close(pid_fd);
        } else {
            fd = h.fd;
        }
    }

    mpl_err = dev_id_to_device(dev_id, &dev_handle);
    if (mpl_err != MPL_SUCCESS) {
        goto fn_fail;
    }
    memcpy(&ze_ipc_handle, &fd, sizeof(fd));

    ret = zeMemOpenIpcHandle(global_ze_context, dev_handle, ze_ipc_handle, 0, ptr);
    ZE_ERR_CHECK(ret);

  fn_exit:
    return mpl_err;
  fn_fail:
    goto fn_exit;
}

int MPL_gpu_ipc_handle_unmap(void *ptr)
{
    int mpl_err = MPL_SUCCESS;

    ze_result_t ret;
    ret = zeMemCloseIpcHandle(global_ze_context, ptr);
    ZE_ERR_CHECK(ret);

  fn_exit:
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_query_pointer_attr(const void *ptr, MPL_pointer_attr_t * attr)
{
    int mpl_err = MPL_SUCCESS;

    ze_result_t ret;
    memset(&attr->device_attr.prop, 0, sizeof(ze_memory_allocation_properties_t));
    ret = zeMemGetAllocProperties(global_ze_context, ptr,
                                  &attr->device_attr.prop, &attr->device_attr.device);
    ZE_ERR_CHECK(ret);
    attr->device = attr->device_attr.device;
    switch (attr->device_attr.prop.type) {
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
        default:
            goto fn_fail;
    }

  fn_exit:
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_malloc(void **ptr, size_t size, MPL_gpu_device_handle_t h_device)
{
    int mpl_err = MPL_SUCCESS;
    int ret;
    size_t mem_alignment;
    ze_device_mem_alloc_desc_t device_desc = {
        .stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC,
        .pNext = NULL,
        .flags = 0,
        .ordinal = 0,   /* We currently support a single memory type */
    };
    /* Currently ZE ignores this argument and uses an internal alignment
     * value. However, this behavior can change in the future. */
    mem_alignment = 1;
    ret = zeMemAllocDevice(global_ze_context, &device_desc, size, mem_alignment, h_device, ptr);

    ZE_ERR_CHECK(ret);

  fn_exit:
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_malloc_host(void **ptr, size_t size)
{
    int mpl_err = MPL_SUCCESS;
    int ret;
    size_t mem_alignment;
    ze_host_mem_alloc_desc_t host_desc = {
        .stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC,
        .pNext = NULL,
        .flags = 0,
    };

    /* Currently ZE ignores this argument and uses an internal alignment
     * value. However, this behavior can change in the future. */
    mem_alignment = 1;
    ret = zeMemAllocHost(global_ze_context, &host_desc, size, mem_alignment, ptr);
    ZE_ERR_CHECK(ret);

  fn_exit:
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_free(void *ptr)
{
    int mpl_err = MPL_SUCCESS;
    mpl_err = zeMemFree(global_ze_context, ptr);
    ZE_ERR_CHECK(mpl_err);

  fn_exit:
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_free_host(void *ptr)
{
    int mpl_err;
    mpl_err = zeMemFree(global_ze_context, ptr);
    ZE_ERR_CHECK(mpl_err);

  fn_exit:
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_register_host(const void *ptr, size_t size)
{
    return MPL_SUCCESS;
}

int MPL_gpu_unregister_host(const void *ptr)
{
    return MPL_SUCCESS;
}

int MPL_gpu_get_dev_id_from_attr(MPL_pointer_attr_t * attr)
{
    int dev_id = -1;

    dev_id = device_to_dev_id(attr->device);

    return dev_id;
}

int MPL_gpu_get_buffer_bounds(const void *ptr, void **pbase, uintptr_t * len)
{
    int mpl_err = MPL_SUCCESS;
    int ret;
    ret = zeMemGetAddressRange(global_ze_context, ptr, pbase, len);
    ZE_ERR_CHECK(ret);

  fn_exit:
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
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

MPL_STATIC_INLINE_PREFIX int gpu_mem_hook_init(void)
{
    if (sys_zeMemFree)
        return MPL_SUCCESS;

    void *libze_handle = dlopen("libze_loader.so", RTLD_LAZY | RTLD_GLOBAL);
    assert(libze_handle);

    sys_zeMemFree = (void *) dlsym(libze_handle, "zeMemFree");
    assert(sys_zeMemFree);

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

int MPL_gpu_launch_hostfn(int stream, MPL_gpu_hostfn fn, void *data)
{
    return -1;
}

bool MPL_gpu_stream_is_valid(MPL_gpu_stream_t stream)
{
    return false;
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

#ifdef HAVE_VISIBILITY
__attribute__ ((visibility("default")))
#endif
ze_result_t ZE_APICALL zeMemFree(ze_context_handle_t hContext, void *dptr)
{
    ze_result_t result;
    /* in case when MPI_Init was skipped */
    gpu_mem_hook_init();
    gpu_free_hooks_cb(dptr);
    result = sys_zeMemFree(hContext, dptr);
    return (result);
}

/* ZE-Specific Functions */

int MPL_ze_init_device_fds(int *num_fds, int *device_fds)
{
    const char *device_directory = "/dev/dri/by-path";
    const char *device_suffix = "-render";
    struct dirent *ent = NULL;
    int n = 0;

#ifndef MPL_ENABLE_DRMFD
    printf("Error> drmfd is not supported!");
    goto fn_fail;
#endif

    DIR *dir = opendir(device_directory);
    if (dir == NULL) {
        goto fn_fail;
    }

    /* Search for all devices in the device directory */
    while ((ent = readdir(dir)) != NULL) {
        char dev_name[128];

        if (ent->d_name[0] == '.') {
            continue;
        }

        /* They must contain the device suffix */
        if (strstr(ent->d_name, device_suffix) == NULL) {
            continue;
        }

        strcpy(dev_name, device_directory);
        strcat(dev_name, "/");
        strcat(dev_name, ent->d_name);

        /* Open the device */
        if (device_fds) {
            device_fds[n] = open(dev_name, O_RDWR);
        }

        n++;
    }

    *num_fds = n;

  fn_exit:
    return MPL_SUCCESS;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

void MPL_ze_set_fds(int num_fds, int *fds)
{
    shared_device_fds = fds;
    shared_device_fd_count = num_fds;
}

#endif /* MPL_HAVE_ZE */
