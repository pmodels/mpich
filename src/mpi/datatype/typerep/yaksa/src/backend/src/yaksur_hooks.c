/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdlib.h>
#include <assert.h>
#include "yaksa.h"
#include "yaksi.h"
#include "yaksu.h"
#include "yaksuri.h"

yaksuri_global_s yaksuri_global;

static void *malloc_fn(uintptr_t size, void *state)
{
    yaksuri_gpudriver_id_e id;

    for (id = YAKSURI_GPUDRIVER_ID__UNSET; id < YAKSURI_GPUDRIVER_ID__LAST; id++) {
        if (id == YAKSURI_GPUDRIVER_ID__UNSET || yaksuri_global.gpudriver[id].hooks == NULL)
            continue;

        if (state == &yaksuri_global.gpudriver[id].host) {
            return yaksuri_global.gpudriver[id].hooks->host_malloc(size);
        } else {
            uintptr_t start = (uintptr_t) yaksuri_global.gpudriver[id].device;
            uintptr_t end =
                (uintptr_t) & yaksuri_global.gpudriver[id].device[yaksuri_global.
                                                                  gpudriver[id].ndevices - 1];

            if ((uintptr_t) state >= start && (uintptr_t) state <= end) {
                int dev = (int) ((uintptr_t) state - start) / sizeof(yaksu_buffer_pool_s);
                return yaksuri_global.gpudriver[id].hooks->gpu_malloc(size, dev);
            }
        }
    }

    return NULL;
}

static void free_fn(void *buf, void *state)
{
    yaksuri_gpudriver_id_e id;

    for (id = YAKSURI_GPUDRIVER_ID__UNSET; id < YAKSURI_GPUDRIVER_ID__LAST; id++) {
        if (id == YAKSURI_GPUDRIVER_ID__UNSET || yaksuri_global.gpudriver[id].hooks == NULL)
            continue;

        if (state == &yaksuri_global.gpudriver[id].host) {
            yaksuri_global.gpudriver[id].hooks->host_free(buf);
            return;
        } else {
            uintptr_t start = (uintptr_t) yaksuri_global.gpudriver[id].device;
            uintptr_t end =
                (uintptr_t) & yaksuri_global.gpudriver[id].device[yaksuri_global.
                                                                  gpudriver[id].ndevices - 1];

            if ((uintptr_t) state >= start && (uintptr_t) state <= end) {
                yaksuri_global.gpudriver[id].hooks->gpu_free(buf);
                return;
            }
        }
    }

    assert(0);
}

int yaksur_init_hook(yaksi_info_s * info)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id;

    rc = yaksuri_seq_init_hook();
    YAKSU_ERR_CHECK(rc, fn_fail);

    if (info) {
        yaksuri_info_s *infopriv = info->backend.priv;
        if (infopriv->gpudriver_id == YAKSURI_GPUDRIVER_ID__LAST) {
            /* gpu disabled */
            goto fn_exit;
        }
        yaksuri_global.has_wait_kernel = infopriv->has_wait_kernel;
    }

    /* CUDA hooks */
    id = YAKSURI_GPUDRIVER_ID__CUDA;
    yaksuri_global.gpudriver[id].hooks = NULL;
    rc = yaksuri_cuda_init_hook(&yaksuri_global.gpudriver[id].hooks);
    YAKSU_ERR_CHECK(rc, fn_fail);

    /* ZE hooks */
    id = YAKSURI_GPUDRIVER_ID__ZE;
    yaksuri_global.gpudriver[id].hooks = NULL;
    rc = yaksuri_ze_init_hook(&yaksuri_global.gpudriver[id].hooks);
    YAKSU_ERR_CHECK(rc, fn_fail);

    /* HIP hooks */
    id = YAKSURI_GPUDRIVER_ID__HIP;
    yaksuri_global.gpudriver[id].hooks = NULL;
    rc = yaksuri_hip_init_hook(&yaksuri_global.gpudriver[id].hooks);
    YAKSU_ERR_CHECK(rc, fn_fail);
    /* final setup for all drivers */
    for (id = YAKSURI_GPUDRIVER_ID__UNSET; id < YAKSURI_GPUDRIVER_ID__LAST; id++) {
        if (id == YAKSURI_GPUDRIVER_ID__UNSET || yaksuri_global.gpudriver[id].hooks == NULL)
            continue;

        rc = yaksu_buffer_pool_alloc(YAKSURI_TMPBUF_EL_SIZE, 1, YAKSURI_TMPBUF_NUM_EL,
                                     malloc_fn, free_fn, &yaksuri_global.gpudriver[id].host,
                                     &yaksuri_global.gpudriver[id].host);
        YAKSU_ERR_CHECK(rc, fn_fail);

        int ndevices;
        rc = yaksuri_global.gpudriver[id].hooks->get_num_devices(&ndevices);
        YAKSU_ERR_CHECK(rc, fn_fail);

        yaksuri_global.gpudriver[id].device = (yaksu_buffer_pool_s *)
            malloc(ndevices * sizeof(yaksu_buffer_pool_s));
        for (int i = 0; i < ndevices; i++) {
            rc = yaksu_buffer_pool_alloc(YAKSURI_TMPBUF_EL_SIZE, 1, YAKSURI_TMPBUF_NUM_EL,
                                         malloc_fn, free_fn,
                                         &yaksuri_global.gpudriver[id].device[i],
                                         &yaksuri_global.gpudriver[id].device[i]);
            YAKSU_ERR_CHECK(rc, fn_fail);
        }

        yaksuri_global.gpudriver[id].ndevices = ndevices;
    }

    rc = yaksuri_progress_init();
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksur_finalize_hook(void)
{
    int rc = YAKSA_SUCCESS;

    rc = yaksuri_progress_finalize();
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = yaksuri_seq_finalize_hook();
    YAKSU_ERR_CHECK(rc, fn_fail);

    for (yaksuri_gpudriver_id_e id = YAKSURI_GPUDRIVER_ID__UNSET;
         id < YAKSURI_GPUDRIVER_ID__LAST; id++) {
        if (id == YAKSURI_GPUDRIVER_ID__UNSET || yaksuri_global.gpudriver[id].hooks == NULL)
            continue;

        rc = yaksu_buffer_pool_free(yaksuri_global.gpudriver[id].host);
        YAKSU_ERR_CHECK(rc, fn_fail);

        int ndevices = yaksuri_global.gpudriver[id].ndevices;
        for (int i = 0; i < ndevices; i++) {
            rc = yaksu_buffer_pool_free(yaksuri_global.gpudriver[id].device[i]);
            YAKSU_ERR_CHECK(rc, fn_fail);
        }
        free(yaksuri_global.gpudriver[id].device);

        rc = yaksuri_global.gpudriver[id].hooks->finalize();
        YAKSU_ERR_CHECK(rc, fn_fail);
        free(yaksuri_global.gpudriver[id].hooks);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksur_type_create_hook(yaksi_type_s * type)
{
    int rc = YAKSA_SUCCESS;

    rc = yaksuri_seq_type_create_hook(type);
    YAKSU_ERR_CHECK(rc, fn_fail);

    for (yaksuri_gpudriver_id_e id = YAKSURI_GPUDRIVER_ID__UNSET;
         id < YAKSURI_GPUDRIVER_ID__LAST; id++) {
        if (id == YAKSURI_GPUDRIVER_ID__UNSET || yaksuri_global.gpudriver[id].hooks == NULL)
            continue;

        rc = yaksuri_global.gpudriver[id].hooks->type_create(type);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksur_type_free_hook(yaksi_type_s * type)
{
    int rc = YAKSA_SUCCESS;

    rc = yaksuri_seq_type_free_hook(type);
    YAKSU_ERR_CHECK(rc, fn_fail);

    for (yaksuri_gpudriver_id_e id = YAKSURI_GPUDRIVER_ID__UNSET;
         id < YAKSURI_GPUDRIVER_ID__LAST; id++) {
        if (id == YAKSURI_GPUDRIVER_ID__UNSET || yaksuri_global.gpudriver[id].hooks == NULL)
            continue;

        rc = yaksuri_global.gpudriver[id].hooks->type_free(type);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksur_request_create_hook(yaksi_request_s * request)
{
    int rc = YAKSA_SUCCESS;

    request->backend.priv = malloc(sizeof(yaksuri_request_s));
    yaksuri_request_s *reqpriv = (yaksuri_request_s *) request->backend.priv;

    reqpriv->request = request;
    reqpriv->info = NULL;
    reqpriv->optype = YAKSURI_OPTYPE__UNSET;
    reqpriv->gpudriver_id = YAKSURI_GPUDRIVER_ID__UNSET;
    reqpriv->subreqs = NULL;

    return rc;
}

int yaksur_request_free_hook(yaksi_request_s * request)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_request_s *reqpriv = (yaksuri_request_s *) request->backend.priv;

    assert(reqpriv->subreqs == NULL);
    free(reqpriv);

    return rc;
}

int yaksur_info_create_hook(yaksi_info_s * info)
{
    int rc = YAKSA_SUCCESS;

    info->backend.pre_init = !yaksu_atomic_load(&yaksi_is_initialized);

    info->backend.priv = (yaksuri_info_s *) malloc(sizeof(yaksuri_info_s));

    yaksuri_info_s *infopriv;
    infopriv = (yaksuri_info_s *) info->backend.priv;
    infopriv->gpudriver_id = YAKSURI_GPUDRIVER_ID__UNSET;
    infopriv->mapped_device = -1;
    infopriv->has_wait_kernel = false;

    rc = yaksuri_seq_info_create_hook(info);
    YAKSU_ERR_CHECK(rc, fn_fail);

    if (info->backend.pre_init) {
        goto fn_exit;
    }

    for (yaksuri_gpudriver_id_e id = YAKSURI_GPUDRIVER_ID__UNSET;
         id < YAKSURI_GPUDRIVER_ID__LAST; id++) {
        if (id == YAKSURI_GPUDRIVER_ID__UNSET || yaksuri_global.gpudriver[id].hooks == NULL)
            continue;

        rc = yaksuri_global.gpudriver[id].hooks->info_create(info);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksur_info_free_hook(yaksi_info_s * info)
{
    int rc = YAKSA_SUCCESS;

    free(info->backend.priv);

    rc = yaksuri_seq_info_free_hook(info);
    YAKSU_ERR_CHECK(rc, fn_fail);

    if (info->backend.pre_init) {
        goto fn_exit;
    }

    for (yaksuri_gpudriver_id_e id = YAKSURI_GPUDRIVER_ID__UNSET;
         id < YAKSURI_GPUDRIVER_ID__LAST; id++) {
        if (id == YAKSURI_GPUDRIVER_ID__UNSET || yaksuri_global.gpudriver[id].hooks == NULL)
            continue;

        rc = yaksuri_global.gpudriver[id].hooks->info_free(info);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksur_info_keyval_append(yaksi_info_s * info, const char *key, const void *val,
                              unsigned int vallen)
{
    int rc = YAKSA_SUCCESS;

    if (!strncmp(key, "yaksa_gpu_driver", YAKSA_INFO_MAX_KEYLEN)) {
        yaksuri_info_s *infopriv = info->backend.priv;
        if (!strncmp(val, "cuda", vallen)) {
            infopriv->gpudriver_id = YAKSURI_GPUDRIVER_ID__CUDA;
        } else if (!strncmp(val, "ze", vallen)) {
            infopriv->gpudriver_id = YAKSURI_GPUDRIVER_ID__ZE;
        } else if (!strncmp(val, "hip", vallen)) {
            infopriv->gpudriver_id = YAKSURI_GPUDRIVER_ID__HIP;
        } else if (!strncmp(val, "nogpu", vallen)) {
            infopriv->gpudriver_id = YAKSURI_GPUDRIVER_ID__LAST;
        } else {
            assert(0);
        }
        goto fn_exit;
    } else if (!strncmp(key, "yaksa_mapped_device", YAKSA_INFO_MAX_KEYLEN)) {
        yaksuri_info_s *infopriv = info->backend.priv;
        assert(vallen == sizeof(int));
        infopriv->mapped_device = *((const int *) val);
        goto fn_exit;
    } else if (strcmp(key, "yaksa_has_wait_kernel") == 0) {
        /* only for yaksa_init, to avoid allocating registered staging buffer pool,
         * due to potential deadlocks with "wait" kernels. */
        yaksuri_info_s *infopriv = info->backend.priv;
        if (strcmp((char *) val, "true") == 0 ||
            strcmp((char *) val, "yes") == 0 || strcmp((char *) val, "1") == 0) {
            infopriv->has_wait_kernel = true;
        }
        goto fn_exit;
    }

    rc = yaksuri_seq_info_keyval_append(info, key, val, vallen);
    YAKSU_ERR_CHECK(rc, fn_fail);

    for (yaksuri_gpudriver_id_e id = YAKSURI_GPUDRIVER_ID__UNSET;
         id < YAKSURI_GPUDRIVER_ID__LAST; id++) {
        if (id == YAKSURI_GPUDRIVER_ID__UNSET || yaksuri_global.gpudriver[id].hooks == NULL)
            continue;

        rc = yaksuri_global.gpudriver[id].hooks->info_keyval_append(info, key, val, vallen);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
