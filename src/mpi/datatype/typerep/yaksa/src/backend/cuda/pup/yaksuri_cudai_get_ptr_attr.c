/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksu.h"
#include "yaksuri_cudai.h"
#include <cuda.h>
#include <cuda_runtime_api.h>

static void attr_convert(struct cudaPointerAttributes cattr, yaksur_ptr_attr_s * attr)
{
    if (cattr.type == cudaMemoryTypeUnregistered) {
        attr->type = YAKSUR_PTR_TYPE__UNREGISTERED_HOST;
        attr->device = -1;
    } else if (cattr.type == cudaMemoryTypeHost) {
        attr->type = YAKSUR_PTR_TYPE__REGISTERED_HOST;
        attr->device = -1;
    } else if (cattr.type == cudaMemoryTypeManaged) {
        attr->type = YAKSUR_PTR_TYPE__MANAGED;
        attr->device = -1;
    } else {
        attr->type = YAKSUR_PTR_TYPE__GPU;
        attr->device = cattr.device;
    }
}

int yaksuri_cudai_get_ptr_attr(const void *inbuf, void *outbuf, yaksi_info_s * info,
                               yaksur_ptr_attr_s * inattr, yaksur_ptr_attr_s * outattr)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_cudai_info_s *infopriv;

    if (info) {
        infopriv = (yaksuri_cudai_info_s *) info->backend.cuda.priv;
    } else {
        infopriv = NULL;
    }

    if (infopriv && infopriv->inbuf.is_valid) {
        attr_convert(infopriv->inbuf.attr, inattr);
    } else {
        struct cudaPointerAttributes attr;
        cudaError_t cerr = cudaPointerGetAttributes(&attr, inbuf);
        if (cerr == cudaErrorInvalidValue) {
            attr.type = cudaMemoryTypeUnregistered;
            attr.device = -1;
            cerr = cudaSuccess;
        }
        YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);
        attr_convert(attr, inattr);
    }

    if (infopriv && infopriv->outbuf.is_valid) {
        attr_convert(infopriv->outbuf.attr, outattr);
    } else {
        struct cudaPointerAttributes attr;
        cudaError_t cerr = cudaPointerGetAttributes(&attr, outbuf);
        if (cerr == cudaErrorInvalidValue) {
            attr.type = cudaMemoryTypeUnregistered;
            attr.device = -1;
            cerr = cudaSuccess;
        }
        YAKSURI_CUDAI_CUDA_ERR_CHKANDJUMP(cerr, rc, fn_fail);
        attr_convert(attr, outattr);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
