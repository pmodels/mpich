/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdlib.h>
#include <assert.h>
#include "yaksi.h"
#include "yaksu.h"
#include "yaksuri_hipi.h"

static uintptr_t get_num_elements(yaksi_type_s * type)
{
    switch (type->kind) {
        case YAKSI_TYPE_KIND__BUILTIN:
            return type->num_contig;

        case YAKSI_TYPE_KIND__CONTIG:
            return type->u.contig.count * get_num_elements(type->u.contig.child);

        case YAKSI_TYPE_KIND__RESIZED:
            return get_num_elements(type->u.resized.child);

        case YAKSI_TYPE_KIND__HVECTOR:
            return type->u.hvector.count * type->u.hvector.blocklength *
                get_num_elements(type->u.hvector.child);

        case YAKSI_TYPE_KIND__BLKHINDX:
            return type->u.blkhindx.count * type->u.blkhindx.blocklength *
                get_num_elements(type->u.blkhindx.child);

        case YAKSI_TYPE_KIND__HINDEXED:
            {
                uintptr_t nelems = 0;
                for (int i = 0; i < type->u.hindexed.count; i++)
                    nelems += type->u.hindexed.array_of_blocklengths[i];
                nelems *= get_num_elements(type->u.hindexed.child);
                return nelems;
            }

        default:
            return 0;
    }
}

int yaksuri_hipi_type_create_hook(yaksi_type_s * type)
{
    int rc = YAKSA_SUCCESS;

    type->backend.hip.priv = malloc(sizeof(yaksuri_hipi_type_s));
    YAKSU_ERR_CHKANDJUMP(!type->backend.hip.priv, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);

    yaksuri_hipi_type_s *hip;
    hip = (yaksuri_hipi_type_s *) type->backend.hip.priv;

    hip->num_elements = get_num_elements(type);
    hip->md = NULL;
    pthread_mutex_init(&hip->mdmutex, NULL);

    rc = yaksuri_hipi_populate_pupfns(type);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksuri_hipi_type_free_hook(yaksi_type_s * type)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_hipi_type_s *hip = (yaksuri_hipi_type_s *) type->backend.hip.priv;
    hipError_t cerr;

    pthread_mutex_destroy(&hip->mdmutex);
    if (hip->md) {
        if (type->kind == YAKSI_TYPE_KIND__BLKHINDX) {
            assert(hip->md->u.blkhindx.array_of_displs);
            cerr = hipFree((void *) hip->md->u.blkhindx.array_of_displs);
            YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);
        } else if (type->kind == YAKSI_TYPE_KIND__HINDEXED) {
            assert(hip->md->u.hindexed.array_of_displs);
            cerr = hipFree((void *) hip->md->u.hindexed.array_of_displs);
            YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

            assert(hip->md->u.hindexed.array_of_blocklengths);
            cerr = hipFree((void *) hip->md->u.hindexed.array_of_blocklengths);
            YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);
        }

        cerr = hipFree(hip->md);
        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);
    }

    free(hip);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
