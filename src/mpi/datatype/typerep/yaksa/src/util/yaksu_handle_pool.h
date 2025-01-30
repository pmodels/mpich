/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef YAKSU_HANDLE_POOL_H_INCLUDED
#define YAKSU_HANDLE_POOL_H_INCLUDED

typedef void *yaksu_handle_pool_s;
typedef uint64_t yaksu_handle_t;

int yaksu_handle_pool_alloc(yaksu_handle_pool_s * pool);
int yaksu_handle_pool_free(yaksu_handle_pool_s pool);
int yaksu_handle_pool_elem_alloc(yaksu_handle_pool_s pool, yaksu_handle_t * handle, void *data);
int yaksu_handle_pool_elem_free(yaksu_handle_pool_s pool, yaksu_handle_t handle);
int yaksu_handle_pool_elem_get(yaksu_handle_pool_s pool, yaksu_handle_t handle, const void **data);

#endif /* YAKSU_HANDLE_POOL_H_INCLUDED */
