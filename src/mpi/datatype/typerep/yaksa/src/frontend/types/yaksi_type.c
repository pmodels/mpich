/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksu.h"
#include <assert.h>

int yaksi_type_handle_alloc(yaksi_type_s * type, yaksa_type_t * handle)
{
    int rc = YAKSA_SUCCESS;
    yaksu_handle_t id;

    rc = yaksu_handle_pool_elem_alloc(yaksi_global.type_handle_pool, &id, type);
    YAKSU_ERR_CHECK(rc, fn_fail);

    *handle = 0;
    YAKSI_TYPE_SET_OBJECT_ID(*handle, id);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksi_type_handle_dealloc(yaksa_type_t handle, yaksi_type_s ** type)
{
    int rc = YAKSA_SUCCESS;
    yaksu_handle_t id = YAKSI_TYPE_GET_OBJECT_ID(handle);

    rc = yaksu_handle_pool_elem_get(yaksi_global.type_handle_pool, id, (const void **) type);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = yaksu_handle_pool_elem_free(yaksi_global.type_handle_pool, id);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksi_type_get(yaksa_type_t handle, yaksi_type_s ** type)
{
    int rc = YAKSA_SUCCESS;
    yaksu_handle_t id = YAKSI_TYPE_GET_OBJECT_ID(handle);

    rc = yaksu_handle_pool_elem_get(yaksi_global.type_handle_pool, id, (const void **) type);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
