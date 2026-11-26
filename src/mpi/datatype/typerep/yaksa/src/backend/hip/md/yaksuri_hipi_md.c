/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "yaksi.h"
#include "yaksu.h"
#include "yaksuri_hipi.h"
#include <assert.h>
#include <hip/hip_runtime_api.h>
#include <hip/hip_runtime.h>
#include <stdlib.h>
#include <string.h>

/* The metadata that the pack and unpack kernels read is kept in device memory, one
 * copy per device. Each copy is built on the host and copied to the device the first
 * time a kernel on that device needs it. Managed memory would instead be migrated to
 * the device when a kernel first touches it, and that migration fails with an illegal
 * memory access in the kernel if other allocations have filled the device memory.
 * With device memory, a full device is reported by hipMalloc.
 *
 * A copy is one allocation: the struct, followed by the arrays it points to. Creating
 * it takes one hipMalloc and one copy, and freeing it one hipFree, since the type
 * tests and applications create and free many types and these calls can synchronize
 * the device. */

/* Build the metadata of the type, and of its children if needed, on the device, which
 * is the current device. On failure *dev_md stays NULL. */
static int build_md(yaksi_type_s * type, int device, yaksuri_hipi_md_s ** dev_md);
/* Copy to the current device and wait, since the source is freed right after and the
 * kernels that read the metadata may run on any stream. The copy runs on a stream of
 * its own, so that the wait does not include pack or unpack work queued on the device. */
static int copy_to_device(int device, void *dst, const void *src, size_t size);

/* Create the metadata of the type, and of its children, in the memory of the device,
 * unless it is there already. yaksuri_hipi_md_get returns it. */
int yaksuri_hipi_md_alloc(yaksi_type_s * type, int device)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_hipi_type_s *hip = (yaksuri_hipi_type_s *) type->backend.hip.priv;
    int cur_device = -1;
    hipError_t cerr;

    pthread_mutex_lock(&hip->mdmutex);

    if (!hip->dev_md) {
        hip->dev_md = (yaksuri_hipi_md_s **) calloc(yaksuri_hipi_global.ndevices,
                                                    sizeof(yaksuri_hipi_md_s *));
        YAKSU_ERR_CHKANDJUMP(!hip->dev_md, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);
    }

    if (!hip->dev_md[device]) {
        cerr = hipGetDevice(&cur_device);
        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

        if (cur_device != device) {
            cerr = hipSetDevice(device);
            YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);
        }

        rc = build_md(type, device, &hip->dev_md[device]);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

  fn_exit:
    if (cur_device >= 0 && cur_device != device) {
        cerr = hipSetDevice(cur_device);
        if (cerr != hipSuccess && rc == YAKSA_SUCCESS) {
            YAKSURI_HIPI_HIP_ERR_CHECK(cerr);
            rc = YAKSA_ERR__INTERNAL;
        }
    }
    pthread_mutex_unlock(&hip->mdmutex);
    return rc;
  fn_fail:
    goto fn_exit;
}

/* free the device copies of the metadata of a type */
int yaksuri_hipi_md_free(yaksi_type_s * type)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_hipi_type_s *hip = (yaksuri_hipi_type_s *) type->backend.hip.priv;
    hipError_t cerr = hipSuccess;

    if (hip->dev_md) {
        for (int i = 0; i < yaksuri_hipi_global.ndevices; i++) {
            if (hip->dev_md[i]) {
                hipError_t e = hipFree(hip->dev_md[i]);
                if (e != hipSuccess && cerr == hipSuccess) {
                    cerr = e;
                }
            }
        }
        free(hip->dev_md);
        hip->dev_md = NULL;
    }
    YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int build_md(yaksi_type_s * type, int device, yaksuri_hipi_md_s ** dev_md)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_hipi_type_s *hip = (yaksuri_hipi_type_s *) type->backend.hip.priv;
    yaksuri_hipi_md_s *md;
    intptr_t *host_arrays, *dev_arrays;
    intptr_t array_len = 0;
    char *host = NULL, *dev = NULL;
    size_t size;
    hipError_t cerr;

    assert(type->kind != YAKSI_TYPE_KIND__STRUCT);
    assert(type->kind != YAKSI_TYPE_KIND__SUBARRAY);

    /* the arrays follow the struct, whose size is a multiple of the alignment of intptr_t */
    if (type->kind == YAKSI_TYPE_KIND__BLKHINDX) {
        array_len = type->u.blkhindx.count;
    } else if (type->kind == YAKSI_TYPE_KIND__HINDEXED) {
        array_len = 2 * type->u.hindexed.count;
    }
    size = sizeof(yaksuri_hipi_md_s) + array_len * sizeof(intptr_t);

    host = (char *) calloc(1, size);
    YAKSU_ERR_CHKANDJUMP(!host, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);

    cerr = hipMalloc((void **) &dev, size);
    if (cerr != hipSuccess) {
        dev = NULL;
    }
    YAKSU_ERR_CHKANDJUMP(cerr == hipErrorOutOfMemory, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);
    YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

    md = (yaksuri_hipi_md_s *) host;
    host_arrays = (intptr_t *) (host + sizeof(yaksuri_hipi_md_s));
    dev_arrays = (intptr_t *) (dev + sizeof(yaksuri_hipi_md_s));

    md->extent = type->extent;
    md->num_elements = hip->num_elements;

    switch (type->kind) {
        case YAKSI_TYPE_KIND__BUILTIN:
            break;

        case YAKSI_TYPE_KIND__RESIZED:
            rc = yaksuri_hipi_md_alloc(type->u.resized.child, device);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md->u.resized.child = yaksuri_hipi_md_get(type->u.resized.child, device);
            break;

        case YAKSI_TYPE_KIND__HVECTOR:
            md->u.hvector.count = type->u.hvector.count;
            md->u.hvector.blocklength = type->u.hvector.blocklength;
            md->u.hvector.stride = type->u.hvector.stride;

            rc = yaksuri_hipi_md_alloc(type->u.hvector.child, device);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md->u.hvector.child = yaksuri_hipi_md_get(type->u.hvector.child, device);
            break;

        case YAKSI_TYPE_KIND__BLKHINDX:
            md->u.blkhindx.count = type->u.blkhindx.count;
            md->u.blkhindx.blocklength = type->u.blkhindx.blocklength;

            memcpy(host_arrays, type->u.blkhindx.array_of_displs,
                   type->u.blkhindx.count * sizeof(intptr_t));
            md->u.blkhindx.array_of_displs = dev_arrays;

            rc = yaksuri_hipi_md_alloc(type->u.blkhindx.child, device);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md->u.blkhindx.child = yaksuri_hipi_md_get(type->u.blkhindx.child, device);
            break;

        case YAKSI_TYPE_KIND__HINDEXED:
            md->u.hindexed.count = type->u.hindexed.count;

            memcpy(host_arrays, type->u.hindexed.array_of_displs,
                   type->u.hindexed.count * sizeof(intptr_t));
            md->u.hindexed.array_of_displs = dev_arrays;

            memcpy(host_arrays + type->u.hindexed.count, type->u.hindexed.array_of_blocklengths,
                   type->u.hindexed.count * sizeof(intptr_t));
            md->u.hindexed.array_of_blocklengths = dev_arrays + type->u.hindexed.count;

            rc = yaksuri_hipi_md_alloc(type->u.hindexed.child, device);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md->u.hindexed.child = yaksuri_hipi_md_get(type->u.hindexed.child, device);
            break;

        case YAKSI_TYPE_KIND__CONTIG:
            md->u.contig.count = type->u.contig.count;
            md->u.contig.stride = type->u.contig.child->extent;

            rc = yaksuri_hipi_md_alloc(type->u.contig.child, device);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md->u.contig.child = yaksuri_hipi_md_get(type->u.contig.child, device);
            break;

        default:
            assert(0);
    }

    rc = copy_to_device(device, dev, host, size);
    YAKSU_ERR_CHECK(rc, fn_fail);

    /* set last, so that it marks a complete copy */
    *dev_md = (yaksuri_hipi_md_s *) dev;

  fn_exit:
    free(host);
    return rc;
  fn_fail:
    if (dev) {
        hipFree(dev);
    }
    goto fn_exit;
}

static int copy_to_device(int device, void *dst, const void *src, size_t size)
{
    int rc = YAKSA_SUCCESS;
    hipStream_t stream = yaksuri_hipi_global.copy_stream[device];
    hipError_t cerr;

    cerr = hipMemcpyAsync(dst, src, size, hipMemcpyHostToDevice, stream);
    YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

    cerr = hipStreamSynchronize(stream);
    YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
