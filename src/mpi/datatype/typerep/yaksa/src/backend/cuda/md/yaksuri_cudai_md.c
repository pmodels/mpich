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
#include <string.h>

static yaksuri_cudai_md_s *type_to_md(yaksi_type_s * type)
{
    yaksuri_cudai_type_s *cuda = type->backend.cuda.priv;

    return cuda->md;
}

int yaksuri_cudai_md_alloc(yaksi_type_s * type)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_cudai_type_s *cuda = (yaksuri_cudai_type_s *) type->backend.cuda.priv;
    cudaError_t cerr;

    pthread_mutex_lock(&cuda->mdmutex);

    assert(type->kind != YAKSI_TYPE_KIND__STRUCT);
    assert(type->kind != YAKSI_TYPE_KIND__SUBARRAY);

    /* if the metadata is already allocated, return */
    if (cuda->md) {
        goto fn_exit;
    } else {
        cerr =
            cudaMallocManaged((void **) &cuda->md, sizeof(yaksuri_cudai_md_s), cudaMemAttachGlobal);
        YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);
    }

    switch (type->kind) {
        case YAKSI_TYPE_KIND__BUILTIN:
            break;

        case YAKSI_TYPE_KIND__RESIZED:
            rc = yaksuri_cudai_md_alloc(type->u.resized.child);
            YAKSU_ERR_CHECK(rc, fn_fail);
            cuda->md->u.resized.child = type_to_md(type->u.resized.child);
            break;

        case YAKSI_TYPE_KIND__HVECTOR:
            cuda->md->u.hvector.count = type->u.hvector.count;
            cuda->md->u.hvector.blocklength = type->u.hvector.blocklength;
            cuda->md->u.hvector.stride = type->u.hvector.stride;

            rc = yaksuri_cudai_md_alloc(type->u.hvector.child);
            YAKSU_ERR_CHECK(rc, fn_fail);
            cuda->md->u.hvector.child = type_to_md(type->u.hvector.child);

            break;

        case YAKSI_TYPE_KIND__BLKHINDX:
            cuda->md->u.blkhindx.count = type->u.blkhindx.count;
            cuda->md->u.blkhindx.blocklength = type->u.blkhindx.blocklength;

            cerr = cudaMallocManaged((void **) &cuda->md->u.blkhindx.array_of_displs,
                                     type->u.blkhindx.count * sizeof(intptr_t),
                                     cudaMemAttachGlobal);
            YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

            memcpy(cuda->md->u.blkhindx.array_of_displs, type->u.blkhindx.array_of_displs,
                   type->u.blkhindx.count * sizeof(intptr_t));

            rc = yaksuri_cudai_md_alloc(type->u.blkhindx.child);
            YAKSU_ERR_CHECK(rc, fn_fail);
            cuda->md->u.blkhindx.child = type_to_md(type->u.blkhindx.child);

            break;

        case YAKSI_TYPE_KIND__HINDEXED:
            cuda->md->u.hindexed.count = type->u.hindexed.count;

            cerr = cudaMallocManaged((void **) &cuda->md->u.hindexed.array_of_displs,
                                     type->u.hindexed.count * sizeof(intptr_t),
                                     cudaMemAttachGlobal);
            YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

            memcpy(cuda->md->u.hindexed.array_of_displs, type->u.hindexed.array_of_displs,
                   type->u.hindexed.count * sizeof(intptr_t));

            cerr = cudaMallocManaged((void **) &cuda->md->u.hindexed.array_of_blocklengths,
                                     type->u.hindexed.count * sizeof(intptr_t),
                                     cudaMemAttachGlobal);
            YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

            memcpy(cuda->md->u.hindexed.array_of_blocklengths,
                   type->u.hindexed.array_of_blocklengths,
                   type->u.hindexed.count * sizeof(intptr_t));

            rc = yaksuri_cudai_md_alloc(type->u.hindexed.child);
            YAKSU_ERR_CHECK(rc, fn_fail);
            cuda->md->u.hindexed.child = type_to_md(type->u.hindexed.child);

            break;

        case YAKSI_TYPE_KIND__CONTIG:
            cuda->md->u.contig.count = type->u.contig.count;
            cuda->md->u.contig.stride = type->u.contig.child->extent;

            rc = yaksuri_cudai_md_alloc(type->u.contig.child);
            YAKSU_ERR_CHECK(rc, fn_fail);
            cuda->md->u.contig.child = type_to_md(type->u.contig.child);

            break;

        default:
            assert(0);
    }

    cuda->md->extent = type->extent;
    cuda->md->num_elements = cuda->num_elements;

  fn_exit:
    pthread_mutex_unlock(&cuda->mdmutex);
    return rc;
  fn_fail:
    goto fn_exit;
}
