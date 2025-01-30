/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include "yaksa_config.h"
#include "yaksa.h"
#include "dtpools.h"
#include "pack-common.h"

#ifdef HAVE_ZE

#include "level_zero/ze_api.h"

extern int use_subdevices;

static ze_driver_handle_t global_ze_driver_handle;
static ze_device_handle_t *global_devices = NULL;
static ze_context_handle_t ze_context;
static uint32_t device_count = 0;

static ze_command_list_handle_t **cmdList;
static ze_command_queue_handle_t **cmdQueue;
static int n_threads = 0;


int pack_ze_get_ndevices(void)
{
    assert(device_count != 0);
    return (int) device_count;
}

void pack_ze_init_devices(int num_threads)
{
    ze_result_t ret;
    ze_init_flag_t flags = ZE_INIT_FLAG_GPU_ONLY;
    ret = zeInit(flags);
    assert(ret == ZE_RESULT_SUCCESS);
    uint32_t driver_count = 0;
    ze_driver_handle_t *all_drivers;
    int loc = 0;
    ret = zeDriverGet(&driver_count, NULL);
    assert(ret == ZE_RESULT_SUCCESS);
    assert(driver_count > 0);

    all_drivers = malloc(driver_count * sizeof(ze_driver_handle_t));
    assert(all_drivers != NULL);
    ret = zeDriverGet(&driver_count, all_drivers);
    assert(ret == ZE_RESULT_SUCCESS);
    global_ze_driver_handle = all_drivers[0];

    ze_device_handle_t *all_devices = NULL;
    ret = zeDeviceGet(all_drivers[0], &device_count, NULL);
    assert(ret == ZE_RESULT_SUCCESS);
    all_devices = malloc(device_count * sizeof(ze_device_handle_t));
    assert(all_devices != NULL);
    ret = zeDeviceGet(all_drivers[0], &device_count, all_devices);
    assert(ret == ZE_RESULT_SUCCESS);
    global_devices = all_devices;
    if (use_subdevices) {
        ze_device_handle_t *subdevices;
        int subdevice_count = 0;
        for (int i = 0; i < device_count; i++) {
            int subcount = 0;
            ret = zeDeviceGetSubDevices(all_devices[i], &subcount, NULL);
            assert(ret == ZE_RESULT_SUCCESS);
            subdevice_count += subcount;
        }
        subdevices = (ze_device_handle_t *) malloc(sizeof(ze_device_handle_t) * subdevice_count);
        int c = 0;
        for (int i = 0; i < device_count; i++) {
            int subcount = 0;
            ret = zeDeviceGetSubDevices(all_devices[i], &subcount, NULL);
            assert(ret == ZE_RESULT_SUCCESS);
            ret = zeDeviceGetSubDevices(all_devices[i], &subcount, subdevices + c);
            assert(ret == ZE_RESULT_SUCCESS);
            c += subcount;
        }
        device_count = subdevice_count;
        global_devices = subdevices;
    }

    ze_context_desc_t contextDesc = {
        .stype = ZE_STRUCTURE_TYPE_CONTEXT_DESC,
        .pNext = NULL,
        .flags = 0,
    };
    ret = zeContextCreate(global_ze_driver_handle, &contextDesc, &ze_context);
    assert(ret == ZE_RESULT_SUCCESS);

    n_threads = num_threads;
    cmdList =
        (ze_command_list_handle_t **) malloc(device_count * sizeof(ze_command_list_handle_t *));
    cmdQueue =
        (ze_command_queue_handle_t **) malloc(device_count * sizeof(ze_command_queue_handle_t *));

    for (int i = 0; i < device_count; i++) {
        int j;
        cmdList[i] =
            (ze_command_list_handle_t *) malloc(sizeof(ze_command_list_handle_t) * num_threads);
        cmdQueue[i] =
            (ze_command_queue_handle_t *) malloc(sizeof(ze_command_queue_handle_t) * num_threads);
        ze_command_list_desc_t cmdListDesc = {
            .stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC,
            .pNext = NULL,
            .commandQueueGroupOrdinal = 0,
            .flags = 0,
        };

        ze_command_queue_desc_t cmdQueueDesc = {
            .stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC,
            .pNext = NULL,
            .index = 0,
            .flags = 0,
            .ordinal = 0,
            .mode = ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS,
            .priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
        };

        int numQueueGroups = 0;
        ret = zeDeviceGetCommandQueueGroupProperties(global_devices[i], &numQueueGroups, NULL);
        assert(ret == ZE_RESULT_SUCCESS && numQueueGroups);
        ze_command_queue_group_properties_t *queueProperties =
            (ze_command_queue_group_properties_t *)
            calloc(numQueueGroups, sizeof(ze_command_queue_group_properties_t));
        ret =
            zeDeviceGetCommandQueueGroupProperties(global_devices[i], &numQueueGroups,
                                                   queueProperties);
        for (j = 0; j < numQueueGroups; j++) {
            if (queueProperties[j].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) {
                cmdQueueDesc.ordinal = j;
                break;
            }
        }
        assert(j != numQueueGroups);
        free(queueProperties);

        cmdListDesc.commandQueueGroupOrdinal = cmdQueueDesc.ordinal;

        for (j = 0; j < num_threads; j++) {
            ret = zeCommandListCreate(ze_context, global_devices[i], &cmdListDesc, &cmdList[i][j]);
            assert(ret == ZE_RESULT_SUCCESS);

            ret =
                zeCommandQueueCreate(ze_context, global_devices[i], &cmdQueueDesc, &cmdQueue[i][j]);
            assert(ret == ZE_RESULT_SUCCESS);
        }
    }

  fn_exit:
    free(all_drivers);
    return;
  fn_fail:
    free(all_devices);
    goto fn_exit;
}

void pack_ze_finalize_devices()
{
    ze_result_t ret;
    for (int i = 0; i < device_count; i++) {
        for (int j = 0; j < n_threads; j++) {
            ret = zeCommandListDestroy(cmdList[i][j]);
            assert(ret == ZE_RESULT_SUCCESS);
            ret = zeCommandQueueDestroy(cmdQueue[i][j]);
            assert(ret == ZE_RESULT_SUCCESS);
        }
        free(cmdList[i]);
        free(cmdQueue[i]);
    }
    free(cmdList);
    free(cmdQueue);
    zeContextDestroy(ze_context);
    free(global_devices);
}

void pack_ze_alloc_mem(int device_id, size_t size, mem_type_e type, void **hostbuf,
                       void **devicebuf)
{
    ze_result_t ret;
    if (type == MEM_TYPE__REGISTERED_HOST) {
        size_t mem_alignment = 1;
        ze_host_mem_alloc_desc_t host_desc = {
            .stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC,
            .pNext = NULL,
            .flags = 0,
        };
        ret = zeMemAllocHost(ze_context, &host_desc, size, mem_alignment, devicebuf);
        assert(ret == ZE_RESULT_SUCCESS);
        if (hostbuf)
            *hostbuf = *devicebuf;
    } else if (type == MEM_TYPE__DEVICE) {
        /* allocate devicebuf */
        size_t mem_alignment = 1;
        ze_device_mem_alloc_desc_t device_desc = {
            .stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC,
            .pNext = NULL,
            .flags = 0,
            .ordinal = 0,
        };
        ret = zeMemAllocDevice(ze_context, &device_desc,
                               size, mem_alignment, global_devices[device_id], devicebuf);
        assert(ret == ZE_RESULT_SUCCESS);

        /* allocate hostbuf */
        if (hostbuf) {
            size_t mem_alignment = 1;
            ze_host_mem_alloc_desc_t host_desc = {
                .stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC,
                .pNext = NULL,
                .flags = 0,
            };
            ret = zeMemAllocHost(ze_context, &host_desc, size, mem_alignment, hostbuf);
            assert(ret == ZE_RESULT_SUCCESS);
        }
    } else if (type == MEM_TYPE__MANAGED) {
        /* allocate devicebuf */
        size_t mem_alignment = 1;
        ze_device_mem_alloc_desc_t device_desc = {
            .stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC,
            .pNext = NULL,
            .flags = 0,
            .ordinal = 0,
        };
        ze_host_mem_alloc_desc_t host_desc = {
            .stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC,
            .pNext = NULL,
            .flags = 0,
        };
        ret = zeMemAllocShared(ze_context, &device_desc, &host_desc,
                               size, mem_alignment,
                               device_id == -1 ? NULL : global_devices[device_id], devicebuf);
        assert(ret == ZE_RESULT_SUCCESS);

        if (hostbuf)
            *hostbuf = *devicebuf;
    } else {
        fprintf(stderr, "ERROR: unsupported memory type\n");
        exit(1);
    }
}

void pack_ze_free_mem(mem_type_e type, void *hostbuf, void *devicebuf)
{
    if (type == MEM_TYPE__REGISTERED_HOST) {
        zeMemFree(ze_context, devicebuf);
    } else if (type == MEM_TYPE__MANAGED) {
        zeMemFree(ze_context, devicebuf);
    } else if (type == MEM_TYPE__DEVICE) {
        zeMemFree(ze_context, devicebuf);
        if (hostbuf) {
            zeMemFree(ze_context, hostbuf);
        }
    } else {
        fprintf(stderr, "ERROR: unsupported memory type\n");
        exit(1);
    }
}

void pack_ze_get_ptr_attr(const void *inbuf, void *outbuf, yaksa_info_t * info, int iter)
{
    if (iter % 2 == 0) {
        int rc;
        ze_result_t zerr;
        typedef struct {
            ze_memory_allocation_properties_t prop;
            ze_device_handle_t device;
        } ze_alloc_attr_t;

        rc = yaksa_info_create(info);
        assert(rc == YAKSA_SUCCESS);

        rc = yaksa_info_keyval_append(*info, "yaksa_gpu_driver", "ze", strlen("ze"));
        assert(rc == YAKSA_SUCCESS);

        ze_alloc_attr_t attr;
        memset(&attr.prop, 0, sizeof(ze_memory_allocation_properties_t));
        zerr = zeMemGetAllocProperties(ze_context, inbuf, &attr.prop, &attr.device);
        assert(zerr == ZE_RESULT_SUCCESS);
        rc = yaksa_info_keyval_append(*info, "yaksa_ze_inbuf_ptr_attr", &attr, sizeof(attr));
        assert(rc == YAKSA_SUCCESS);

        zerr = zeMemGetAllocProperties(ze_context, outbuf, &attr.prop, &attr.device);
        assert(zerr == ZE_RESULT_SUCCESS);
        rc = yaksa_info_keyval_append(*info, "yaksa_ze_outbuf_ptr_attr", &attr, sizeof(attr));
        assert(rc == YAKSA_SUCCESS);
    } else
        *info = NULL;
}

void pack_ze_copy_content(int tid, const void *sbuf, void *dbuf, size_t size, mem_type_e type)
{
    int i;
    if (type == MEM_TYPE__DEVICE) {
        int ret;
        ze_device_handle_t device = NULL;

        struct {
            ze_memory_allocation_properties_t prop;
            ze_device_handle_t device;
        } s_attr, d_attr;
        memset(&s_attr, 0, sizeof(s_attr));
        memset(&d_attr, 0, sizeof(d_attr));
        ret = zeMemGetAllocProperties(ze_context, sbuf, &s_attr.prop, &s_attr.device);
        assert(ret == ZE_RESULT_SUCCESS);
        ret = zeMemGetAllocProperties(ze_context, dbuf, &d_attr.prop, &d_attr.device);
        assert(ret == ZE_RESULT_SUCCESS);
        assert(s_attr.device == NULL || d_attr.device == NULL);
        if (s_attr.device) {
            device = s_attr.device;
        } else if (d_attr.device) {
            device = d_attr.device;
        }
        assert(device);

        int dev_id;
        for (dev_id = 0; dev_id < device_count; dev_id++) {
            if (device == global_devices[dev_id])
                break;
        }
        assert(dev_id < device_count);

        ret = zeCommandListReset(cmdList[dev_id][tid]);
        ret = zeCommandListAppendMemoryCopy(cmdList[dev_id][tid], dbuf, sbuf, size, NULL, 0, NULL);
        assert(ret == ZE_RESULT_SUCCESS);
        ret = zeCommandListClose(cmdList[dev_id][tid]);
        assert(ret == ZE_RESULT_SUCCESS);
        ret =
            zeCommandQueueExecuteCommandLists(cmdQueue[dev_id][tid], 1, &cmdList[dev_id][tid],
                                              NULL);
        assert(ret == ZE_RESULT_SUCCESS);
    }
}

void *pack_ze_create_stream(void)
{
    assert(0 && "not supported");
    return NULL;
}

void pack_ze_destroy_stream(void *stream_p)
{
    assert(0 && "not supported");
}

void pack_ze_stream_synchronize(void *stream_p)
{
    assert(0 && "not supported");
}

#endif /* HAVE_ZE */
