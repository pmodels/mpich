/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <cuda.h>
#include <cuda_runtime_api.h>
#include "yaksi.h"
#include "yaksu.h"
#include "yaksuri_cudai.h"

int yaksuri_cudai_event_record(int device, void **event_)
{
    int rc = YAKSA_SUCCESS;
    cudaError_t cerr;
    yaksuri_cudai_event_s *event;

    event = (yaksuri_cudai_event_s *) malloc(sizeof(yaksuri_cudai_event_s));

    int cur_device;
    cerr = cudaGetDevice(&cur_device);
    YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

    if (cur_device != device) {
        cerr = cudaSetDevice(device);
        YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);
    }

    cerr = cudaEventCreate(&event->event);
    YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

    cerr = cudaEventRecord(event->event, *yaksuri_cudai_get_stream(device));
    YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

    if (cur_device != device) {
        cerr = cudaSetDevice(cur_device);
        YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);
    }

    *event_ = event;

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksuri_cudai_event_query(void *event_, int *completed)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_cudai_event_s *event = (yaksuri_cudai_event_s *) event_;

    cudaError_t cerr = cudaEventQuery(event->event);
    if (cerr == cudaSuccess) {
        cerr = cudaEventDestroy(event->event);
        YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

        free(event);

        *completed = 1;
    } else if (cerr == cudaErrorNotReady) {
        *completed = 0;
    } else {
        YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksuri_cudai_add_dependency(int device1, int device2)
{
    int rc = YAKSA_SUCCESS;
    cudaError_t cerr;

    /* create a temporary event on the first device */
    int cur_device;
    cerr = cudaGetDevice(&cur_device);
    YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

    if (cur_device != device1) {
        cerr = cudaSetDevice(device1);
        YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);
    }

    cudaEvent_t event;
    cerr = cudaEventCreate(&event);
    YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

    cerr = cudaEventRecord(event, *yaksuri_cudai_get_stream(device1));
    YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

    if (cur_device != device1) {
        cerr = cudaSetDevice(cur_device);
        YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);
    }

    /* add a dependency on that event for the second device */
    cerr = cudaStreamWaitEvent(*yaksuri_cudai_get_stream(device2), event, 0);
    YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

    /* destroy the temporary event */
    cerr = cudaEventDestroy(event);
    YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksuri_cudai_launch_hostfn(void *stream, yaksur_hostfn_t fn, void *userData)
{
    int rc = YAKSA_SUCCESS;
    cudaError_t cerr;
    cerr = cudaLaunchHostFunc(*(cudaStream_t *) stream, fn, userData);
    YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
