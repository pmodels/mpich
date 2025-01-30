/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksu.h"
#include "yaksuri_seqi.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

int yaksuri_seq_init_hook(void)
{
    return YAKSA_SUCCESS;
}

int yaksuri_seq_finalize_hook(void)
{
    return YAKSA_SUCCESS;
}

int yaksuri_seq_type_create_hook(yaksi_type_s * type)
{
    int rc = YAKSA_SUCCESS;

    type->backend.seq.priv = malloc(sizeof(yaksuri_seqi_type_s));
    YAKSU_ERR_CHKANDJUMP(!type->backend.seq.priv, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);

    rc = yaksuri_seqi_populate_pupfns(type);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksuri_seq_type_free_hook(yaksi_type_s * type)
{
    int rc = YAKSA_SUCCESS;

    free(type->backend.seq.priv);

    return rc;
}

int yaksuri_seq_info_create_hook(yaksi_info_s * info)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_seqi_info_s *seq;

    seq = (yaksuri_seqi_info_s *) malloc(sizeof(yaksuri_seqi_info_s));
    YAKSU_ERR_CHKANDJUMP(!seq, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);

    /* set default values for info keys */
    seq->iov_pack_threshold = YAKSURI_SEQI_INFO__DEFAULT_IOV_PUP_THRESHOLD;
    seq->iov_unpack_threshold = YAKSURI_SEQI_INFO__DEFAULT_IOV_PUP_THRESHOLD;

    info->backend.seq.priv = (void *) seq;

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksuri_seq_info_free_hook(yaksi_info_s * info)
{
    free(info->backend.seq.priv);

    return YAKSA_SUCCESS;
}

int yaksuri_seq_info_keyval_append(yaksi_info_s * info, const char *key, const void *val,
                                   unsigned int vallen)
{
    yaksuri_seqi_info_s *seq = (yaksuri_seqi_info_s *) info->backend.seq.priv;

    if (!strncmp(key, "yaksa_seq_iov_pack_threshold", YAKSA_INFO_MAX_KEYLEN)) {
        assert(vallen == sizeof(uintptr_t));
        seq->iov_pack_threshold = (uintptr_t) val;
    } else if (!strncmp(key, "yaksa_seq_iov_unpack_threshold", YAKSA_INFO_MAX_KEYLEN)) {
        assert(vallen == sizeof(uintptr_t));
        seq->iov_unpack_threshold = (uintptr_t) val;
    }

    return YAKSA_SUCCESS;
}
