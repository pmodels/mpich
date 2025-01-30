/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <assert.h>
#include "yaksa.h"
#include "yaksi.h"
#include "yaksu.h"
#include "yaksuri.h"

static int ipup(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type,
                yaksi_info_s * info, yaksa_op_t op, yaksi_request_s * request,
                bool always_query_ptr_attr)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_request_s *reqpriv = (yaksuri_request_s *) request->backend.priv;

    if (!always_query_ptr_attr && reqpriv->gpudriver_id != YAKSURI_GPUDRIVER_ID__UNSET)
        goto query_done;

    yaksuri_info_s *infopriv;
    int (*hookfn) (const void *inbuf, void *outbuf, yaksi_info_s * info,
                   yaksur_ptr_attr_s * inattr, yaksur_ptr_attr_s * outattr);

    if (info) {
        infopriv = (yaksuri_info_s *) info->backend.priv;
    }

    yaksuri_gpudriver_id_e id;
    id = always_query_ptr_attr ? YAKSURI_GPUDRIVER_ID__UNSET : reqpriv->gpudriver_id;
    if (id == YAKSURI_GPUDRIVER_ID__UNSET) {
        for (id = YAKSURI_GPUDRIVER_ID__UNSET; id < YAKSURI_GPUDRIVER_ID__LAST; id++) {
            if (id == YAKSURI_GPUDRIVER_ID__UNSET || yaksuri_global.gpudriver[id].hooks == NULL)
                continue;

            if (info && infopriv->gpudriver_id != YAKSURI_GPUDRIVER_ID__UNSET &&
                infopriv->gpudriver_id != id)
                continue;

            hookfn = yaksuri_global.gpudriver[id].hooks->get_ptr_attr;
            if (reqpriv->optype == YAKSURI_OPTYPE__PACK) {
                rc = hookfn((const char *) inbuf + type->true_lb, outbuf, info,
                            &request->backend.inattr, &request->backend.outattr);
            } else {
                rc = hookfn(inbuf, (char *) outbuf + type->true_lb, info,
                            &request->backend.inattr, &request->backend.outattr);
            }
            YAKSU_ERR_CHECK(rc, fn_fail);

            if (request->backend.inattr.type == YAKSUR_PTR_TYPE__GPU ||
                request->backend.outattr.type == YAKSUR_PTR_TYPE__GPU) {
                reqpriv->gpudriver_id = id;
                break;
            }
        }
    }

    if (id == YAKSURI_GPUDRIVER_ID__LAST)
        reqpriv->gpudriver_id = YAKSURI_GPUDRIVER_ID__LAST;

  query_done:
    /* if this can be handled by the CPU, wrap it up */
    if (reqpriv->gpudriver_id == YAKSURI_GPUDRIVER_ID__LAST) {
        bool is_supported;
        rc = yaksuri_seq_pup_is_supported(type, op, &is_supported);
        YAKSU_ERR_CHECK(rc, fn_fail);

        /* FIXME: check request-kind == YAKSI_REQUEST_KIND__GPU_STREAM
         *        if stream, we need enqueue seq_ipack/iunpack */
        if (!is_supported) {
            rc = YAKSA_ERR__NOT_SUPPORTED;
        } else {
            if (reqpriv->optype == YAKSURI_OPTYPE__PACK) {
                rc = yaksuri_seq_ipack(inbuf, outbuf, count, type, info, op);
                YAKSU_ERR_CHECK(rc, fn_fail);
            } else {
                rc = yaksuri_seq_iunpack(inbuf, outbuf, count, type, info, op);
                YAKSU_ERR_CHECK(rc, fn_fail);
            }
        }
    } else {
        rc = yaksuri_progress_enqueue(inbuf, outbuf, count, type, info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksur_ipack(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type,
                 yaksi_info_s * info, yaksa_op_t op, yaksi_request_s * request)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_request_s *reqpriv = (yaksuri_request_s *) request->backend.priv;

    reqpriv->optype = YAKSURI_OPTYPE__PACK;
    rc = ipup(inbuf, outbuf, count, type, info, op, request, request->always_query_ptr_attr);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksur_iunpack(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type,
                   yaksi_info_s * info, yaksa_op_t op, yaksi_request_s * request)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_request_s *reqpriv = (yaksuri_request_s *) request->backend.priv;

    reqpriv->optype = YAKSURI_OPTYPE__UNPACK;
    rc = ipup(inbuf, outbuf, count, type, info, op, request, request->always_query_ptr_attr);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
