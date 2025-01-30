/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdlib.h>
#include <assert.h>
#include "yaksi.h"
#include "yaksu.h"
#include "yaksuri_cudai.h"

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

int yaksuri_cudai_type_create_hook(yaksi_type_s * type)
{
    int rc = YAKSA_SUCCESS;

    type->backend.cuda.priv = malloc(sizeof(yaksuri_cudai_type_s));
    YAKSU_ERR_CHKANDJUMP(!type->backend.cuda.priv, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);

    yaksuri_cudai_type_s *cuda;
    cuda = (yaksuri_cudai_type_s *) type->backend.cuda.priv;

    cuda->num_elements = get_num_elements(type);
    cuda->md = NULL;
    pthread_mutex_init(&cuda->mdmutex, NULL);

    rc = yaksuri_cudai_populate_pupfns(type);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksuri_cudai_type_free_hook(yaksi_type_s * type)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_cudai_type_s *cuda = (yaksuri_cudai_type_s *) type->backend.cuda.priv;
    cudaError_t cerr;

    pthread_mutex_destroy(&cuda->mdmutex);
    if (cuda->md) {
        if (type->kind == YAKSI_TYPE_KIND__BLKHINDX) {
            assert(cuda->md->u.blkhindx.array_of_displs);
            cerr = cudaFree((void *) cuda->md->u.blkhindx.array_of_displs);
            YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);
        } else if (type->kind == YAKSI_TYPE_KIND__HINDEXED) {
            assert(cuda->md->u.hindexed.array_of_displs);
            cerr = cudaFree((void *) cuda->md->u.hindexed.array_of_displs);
            YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);

            assert(cuda->md->u.hindexed.array_of_blocklengths);
            cerr = cudaFree((void *) cuda->md->u.hindexed.array_of_blocklengths);
            YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);
        }

        cerr = cudaFree(cuda->md);
        YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);
    }

    free(cuda);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
