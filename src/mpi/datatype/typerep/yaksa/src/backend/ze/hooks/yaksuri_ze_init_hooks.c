/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksuri_zei.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <level_zero/ze_api.h>

static void *ze_host_malloc(uintptr_t size)
{
    void *ptr = NULL;

    ze_host_mem_alloc_desc_t host_desc = {
        .stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC,
        .pNext = NULL,
        .flags = 0,
    };

    ze_result_t zerr = zeMemAllocHost(yaksuri_zei_global.context, &host_desc, size, 1, &ptr);
    YAKSURI_ZEI_ZE_ERR_CHECK(zerr);

    return ptr;
}

static void *ze_gpu_malloc(uintptr_t size, int dev)
{
    void *ptr = NULL;

    ze_device_mem_alloc_desc_t device_desc = {
        .stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC,
        .pNext = NULL,
        .flags = 0,
        .ordinal = 0,
    };

    ze_result_t zerr = zeMemAllocDevice(yaksuri_zei_global.context, &device_desc, size, 1,
                                        yaksuri_zei_global.device[dev], &ptr);
    YAKSURI_ZEI_ZE_ERR_CHECK(zerr);

    return ptr;
}

static void ze_host_free(void *ptr)
{
    ze_result_t zerr = zeMemFree(yaksuri_zei_global.context, ptr);
    YAKSURI_ZEI_ZE_ERR_CHECK(zerr);
}

static void ze_gpu_free(void *ptr)
{
    ze_result_t zerr = zeMemFree(yaksuri_zei_global.context, ptr);
    YAKSURI_ZEI_ZE_ERR_CHECK(zerr);
}

yaksuri_zei_global_s yaksuri_zei_global;

static int finalize_hook(void)
{
    int rc = YAKSA_SUCCESS;
    ze_result_t zerr;

    zerr = yaksuri_ze_finalize_module_kernel();
    YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);

    for (int i = 0; i < yaksuri_zei_global.ndevices; i++) {
        free(yaksuri_zei_global.p2p[i]);
    }
    zerr = zeContextDestroy(yaksuri_zei_global.context);
    YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);

    free(yaksuri_zei_global.p2p);
    free(yaksuri_zei_global.device);

    for (int i = 0; i < yaksuri_zei_global.ndevices; i++) {
        yaksuri_zei_device_state_s *device_state = yaksuri_zei_global.device_states + i;
        zerr = zeEventPoolDestroy(device_state->ep);
        YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
        if (device_state->cmdlist)
            zerr = zeCommandListDestroy(device_state->cmdlist);
        YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
        free(device_state->events);
        free(device_state->queueProperties);
    }
    free(yaksuri_zei_global.device_states);

    pthread_mutex_destroy(&yaksuri_zei_global.ze_mutex);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int get_num_devices(int *ndevices)
{
    *ndevices = yaksuri_zei_global.ndevices;

    return YAKSA_SUCCESS;
}

static bool check_p2p_comm(int sdev, int ddev)
{
    bool is_enabled = 0;
#if ZE_P2P == ZE_P2P_ENABLED
    is_enabled = yaksuri_zei_global.p2p[sdev][ddev];
#elif ZE_P2P == ZE_P2P_CLIQUES
    if ((sdev + ddev) % 2 == 0)
        is_enabled = yaksuri_zei_global.p2p[sdev][ddev];
#endif

    return is_enabled;
}

int yaksuri_ze_init_hook(yaksur_gpudriver_hooks_s ** hooks)
{
    int rc = YAKSA_SUCCESS;
    ze_result_t zerr;
    uint32_t num_drivers = 0;
    ze_driver_handle_t *all_drivers = NULL;
    uint32_t num_devices = 0;
    ze_device_handle_t *all_devices = NULL;
    ze_context_desc_t contextDesc;
    ze_event_pool_desc_t pool_desc;

    ze_init_flag_t flags = ZE_INIT_FLAG_GPU_ONLY;
    zerr = zeInit(flags);
    /* exit normally when GPU device can not be initialized */
    if (zerr != ZE_RESULT_SUCCESS) {
        *hooks = NULL;
        goto fn_exit;
    }

    yaksuri_zei_global.driver = NULL;
    yaksuri_zei_global.device = NULL;
    yaksuri_zei_global.context = NULL;

    /* get driver for Intel GPUs by first discovering all the drivers,
     * and then picks the first driver that supports GPU devices */
    zerr = zeDriverGet(&num_drivers, NULL);
    YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);

    all_drivers = (ze_driver_handle_t *) malloc(num_drivers * sizeof(ze_driver_handle_t));
    zerr = zeDriverGet(&num_drivers, all_drivers);
    YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);

    yaksuri_zei_global.driver = all_drivers[0];
    free(all_drivers);

    zerr = zeDeviceGet(yaksuri_zei_global.driver, &num_devices, NULL);
    YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);

    all_devices = (ze_device_handle_t *) malloc(num_devices * sizeof(ze_device_handle_t));
    zerr = zeDeviceGet(yaksuri_zei_global.driver, &num_devices, all_devices);
    YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);

    yaksuri_zei_global.ndevices = num_devices;
    yaksuri_zei_global.device = all_devices;

    contextDesc.stype = ZE_STRUCTURE_TYPE_CONTEXT_DESC;
    contextDesc.pNext = NULL;
    contextDesc.flags = 0;
    zerr = zeContextCreate(yaksuri_zei_global.driver, &contextDesc, &yaksuri_zei_global.context);
    assert(zerr == ZE_RESULT_SUCCESS);

#if ZE_DEBUG
    /* assuming homogeneous devices */
    ze_device_properties_t deviceProperties = {
        .stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES,
        .pNext = NULL,
    };
    zerr = zeDeviceGetProperties(yaksuri_zei_global.device[0], &deviceProperties);
    assert(zerr == ZE_RESULT_SUCCESS);

    printf("maxHardwareContexts: %u\n", deviceProperties.maxHardwareContexts);
    printf("maxMemAllocSize: %ld\n", deviceProperties.maxMemAllocSize);
    printf("numThreadsPerEU: %d\n", deviceProperties.numThreadsPerEU);
#endif

    /* device state */
    yaksuri_zei_global.device_states =
        (yaksuri_zei_device_state_s *) calloc(yaksuri_zei_global.ndevices,
                                              sizeof(yaksuri_zei_device_state_s));
    yaksuri_zei_global.throttle_threshold = ZE_THROTTLE_THRESHOLD;
    pool_desc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
    pool_desc.pNext = NULL;
    pool_desc.flags = 0;
    pool_desc.count = ZE_EVENT_POOL_CAP;

    for (int i = 0; i < yaksuri_zei_global.ndevices; i++) {
        yaksuri_zei_device_state_s *device_state = yaksuri_zei_global.device_states + i;
        device_state->dev_id = i;
        ze_device_properties_t deviceProperties = {
            .stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES,
            .pNext = NULL,
        };
        zerr = zeDeviceGetProperties(yaksuri_zei_global.device[i], &deviceProperties);
        YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
        device_state->deviceId = deviceProperties.deviceId;

        /* discover sub-devices */
        uint32_t subcount = 0;
        zerr = zeDeviceGetSubDevices(yaksuri_zei_global.device[i], &subcount, NULL);
        YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
        if (subcount == 1)
            subcount = 0;
        device_state->nsubdevices = subcount;
        if (subcount > 1) {
            device_state->subdevices =
                (ze_device_handle_t *) malloc(sizeof(ze_device_handle_t) * subcount);
            zerr =
                zeDeviceGetSubDevices(yaksuri_zei_global.device[i], &subcount,
                                      device_state->subdevices);
            YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
        }

        /* create one event pool for each device */
        zerr = zeEventPoolCreate(yaksuri_zei_global.context, &pool_desc, 1,
                                 &yaksuri_zei_global.device[i], &device_state->ep);
        YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
        device_state->events =
            (ze_event_handle_t *) calloc(ZE_EVENT_POOL_CAP, sizeof(ze_event_handle_t));
        device_state->last_event_idx = -1;
        device_state->ev_lb = device_state->ev_ub = -1;
        pthread_mutex_init(&device_state->mutex, NULL);
    }

    /* check for p2p capability */
    yaksuri_zei_global.p2p = (bool **) malloc(yaksuri_zei_global.ndevices * sizeof(bool *));
    for (int i = 0; i < yaksuri_zei_global.ndevices; i++) {
        yaksuri_zei_global.p2p[i] = (bool *) malloc(yaksuri_zei_global.ndevices * sizeof(bool));

        for (int j = 0; j < yaksuri_zei_global.ndevices; j++) {
            if (i == j) {
                yaksuri_zei_global.p2p[i][j] = 1;
            } else {
                ze_bool_t val;
                zerr =
                    zeDeviceCanAccessPeer(yaksuri_zei_global.device[i],
                                          yaksuri_zei_global.device[j], &val);
                YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);

                if (val) {
                    yaksuri_zei_global.p2p[i][j] = true;
                } else {
                    yaksuri_zei_global.p2p[i][j] = false;
                }
            }
        }
    }

    /* init module and kernels */
    zerr = yaksuri_ze_init_module_kernel();
    YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);

    pthread_mutex_init(&yaksuri_zei_global.ze_mutex, NULL);

    /* set function pointers for the remaining functions */
    *hooks = (yaksur_gpudriver_hooks_s *) malloc(sizeof(yaksur_gpudriver_hooks_s));
    (*hooks)->get_num_devices = get_num_devices;
    (*hooks)->check_p2p_comm = check_p2p_comm;
    (*hooks)->finalize = finalize_hook;
    (*hooks)->ipack = yaksuri_zei_ipack;
    (*hooks)->iunpack = yaksuri_zei_iunpack;
    (*hooks)->synchronize = yaksuri_zei_synchronize;
    (*hooks)->flush_all = yaksuri_zei_flush_all;
    (*hooks)->pup_is_supported = yaksuri_zei_pup_is_supported;
    (*hooks)->get_iov_pack_threshold = yaksuri_zei_get_iov_pack_threshold;
    (*hooks)->get_iov_unpack_threshold = yaksuri_zei_get_iov_unpack_threshold;
    (*hooks)->host_malloc = ze_host_malloc;
    (*hooks)->host_free = ze_host_free;
    (*hooks)->gpu_malloc = ze_gpu_malloc;
    (*hooks)->gpu_free = ze_gpu_free;
    (*hooks)->get_ptr_attr = yaksuri_zei_get_ptr_attr;
    (*hooks)->event_record = yaksuri_zei_event_record;
    (*hooks)->event_query = yaksuri_zei_event_query;
    (*hooks)->add_dependency = yaksuri_zei_add_dependency;
    (*hooks)->type_create = yaksuri_zei_type_create_hook;
    (*hooks)->type_free = yaksuri_zei_type_free_hook;
    (*hooks)->info_create = yaksuri_zei_info_create_hook;
    (*hooks)->info_free = yaksuri_zei_info_free_hook;
    (*hooks)->info_keyval_append = yaksuri_zei_info_keyval_append;

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
