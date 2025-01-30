/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksuri_zei.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

int yaksuri_zei_info_create_hook(yaksi_info_s * info)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_zei_info_s *ze;

    ze = (yaksuri_zei_info_s *) malloc(sizeof(yaksuri_zei_info_s));
    YAKSU_ERR_CHKANDJUMP(!ze, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);

    /* set default values for info keys */
    ze->yaksa_ze_use_copy_engine = 0;
    ze->iov_pack_threshold = YAKSURI_ZEI_INFO__DEFAULT_IOV_PUP_THRESHOLD;
    ze->iov_unpack_threshold = YAKSURI_ZEI_INFO__DEFAULT_IOV_PUP_THRESHOLD;
    ze->inbuf.is_valid = false;
    ze->outbuf.is_valid = false;

    info->backend.ze.priv = (void *) ze;

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksuri_zei_info_free_hook(yaksi_info_s * info)
{
    free(info->backend.ze.priv);

    return YAKSA_SUCCESS;
}

int yaksuri_zei_info_keyval_append(yaksi_info_s * info, const char *key, const void *val,
                                   unsigned int vallen)
{
    yaksuri_zei_info_s *ze = (yaksuri_zei_info_s *) info->backend.ze.priv;

    typedef struct {
        ze_memory_allocation_properties_t prop;
        ze_device_handle_t device;
    } ze_alloc_attr_t;

    if (!strncmp(key, "yaksa_ze_use_copy_engine", YAKSA_INFO_MAX_KEYLEN)) {
        assert(vallen == sizeof(int));
        ze->yaksa_ze_use_copy_engine = *(int *) val;
    } else if (!strncmp(key, "yaksa_ze_iov_unpack_threshold", YAKSA_INFO_MAX_KEYLEN)) {
        assert(vallen == sizeof(uintptr_t));
        ze->iov_unpack_threshold = (uintptr_t) val;
    } else if (!strncmp(key, "yaksa_ze_inbuf_ptr_attr", YAKSA_INFO_MAX_KEYLEN)) {
        assert(vallen == sizeof(ze_alloc_attr_t));
        ze->inbuf.is_valid = true;
        memcpy(&ze->inbuf.attr, val, sizeof(ze_alloc_attr_t));
    } else if (!strncmp(key, "yaksa_ze_outbuf_ptr_attr", YAKSA_INFO_MAX_KEYLEN)) {
        assert(vallen == sizeof(ze_alloc_attr_t));
        ze->outbuf.is_valid = true;
        memcpy(&ze->outbuf.attr, val, sizeof(ze_alloc_attr_t));
    }

    return YAKSA_SUCCESS;
}
