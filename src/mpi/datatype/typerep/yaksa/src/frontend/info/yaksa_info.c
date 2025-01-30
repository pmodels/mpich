/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksu.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

YAKSA_API_PUBLIC int yaksa_info_create(yaksa_info_t * info)
{
    int rc = YAKSA_SUCCESS;
    yaksi_info_s *yaksi_info;

    yaksi_info = (yaksi_info_s *) malloc(sizeof(yaksi_info_s));

    yaksu_atomic_store(&yaksi_info->refcount, 1);

    rc = yaksur_info_create_hook(yaksi_info);
    YAKSU_ERR_CHECK(rc, fn_fail);

    *info = yaksi_info;

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

YAKSA_API_PUBLIC int yaksa_info_free(yaksa_info_t info)
{
    int rc = YAKSA_SUCCESS;
    yaksi_info_s *yaksi_info = (yaksi_info_s *) info;

    if (yaksu_atomic_decr(&yaksi_info->refcount) > 1)
        goto fn_exit;

    rc = yaksur_info_free_hook(yaksi_info);
    YAKSU_ERR_CHECK(rc, fn_fail);

    free(yaksi_info);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

YAKSA_API_PUBLIC int yaksa_info_keyval_append(yaksa_info_t info, const char *key, const void *val,
                                              unsigned int vallen)
{
    int rc = YAKSA_SUCCESS;
    yaksi_info_s *yaksi_info = (yaksi_info_s *) info;

    rc = yaksur_info_keyval_append(yaksi_info, key, val, vallen);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
