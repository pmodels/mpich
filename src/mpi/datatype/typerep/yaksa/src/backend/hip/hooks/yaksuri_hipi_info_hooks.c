/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksuri_hipi.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

int yaksuri_hipi_info_create_hook(yaksi_info_s * info)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_hipi_info_s *infopriv;

    infopriv = (yaksuri_hipi_info_s *) malloc(sizeof(yaksuri_hipi_info_s));
    YAKSU_ERR_CHKANDJUMP(!infopriv, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);

    /* set default values for info keys */
    infopriv->iov_pack_threshold = YAKSURI_HIPI_INFO__DEFAULT_IOV_PUP_THRESHOLD;
    infopriv->iov_unpack_threshold = YAKSURI_HIPI_INFO__DEFAULT_IOV_PUP_THRESHOLD;
    infopriv->inbuf.is_valid = false;
    infopriv->outbuf.is_valid = false;

    info->backend.hip.priv = (void *) infopriv;

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksuri_hipi_info_free_hook(yaksi_info_s * info)
{
    free(info->backend.hip.priv);

    return YAKSA_SUCCESS;
}

int yaksuri_hipi_info_keyval_append(yaksi_info_s * info, const char *key, const void *val,
                                    unsigned int vallen)
{
    yaksuri_hipi_info_s *infopriv = (yaksuri_hipi_info_s *) info->backend.hip.priv;

    if (!strncmp(key, "yaksa_hip_iov_pack_threshold", YAKSA_INFO_MAX_KEYLEN)) {
        assert(vallen == sizeof(uintptr_t));
        infopriv->iov_pack_threshold = (uintptr_t) val;
    } else if (!strncmp(key, "yaksa_hip_iov_unpack_threshold", YAKSA_INFO_MAX_KEYLEN)) {
        assert(vallen == sizeof(uintptr_t));
        infopriv->iov_unpack_threshold = (uintptr_t) val;
    } else if (!strncmp(key, "yaksa_hip_inbuf_ptr_attr", YAKSA_INFO_MAX_KEYLEN)) {
        assert(vallen == sizeof(struct hipPointerAttribute_t));
        infopriv->inbuf.is_valid = true;
        memcpy(&infopriv->inbuf.attr, val, sizeof(struct hipPointerAttribute_t));
    } else if (!strncmp(key, "yaksa_hip_outbuf_ptr_attr", YAKSA_INFO_MAX_KEYLEN)) {
        assert(vallen == sizeof(struct hipPointerAttribute_t));
        infopriv->outbuf.is_valid = true;
        memcpy(&infopriv->outbuf.attr, val, sizeof(struct hipPointerAttribute_t));
    }

    return YAKSA_SUCCESS;
}
