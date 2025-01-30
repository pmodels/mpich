/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksu.h"
#include "yaksuri_hipi.h"
#include <hip/hip_runtime.h>
#include <hip/hip_runtime_api.h>

#ifdef HIP_USE_MEMORYTYPE
#define ATTRTYPE memoryType
static hipError_t attr_convert(hipError_t cerr, struct hipPointerAttribute_t cattr,
                               yaksur_ptr_attr_s * attr)
{
    if (cerr == hipErrorInvalidValue) {
        /* attr.ATTRTYPE = hipMemoryTypeUnregistered;  */
        /* HIP does not seem to have something corresponding to cudaMemoryTypeUnregistered */
        attr->type = YAKSUR_PTR_TYPE__UNREGISTERED_HOST;
        attr->device = -1;
        return hipSuccess;
    }

    if (cattr.ATTRTYPE == hipMemoryTypeHost) {
        attr->type = YAKSUR_PTR_TYPE__REGISTERED_HOST;
        attr->device = cattr.device;
    } else if (cattr.isManaged) {
        attr->type = YAKSUR_PTR_TYPE__MANAGED;
        attr->device = cattr.device;
    } else if (cattr.ATTRTYPE == hipMemoryTypeDevice) {
        attr->type = YAKSUR_PTR_TYPE__GPU;
        attr->device = cattr.device;
    } else {
        attr->type = YAKSUR_PTR_TYPE__UNREGISTERED_HOST;
        attr->device = -1;
    }

    return cerr;
}
#else
#define ATTRTYPE type
static hipError_t attr_convert(hipError_t cerr, struct hipPointerAttribute_t cattr,
                               yaksur_ptr_attr_s * attr)
{
    if (cattr.ATTRTYPE == hipMemoryTypeHost) {
        attr->type = YAKSUR_PTR_TYPE__REGISTERED_HOST;
        attr->device = cattr.device;
    } else if (cattr.ATTRTYPE == hipMemoryTypeManaged) {
        attr->type = YAKSUR_PTR_TYPE__MANAGED;
        attr->device = cattr.device;
    } else if (cattr.ATTRTYPE == hipMemoryTypeDevice) {
        attr->type = YAKSUR_PTR_TYPE__GPU;
        attr->device = cattr.device;
    } else if (cattr.ATTRTYPE == hipMemoryTypeUnregistered) {
        attr->type = YAKSUR_PTR_TYPE__UNREGISTERED_HOST;
        attr->device = -1;
    }

    return cerr;
}
#endif

int yaksuri_hipi_get_ptr_attr(const void *inbuf, void *outbuf, yaksi_info_s * info,
                              yaksur_ptr_attr_s * inattr, yaksur_ptr_attr_s * outattr)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_hipi_info_s *infopriv;

    if (info) {
        infopriv = (yaksuri_hipi_info_s *) info->backend.hip.priv;
    } else {
        infopriv = NULL;
    }

    if (infopriv && infopriv->inbuf.is_valid) {
        (void) attr_convert(hipSuccess, infopriv->inbuf.attr, inattr);
    } else {
        struct hipPointerAttribute_t attr;
        hipError_t cerr = hipPointerGetAttributes(&attr, inbuf);
        cerr = attr_convert(cerr, attr, inattr);
        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);
    }

    if (infopriv && infopriv->outbuf.is_valid) {
        (void) attr_convert(hipSuccess, infopriv->outbuf.attr, outattr);
    } else {
        struct hipPointerAttribute_t attr;
        hipError_t cerr = hipPointerGetAttributes(&attr, outbuf);
        cerr = attr_convert(cerr, attr, outattr);
        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
