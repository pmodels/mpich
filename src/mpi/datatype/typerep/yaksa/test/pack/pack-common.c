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

const char *memtype_str[] = { "unreg-host", "reg-host", "managed", "device" };

int pack_get_ndevices(void)
{
#ifdef HAVE_CUDA
    return pack_cuda_get_ndevices();
#elif defined(HAVE_ZE)
    return pack_ze_get_ndevices();
#elif defined(HAVE_HIP)
    return pack_hip_get_ndevices();
#else
    return 0;
#endif
}

void pack_init_devices(int num_threads)
{
#ifdef HAVE_CUDA
    pack_cuda_init_devices(num_threads);
#elif defined(HAVE_ZE)
    pack_ze_init_devices(num_threads);
#elif defined(HAVE_HIP)
    pack_hip_init_devices(num_threads);
#endif
}

void pack_finalize_devices()
{
#ifdef HAVE_CUDA
    pack_cuda_finalize_devices();
#elif defined(HAVE_ZE)
    pack_ze_finalize_devices();
#elif defined(HAVE_HIP)
    pack_hip_finalize_devices();
#endif
}

void pack_alloc_mem(int device_id, size_t size, mem_type_e type, void **hostbuf, void **devicebuf)
{
    if (type == MEM_TYPE__UNREGISTERED_HOST) {
        *devicebuf = malloc(size);
        if (hostbuf)
            *hostbuf = *devicebuf;
    } else {
#ifdef HAVE_CUDA
        pack_cuda_alloc_mem(device_id, size, type, hostbuf, devicebuf);
#elif defined(HAVE_ZE)
        pack_ze_alloc_mem(device_id, size, type, hostbuf, devicebuf);
#elif defined(HAVE_HIP)
        pack_hip_alloc_mem(device_id, size, type, hostbuf, devicebuf);
#else
        fprintf(stderr, "ERROR: no GPU device is supported\n");
        exit(1);
#endif
    }
}

void pack_free_mem(mem_type_e type, void *hostbuf, void *devicebuf)
{
    if (type == MEM_TYPE__UNREGISTERED_HOST) {
        free(hostbuf);
    } else {
#ifdef HAVE_CUDA
        pack_cuda_free_mem(type, hostbuf, devicebuf);
#elif defined(HAVE_ZE)
        pack_ze_free_mem(type, hostbuf, devicebuf);
#elif defined(HAVE_HIP)
        pack_hip_free_mem(type, hostbuf, devicebuf);
#else
        fprintf(stderr, "ERROR: no GPU device is supported\n");
        exit(1);
#endif
    }
}

void pack_get_ptr_attr(const void *inbuf, void *outbuf, yaksa_info_t * info, int iter)
{
#ifdef HAVE_CUDA
    pack_cuda_get_ptr_attr(inbuf, outbuf, info, iter);
#elif defined(HAVE_ZE)
    pack_ze_get_ptr_attr(inbuf, outbuf, info, iter);
#elif defined(HAVE_HIP)
    pack_hip_get_ptr_attr(inbuf, outbuf, info, iter);
#else
    *info = NULL;
#endif
}

void pack_copy_content(int tid, const void *sbuf, void *dbuf, size_t size, mem_type_e type)
{
#ifdef HAVE_CUDA
    pack_cuda_copy_content(tid, sbuf, dbuf, size, type);
#elif defined(HAVE_ZE)
    pack_ze_copy_content(tid, sbuf, dbuf, size, type);
#elif defined(HAVE_HIP)
    pack_hip_copy_content(tid, sbuf, dbuf, size, type);
#endif
}

void *pack_create_stream(void)
{
#ifdef HAVE_CUDA
    return pack_cuda_create_stream();
#elif defined(HAVE_ZE)
    return pack_ze_create_stream();
#elif defined(HAVE_HIP)
    return pack_hip_create_stream();
#else
    assert(0 && "yaksa_pack_stream/yaksa_unpack_stream not supported");
#endif
}

void pack_destroy_stream(void *stream)
{
#ifdef HAVE_CUDA
    pack_cuda_destroy_stream(stream);
#elif defined(HAVE_ZE)
    pack_ze_destroy_stream(stream);
#elif defined(HAVE_HIP)
    pack_hip_destroy_stream(stream);
#else
    assert(0 && "yaksa_pack_stream/yaksa_unpack_stream not supported");
#endif
}

void pack_stream_synchronize(void *stream)
{
#ifdef HAVE_CUDA
    pack_cuda_stream_synchronize(stream);
#elif defined(HAVE_ZE)
    pack_ze_stream_synchronize(stream);
#elif defined(HAVE_HIP)
    pack_hip_stream_synchronize(stream);
#else
    assert(0 && "yaksa_pack_stream/yaksa_unpack_stream not supported");
#endif
}
