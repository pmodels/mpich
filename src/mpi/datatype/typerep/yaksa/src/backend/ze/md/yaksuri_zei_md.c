/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksu.h"
#include "yaksuri_zei.h"
#include <stdlib.h>
#include <assert.h>
#include <level_zero/ze_api.h>
#include <string.h>

static yaksuri_zei_md_s *type_to_md(yaksi_type_s * type, int dev_id)
{
    yaksuri_zei_type_s *ze = type->backend.ze.priv;

#if ZE_MD_HOST
    if (ze->md[dev_id] == NULL) {
        ze->md[dev_id] = ze->md[0];
    }
#endif
    return ze->md[dev_id];
}

int yaksuri_zei_md_alloc(yaksi_type_s * type, int dev_id)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_zei_md_s *md = NULL;
    yaksuri_zei_type_s *ze = (yaksuri_zei_type_s *) type->backend.ze.priv;
    ze_result_t zerr;

    ze_host_mem_alloc_desc_t host_desc = {
        .stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC,
        .pNext = NULL,
        .flags = 0,
    };

#if ! ZE_MD_HOST
    ze_device_mem_alloc_desc_t device_desc = {
        .stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC,
        .pNext = NULL,
        .flags = 0,
        .ordinal = 0,
    };
#endif

    pthread_mutex_lock(&ze->mdmutex);

    assert(type->kind != YAKSI_TYPE_KIND__STRUCT);
    assert(type->kind != YAKSI_TYPE_KIND__SUBARRAY);

    /* if the metadata is already allocated, return */
    if (ze->md && ze->md[dev_id]) {
        goto fn_exit;
    } else {
        if (ze->md == NULL) {
            ze->md =
                (yaksuri_zei_md_s **) calloc(yaksuri_zei_global.ndevices,
                                             sizeof(yaksuri_zei_md_s *));
            assert(ze->md);
        }
#if ZE_MD_HOST
        if (ze->md[0] == NULL) {
            zerr =
                zeMemAllocHost(yaksuri_zei_global.context, &host_desc,
                               sizeof(yaksuri_zei_md_s), 1, (void **) &ze->md[0]);
            YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
            ze->md[dev_id] = ze->md[0];
        } else {
            ze->md[dev_id] = ze->md[0];
            goto fn_exit;
        }
#else
        zerr =
            zeMemAllocShared(yaksuri_zei_global.context, &device_desc, &host_desc,
                             sizeof(yaksuri_zei_md_s), 1, yaksuri_zei_global.device[dev_id],
                             (void **) &ze->md[dev_id]);
        YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
#endif
    }

    md = ze->md[dev_id];

    switch (type->kind) {
        case YAKSI_TYPE_KIND__BUILTIN:
            break;

        case YAKSI_TYPE_KIND__DUP:
            rc = yaksuri_zei_md_alloc(type->u.dup.child, dev_id);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md->u.dup.child = type_to_md(type->u.dup.child, dev_id);
            break;

        case YAKSI_TYPE_KIND__RESIZED:
            rc = yaksuri_zei_md_alloc(type->u.resized.child, dev_id);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md->u.resized.child = type_to_md(type->u.resized.child, dev_id);
            break;

        case YAKSI_TYPE_KIND__HVECTOR:
            md->u.hvector.count = type->u.hvector.count;
            md->u.hvector.blocklength = type->u.hvector.blocklength;
            md->u.hvector.stride = type->u.hvector.stride;

            rc = yaksuri_zei_md_alloc(type->u.hvector.child, dev_id);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md->u.hvector.child = type_to_md(type->u.hvector.child, dev_id);
            break;

        case YAKSI_TYPE_KIND__BLKHINDX:
            md->u.blkhindx.count = type->u.blkhindx.count;
            md->u.blkhindx.blocklength = type->u.blkhindx.blocklength;

#if ZE_MD_HOST
            zerr = zeMemAllocHost(yaksuri_zei_global.context, &host_desc,
                                  type->u.blkhindx.count * sizeof(intptr_t), 1,
                                  (void **) &md->u.blkhindx.array_of_displs);
#else
            zerr = zeMemAllocShared(yaksuri_zei_global.context, &device_desc, &host_desc,
                                    type->u.blkhindx.count * sizeof(intptr_t), 1,
                                    yaksuri_zei_global.device[dev_id],
                                    (void **) &md->u.blkhindx.array_of_displs);
#endif
            YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);

            memcpy(md->u.blkhindx.array_of_displs, type->u.blkhindx.array_of_displs,
                   type->u.blkhindx.count * sizeof(intptr_t));

            rc = yaksuri_zei_md_alloc(type->u.blkhindx.child, dev_id);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md->u.blkhindx.child = type_to_md(type->u.blkhindx.child, dev_id);
            break;

        case YAKSI_TYPE_KIND__HINDEXED:
            md->u.hindexed.count = type->u.hindexed.count;

#if ZE_MD_HOST
            zerr = zeMemAllocHost(yaksuri_zei_global.context, &host_desc,
                                  type->u.hindexed.count * sizeof(intptr_t), 1,
                                  (void **) &md->u.hindexed.array_of_displs);
#else
            zerr = zeMemAllocShared(yaksuri_zei_global.context, &device_desc, &host_desc,
                                    type->u.hindexed.count * sizeof(intptr_t), 1,
                                    yaksuri_zei_global.device[dev_id],
                                    (void **) &md->u.hindexed.array_of_displs);
#endif
            YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);

            memcpy(md->u.hindexed.array_of_displs, type->u.hindexed.array_of_displs,
                   type->u.hindexed.count * sizeof(intptr_t));

#if ZE_MD_HOST
            zerr = zeMemAllocHost(yaksuri_zei_global.context, &host_desc,
                                  type->u.hindexed.count * sizeof(intptr_t), 1,
                                  (void **) &md->u.hindexed.array_of_blocklengths);
#else
            zerr = zeMemAllocShared(yaksuri_zei_global.context, &device_desc, &host_desc,
                                    type->u.hindexed.count * sizeof(intptr_t), 1,
                                    yaksuri_zei_global.device[dev_id],
                                    (void **) &md->u.hindexed.array_of_blocklengths);
#endif
            YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);

            memcpy(md->u.hindexed.array_of_blocklengths,
                   type->u.hindexed.array_of_blocklengths,
                   type->u.hindexed.count * sizeof(intptr_t));

            rc = yaksuri_zei_md_alloc(type->u.hindexed.child, dev_id);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md->u.hindexed.child = type_to_md(type->u.hindexed.child, dev_id);
            break;

        case YAKSI_TYPE_KIND__CONTIG:
            md->u.contig.count = type->u.contig.count;
            md->u.contig.stride = type->u.contig.child->extent;

            rc = yaksuri_zei_md_alloc(type->u.contig.child, dev_id);
            YAKSU_ERR_CHECK(rc, fn_fail);
            md->u.contig.child = type_to_md(type->u.contig.child, dev_id);
            break;

        default:
            assert(0);
    }

    md->extent = type->extent;
    md->num_elements = ze->num_elements;
    md->true_lb = type->true_lb;

  fn_exit:
    pthread_mutex_unlock(&ze->mdmutex);
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksuri_zei_type_make_resident(yaksi_type_s * type, int dev_id)
{
    int rc = YAKSA_SUCCESS;
    ze_result_t zerr;
    ze_device_handle_t device = yaksuri_zei_global.device[dev_id];
    yaksuri_zei_md_s *md = type_to_md(type, dev_id);

    if (type->kind == YAKSI_TYPE_KIND__DUP) {
        if (md->u.dup.child) {
            zerr =
                zeContextMakeMemoryResident(yaksuri_zei_global.context, device, md->u.dup.child,
                                            sizeof(yaksuri_zei_md_s));
            YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
            yaksuri_zei_type_make_resident(type->u.dup.child, dev_id);
        }
    } else if (type->kind == YAKSI_TYPE_KIND__RESIZED) {
        if (md->u.resized.child) {
            zerr =
                zeContextMakeMemoryResident(yaksuri_zei_global.context, device, md->u.resized.child,
                                            sizeof(yaksuri_zei_md_s));
            YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
            yaksuri_zei_type_make_resident(type->u.resized.child, dev_id);
        }
    } else if (type->kind == YAKSI_TYPE_KIND__HVECTOR) {
        if (md->u.hvector.child) {
            zerr =
                zeContextMakeMemoryResident(yaksuri_zei_global.context, device, md->u.hvector.child,
                                            sizeof(yaksuri_zei_md_s));
            YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
            yaksuri_zei_type_make_resident(type->u.hvector.child, dev_id);
        }
    } else if (type->kind == YAKSI_TYPE_KIND__BLKHINDX) {
        assert(md->u.blkhindx.array_of_displs);
        zerr =
            zeContextMakeMemoryResident(yaksuri_zei_global.context, device,
                                        md->u.blkhindx.array_of_displs,
                                        md->u.blkhindx.count * sizeof(intptr_t));
        YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
        assert(md->u.blkhindx.child);
        if (md->u.blkhindx.child) {
            zerr =
                zeContextMakeMemoryResident(yaksuri_zei_global.context, device,
                                            md->u.blkhindx.child, sizeof(yaksuri_zei_md_s));
            YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
            yaksuri_zei_type_make_resident(type->u.blkhindx.child, dev_id);
        }
    } else if (type->kind == YAKSI_TYPE_KIND__HINDEXED) {
        assert(md->u.hindexed.array_of_displs);
        zerr =
            zeContextMakeMemoryResident(yaksuri_zei_global.context, device,
                                        md->u.hindexed.array_of_displs,
                                        md->u.hindexed.count * sizeof(intptr_t));
        YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
        assert(md->u.hindexed.array_of_blocklengths);
        zerr =
            zeContextMakeMemoryResident(yaksuri_zei_global.context, device,
                                        md->u.hindexed.array_of_blocklengths,
                                        md->u.hindexed.count * sizeof(intptr_t));
        YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
        if (md->u.hindexed.child) {
            zerr =
                zeContextMakeMemoryResident(yaksuri_zei_global.context, device,
                                            md->u.hindexed.child, sizeof(yaksuri_zei_md_s));
            YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
            yaksuri_zei_type_make_resident(type->u.hindexed.child, dev_id);
        }
    } else if (type->kind == YAKSI_TYPE_KIND__CONTIG) {
        if (md->u.contig.child) {
            zerr =
                zeContextMakeMemoryResident(yaksuri_zei_global.context, device, md->u.contig.child,
                                            sizeof(yaksuri_zei_md_s));
            YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
            yaksuri_zei_type_make_resident(type->u.contig.child, dev_id);
        }
    } else if (type->kind == YAKSI_TYPE_KIND__SUBARRAY) {
        assert(0);
    } else if (type->kind == YAKSI_TYPE_KIND__STRUCT) {
        assert(0);
    }

  fn_exit:
    return rc;
  fn_fail:
    assert(0);
    goto fn_exit;
}

int yaksuri_zei_type_evict_resident(yaksi_type_s * type, int dev_id)
{
    int rc = YAKSA_SUCCESS;
    ze_result_t zerr;
    ze_device_handle_t device = yaksuri_zei_global.device[dev_id];
    yaksuri_zei_md_s *md = type_to_md(type, dev_id);

    if (type->kind == YAKSI_TYPE_KIND__DUP) {
        zerr =
            zeContextEvictMemory(yaksuri_zei_global.context, device, md->u.dup.child,
                                 sizeof(yaksuri_zei_md_s));
        YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
    } else if (type->kind == YAKSI_TYPE_KIND__RESIZED) {
        zerr =
            zeContextEvictMemory(yaksuri_zei_global.context, device, md->u.resized.child,
                                 sizeof(yaksuri_zei_md_s));
        YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
    } else if (type->kind == YAKSI_TYPE_KIND__HVECTOR) {
        zerr =
            zeContextEvictMemory(yaksuri_zei_global.context, device, md->u.hvector.child,
                                 sizeof(yaksuri_zei_md_s));
        YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
    } else if (type->kind == YAKSI_TYPE_KIND__BLKHINDX) {
        assert(md->u.blkhindx.array_of_displs);
        zerr =
            zeContextEvictMemory(yaksuri_zei_global.context, device, md->u.blkhindx.array_of_displs,
                                 md->u.blkhindx.count * sizeof(intptr_t));
        YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
        assert(md->u.blkhindx.child);
        zerr =
            zeContextEvictMemory(yaksuri_zei_global.context, device, md->u.blkhindx.child,
                                 sizeof(yaksuri_zei_md_s));
        YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
    } else if (type->kind == YAKSI_TYPE_KIND__HINDEXED) {
        assert(md->u.hindexed.array_of_displs);
        zerr =
            zeContextEvictMemory(yaksuri_zei_global.context, device, md->u.hindexed.array_of_displs,
                                 md->u.hindexed.count * sizeof(intptr_t));
        YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
        assert(md->u.hindexed.array_of_blocklengths);
        zerr =
            zeContextEvictMemory(yaksuri_zei_global.context, device,
                                 md->u.hindexed.array_of_blocklengths,
                                 md->u.hindexed.count * sizeof(intptr_t));
        YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
        assert(md->u.hindexed.child);
        zerr =
            zeContextEvictMemory(yaksuri_zei_global.context, device, md->u.hindexed.child,
                                 sizeof(yaksuri_zei_md_s));
        YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
    } else if (type->kind == YAKSI_TYPE_KIND__CONTIG) {
        assert(md->u.contig.child);
        zerr =
            zeContextEvictMemory(yaksuri_zei_global.context, device, md->u.contig.child,
                                 sizeof(yaksuri_zei_md_s));
        YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
    } else if (type->kind == YAKSI_TYPE_KIND__SUBARRAY) {
        assert(0);
    } else if (type->kind == YAKSI_TYPE_KIND__STRUCT) {
        assert(0);
    }

  fn_exit:
    return rc;
  fn_fail:
    assert(0);
    goto fn_exit;
}
