/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <hip/hip_runtime.h>
#include <hip/hip_runtime_api.h>
#include "yaksi.h"
#include "yaksu.h"
#include "yaksuri_hipi.h"

int yaksuri_hipi_event_record(int device, void **event_)
{
    int rc = YAKSA_SUCCESS;
    hipError_t cerr;
    yaksuri_hipi_event_s *event;

    event = (yaksuri_hipi_event_s *) malloc(sizeof(yaksuri_hipi_event_s));

    int cur_device;
    cerr = hipGetDevice(&cur_device);
    YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

    if (cur_device != device) {
        cerr = hipSetDevice(device);
        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);
    }

    cerr = hipEventCreate(&event->event);
    YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

    cerr = hipEventRecord(event->event, yaksuri_hipi_global.stream[device]);
    YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

    if (cur_device != device) {
        cerr = hipSetDevice(cur_device);
        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);
    }

    *event_ = event;

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksuri_hipi_event_query(void *event_, int *completed)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_hipi_event_s *event = (yaksuri_hipi_event_s *) event_;

    hipError_t cerr = hipEventQuery(event->event);
    if (cerr == hipSuccess) {
        cerr = hipEventDestroy(event->event);
        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

        free(event);

        *completed = 1;
    } else if (cerr == hipErrorNotReady) {
        *completed = 0;
    } else {
        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksuri_hipi_add_dependency(int device1, int device2)
{
    int rc = YAKSA_SUCCESS;
    hipError_t cerr;

    /* create a temporary event on the first device */
    int cur_device;
    cerr = hipGetDevice(&cur_device);
    YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

    if (cur_device != device1) {
        cerr = hipSetDevice(device1);
        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);
    }

    hipEvent_t event;
    cerr = hipEventCreate(&event);
    YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

    cerr = hipEventRecord(event, yaksuri_hipi_global.stream[device1]);
    YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

    if (cur_device != device1) {
        cerr = hipSetDevice(cur_device);
        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);
    }

    /* add a dependency on that event for the second device */
    cerr = hipStreamWaitEvent(yaksuri_hipi_global.stream[device2], event, 0);
    YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

    /* destroy the temporary event */
    cerr = hipEventDestroy(event);
    YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

/* ---- */
struct stream_callback_wrapper {
    yaksur_hostfn_t fn;
    void *userData;
};

static void stream_callback(hipStream_t stream, hipError_t status, void *wrapper_data)
{
    struct stream_callback_wrapper *p = wrapper_data;
    p->fn(p->userData);
    free(p);
}

int yaksuri_hipi_launch_hostfn(void *stream, yaksur_hostfn_t fn, void *userData)
{
    int rc = YAKSA_SUCCESS;
    hipError_t cerr;

    struct stream_callback_wrapper *p = malloc(sizeof(struct stream_callback_wrapper));
    p->fn = fn;
    p->userData = userData;

    cerr = hipStreamAddCallback(*(hipStream_t *) stream, stream_callback, p, 0);
    YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
