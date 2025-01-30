/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksa.h"
#include "yaksu.h"
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <yutlist.h>
#include <yuthash.h>
#include <pthread.h>

/*
 * If there are any free handles that were previously allocated and
 * freed, use those first.  If there are no such handles, it means
 * that everything before the "brand-new" handle range is currently in
 * use.  In this case, return the next handle and increment the
 * "next_handle" count.
 *
 * Both allocation and free are O(1) operations in this algorithm,
 * although the free operation might need to allocate a handle
 * structure and the allocation operation might need to free a handle
 * structure.
 */

typedef struct handle {
    yaksu_handle_t id;
    const void *data;

    /* free handles are maintained as a linked list */
    struct handle *next;
    struct handle *prev;

    /* used handles are maintained as a hashmap */
    UT_hash_handle hh;
} handle_s;

#define HANDLE_CACHE_SIZE   (16384)

typedef struct handle_pool {
    pthread_mutex_t mutex;

    yaksu_handle_t next_handle; /* next brand-new handle (never allocated) */
    handle_s *free_handles;     /* list of handles that were allocated, but later freed */
    handle_s *used_handles;     /* hashmap of handles in use */

    /* the first few handles are stored in a direct access cache, so
     * we can get to these objects without having to grab a lock */
    handle_s *handle_cache[HANDLE_CACHE_SIZE];
} handle_pool_s;

static pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;

int yaksu_handle_pool_alloc(yaksu_handle_pool_s * pool)
{
    int rc = YAKSA_SUCCESS;
    handle_pool_s *handle_pool;

    pthread_mutex_lock(&global_mutex);

    handle_pool = malloc(sizeof(handle_pool_s));

    pthread_mutex_init(&handle_pool->mutex, NULL);
    handle_pool->next_handle = 0;
    handle_pool->free_handles = NULL;
    handle_pool->used_handles = NULL;

    for (int i = 0; i < HANDLE_CACHE_SIZE; i++)
        handle_pool->handle_cache[i] = NULL;

    *pool = (void *) handle_pool;

    pthread_mutex_unlock(&global_mutex);
    return rc;
}

int yaksu_handle_pool_free(yaksu_handle_pool_s pool)
{
    int rc = YAKSA_SUCCESS;
    handle_pool_s *handle_pool = (handle_pool_s *) pool;
    handle_s *el, *el_tmp;

    pthread_mutex_lock(&global_mutex);

    /* free objects from the used list if there are any */
    int count = HASH_COUNT(handle_pool->used_handles);
    if (count) {
#ifdef YAKSA_DEBUG
        fprintf(stderr, "[WARNING] yaksa: %d leaked handle pool objects\n", count);
        fflush(stderr);
#endif

        HASH_ITER(hh, handle_pool->used_handles, el, el_tmp) {
            HASH_DEL(handle_pool->used_handles, el);
            free(el);
        }
    }

    /* free objects from the free list */
    DL_FOREACH_SAFE(handle_pool->free_handles, el, el_tmp) {
        DL_DELETE(handle_pool->free_handles, el);
        free(el);
    }

    /* free self */
    pthread_mutex_destroy(&handle_pool->mutex);
    free(handle_pool);

    pthread_mutex_unlock(&global_mutex);
    return rc;
}

int yaksu_handle_pool_elem_alloc(yaksu_handle_pool_s pool, yaksu_handle_t * handle, void *data)
{
    int rc = YAKSA_SUCCESS;
    handle_pool_s *handle_pool = (handle_pool_s *) pool;

    pthread_mutex_lock(&handle_pool->mutex);

    handle_s *el;
    if (handle_pool->free_handles) {
        el = handle_pool->free_handles;
        DL_DELETE(handle_pool->free_handles, el);
    } else {
        el = (handle_s *) malloc(sizeof(handle_s));
        YAKSU_ERR_CHKANDJUMP(!el, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);

        el->id = handle_pool->next_handle;
        handle_pool->next_handle++;
    }

    el->data = data;
    HASH_ADD(hh, handle_pool->used_handles, id, sizeof(yaksu_handle_t), el);

    /* add object to cache */
    if (el->id < HANDLE_CACHE_SIZE) {
        handle_pool->handle_cache[el->id] = el;
    }

    *handle = el->id;

  fn_exit:
    pthread_mutex_unlock(&handle_pool->mutex);
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksu_handle_pool_elem_free(yaksu_handle_pool_s pool, yaksu_handle_t handle)
{
    int rc = YAKSA_SUCCESS;
    handle_pool_s *handle_pool = (handle_pool_s *) pool;

    pthread_mutex_lock(&handle_pool->mutex);

    handle_s *el = NULL;
    HASH_FIND(hh, handle_pool->used_handles, &handle, sizeof(yaksu_handle_t), el);
    assert(el);

    DL_PREPEND(handle_pool->free_handles, el);
    HASH_DEL(handle_pool->used_handles, el);

    /* remove object from cache */
    if (handle < HANDLE_CACHE_SIZE) {
        handle_pool->handle_cache[handle] = NULL;
    }

    pthread_mutex_unlock(&handle_pool->mutex);
    return rc;
}

int yaksu_handle_pool_elem_get(yaksu_handle_pool_s pool, yaksu_handle_t handle, const void **data)
{
    int rc = YAKSA_SUCCESS;
    handle_pool_s *handle_pool = (handle_pool_s *) pool;
    handle_s *el = NULL;

    if (handle < HANDLE_CACHE_SIZE) {
        assert(handle_pool->handle_cache[handle]);
        el = handle_pool->handle_cache[handle];
    } else {
        pthread_mutex_lock(&handle_pool->mutex);
        HASH_FIND(hh, handle_pool->used_handles, &handle, sizeof(yaksu_handle_t), el);
        pthread_mutex_unlock(&handle_pool->mutex);
    }

    assert(el);
    *data = el->data;

    return rc;
}
