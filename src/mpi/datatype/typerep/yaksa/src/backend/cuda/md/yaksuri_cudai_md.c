/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "yaksi.h"
#include "yaksu.h"
#include "yaksuri_cudai.h"
#include <assert.h>
#include <cuda_runtime_api.h>
#include <cuda.h>
#include <stdlib.h>
#include <string.h>

/* The metadata that the pack and unpack kernels read is kept in device memory, one
 * copy per device. Each copy is built on the host and copied to the device the first
 * time a kernel on that device needs it. Managed memory would instead be migrated to
 * the device when a kernel first touches it, and that migration fails with an illegal
 * memory access in the kernel if other allocations have filled the device memory.
 * With device memory, a full device is reported by cudaMalloc.
 *
 * A copy is one allocation: the struct, followed by the arrays it points to. Creating
 * it takes one cudaMalloc and one copy, and freeing it one cudaFree, since the type
 * tests and applications create and free many types and these calls can synchronize
 * the device. */

/* Build the metadata of the type, and of its children if needed, on the device, which
 * is the current device. On failure *dev_md stays NULL. */
static int build_md(yaksi_type_s * type, int device, yaksuri_cudai_md_s ** dev_md);
/* Copy to the current device and wait, since the source is freed right after and the
 * kernels that read the metadata may run on any stream. The copy runs on a stream of
 * its own, so that the wait does not include pack or unpack work queued on the device. */
static int copy_to_device(int device, void *dst, const void *src, size_t size);

/* guards the creation of the copy streams, which may be asked for by several threads */
static pthread_mutex_t copy_stream_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Create the metadata of the type, and of its children, in the memory of the device,
 * unless it is there already. yaksuri_cudai_md_get returns it. */
int yaksuri_cudai_md_alloc(yaksi_type_s * type, int device)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_cudai_type_s *cuda = (yaksuri_cudai_type_s *) type->backend.cuda.priv;
    int cur_device = -1;
    cudaError_t cerr;

    pthread_mutex_lock(&cuda->mdmutex);

    if (!cuda->dev_md) {
        cuda->dev_md = (yaksuri_cudai_md_s **) calloc(yaksuri_cudai_global.ndevices,
                                                      sizeof(yaksuri_cudai_md_s *));
        YAKSU_ERR_CHKANDJUMP(!cuda->dev_md, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);
    }

    if (!cuda->dev_md[device]) {
        cerr = cudaGetDevice(&cur_device);
        YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

        if (cur_device != device) {
            cerr = cudaSetDevice(device);
            YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);
        }

        rc = build_md(type, device, &cuda->dev_md[device]);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

  fn_exit:
    if (cur_device >= 0 && cur_device != device) {
        cerr = cudaSetDevice(cur_device);
        if (cerr != cudaSuccess && rc == YAKSA_SUCCESS) {
            YAKSURI_CUDAI_CUDA_ERR_CHECK(cerr);
            rc = YAKSA_ERR__INTERNAL;
        }
    }
    pthread_mutex_unlock(&cuda->mdmutex);
    return rc;
  fn_fail:
    goto fn_exit;
}

/* free the device copies of the metadata of a type */
int yaksuri_cudai_md_free(yaksi_type_s * type)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_cudai_type_s *cuda = (yaksuri_cudai_type_s *) type->backend.cuda.priv;
    cudaError_t cerr = cudaSuccess;

    if (cuda->dev_md) {
        for (int i = 0; i < yaksuri_cudai_global.ndevices; i++) {
            if (cuda->dev_md[i]) {
                cudaError_t e = cudaFree(cuda->dev_md[i]);
                if (e != cudaSuccess && cerr == cudaSuccess) {
                    cerr = e;
                }
            }
        }
        free(cuda->dev_md);
        cuda->dev_md = NULL;
    }
    YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

/* The stream for metadata copies to the device, created on first use. When the wait
 * kernel is enabled, yaksuri_cuda_init_hook creates it up front instead, since creating
 * a stream would deadlock with the wait kernel. */
int yaksuri_cudai_get_copy_stream(int device, cudaStream_t * stream)
{
    int rc = YAKSA_SUCCESS;
    cudai_stream *s = &yaksuri_cudai_global.copy_streams[device];
    int cur_device = -1;
    cudaError_t cerr;

    pthread_mutex_lock(&copy_stream_mutex);

    if (!s->created) {
        cerr = cudaGetDevice(&cur_device);
        YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

        if (cur_device != device) {
            cerr = cudaSetDevice(device);
            YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);
        }

        cerr = cudaStreamCreateWithFlags(&s->stream, cudaStreamNonBlocking);
        YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);
        s->created = true;
    }
    *stream = s->stream;

  fn_exit:
    if (cur_device >= 0 && cur_device != device) {
        cerr = cudaSetDevice(cur_device);
        if (cerr != cudaSuccess && rc == YAKSA_SUCCESS) {
            YAKSURI_CUDAI_CUDA_ERR_CHECK(cerr);
            rc = YAKSA_ERR__INTERNAL;
        }
    }
    pthread_mutex_unlock(&copy_stream_mutex);
    return rc;
  fn_fail:
    goto fn_exit;
}

static int build_md(yaksi_type_s * type, int device, yaksuri_cudai_md_s ** dev_md)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_cudai_type_s *cuda = (yaksuri_cudai_type_s *) type->backend.cuda.priv;
    yaksuri_cudai_md_s *md;
    intptr_t *host_arrays, *dev_arrays;
    intptr_t array_len = 0;
    char *host = NULL, *dev = NULL;
    size_t size;
    cudaError_t cerr;

    assert(type->kind != YAKSI_TYPE_KIND__STRUCT);
    assert(type->kind != YAKSI_TYPE_KIND__SUBARRAY);

    /* the arrays follow the struct, whose size is a multiple of the alignment of intptr_t */
    if (type->kind == YAKSI_TYPE_KIND__BLKHINDX) {
        array_len = type->u.blkhindx.count;
    } else if (type->kind == YAKSI_TYPE_KIND__HINDEXED) {
        array_len = 2 * type->u.hindexed.count;
    }
    size = sizeof(yaksuri_cudai_md_s) + array_len * sizeof(intptr_t);

    host = (char *) calloc(1, size);
    YAKSU_ERR_CHKANDJUMP(!host, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);

    cerr = cudaMalloc((void **) &dev, size);
    if (cerr != cudaSuccess) {
        dev = NULL;
    }
    YAKSU_ERR_CHKANDJUMP(cerr == cudaErrorMemoryAllocation, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);
    YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

    md = (yaksuri_cudai_md_s *) host;
    host_arrays = (intptr_t *) (host + sizeof(yaksuri_cudai_md_s));
    dev_arrays = (intptr_t *) (dev + sizeof(yaksuri_cudai_md_s));

    md->extent = type->extent;
    md->num_elements = cuda->num_elements;

    switch (type->kind) {
        case YAKSI_TYPE_KIND__BUILTIN:
            break;

        case YAKSI_TYPE_KIND__RESIZED:
            rc = yaksuri_cudai_md_alloc(type->u.resized.child, device);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md->u.resized.child = yaksuri_cudai_md_get(type->u.resized.child, device);
            break;

        case YAKSI_TYPE_KIND__HVECTOR:
            md->u.hvector.count = type->u.hvector.count;
            md->u.hvector.blocklength = type->u.hvector.blocklength;
            md->u.hvector.stride = type->u.hvector.stride;

            rc = yaksuri_cudai_md_alloc(type->u.hvector.child, device);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md->u.hvector.child = yaksuri_cudai_md_get(type->u.hvector.child, device);
            break;

        case YAKSI_TYPE_KIND__BLKHINDX:
            md->u.blkhindx.count = type->u.blkhindx.count;
            md->u.blkhindx.blocklength = type->u.blkhindx.blocklength;

            memcpy(host_arrays, type->u.blkhindx.array_of_displs,
                   type->u.blkhindx.count * sizeof(intptr_t));
            md->u.blkhindx.array_of_displs = dev_arrays;

            rc = yaksuri_cudai_md_alloc(type->u.blkhindx.child, device);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md->u.blkhindx.child = yaksuri_cudai_md_get(type->u.blkhindx.child, device);
            break;

        case YAKSI_TYPE_KIND__HINDEXED:
            md->u.hindexed.count = type->u.hindexed.count;

            memcpy(host_arrays, type->u.hindexed.array_of_displs,
                   type->u.hindexed.count * sizeof(intptr_t));
            md->u.hindexed.array_of_displs = dev_arrays;

            memcpy(host_arrays + type->u.hindexed.count, type->u.hindexed.array_of_blocklengths,
                   type->u.hindexed.count * sizeof(intptr_t));
            md->u.hindexed.array_of_blocklengths = dev_arrays + type->u.hindexed.count;

            rc = yaksuri_cudai_md_alloc(type->u.hindexed.child, device);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md->u.hindexed.child = yaksuri_cudai_md_get(type->u.hindexed.child, device);
            break;

        case YAKSI_TYPE_KIND__CONTIG:
            md->u.contig.count = type->u.contig.count;
            md->u.contig.stride = type->u.contig.child->extent;

            rc = yaksuri_cudai_md_alloc(type->u.contig.child, device);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md->u.contig.child = yaksuri_cudai_md_get(type->u.contig.child, device);
            break;

        default:
            assert(0);
    }

    rc = copy_to_device(device, dev, host, size);
    YAKSU_ERR_CHECK(rc, fn_fail);

    /* set last, so that it marks a complete copy */
    *dev_md = (yaksuri_cudai_md_s *) dev;

  fn_exit:
    free(host);
    return rc;
  fn_fail:
    if (dev) {
        cudaFree(dev);
    }
    goto fn_exit;
}

static int copy_to_device(int device, void *dst, const void *src, size_t size)
{
    int rc = YAKSA_SUCCESS;
    cudaStream_t stream;
    cudaError_t cerr;

    rc = yaksuri_cudai_get_copy_stream(device, &stream);
    YAKSU_ERR_CHECK(rc, fn_fail);

    cerr = cudaMemcpyAsync(dst, src, size, cudaMemcpyHostToDevice, stream);
    YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

    cerr = cudaStreamSynchronize(stream);
    YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
