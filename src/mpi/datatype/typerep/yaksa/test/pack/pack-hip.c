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

#ifdef HAVE_HIP

#include <hip/hip_runtime_api.h>

int pack_hip_get_ndevices(void)
{
    int ndevices;
    hipGetDeviceCount(&ndevices);
    assert(ndevices != -1);

    return ndevices;
}

void pack_hip_init_devices(int num_threads)
{
}

void pack_hip_finalize_devices()
{
}

void pack_hip_alloc_mem(int device_id, size_t size, mem_type_e type, void **hostbuf,
                        void **devicebuf)
{
    if (type == MEM_TYPE__REGISTERED_HOST) {
        hipHostMalloc(devicebuf, size, hipHostMallocDefault);
        if (hostbuf)
            *hostbuf = *devicebuf;
    } else if (type == MEM_TYPE__MANAGED) {
        hipMallocManaged(devicebuf, size, hipMemAttachGlobal);
        if (hostbuf)
            *hostbuf = *devicebuf;
    } else if (type == MEM_TYPE__DEVICE) {
        hipSetDevice(device_id);
        hipMalloc(devicebuf, size);
        if (hostbuf)
            hipHostMalloc(hostbuf, size, hipHostMallocDefault);
    } else {
        fprintf(stderr, "ERROR: unsupported memory type\n");
        exit(1);
    }
}

void pack_hip_free_mem(mem_type_e type, void *hostbuf, void *devicebuf)
{
    if (type == MEM_TYPE__REGISTERED_HOST) {
        hipHostFree(devicebuf);
    } else if (type == MEM_TYPE__MANAGED) {
        hipFree(devicebuf);
    } else if (type == MEM_TYPE__DEVICE) {
        hipFree(devicebuf);
        if (hostbuf) {
            hipHostFree(hostbuf);
        }
    }
}

void pack_hip_get_ptr_attr(const void *inbuf, void *outbuf, yaksa_info_t * info, int iter)
{
    if (iter % 2 == 0) {
        int rc;

        rc = yaksa_info_create(info);
        assert(rc == YAKSA_SUCCESS);

        rc = yaksa_info_keyval_append(*info, "yaksa_gpu_driver", "hip", strlen("hip"));
        assert(rc == YAKSA_SUCCESS);

        struct hipPointerAttribute_t attr;

        hipError_t cerr = hipPointerGetAttributes(&attr, inbuf);
        if (cerr == hipSuccess) {
            rc = yaksa_info_keyval_append(*info, "yaksa_hip_inbuf_ptr_attr", &attr, sizeof(attr));
            assert(rc == YAKSA_SUCCESS);
        }

        cerr = hipPointerGetAttributes(&attr, outbuf);
        if (cerr == hipSuccess) {
            rc = yaksa_info_keyval_append(*info, "yaksa_hip_outbuf_ptr_attr", &attr, sizeof(attr));
            assert(rc == YAKSA_SUCCESS);
        }
    } else
        *info = NULL;
}

void pack_hip_copy_content(int tid, const void *sbuf, void *dbuf, size_t size, mem_type_e type)
{
    int rc;
    if (type == MEM_TYPE__DEVICE) {
        rc = hipMemcpy(dbuf, sbuf, size, hipMemcpyDefault);
        //printf("rc: %d\n", rc);
        assert(rc == hipSuccess);
    }
}

void *pack_hip_create_stream(void)
{
    static hipStream_t stream;
    /* create stream on the 1st device */
    hipSetDevice(0);
    hipStreamCreate(&stream);
    return &stream;
}

void pack_hip_destroy_stream(void *stream_p)
{
    hipStream_t stream = *(hipStream_t *) stream_p;
    hipStreamDestroy(stream);
}

void pack_hip_stream_synchronize(void *stream_p)
{
    hipStream_t stream = *(hipStream_t *) stream_p;
    hipStreamSynchronize(stream);
}

#endif /* HAVE_HIP */
