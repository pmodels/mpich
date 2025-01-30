/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksuri_hipi.h"
#include <assert.h>
#include <string.h>
#include <hip/hip_runtime.h>
#include <hip/hip_runtime_api.h>

static void *hip_host_malloc(uintptr_t size)
{
    void *ptr = NULL;

    hipError_t cerr = hipHostMalloc(&ptr, size, hipHostMallocDefault);
    YAKSURI_HIPI_HIP_ERR_CHECK(cerr);

    return ptr;
}

static void *hip_gpu_malloc(uintptr_t size, int device)
{
    void *ptr = NULL;
    hipError_t cerr;

    int cur_device;
    cerr = hipGetDevice(&cur_device);
    YAKSURI_HIPI_HIP_ERR_CHECK(cerr);

    if (cur_device != device) {
        cerr = hipSetDevice(device);
        YAKSURI_HIPI_HIP_ERR_CHECK(cerr);
    }

    cerr = hipMalloc(&ptr, size);
    YAKSURI_HIPI_HIP_ERR_CHECK(cerr);

    if (cur_device != device) {
        cerr = hipSetDevice(cur_device);
        YAKSURI_HIPI_HIP_ERR_CHECK(cerr);
    }

    return ptr;
}

static void hip_host_free(void *ptr)
{
    hipError_t cerr = hipHostFree(ptr);
    YAKSURI_HIPI_HIP_ERR_CHECK(cerr);
}

static void hip_gpu_free(void *ptr)
{
    hipError_t cerr = hipFree(ptr);
    YAKSURI_HIPI_HIP_ERR_CHECK(cerr);
}

yaksuri_hipi_global_s yaksuri_hipi_global;

static int finalize_hook(void)
{
    int rc = YAKSA_SUCCESS;
    hipError_t cerr;

    for (int i = 0; i < yaksuri_hipi_global.ndevices; i++) {
        cerr = hipSetDevice(i);
        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

        cerr = hipStreamDestroy(yaksuri_hipi_global.stream[i]);
        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

        free(yaksuri_hipi_global.p2p[i]);
    }
    free(yaksuri_hipi_global.stream);
    free(yaksuri_hipi_global.p2p);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int get_num_devices(int *ndevices)
{
    *ndevices = yaksuri_hipi_global.ndevices;

    return YAKSA_SUCCESS;
}

static bool check_p2p_comm(int sdev, int ddev)
{
    bool is_enabled = 0;
#if HIP_P2P == HIP_P2P_ENABLED
    is_enabled = yaksuri_hipi_global.p2p[sdev][ddev];
#elif HIP_P2P == HIP_P2P_CLIQUES
    if ((sdev + ddev) % 2 == 0)
        is_enabled = yaksuri_hipi_global.p2p[sdev][ddev];
#endif

    return is_enabled;
}

int yaksuri_hip_init_hook(yaksur_gpudriver_hooks_s ** hooks)
{
    int rc = YAKSA_SUCCESS;
    hipError_t cerr;

    cerr = hipGetDeviceCount(&yaksuri_hipi_global.ndevices);
    YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

    if (getenv("HIP_VISIBLE_DEVICES") == NULL) {
        /* user did not do any filtering for us; if any of the devices
         * is in exclusive mode, disable GPU support to avoid
         * incorrect device sharing */
        bool excl = false;
        for (int i = 0; i < yaksuri_hipi_global.ndevices; i++) {
            struct hipDeviceProp_t prop;

            cerr = hipGetDeviceProperties(&prop, i);
            YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

            if (prop.computeMode != hipComputeModeDefault) {
                excl = true;
                break;
            }
        }

        if (excl == true) {
            fprintf(stderr, "[yaksa] ====> Disabling HIP support <====\n");
            fprintf(stderr,
                    "[yaksa] HIP is setup in exclusive compute mode, but HIP_VISIBLE_DEVICES is not set\n");
            fprintf(stderr,
                    "[yaksa] You can silence this warning by setting HIP_VISIBLE_DEVICES\n");
            fflush(stderr);
            *hooks = NULL;
            goto fn_exit;
        }
    }

    yaksuri_hipi_global.stream = (hipStream_t *)
        malloc(yaksuri_hipi_global.ndevices * sizeof(hipStream_t));

    yaksuri_hipi_global.p2p = (bool **) malloc(yaksuri_hipi_global.ndevices * sizeof(bool *));
    for (int i = 0; i < yaksuri_hipi_global.ndevices; i++) {
        yaksuri_hipi_global.p2p[i] = (bool *) malloc(yaksuri_hipi_global.ndevices * sizeof(bool));
    }

    int cur_device;
    cerr = hipGetDevice(&cur_device);
    YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

    for (int i = 0; i < yaksuri_hipi_global.ndevices; i++) {
        cerr = hipSetDevice(i);
        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

        cerr = hipStreamCreateWithFlags(&yaksuri_hipi_global.stream[i], hipStreamNonBlocking);
        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

        for (int j = 0; j < yaksuri_hipi_global.ndevices; j++) {
            if (i == j) {
                yaksuri_hipi_global.p2p[i][j] = 1;
            } else {
                int val;
                cerr = hipDeviceCanAccessPeer(&val, i, j);
                YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

                if (val) {
                    cerr = hipDeviceEnablePeerAccess(j, 0);
                    if (cerr != hipErrorPeerAccessAlreadyEnabled) {
                        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);
                    }
                    yaksuri_hipi_global.p2p[i][j] = 1;
                } else {
                    yaksuri_hipi_global.p2p[i][j] = 0;
                }
            }
        }
    }

    cerr = hipSetDevice(cur_device);
    YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

    *hooks = (yaksur_gpudriver_hooks_s *) malloc(sizeof(yaksur_gpudriver_hooks_s));
    (*hooks)->get_num_devices = get_num_devices;
    (*hooks)->check_p2p_comm = check_p2p_comm;
    (*hooks)->finalize = finalize_hook;
    (*hooks)->get_iov_pack_threshold = yaksuri_hipi_get_iov_pack_threshold;
    (*hooks)->get_iov_unpack_threshold = yaksuri_hipi_get_iov_unpack_threshold;
    (*hooks)->ipack = yaksuri_hipi_ipack;
    (*hooks)->iunpack = yaksuri_hipi_iunpack;
    (*hooks)->pack_with_stream = yaksuri_hipi_ipack_with_stream;
    (*hooks)->unpack_with_stream = yaksuri_hipi_iunpack_with_stream;
    (*hooks)->synchronize = yaksuri_hipi_synchronize;
    (*hooks)->flush_all = yaksuri_hipi_flush_all;
    (*hooks)->pup_is_supported = yaksuri_hipi_pup_is_supported;
    (*hooks)->host_malloc = hip_host_malloc;
    (*hooks)->host_free = hip_host_free;
    (*hooks)->gpu_malloc = hip_gpu_malloc;
    (*hooks)->gpu_free = hip_gpu_free;
    (*hooks)->get_ptr_attr = yaksuri_hipi_get_ptr_attr;
    (*hooks)->event_record = yaksuri_hipi_event_record;
    (*hooks)->event_query = yaksuri_hipi_event_query;
    (*hooks)->add_dependency = yaksuri_hipi_add_dependency;
    (*hooks)->launch_hostfn = yaksuri_hipi_launch_hostfn;
    (*hooks)->type_create = yaksuri_hipi_type_create_hook;
    (*hooks)->type_free = yaksuri_hipi_type_free_hook;
    (*hooks)->info_create = yaksuri_hipi_info_create_hook;
    (*hooks)->info_free = yaksuri_hipi_info_free_hook;
    (*hooks)->info_keyval_append = yaksuri_hipi_info_keyval_append;

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
