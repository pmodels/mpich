/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpl.h"
#include <assert.h>
#include <dlfcn.h>

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#ifdef MPL_HAVE_ZE
#include <sys/syscall.h>        /* Definition of syscall constants */

static int gpu_initialized = 0;
static int device_count;
static int max_dev_id;

static int *local_to_global_map;        /* [device_count] */
static int *global_to_local_map;        /* [max_dev_id + 1]   */

/* pidfd */
#ifndef __NR_pidfd_open
#define __NR_pidfd_open 434     /* Syscall id for most architectures */
#endif
#ifndef __NR_pidfd_getfd
#define __NR_pidfd_getfd 438    /* Syscall id for most architectures */
#endif

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
ze_device_handle_t *global_ze_devices_handle = NULL;
ze_context_handle_t global_ze_context;
uint32_t global_ze_device_count;
static int gpu_ze_init_driver(void);

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
    *dev_cnt = device_count;
    *dev_id = max_dev_id;
    return ret;
}

int MPL_gpu_init(MPL_gpu_info_t * info)
{
    int mpl_err;
    int fd = -1, pidfd = -1, dupfd = -1;
    char tmp_fname[] = "/tmp/mpich_ze_pidfd_getXXXXXXXXXX";
    char *chosen_fname = tmp_fname;

    mpl_err = gpu_ze_init_driver();
    if (mpl_err != MPL_SUCCESS)
        goto fn_fail;

    info->enable_ipc = false;

    device_count = global_ze_device_count;
    max_dev_id = device_count - 1;

    local_to_global_map = MPL_malloc(device_count * sizeof(int), MPL_MEM_OTHER);
    global_to_local_map = MPL_malloc(device_count * sizeof(int), MPL_MEM_OTHER);
    for (int i = 0; i < device_count; i++) {
        local_to_global_map[i] = i;
        global_to_local_map[i] = i;
    }

    mypid = getpid();

    gpu_mem_hook_init();
    gpu_initialized = 1;

    /* Test if ipc can be supported via pidfd_open/pidfd_get */
    fd = MPL_mkstemp(chosen_fname);
    if (fd == -1)
        goto fn_fail;

    pidfd = syscall(__NR_pidfd_open, mypid, 0);
    if (pidfd == -1)
        goto fn_fail;

    dupfd = syscall(__NR_pidfd_getfd, pidfd, fd, 0);
    if (pidfd == -1)
        goto fn_fail;

    info->enable_ipc = true;

    if (info->debug_summary) {
        printf("==== GPU Init (ZE) ====\n");
        printf("device_count: %d\n", device_count);
        printf("=========================\n");
    }

  fn_exit:
    if (dupfd)
        close(dupfd);
    if (pidfd)
        close(pidfd);
    if (fd) {
        close(fd);
        unlink(chosen_fname);
    }
    return mpl_err;
  fn_fail:
    goto fn_exit;
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
        global_ze_device_count = 0;
        ret = zeDeviceGet(all_drivers[i], &global_ze_device_count, NULL);
        ZE_ERR_CHECK(ret);
        global_ze_devices_handle =
            MPL_malloc(global_ze_device_count * sizeof(ze_device_handle_t), MPL_MEM_OTHER);
        if (global_ze_devices_handle == NULL) {
            ret_error = MPL_ERR_GPU_NOMEM;
            goto fn_fail;
        }
        ret = zeDeviceGet(all_drivers[i], &global_ze_device_count, global_ze_devices_handle);
        ZE_ERR_CHECK(ret);
        /* Check if the driver supports a gpu */
        for (d = 0; d < global_ze_device_count; ++d) {
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
    MPL_free(local_to_global_map);
    MPL_free(global_to_local_map);
    MPL_free(global_ze_devices_handle);

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
    assert(local_dev_id < device_count);
    return local_to_global_map[local_dev_id];
}

int MPL_gpu_ipc_handle_create(const void *ptr, MPL_gpu_ipc_mem_handle_t * ipc_handle)
{
    int mpl_err = MPL_SUCCESS;
    ze_result_t ret;
    int i, dev_id = -1;
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

    for (i = 0; i < device_count; i++) {
        if (device == global_ze_devices_handle[i]) {
            dev_id = i;
            break;
        }
    }

    if (dev_id == -1) {
        goto fn_fail;
    }

    fd_pid_t h;
    memcpy(&h.fd, &ze_ipc_handle, sizeof(int));
    h.pid = mypid;
    h.dev_id = dev_id;
    memcpy(ipc_handle, &h, sizeof(fd_pid_t));

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
    int fd;
    MPL_gpu_device_handle_t dev_handle;
    ze_ipc_mem_handle_t ze_ipc_handle;

    memset(&ze_ipc_handle, 0, sizeof(ze_ipc_mem_handle_t));

    /* pidfd_getfd */
    fd_pid_t h;
    memcpy(&h, &ipc_handle, sizeof(fd_pid_t));
    if (h.pid != mypid) {
        int pid_fd = syscall(__NR_pidfd_open, h.pid, 0);
        assert(pid_fd != -1);
        fd = syscall(__NR_pidfd_getfd, pid_fd, h.fd, 0);
        if (fd == -1) {
            mpl_err = MPL_ERR_GPU_INTERNAL;
            goto fn_fail;
        }
        close(pid_fd);
    } else {
        fd = h.fd;
    }

    dev_handle = global_ze_devices_handle[dev_id];
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

int MPL_gpu_compare_pointer_attr(MPL_pointer_attr_t attr1, MPL_pointer_attr_t attr2, bool * same)
{
    int mpl_err = MPL_SUCCESS;
    *same = (attr1.device_attr.prop.id == attr2.device_attr.prop.id);
    return mpl_err;
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
    for (int i = 0; i < global_ze_device_count; i++) {
        if (global_ze_devices_handle[i] == attr->device) {
            dev_id = i;
            break;
        }
    }
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

static int gpu_mem_hook_init(void)
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

ze_result_t ZE_APICALL zeMemFree(ze_context_handle_t hContext, void *dptr)
{
    ze_result_t result;
    /* in case when MPI_Init was skipped */
    gpu_mem_hook_init();
    gpu_free_hooks_cb(dptr);
    result = sys_zeMemFree(hContext, dptr);
    return (result);
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

#endif /* MPL_HAVE_ZE */
