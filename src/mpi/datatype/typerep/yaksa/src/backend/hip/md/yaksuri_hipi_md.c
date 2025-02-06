/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksu.h"
#include "yaksuri_hipi.h"
#include <assert.h>
#include <hip/hip_runtime_api.h>
#include <hip/hip_runtime.h>
#include <string.h>

static yaksuri_hipi_md_s *type_to_md(yaksi_type_s * type)
{
    yaksuri_hipi_type_s *hip = type->backend.hip.priv;

    return hip->md;
}

int yaksuri_hipi_md_alloc(yaksi_type_s * type)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_hipi_type_s *hip = (yaksuri_hipi_type_s *) type->backend.hip.priv;
    hipError_t cerr;

    pthread_mutex_lock(&hip->mdmutex);

    assert(type->kind != YAKSI_TYPE_KIND__STRUCT);
    assert(type->kind != YAKSI_TYPE_KIND__SUBARRAY);

    /* if the metadata is already allocated, return */
    if (hip->md) {
        goto fn_exit;
    } else {
        cerr = hipMallocManaged((void **) &hip->md, sizeof(yaksuri_hipi_md_s), hipMemAttachGlobal);
        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);
    }

    switch (type->kind) {
        case YAKSI_TYPE_KIND__BUILTIN:
            break;

        case YAKSI_TYPE_KIND__RESIZED:
            rc = yaksuri_hipi_md_alloc(type->u.resized.child);
            YAKSU_ERR_CHECK(rc, fn_fail);
            hip->md->u.resized.child = type_to_md(type->u.resized.child);
            break;

        case YAKSI_TYPE_KIND__HVECTOR:
            hip->md->u.hvector.count = type->u.hvector.count;
            hip->md->u.hvector.blocklength = type->u.hvector.blocklength;
            hip->md->u.hvector.stride = type->u.hvector.stride;

            rc = yaksuri_hipi_md_alloc(type->u.hvector.child);
            YAKSU_ERR_CHECK(rc, fn_fail);
            hip->md->u.hvector.child = type_to_md(type->u.hvector.child);

            break;

        case YAKSI_TYPE_KIND__BLKHINDX:
            hip->md->u.blkhindx.count = type->u.blkhindx.count;
            hip->md->u.blkhindx.blocklength = type->u.blkhindx.blocklength;

            cerr = hipMallocManaged((void **) &hip->md->u.blkhindx.array_of_displs,
                                    type->u.blkhindx.count * sizeof(intptr_t), hipMemAttachGlobal);
            YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

            memcpy(hip->md->u.blkhindx.array_of_displs, type->u.blkhindx.array_of_displs,
                   type->u.blkhindx.count * sizeof(intptr_t));

            rc = yaksuri_hipi_md_alloc(type->u.blkhindx.child);
            YAKSU_ERR_CHECK(rc, fn_fail);
            hip->md->u.blkhindx.child = type_to_md(type->u.blkhindx.child);

            break;

        case YAKSI_TYPE_KIND__HINDEXED:
            hip->md->u.hindexed.count = type->u.hindexed.count;

            cerr = hipMallocManaged((void **) &hip->md->u.hindexed.array_of_displs,
                                    type->u.hindexed.count * sizeof(intptr_t), hipMemAttachGlobal);
            YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

            memcpy(hip->md->u.hindexed.array_of_displs, type->u.hindexed.array_of_displs,
                   type->u.hindexed.count * sizeof(intptr_t));

            cerr = hipMallocManaged((void **) &hip->md->u.hindexed.array_of_blocklengths,
                                    type->u.hindexed.count * sizeof(intptr_t), hipMemAttachGlobal);
            YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

            memcpy(hip->md->u.hindexed.array_of_blocklengths,
                   type->u.hindexed.array_of_blocklengths,
                   type->u.hindexed.count * sizeof(intptr_t));

            rc = yaksuri_hipi_md_alloc(type->u.hindexed.child);
            YAKSU_ERR_CHECK(rc, fn_fail);
            hip->md->u.hindexed.child = type_to_md(type->u.hindexed.child);

            break;

        case YAKSI_TYPE_KIND__CONTIG:
            hip->md->u.contig.count = type->u.contig.count;
            hip->md->u.contig.stride = type->u.contig.child->extent;

            rc = yaksuri_hipi_md_alloc(type->u.contig.child);
            YAKSU_ERR_CHECK(rc, fn_fail);
            hip->md->u.contig.child = type_to_md(type->u.contig.child);

            break;

        default:
            assert(0);
    }

    hip->md->extent = type->extent;
    hip->md->num_elements = hip->num_elements;

  fn_exit:
    pthread_mutex_unlock(&hip->mdmutex);
    return rc;
  fn_fail:
    goto fn_exit;
}
