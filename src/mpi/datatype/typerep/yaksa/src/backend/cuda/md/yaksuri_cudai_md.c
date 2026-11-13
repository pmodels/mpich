/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
 * With device memory, a full device is reported by cudaMalloc. */

/* Build the metadata of the type, and of its children if needed, on the device, which
 * is the current device. A failure leaves dev empty. */
static int build_md(yaksi_type_s * type, int device, yaksuri_cudai_dev_md_s * dev);
/* copy the array to new memory on the current device */
static int alloc_array(const intptr_t * array, intptr_t count, intptr_t ** dev_array);
/* allocate on the current device; a full device is reported as out of memory */
static int dev_malloc(void **ptr, size_t size);
/* Copy to the current device and wait, since the source may be on the stack and the
 * kernels that read the metadata may run on any stream. The copy runs on a stream of
 * its own, so that the wait does not include pack or unpack work queued on the device. */
static int copy_to_device(void *dst, const void *src, size_t size);
/* free one device copy; returns the first error, but frees everything */
static cudaError_t free_dev_md(yaksuri_cudai_dev_md_s * dev);

/* Create the metadata of the type, and of its children, in the memory of the device,
 * unless it is there already. yaksuri_cudai_md_get returns it. */
int yaksuri_cudai_md_alloc(yaksi_type_s * type, int device)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_cudai_type_s *cuda = (yaksuri_cudai_type_s *) type->backend.cuda.priv;
    yaksuri_cudai_dev_md_s *dev;
    int cur_device = -1;
    cudaError_t cerr;

    pthread_mutex_lock(&cuda->mdmutex);

    if (!cuda->dev_md) {
        cuda->dev_md = (yaksuri_cudai_dev_md_s *) calloc(yaksuri_cudai_global.ndevices,
                                                         sizeof(yaksuri_cudai_dev_md_s));
        YAKSU_ERR_CHKANDJUMP(!cuda->dev_md, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);
    }

    dev = &cuda->dev_md[device];
    if (!dev->md) {
        cerr = cudaGetDevice(&cur_device);
        YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

        if (cur_device != device) {
            cerr = cudaSetDevice(device);
            YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);
        }

        rc = build_md(type, device, dev);
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
            cudaError_t e = free_dev_md(&cuda->dev_md[i]);
            if (e != cudaSuccess && cerr == cudaSuccess) {
                cerr = e;
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

static int build_md(yaksi_type_s * type, int device, yaksuri_cudai_dev_md_s * dev)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_cudai_type_s *cuda = (yaksuri_cudai_type_s *) type->backend.cuda.priv;
    yaksuri_cudai_md_s md, *dev_ptr;

    assert(type->kind != YAKSI_TYPE_KIND__STRUCT);
    assert(type->kind != YAKSI_TYPE_KIND__SUBARRAY);

    memset(&md, 0, sizeof(md));
    md.extent = type->extent;
    md.num_elements = cuda->num_elements;

    switch (type->kind) {
        case YAKSI_TYPE_KIND__BUILTIN:
            break;

        case YAKSI_TYPE_KIND__RESIZED:
            rc = yaksuri_cudai_md_alloc(type->u.resized.child, device);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md.u.resized.child = yaksuri_cudai_md_get(type->u.resized.child, device);
            break;

        case YAKSI_TYPE_KIND__HVECTOR:
            md.u.hvector.count = type->u.hvector.count;
            md.u.hvector.blocklength = type->u.hvector.blocklength;
            md.u.hvector.stride = type->u.hvector.stride;

            rc = yaksuri_cudai_md_alloc(type->u.hvector.child, device);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md.u.hvector.child = yaksuri_cudai_md_get(type->u.hvector.child, device);
            break;

        case YAKSI_TYPE_KIND__BLKHINDX:
            md.u.blkhindx.count = type->u.blkhindx.count;
            md.u.blkhindx.blocklength = type->u.blkhindx.blocklength;

            rc = alloc_array(type->u.blkhindx.array_of_displs, type->u.blkhindx.count,
                             &dev->array_of_displs);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md.u.blkhindx.array_of_displs = dev->array_of_displs;

            rc = yaksuri_cudai_md_alloc(type->u.blkhindx.child, device);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md.u.blkhindx.child = yaksuri_cudai_md_get(type->u.blkhindx.child, device);
            break;

        case YAKSI_TYPE_KIND__HINDEXED:
            md.u.hindexed.count = type->u.hindexed.count;

            rc = alloc_array(type->u.hindexed.array_of_displs, type->u.hindexed.count,
                             &dev->array_of_displs);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md.u.hindexed.array_of_displs = dev->array_of_displs;

            rc = alloc_array(type->u.hindexed.array_of_blocklengths, type->u.hindexed.count,
                             &dev->array_of_blocklengths);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md.u.hindexed.array_of_blocklengths = dev->array_of_blocklengths;

            rc = yaksuri_cudai_md_alloc(type->u.hindexed.child, device);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md.u.hindexed.child = yaksuri_cudai_md_get(type->u.hindexed.child, device);
            break;

        case YAKSI_TYPE_KIND__CONTIG:
            md.u.contig.count = type->u.contig.count;
            md.u.contig.stride = type->u.contig.child->extent;

            rc = yaksuri_cudai_md_alloc(type->u.contig.child, device);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md.u.contig.child = yaksuri_cudai_md_get(type->u.contig.child, device);
            break;

        default:
            assert(0);
    }

    /* dev->md is set last, so that it marks a complete copy */
    rc = dev_malloc((void **) &dev_ptr, sizeof(md));
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = copy_to_device(dev_ptr, &md, sizeof(md));
    if (rc) {
        cudaFree(dev_ptr);
    }
    YAKSU_ERR_CHECK(rc, fn_fail);

    dev->md = dev_ptr;

  fn_exit:
    return rc;
  fn_fail:
    /* drop the arrays of a partial copy, so that the next call starts over */
    free_dev_md(dev);
    goto fn_exit;
}

static int alloc_array(const intptr_t * array, intptr_t count, intptr_t ** dev_array)
{
    int rc = YAKSA_SUCCESS;

    rc = dev_malloc((void **) dev_array, count * sizeof(intptr_t));
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = copy_to_device(*dev_array, array, count * sizeof(intptr_t));
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int dev_malloc(void **ptr, size_t size)
{
    int rc = YAKSA_SUCCESS;
    cudaError_t cerr;

    cerr = cudaMalloc(ptr, size);
    if (cerr != cudaSuccess) {
        *ptr = NULL;
    }
    YAKSU_ERR_CHKANDJUMP(cerr == cudaErrorMemoryAllocation, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);
    YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int copy_to_device(void *dst, const void *src, size_t size)
{
    int rc = YAKSA_SUCCESS;
    cudaError_t cerr;
    cudaStream_t stream;

    cerr = cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking);
    YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

    cerr = cudaMemcpyAsync(dst, src, size, cudaMemcpyHostToDevice, stream);
    if (cerr == cudaSuccess) {
        cerr = cudaStreamSynchronize(stream);
    }
    cudaStreamDestroy(stream);
    YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static cudaError_t free_dev_md(yaksuri_cudai_dev_md_s * dev)
{
    cudaError_t ret = cudaSuccess;
    void *ptrs[3] = { dev->md, dev->array_of_displs, dev->array_of_blocklengths };

    for (int i = 0; i < 3; i++) {
        if (ptrs[i]) {
            cudaError_t cerr = cudaFree(ptrs[i]);
            if (cerr != cudaSuccess && ret == cudaSuccess) {
                ret = cerr;
            }
        }
    }
    memset(dev, 0, sizeof(*dev));

    return ret;
}
