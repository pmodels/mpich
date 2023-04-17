/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mplconfig.h"
#ifdef MPL_HAVE_X86INTRIN_H
/* must come before mpl_trmem.h */
#include <x86intrin.h>
#endif
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
static uint32_t max_dev_id;     /* Does not include subdevices */
static uint32_t max_subdev_id;
static char **device_list = NULL;
#define MAX_GPU_STR_LEN 256
static char affinity_env[MAX_GPU_STR_LEN] = { 0 };

static int *local_to_global_map;        /* [local_ze_device_count] */
static int *global_to_local_map;        /* [global_ze_device_count] */

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

typedef struct {
    uint64_t mem_id;
    MPL_gpu_ipc_mem_handle_t ipc_handle;
    void *mapped_ptr;
    size_t mapped_size;
    bool handle_cached;
    UT_hash_handle hh;
} MPL_ze_ipc_handle_entry_t;

typedef struct {
    void *ipc_buf;
    void *mapped_ptr;
    size_t mapped_size;

    /* Multiple keys */
    uint64_t remote_mem_id;
    int remote_dev_id;
    pid_t remote_pid;
    UT_hash_handle hh;
} MPL_ze_mapped_buffer_entry_t;

typedef struct {
    uint64_t remote_mem_id;
    int remote_dev_id;
    pid_t remote_pid;
} MPL_ze_mapped_buffer_lookup_t;

static MPL_ze_gem_hash_entry_t *gem_hash = NULL;
static MPL_ze_ipc_handle_entry_t **ipc_cache_tracked = NULL;
static MPL_ze_mapped_buffer_entry_t **ipc_cache_mapped = NULL;
static MPL_ze_mapped_buffer_entry_t **ipc_cache_removal = NULL;
static MPL_ze_mapped_buffer_entry_t **mmap_cache_removal = NULL;

typedef struct {
    int fd;
    pid_t pid;
    int dev_id;
    uint64_t mem_id;
} fd_pid_t;

typedef struct gpu_free_hook {
    void (*free_hook) (void *dptr);
    struct gpu_free_hook *next;
} gpu_free_hook_s;

pid_t mypid;

/* Level-zero API v1.0:
 * http://spec.oneapi.com/level-zero/latest/index.html
 */
ze_driver_handle_t ze_driver_handle;
/* ze_devices_handle contains all devices and subdevices. Indices [0, device_count) are
 * devices while the rest are subdevices. Keeping them all in the same array allows for easy
 * comparison of device handle when a device id is passed from the upper layer. The only time it
 * matters if we have a subdevice vs a device is when with creating or mapping an ipc handle. In
 * situations, we use the subdevice_map to find the upper device id for indexing into
 * shared_device_fds, since these are only opened on the upper devices. */
ze_device_handle_t *ze_devices_handle = NULL;
ze_context_handle_t ze_context;
uint32_t local_ze_device_count; /* This counts both devices and subdevices */
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

int MPL_gpu_get_dev_count(int *dev_cnt, int *dev_id, int *subdev_id)
{
    int ret = MPL_SUCCESS;
    if (!gpu_initialized) {
        ret = MPL_gpu_init(0);
    }

    *dev_cnt = local_ze_device_count;
    *dev_id = max_dev_id;
    *subdev_id = max_subdev_id;
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
            local_ze_device_count = 0;
            ret = zeDeviceGet(all_drivers[i], &local_ze_device_count, NULL);
            ze_devices_handle =
                MPL_malloc(local_ze_device_count * sizeof(ze_device_handle_t), MPL_MEM_OTHER);
            assert(ze_devices_handle);
            subdev_counts = MPL_malloc(local_ze_device_count * sizeof(int), MPL_MEM_OTHER);
            memset(subdev_counts, 0, local_ze_device_count * sizeof(int));
            assert(subdev_counts);

            ret = zeDeviceGet(all_drivers[i], &local_ze_device_count, ze_devices_handle);
            assert(ret == ZE_RESULT_SUCCESS);

            /* Check if the driver supports a gpu */
            for (int d = 0; d < local_ze_device_count; ++d) {
                ze_device_properties_t device_properties;
                ret = zeDeviceGetProperties(ze_devices_handle[d], &device_properties);
                assert(ret == ZE_RESULT_SUCCESS);

                if (!(device_properties.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE)) {
                    zeDeviceGetSubDevices(ze_devices_handle[d], &subdev_counts[d], NULL);
                    if (subdev_counts[d] == 0) {
                        /* ZE reports no subdevice when there is only one subdevice */
                        subdev_counts[d] = 1;
                    }
                    total_subdev_count += subdev_counts[d];

                }
            }
            MPL_free(ze_devices_handle);
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
        MPL_snprintf(affinity_env, 3, "-1");
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

int MPL_gpu_init_device_mappings(int max_devid, int max_subdevid)
{
    int mpl_err = MPL_SUCCESS;
    int global_dev_count = max_devid + 1;
    int global_subdev_count = 0;

    /* If max_subdevid is 0, then all procs use tile 0 as root devices, so subdevices aren't
     * needed. */
    if (max_subdevid == 0) {
        global_ze_device_count = global_dev_count;
    } else {
        /* We can still have the situation where all procs use non-zero tile as root devices, but
         * this can't be detected unless we also reduce subdevice count. Thus, consider them as
         * subdevices in the global_to_local_map even if they are all root devices. */
        global_subdev_count = max_subdevid + 1;
        global_ze_device_count = global_dev_count * (global_subdev_count + 1);
    }

    /* Initialize local_to_global_map */
    local_to_global_map = MPL_malloc(local_ze_device_count * sizeof(int), MPL_MEM_OTHER);
    if (local_to_global_map == NULL) {
        mpl_err = MPL_ERR_GPU_NOMEM;
        goto fn_fail;
    }

    for (int i = 0; i < local_ze_device_count; ++i) {
        local_to_global_map[i] = -1;
    }

    /* Initialize global_to_local_map */
    global_to_local_map = MPL_malloc(global_ze_device_count * sizeof(int), MPL_MEM_OTHER);
    if (global_to_local_map == NULL) {
        mpl_err = MPL_ERR_GPU_NOMEM;
        goto fn_fail;
    }

    for (int i = 0; i < global_ze_device_count; ++i) {
        global_to_local_map[i] = -1;
    }

    /* Parse ZE_AFFINITY_MASK to find device and subdevice visibility based on global device id. */
    char *visible_devices = getenv("ZE_AFFINITY_MASK");
    if (visible_devices) {
        size_t len = strlen(visible_devices);

        char *devices = MPL_malloc(len + 1, MPL_MEM_OTHER);
        if (devices == NULL) {
            mpl_err = MPL_ERR_GPU_NOMEM;
            goto fn_fail;
        }
        char *free_ptr, *dev;

        memcpy(devices, visible_devices, len + 1);
        free_ptr = devices;
        char *tmp = strtok_r(devices, ",", &dev);

        while (tmp != NULL) {
            char *subdev;
            char *subdevices = strtok_r(tmp, ".", &subdev);
            int device = atoi(subdevices);
            tmp = NULL;

            /* Temporarily mark the device as visible. It might only be a subdevice that is
             * visible. */
            global_to_local_map[device] = 1;

            /* Mark the subdevice(s) as visible. */
            subdevices = strtok_r(tmp, ".", &subdev);
            if (subdevices != NULL) {
                /* Handle special case where there are no subdevices among any device. */
                if (global_subdev_count > 0) {
                    int subdevice = atoi(subdevices);
                    int idx = global_dev_count + device * global_subdev_count + subdevice;
                    global_to_local_map[idx] = 1;
                }
            } else {
                int idx = global_dev_count + device * global_subdev_count;
                for (int i = 0; i < global_subdev_count; ++i) {
                    global_to_local_map[idx + i] = 1;
                }
            }

            devices = NULL;
            tmp = strtok_r(devices, ",", &dev);
        }

        MPL_free(free_ptr);
    } else {
        for (int i = 0; i < global_dev_count; ++i) {
            /* Set device as visible */
            global_to_local_map[i] = 1;

            /* Set subdevices as visible */
            int idx = global_dev_count + i * global_subdev_count;
            for (int j = 0; j < subdevice_count[i]; ++j) {
                global_to_local_map[idx + j] = 1;
            }
        }
    }

    /* Setup global_to_local_map */
    int local_dev_id = 0;

    /* The root devices first */
    for (int i = 0; i < global_dev_count; ++i) {
        if (global_to_local_map[i] == 1) {
            /* Check if the device has subdevices before setting its index. If it does not, then
             * only the subdevice is visible. However, need to check for the special case that
             * there are no subdevices among any device. */
            if (subdevice_count[local_dev_id] || global_subdev_count == 0) {
                global_to_local_map[i] = local_dev_id;
            } else {
                /* Find which subdevice is visible and give it the local device id since it is the
                 * root device. */
                int idx = global_dev_count + i * global_subdev_count;
                for (int j = 0; j < global_subdev_count; ++j) {
                    if (global_to_local_map[idx + j] == 1) {
                        global_to_local_map[idx + j] = local_dev_id;
                    }
                }
                /* Unset the device as the root device, since its subdevice is the root. */
                global_to_local_map[i] = -1;
            }
            ++local_dev_id;
        }
    }

    /* The subdevices next */
    for (int i = 0; i < global_dev_count; ++i) {
        if (global_to_local_map[i] != -1) {
            int idx = global_dev_count + i * global_subdev_count;
            for (int j = 0; j < global_subdev_count; ++j) {
                if (global_to_local_map[idx + j] == 1) {
                    global_to_local_map[idx + j] = local_dev_id;
                    ++local_dev_id;
                }
            }
        }
    }

    assert(local_dev_id == local_ze_device_count);

    /* Setup local_to_global_map */
    local_dev_id = 0;
    for (int i = 0; i < global_ze_device_count; ++i) {
        int local_id = global_to_local_map[i];
        if (local_id != -1) {
            local_to_global_map[local_id] = i;
        }
    }

  fn_exit:
    return mpl_err;
  fn_fail:
    goto fn_exit;
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

    max_dev_id = 0;
    max_subdev_id = 0;

    if (local_ze_device_count <= 0) {
        gpu_initialized = 1;
        goto fn_exit;
    }

    /* Parse ZE_AFFINITY_MASK to get max_dev_id and max_subdev_id */
    char *visible_devices = getenv("ZE_AFFINITY_MASK");
    if (visible_devices) {
        size_t len = strlen(visible_devices);

        char *devices = MPL_malloc(len + 1, MPL_MEM_OTHER);
        if (devices == NULL) {
            mpl_err = MPL_ERR_GPU_NOMEM;
            goto fn_fail;
        }
        char *dev, *free_ptr = devices;

        memcpy(devices, visible_devices, len + 1);
        char *tmp = strtok_r(devices, ",", &dev);

        while (tmp != NULL) {
            char *subdev;
            char *subdevices = strtok_r(tmp, ".", &subdev);
            int device = atoi(subdevices);
            tmp = NULL;

            /* Get max_dev_id (ignores subdevices) */
            if (device > max_dev_id) {
                max_dev_id = device;
            }

            subdevices = strtok_r(tmp, ".", &subdev);
            tmp = NULL;

            /* Get max_subdev_id */
            if (subdevices != NULL) {
                int subdev_id = atoi(subdevices);
                if (subdev_id > max_subdev_id) {
                    max_subdev_id = subdev_id;
                }
            }

            devices = NULL;
            tmp = strtok_r(devices, ",", &dev);
        }

        MPL_free(free_ptr);
    } else {
        max_dev_id = device_count - 1;
    }

    /* Make sure to include subdevices that weren't detected above in the ZE_AFFINITY_MASK. */
    for (int i = 0; i < device_count; ++i) {
        if (subdevice_count[i] > 0 && (subdevice_count[i] - 1) > max_subdev_id) {
            max_subdev_id = subdevice_count[i] - 1;
        }
    }

    if (likely(MPL_gpu_info.specialized_cache)) {
        ipc_cache_tracked =
            MPL_malloc(local_ze_device_count * sizeof(MPL_ze_ipc_handle_entry_t *), MPL_MEM_OTHER);
        if (ipc_cache_tracked == NULL) {
            mpl_err = MPL_ERR_GPU_NOMEM;
            goto fn_fail;
        }

        ipc_cache_mapped =
            MPL_malloc(local_ze_device_count * sizeof(MPL_ze_mapped_buffer_entry_t *),
                       MPL_MEM_OTHER);
        if (ipc_cache_mapped == NULL) {
            mpl_err = MPL_ERR_GPU_NOMEM;
            goto fn_fail;
        }

        ipc_cache_removal =
            MPL_malloc(local_ze_device_count * sizeof(MPL_ze_mapped_buffer_entry_t *),
                       MPL_MEM_OTHER);
        if (ipc_cache_removal == NULL) {
            mpl_err = MPL_ERR_GPU_NOMEM;
            goto fn_fail;
        }

        mmap_cache_removal =
            MPL_malloc(local_ze_device_count * sizeof(MPL_ze_mapped_buffer_entry_t *),
                       MPL_MEM_OTHER);
        if (mmap_cache_removal == NULL) {
            mpl_err = MPL_ERR_GPU_NOMEM;
            goto fn_fail;
        }

        for (int i = 0; i < local_ze_device_count; ++i) {
            ipc_cache_tracked[i] = NULL;
            ipc_cache_mapped[i] = NULL;
            ipc_cache_removal[i] = NULL;
            mmap_cache_removal[i] = NULL;
        }

        MPL_gpu_free_hook_register(MPL_ze_ipc_remove_cache_handle);
    }

    mypid = getpid();

    gpu_mem_hook_init();
    gpu_initialized = 1;

    if (MPL_gpu_info.debug_summary) {
        printf("==== GPU Init (ZE) ====\n");
        printf("device_count: %d\n", device_count);
        printf("subdevice_count: %d\n", local_ze_device_count - device_count);
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

/* Get dev_id from device */
MPL_STATIC_INLINE_PREFIX int device_to_dev_id(ze_device_handle_t device)
{
    int dev_id = -1;
    for (int d = 0; d < local_ze_device_count; d++) {
        if (ze_devices_handle[d] == device) {
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

    if (dev_id > local_ze_device_count) {
        goto fn_fail;
    }

    *device = ze_devices_handle[dev_id];

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
    if (ret == -1)
        perror("handle_to_fd");
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
        ze_devices_handle = MPL_malloc(device_count * sizeof(ze_device_handle_t), MPL_MEM_OTHER);
        if (ze_devices_handle == NULL) {
            ret_error = MPL_ERR_GPU_NOMEM;
            goto fn_fail;
        }
        ret = zeDeviceGet(all_drivers[i], &device_count, ze_devices_handle);
        ZE_ERR_CHECK(ret);
        /* Check if the driver supports a gpu */
        for (d = 0; d < device_count; ++d) {
            ze_device_properties_t device_properties;
            device_properties.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
            device_properties.pNext = NULL;
            ret = zeDeviceGetProperties(ze_devices_handle[d], &device_properties);
            ZE_ERR_CHECK(ret);

            if (ZE_DEVICE_TYPE_GPU == device_properties.type) {
                ze_driver_handle = all_drivers[i];
                break;
            }
        }

        if (NULL != ze_driver_handle) {
            break;
        } else {
            MPL_free(ze_devices_handle);
            ze_devices_handle = NULL;
        }
    }

    /* Setup subdevices */
    local_ze_device_count = device_count;
    if (ze_devices_handle != NULL) {
        /* Count the subdevices */
        subdevice_count = MPL_malloc(device_count * sizeof(uint32_t), MPL_MEM_OTHER);
        if (subdevice_count == NULL) {
            ret_error = MPL_ERR_GPU_NOMEM;
            goto fn_fail;
        }

        for (d = 0; d < device_count; ++d) {
            subdevice_count[d] = 0;
            ret = zeDeviceGetSubDevices(ze_devices_handle[d], &subdevice_count[d], NULL);
            ZE_ERR_CHECK(ret);

            local_ze_device_count += subdevice_count[d];
        }

        subdevice_map = MPL_malloc(local_ze_device_count * sizeof(int), MPL_MEM_OTHER);
        if (subdevice_map == NULL) {
            ret_error = MPL_ERR_GPU_NOMEM;
            goto fn_fail;
        }

        /* Add the subdevices to the device array */
        ze_devices_handle =
            MPL_realloc(ze_devices_handle,
                        local_ze_device_count * sizeof(ze_device_handle_t), MPL_MEM_OTHER);
        if (ze_devices_handle == NULL) {
            ret_error = MPL_ERR_GPU_NOMEM;
            goto fn_fail;
        }

        int dev_id = device_count;
        for (d = 0; d < device_count; ++d) {
            ret =
                zeDeviceGetSubDevices(ze_devices_handle[d], &subdevice_count[d],
                                      &ze_devices_handle[dev_id]);
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
    ret = zeContextCreate(ze_driver_handle, &contextDesc, &ze_context);
    ZE_ERR_CHECK(ret);

  fn_exit:
    MPL_free(all_drivers);
    return ret_error;
  fn_fail:
    MPL_free(ze_devices_handle);
    /* If error code is already set, preserve it */
    if (ret_error == MPL_SUCCESS)
        ret_error = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_finalize(void)
{
    int i;

    if (likely(MPL_gpu_info.specialized_cache)) {
        for (i = 0; i < local_ze_device_count; ++i) {
            if (ipc_cache_removal[i]) {
                MPL_ze_mapped_buffer_entry_t *entry = NULL, *tmp = NULL;
                HASH_ITER(hh, ipc_cache_removal[i], entry, tmp) {
                    MPL_gpu_ipc_handle_unmap(entry->ipc_buf);
                }
            }

            if (mmap_cache_removal[i]) {
                MPL_ze_mapped_buffer_entry_t *entry = NULL, *tmp = NULL;
                HASH_ITER(hh, mmap_cache_removal[i], entry, tmp) {
                    MPL_ze_mmap_handle_unmap(entry->mapped_ptr, i);
                }
            }
        }

        for (i = 0; i < local_ze_device_count; ++i) {
            MPL_ze_ipc_handle_entry_t *entry = NULL, *tmp = NULL;
            HASH_ITER(hh, ipc_cache_tracked[i], entry, tmp) {
                if (entry->mapped_ptr != NULL) {
                    munmap(entry->mapped_ptr, entry->mapped_size);
                }
                HASH_DELETE(hh, ipc_cache_tracked[i], entry);
                MPL_free(entry);
            }
        }

        MPL_free(ipc_cache_tracked);
        MPL_free(ipc_cache_removal);
        MPL_free(mmap_cache_removal);
        MPL_free(ipc_cache_mapped);
    }

    MPL_free(local_to_global_map);
    MPL_free(global_to_local_map);
    MPL_free(ze_devices_handle);
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
    assert(global_dev_id < global_ze_device_count);
    return global_to_local_map[global_dev_id];
}

int MPL_gpu_local_to_global_dev_id(int local_dev_id)
{
    assert(local_dev_id < local_ze_device_count);
    return local_to_global_map[local_dev_id];
}

int MPL_gpu_ipc_handle_create(const void *ptr, MPL_gpu_ipc_mem_handle_t * ipc_handle)
{
    int mpl_err = MPL_SUCCESS;
    ze_result_t ret;
    int local_dev_id = -1;
    MPL_ze_ipc_handle_entry_t *cache_entry = NULL;
    uint64_t mem_id = 0;

    MPL_gpu_device_attr ptr_attr;
    memset(&ptr_attr, 0, sizeof(MPL_gpu_device_attr));
    memset(ipc_handle, 0, sizeof(MPL_gpu_ipc_mem_handle_t));

    ret = zeMemGetAllocProperties(ze_context, ptr, &ptr_attr.prop, &ptr_attr.device);
    ZE_ERR_CHECK(ret);

    local_dev_id = device_to_dev_id(ptr_attr.device);
    if (local_dev_id == -1) {
        goto fn_fail;
    }

    /* Check if ipc_handle is already cached */
    if (likely(MPL_gpu_info.specialized_cache)) {
        mem_id = ptr_attr.prop.id;
        HASH_FIND(hh, ipc_cache_tracked[local_dev_id], &mem_id, sizeof(uint64_t), cache_entry);
    }

    if (cache_entry && cache_entry->handle_cached) {
        memcpy(ipc_handle, &cache_entry->ipc_handle, sizeof(MPL_gpu_ipc_mem_handle_t));
    } else {
        mpl_err = MPL_ze_ipc_handle_create(ptr, &ptr_attr, local_dev_id, true, ipc_handle);
        if (mpl_err != MPL_SUCCESS) {
            goto fn_fail;
        }

        if (likely(MPL_gpu_info.specialized_cache)) {
            if (cache_entry) {
                /* In the case where there was an entry, but no ipc handle yet */
                cache_entry->handle_cached = true;
                memcpy(&cache_entry->ipc_handle, ipc_handle, sizeof(MPL_gpu_ipc_mem_handle_t));
            } else {
                /* Insert into the cache */
                cache_entry =
                    (MPL_ze_ipc_handle_entry_t *) MPL_malloc(sizeof(MPL_ze_ipc_handle_entry_t),
                                                             MPL_MEM_OTHER);

                if (cache_entry == NULL) {
                    mpl_err = MPL_ERR_GPU_NOMEM;
                    goto fn_fail;
                }

                memset(cache_entry, 0, sizeof(MPL_ze_ipc_handle_entry_t));

                cache_entry->mem_id = mem_id;
                cache_entry->handle_cached = true;
                memcpy(&cache_entry->ipc_handle, ipc_handle, sizeof(MPL_gpu_ipc_mem_handle_t));
                HASH_ADD(hh, ipc_cache_tracked[local_dev_id], mem_id, sizeof(uint64_t), cache_entry,
                         MPL_MEM_OTHER);
            }
        }
    }

  fn_exit:
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_ipc_handle_destroy(const void *ptr, MPL_pointer_attr_t * gpu_attr)
{
    int status, mpl_err = MPL_SUCCESS;
    MPL_ze_ipc_handle_entry_t *cache_entry = NULL;
    int dev_id;
    uint64_t mem_id;

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

    if (likely(MPL_gpu_info.specialized_cache)) {
        dev_id = device_to_dev_id(gpu_attr->device);
        if (dev_id == -1) {
            goto fn_fail;
        }

        mem_id = gpu_attr->device_attr.prop.id;
        HASH_FIND(hh, ipc_cache_tracked[dev_id], &mem_id, sizeof(uint64_t), cache_entry);

        if (cache_entry != NULL) {
            HASH_DELETE(hh, ipc_cache_tracked[dev_id], cache_entry);
            MPL_free(cache_entry);
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
    ze_ipc_mem_handle_t ze_ipc_handle;
    MPL_ze_mapped_buffer_entry_t *cache_entry = NULL;
    MPL_ze_mapped_buffer_entry_t *removal_entry = NULL;
    MPL_ze_mapped_buffer_lookup_t lookup_entry;
    unsigned keylen = 0;

    memset(&ze_ipc_handle, 0, sizeof(ze_ipc_mem_handle_t));

    fd_pid_t h;
    memcpy(&h, &ipc_handle, sizeof(fd_pid_t));

    if (likely(MPL_gpu_info.specialized_cache)) {
        /* Check if ipc-mapped buffer is already cached */
        memset(&lookup_entry, 0, sizeof(MPL_ze_mapped_buffer_lookup_t));
        lookup_entry.remote_mem_id = h.mem_id;
        lookup_entry.remote_dev_id = h.dev_id;
        lookup_entry.remote_pid = h.pid;

        keylen = offsetof(MPL_ze_mapped_buffer_entry_t, remote_pid) + sizeof(pid_t) -
            offsetof(MPL_ze_mapped_buffer_entry_t, remote_mem_id);

        HASH_FIND(hh, ipc_cache_mapped[dev_id], &lookup_entry.remote_mem_id, keylen, cache_entry);
    }

    if (cache_entry && cache_entry->ipc_buf) {
        *ptr = cache_entry->ipc_buf;
    } else {
        mpl_err = MPL_ze_ipc_handle_map(ipc_handle, true, dev_id, false, 0, ptr);
        if (mpl_err != MPL_SUCCESS) {
            goto fn_fail;
        }

        if (likely(MPL_gpu_info.specialized_cache)) {
            if (cache_entry) {
                cache_entry->ipc_buf = *ptr;

                removal_entry = (MPL_ze_mapped_buffer_entry_t *)
                    MPL_malloc(sizeof(MPL_ze_mapped_buffer_entry_t), MPL_MEM_OTHER);
                if (removal_entry == NULL) {
                    mpl_err = MPL_ERR_GPU_NOMEM;
                    goto fn_fail;
                }

                memset(removal_entry, 0, sizeof(MPL_ze_mapped_buffer_entry_t));
                memcpy(removal_entry, cache_entry, sizeof(MPL_ze_mapped_buffer_entry_t));
                removal_entry->mapped_ptr = NULL;

                HASH_ADD(hh, ipc_cache_removal[dev_id], ipc_buf, sizeof(void *), removal_entry,
                         MPL_MEM_OTHER);
            } else {
                /* Insert into the cache */
                cache_entry = (MPL_ze_mapped_buffer_entry_t *)
                    MPL_malloc(sizeof(MPL_ze_mapped_buffer_entry_t), MPL_MEM_OTHER);
                if (cache_entry == NULL) {
                    mpl_err = MPL_ERR_GPU_NOMEM;
                    goto fn_fail;
                }

                removal_entry = (MPL_ze_mapped_buffer_entry_t *)
                    MPL_malloc(sizeof(MPL_ze_mapped_buffer_entry_t), MPL_MEM_OTHER);
                if (removal_entry == NULL) {
                    mpl_err = MPL_ERR_GPU_NOMEM;
                    goto fn_fail;
                }

                memset(removal_entry, 0, sizeof(MPL_ze_mapped_buffer_entry_t));
                memset(cache_entry, 0, sizeof(MPL_ze_mapped_buffer_entry_t));

                cache_entry->remote_mem_id = lookup_entry.remote_mem_id;
                cache_entry->remote_dev_id = lookup_entry.remote_dev_id;
                cache_entry->remote_pid = lookup_entry.remote_pid;
                cache_entry->ipc_buf = *ptr;
                memcpy(removal_entry, cache_entry, sizeof(MPL_ze_mapped_buffer_entry_t));

                HASH_ADD(hh, ipc_cache_mapped[dev_id], remote_mem_id, keylen, cache_entry,
                         MPL_MEM_OTHER);
                HASH_ADD(hh, ipc_cache_removal[dev_id], ipc_buf, sizeof(void *), removal_entry,
                         MPL_MEM_OTHER);
            }
        }
    }

  fn_exit:
    return mpl_err;
  fn_fail:
    goto fn_exit;
}

int MPL_ze_mmap_handle_unmap(void *ptr, int dev_id)
{
    int ret_err, mpl_err = MPL_SUCCESS;
    unsigned keylen;
    size_t size = 0;
    MPL_ze_mapped_buffer_entry_t *cache_entry = NULL;
    MPL_ze_mapped_buffer_lookup_t lookup_entry;

    if (likely(MPL_gpu_info.specialized_cache)) {
        HASH_FIND(hh, mmap_cache_removal[dev_id], &ptr, sizeof(void *), cache_entry);

        if (cache_entry != NULL) {
            size = cache_entry->mapped_size;
        }

        if (cache_entry != NULL) {
            memset(&lookup_entry, 0, sizeof(MPL_ze_mapped_buffer_lookup_t));
            lookup_entry.remote_mem_id = cache_entry->remote_mem_id;
            lookup_entry.remote_dev_id = cache_entry->remote_dev_id;
            lookup_entry.remote_pid = cache_entry->remote_pid;

            keylen = offsetof(MPL_ze_mapped_buffer_entry_t, remote_pid) + sizeof(pid_t) -
                offsetof(MPL_ze_mapped_buffer_entry_t, remote_mem_id);

            HASH_DEL(mmap_cache_removal[dev_id], cache_entry);
            MPL_free(cache_entry);
            cache_entry = NULL;

            HASH_FIND(hh, ipc_cache_mapped[dev_id], &lookup_entry.remote_mem_id, keylen,
                      cache_entry);

            if (cache_entry != NULL) {
                HASH_DEL(ipc_cache_mapped[dev_id], cache_entry);
                MPL_free(cache_entry);
                cache_entry = NULL;
            }
        }
    }

    ret_err = munmap(ptr, size);
    if (ret_err == -1) {
        goto fn_fail;
    }

  fn_exit:
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_gpu_ipc_handle_unmap(void *ptr)
{
    int mpl_err = MPL_SUCCESS;
    int dev_id;
    unsigned keylen;
    ze_result_t ret;
    ze_device_handle_t device = NULL;
    MPL_ze_mapped_buffer_entry_t *cache_entry = NULL;
    MPL_ze_mapped_buffer_lookup_t lookup_entry;

    ze_memory_allocation_properties_t ptr_attr = {
        .stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES,
        .pNext = NULL,
        .type = 0,
        .id = 0,
        .pageSize = 0,
    };

    ret = zeMemGetAllocProperties(ze_context, ptr, &ptr_attr, &device);
    ZE_ERR_CHECK(ret);

    dev_id = device_to_dev_id(device);
    if (dev_id == -1) {
        goto fn_fail;
    }

    if (likely(MPL_gpu_info.specialized_cache)) {
        /* Remove from the caches */
        HASH_FIND(hh, ipc_cache_removal[dev_id], &ptr, sizeof(void *), cache_entry);

        if (cache_entry != NULL) {
            memset(&lookup_entry, 0, sizeof(MPL_ze_mapped_buffer_lookup_t));
            lookup_entry.remote_mem_id = cache_entry->remote_mem_id;
            lookup_entry.remote_dev_id = cache_entry->remote_dev_id;
            lookup_entry.remote_pid = cache_entry->remote_pid;

            keylen = offsetof(MPL_ze_mapped_buffer_entry_t, remote_pid) + sizeof(pid_t) -
                offsetof(MPL_ze_mapped_buffer_entry_t, remote_mem_id);

            HASH_DEL(ipc_cache_removal[dev_id], cache_entry);
            MPL_free(cache_entry);
            cache_entry = NULL;

            HASH_FIND(hh, ipc_cache_mapped[dev_id], &lookup_entry.remote_mem_id, keylen,
                      cache_entry);

            if (cache_entry != NULL) {
                HASH_DEL(ipc_cache_mapped[dev_id], cache_entry);
                MPL_free(cache_entry);
                cache_entry = NULL;
            }
        }
    }

    /* Unmap the buffer */
    ret = zeMemCloseIpcHandle(ze_context, ptr);
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
    ret = zeMemGetAllocProperties(ze_context, ptr,
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
    ret = zeMemAllocDevice(ze_context, &device_desc, size, mem_alignment, h_device, ptr);

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
    ret = zeMemAllocHost(ze_context, &host_desc, size, mem_alignment, ptr);
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
    mpl_err = zeMemFree(ze_context, ptr);
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
    mpl_err = zeMemFree(ze_context, ptr);
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
    ret = zeMemGetAddressRange(ze_context, ptr, pbase, len);
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

void MPL_ze_ipc_remove_cache_handle(void *dptr)
{
    ze_result_t ret;
    ze_device_handle_t device;
    int local_dev_id = -1;
    uint64_t mem_id = 0;
    MPL_ze_ipc_handle_entry_t *cache_entry = NULL;

    ze_memory_allocation_properties_t ptr_attr = {
        .stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES,
        .pNext = NULL,
        .type = 0,
        .id = 0,
        .pageSize = 0,
    };

    ret = zeMemGetAllocProperties(ze_context, dptr, &ptr_attr, &device);
    ZE_ERR_CHECK(ret);

    if (ptr_attr.type == ZE_MEMORY_TYPE_DEVICE) {
        local_dev_id = device_to_dev_id(device);
        if (local_dev_id == -1) {
            goto fn_fail;
        }

        /* Remove entry if mem_id is cached */
        mem_id = ptr_attr.id;
        HASH_FIND(hh, ipc_cache_tracked[local_dev_id], &mem_id, sizeof(uint64_t), cache_entry);

        if (cache_entry) {
            HASH_DEL(ipc_cache_tracked[local_dev_id], cache_entry);
            MPL_free(cache_entry);
        }
    }

  fn_exit:
    return;
  fn_fail:
    fprintf(stderr, "Error > failed to complete MPL_ze_ipc_remove_cache_handle\n");
    goto fn_exit;
}

int MPL_ze_ipc_handle_create(const void *ptr, MPL_gpu_device_attr * ptr_attr, int local_dev_id,
                             int use_shared_fd, MPL_gpu_ipc_mem_handle_t * ipc_handle)
{
    int mpl_err = MPL_SUCCESS;
    ze_result_t ret;
    int fd, handle, status;
    ze_ipc_mem_handle_t ze_ipc_handle;
    fd_pid_t h;
    uint64_t mem_id = 0;

    mem_id = ptr_attr->prop.id;

    ret = zeMemGetIpcHandle(ze_context, ptr, &ze_ipc_handle);
    ZE_ERR_CHECK(ret);

    if (shared_device_fds != NULL) {
        if (use_shared_fd) {
            int shared_dev_id = MPL_gpu_get_root_device(local_dev_id);
            /* convert dma_buf fd to GEM handle */
            memcpy(&fd, &ze_ipc_handle, sizeof(fd));
            status = fd_to_handle(shared_device_fds[shared_dev_id], fd, &handle);
            if (status) {
                goto fn_fail;
            }

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

            memcpy(&h.fd, &handle, sizeof(int));
            h.dev_id = shared_dev_id;
        } else {
            memcpy(&h.fd, &ze_ipc_handle, sizeof(int));
            h.dev_id = MPL_gpu_local_to_global_dev_id(local_dev_id);
        }
    } else {
        memcpy(&h.fd, &ze_ipc_handle, sizeof(fd));
        h.dev_id = MPL_gpu_local_to_global_dev_id(local_dev_id);
        assert(h.dev_id != -1);
    }

    h.pid = mypid;
    h.mem_id = mem_id;
    memcpy(ipc_handle, &h, sizeof(fd_pid_t));

  fn_exit:
    return mpl_err;
  fn_fail:
    mpl_err = MPL_ERR_GPU_INTERNAL;
    goto fn_exit;
}

int MPL_ze_ipc_handle_map(MPL_gpu_ipc_mem_handle_t ipc_handle, int is_shared_handle, int dev_id,
                          int is_mmap, size_t size, void **ptr)
{
    int mpl_err = MPL_SUCCESS;
    ze_result_t ret;
    int fd, status;
    MPL_gpu_device_handle_t dev_handle;
    ze_ipc_mem_handle_t ze_ipc_handle;

    memset(&ze_ipc_handle, 0, sizeof(ze_ipc_mem_handle_t));

    fd_pid_t h;
    memcpy(&h, &ipc_handle, sizeof(fd_pid_t));

    if (shared_device_fds != NULL) {
        /* convert GEM handle to fd */
        status = handle_to_fd(shared_device_fds[h.dev_id], h.fd, &fd);
        if (status) {
            goto fn_fail;
        }
    } else {
        /* pidfd_getfd */
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

    if (is_mmap) {
        void *buf;
        buf = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (buf == (void *) -1) {
            mpl_err = MPL_ERR_GPU_INTERNAL;
            perror("mmap device to host");
            printf("gdr_handle_open failed\n");
            goto fn_fail;
        }
        *ptr = buf;
    } else {
        mpl_err = dev_id_to_device(dev_id, &dev_handle);
        if (mpl_err != MPL_SUCCESS) {
            goto fn_fail;
        }
        memcpy(&ze_ipc_handle, &fd, sizeof(fd));

        ret = zeMemOpenIpcHandle(ze_context, dev_handle, ze_ipc_handle, 0, ptr);
        ZE_ERR_CHECK(ret);
    }

  fn_exit:
    return mpl_err;
  fn_fail:
    goto fn_exit;
}

int MPL_ze_ipc_handle_mmap_host(MPL_gpu_ipc_mem_handle_t ipc_handle, int is_shared_handle,
                                int dev_id, size_t size, void **ptr)
{
    int mpl_err = MPL_SUCCESS;
    unsigned keylen;
    MPL_ze_mapped_buffer_entry_t *cache_entry = NULL;
    MPL_ze_mapped_buffer_entry_t *removal_entry = NULL;
    MPL_ze_mapped_buffer_lookup_t lookup_entry;

    fd_pid_t h;
    memcpy(&h, &ipc_handle, sizeof(fd_pid_t));

    if (likely(MPL_gpu_info.specialized_cache)) {
        /* Check if ipc-mapped buffer is already cached */
        memset(&lookup_entry, 0, sizeof(MPL_ze_mapped_buffer_lookup_t));
        lookup_entry.remote_mem_id = h.mem_id;
        lookup_entry.remote_dev_id = h.dev_id;
        lookup_entry.remote_pid = h.pid;

        keylen = offsetof(MPL_ze_mapped_buffer_entry_t, remote_pid) + sizeof(pid_t) -
            offsetof(MPL_ze_mapped_buffer_entry_t, remote_mem_id);

        HASH_FIND(hh, ipc_cache_mapped[dev_id], &lookup_entry.remote_mem_id, keylen, cache_entry);
    }

    if (cache_entry) {
        *ptr = cache_entry->mapped_ptr;
    } else {
        mpl_err = MPL_ze_ipc_handle_map(ipc_handle, is_shared_handle, dev_id, true, size, ptr);
        if (mpl_err != MPL_SUCCESS) {
            goto fn_fail;
        }

        if (likely(MPL_gpu_info.specialized_cache)) {
            /* Insert into the cache */
            cache_entry =
                (MPL_ze_mapped_buffer_entry_t *) MPL_malloc(sizeof(MPL_ze_mapped_buffer_entry_t),
                                                            MPL_MEM_OTHER);
            if (cache_entry == NULL) {
                mpl_err = MPL_ERR_GPU_NOMEM;
                goto fn_fail;
            }

            removal_entry =
                (MPL_ze_mapped_buffer_entry_t *) MPL_malloc(sizeof(MPL_ze_mapped_buffer_entry_t),
                                                            MPL_MEM_OTHER);
            if (removal_entry == NULL) {
                mpl_err = MPL_ERR_GPU_NOMEM;
                goto fn_fail;
            }

            memset(removal_entry, 0, sizeof(MPL_ze_mapped_buffer_entry_t));
            memset(cache_entry, 0, sizeof(MPL_ze_mapped_buffer_entry_t));

            cache_entry->remote_mem_id = lookup_entry.remote_mem_id;
            cache_entry->remote_dev_id = lookup_entry.remote_dev_id;
            cache_entry->remote_pid = lookup_entry.remote_pid;
            cache_entry->mapped_ptr = *ptr;
            cache_entry->mapped_size = size;
            memcpy(removal_entry, cache_entry, sizeof(MPL_ze_mapped_buffer_entry_t));

            HASH_ADD(hh, ipc_cache_mapped[dev_id], remote_mem_id, keylen, cache_entry,
                     MPL_MEM_OTHER);
            HASH_ADD(hh, mmap_cache_removal[dev_id], mapped_ptr, sizeof(void *), removal_entry,
                     MPL_MEM_OTHER);
        }
    }

  fn_exit:
    return mpl_err;
  fn_fail:
    goto fn_exit;
}

int MPL_ze_mmap_device_pointer(void *dptr, MPL_gpu_device_attr * attr,
                               MPL_gpu_device_handle_t device, void **mmaped_ptr)
{
    ze_result_t ret;
    ze_ipc_mem_handle_t ze_ipc_handle;
    int fd, mpl_err = MPL_SUCCESS, local_dev_id = -1;
    uint64_t mem_id, offset, len;
    void *pbase, *base;
    MPL_ze_ipc_handle_entry_t *cache_entry = NULL;

    ret = zeMemGetAddressRange(ze_context, dptr, &pbase, &len);
    ZE_ERR_CHECK(ret);

    offset = (char *) dptr - (char *) pbase;

    mem_id = attr->prop.id;
    local_dev_id = device_to_dev_id(device);
    if (local_dev_id == -1) {
        goto fn_fail;
    }

    /* Search cache for mapped base address */
    if (likely(MPL_gpu_info.specialized_cache)) {
        HASH_FIND(hh, ipc_cache_tracked[local_dev_id], &mem_id, sizeof(uint64_t), cache_entry);
    }

    if (cache_entry && cache_entry->mapped_ptr) {
        base = cache_entry->mapped_ptr;
    } else {
        ret = zeMemGetIpcHandle(ze_context, pbase, &ze_ipc_handle);
        ZE_ERR_CHECK(ret);

        memcpy(&fd, &ze_ipc_handle, sizeof(int));

        base = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (base == (void *) -1) {
            mpl_err = MPL_ERR_GPU_INTERNAL;
            perror("mmap_device_pointer:");
            printf("gdr_handle_open failed\n");
            goto fn_fail;
        }

        if (likely(MPL_gpu_info.specialized_cache)) {
            if (cache_entry) {
                /* This could have been cached already, but via the ipc path */
                cache_entry->mapped_ptr = base;
                cache_entry->mapped_size = len;
            } else {
                /* Insert into the cache */
                cache_entry =
                    (MPL_ze_ipc_handle_entry_t *) MPL_malloc(sizeof(MPL_ze_ipc_handle_entry_t),
                                                             MPL_MEM_OTHER);

                if (cache_entry == NULL) {
                    mpl_err = MPL_ERR_GPU_NOMEM;
                    goto fn_fail;
                }

                memset(cache_entry, 0, sizeof(MPL_ze_ipc_handle_entry_t));

                cache_entry->mem_id = mem_id;
                cache_entry->mapped_ptr = base;
                cache_entry->mapped_size = len;
                cache_entry->handle_cached = false;
                HASH_ADD(hh, ipc_cache_tracked[local_dev_id], mem_id, sizeof(uint64_t), cache_entry,
                         MPL_MEM_OTHER);
            }
        }
    }

    *mmaped_ptr = (char *) base + offset;

  fn_exit:
    return mpl_err;
  fn_fail:
    goto fn_exit;
}

int MPL_gpu_fast_memcpy(void *src, MPL_pointer_attr_t * src_attr, void *dest,
                        MPL_pointer_attr_t * dest_attr, size_t size)
{
    int mpl_err = MPL_SUCCESS;
    char *d = (char *) dest;
    const char *s = (const char *) src;
    size_t n = size;

    if (src_attr && src_attr->type == MPL_GPU_POINTER_DEV) {
        mpl_err =
            MPL_ze_mmap_device_pointer(src, &src_attr->device_attr, src_attr->device, (void **) &s);
        if (mpl_err != MPL_SUCCESS)
            goto fn_fail;
    }

    if (dest_attr && dest_attr->type == MPL_GPU_POINTER_DEV) {
        mpl_err =
            MPL_ze_mmap_device_pointer(dest, &dest_attr->device_attr, dest_attr->device,
                                       (void **) &d);
        if (mpl_err != MPL_SUCCESS)
            goto fn_fail;
    }
#if defined(MPL_HAVE_MM512_STOREU_SI512)
    while (n >= 64) {
        _mm512_storeu_si512((__m512i *) d, _mm512_loadu_si512((__m512i const *) s));
        d += 64;
        s += 64;
        n -= 64;
    }
    if (n & 32) {
        _mm256_storeu_si256((__m256i *) d, _mm256_loadu_si256((__m256i const *) s));
        d += 32;
        s += 32;
        n -= 32;
    }
#elif defined(MPL_HAVE_MM256_STOREU_SI256)
    while (n >= 32) {
        _mm256_storeu_si256((__m256i *) d, _mm256_loadu_si256((__m256i const *) s));
        d += 32;
        s += 32;
        n -= 32;
    }
#else
    goto fallback;
#endif
    if (n & 16) {
        _mm_storeu_si128((__m128i *) d, _mm_loadu_si128((__m128i const *) s));
        d += 16;
        s += 16;
        n -= 16;
    }
    if (n & 8) {
        *(int64_t *) d = *(int64_t *) s;
        d += 8;
        s += 8;
        n -= 8;
    }
    if (n & 4) {
        *(int *) d = *(int *) s;
        d += 4;
        s += 4;
        n -= 4;
    }
    if (n & 2) {
        *(int16_t *) d = *(int16_t *) s;
        d += 2;
        s += 2;
        n -= 2;
    }
    if (n == 1) {
        *(char *) d = *(char *) s;
    }
#if defined(MPL_HAVE_MM256_STOREU_SI256)
    _mm_sfence();
#endif
    goto fn_exit;

  fallback:
    memcpy(d, s, n);

  fn_exit:
    return mpl_err;
  fn_fail:
    return MPL_ERR_GPU_INTERNAL;
}

#endif /* MPL_HAVE_ZE */
