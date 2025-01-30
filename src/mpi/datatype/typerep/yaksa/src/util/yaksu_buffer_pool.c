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
 * The pool contains three lists/hashmaps:
 *
 *  1. chunks -- this is the list of chunks that were allocated.  This
 *     is mainly to keep track of the start addresses of the buffers
 *     that were allocated, so we can eventually free them.
 *
 *  2. free_elems -- this is a doubly-linked list of free elements.
 *     Whenever a new chunk is allocated, it is split into a number of
 *     elements and each element is added to this list.  This makes
 *     element allocation O(1) time, when there is a free element
 *     available.  When no free element is available, we need
 *     O(elements_per_chunk) time to add all the elements in the new
 *     chunk to this list.
 *
 *  3. used_elems -- this is a hashmap that allows us to lookup the
 *     element address based on the user buffer address.  This makes
 *     freeing an element O(1) time (assuming that the hashmap has no
 *     conflicts), because we can simply lookup the element address
 *     based on the element ID or buffer address and then add it back
 *     to the free_elems list.
 */

typedef struct elem {
    void *buf;

    /* for the free list */
    struct elem *next;
    struct elem *prev;

    /* for the used list */
    UT_hash_handle hh;
} elem_s;

typedef struct chunk {
    void *slab;
    struct chunk *next;
    struct chunk *prev;
} chunk_s;

typedef struct pool_head {
    uintptr_t elemsize;
    unsigned int elems_in_chunk;
    unsigned int maxelems;

    yaksu_malloc_fn malloc_fn;
    yaksu_free_fn free_fn;
    void *state;

    pthread_mutex_t mutex;

    unsigned int current_num_chunks;
    unsigned int max_num_chunks;
    chunk_s *chunks;

    elem_s *free_elems;         /* list of free elements */
    elem_s *used_elems;         /* hash table of used elements */
} pool_head_s;

static pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;

int yaksu_buffer_pool_alloc(uintptr_t elemsize, unsigned int elems_in_chunk, unsigned int maxelems,
                            yaksu_malloc_fn malloc_fn, yaksu_free_fn free_fn, void *state,
                            yaksu_buffer_pool_s * pool)
{
    int rc = YAKSA_SUCCESS;
    pool_head_s *pool_head;

    pthread_mutex_lock(&global_mutex);

    pool_head = malloc(sizeof(pool_head_s));

    pool_head->elemsize = elemsize;
    pool_head->elems_in_chunk = elems_in_chunk;
    pool_head->maxelems = maxelems;

    pool_head->malloc_fn = malloc_fn;
    pool_head->free_fn = free_fn;
    pool_head->state = state;

    pthread_mutex_init(&pool_head->mutex, NULL);

    pool_head->current_num_chunks = 0;
    pool_head->max_num_chunks = (maxelems / elems_in_chunk) + ! !(maxelems % elems_in_chunk);
    pool_head->chunks = NULL;
    pool_head->free_elems = NULL;
    pool_head->used_elems = NULL;

    *pool = (void *) pool_head;

    pthread_mutex_unlock(&global_mutex);
    return rc;
}

int yaksu_buffer_pool_free(yaksu_buffer_pool_s pool)
{
    int rc = YAKSA_SUCCESS;
    pool_head_s *pool_head = (pool_head_s *) pool;

    pthread_mutex_lock(&global_mutex);

    /* throw a warning if the used hashmap is not empty */
#ifdef YAKSA_DEBUG
    int count = HASH_COUNT(pool_head->used_elems);
    if (count) {
        fprintf(stderr, "[WARNING] yaksa: %d leaked buffer pool objects\n", count);
        fflush(stderr);
    }
#endif

    /* free the freelist elements */
    elem_s *el, *el_tmp;
    DL_FOREACH_SAFE(pool_head->free_elems, el, el_tmp) {
        DL_DELETE(pool_head->free_elems, el);
        free(el);
    }

    /* free the buffers associated with each chunk */
    chunk_s *chunk, *tmp;
    DL_FOREACH_SAFE(pool_head->chunks, chunk, tmp) {
        DL_DELETE(pool_head->chunks, chunk);
        pool_head->free_fn(chunk->slab, pool_head->state);
        free(chunk);
    }

    /* free self */
    pthread_mutex_destroy(&pool_head->mutex);
    free(pool_head);

    pthread_mutex_unlock(&global_mutex);
    return rc;
}

int yaksu_buffer_pool_elem_alloc(yaksu_buffer_pool_s pool, void **elem)
{
    int rc = YAKSA_SUCCESS;
    pool_head_s *pool_head = (pool_head_s *) pool;

    pthread_mutex_lock(&pool_head->mutex);

    *elem = NULL;
    chunk_s *chunk = NULL;
    if (pool_head->free_elems == NULL) {
        /* no more free elements left; see if we can allocate more
         * chunks */
        assert(pool_head->current_num_chunks <= pool_head->max_num_chunks);
        if (pool_head->current_num_chunks == pool_head->max_num_chunks)
            goto fn_exit;

        /* allocate a new chunk */
        chunk = (chunk_s *) malloc(sizeof(chunk_s));
        YAKSU_ERR_CHKANDJUMP(!chunk, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);

        chunk->slab = pool_head->malloc_fn(pool_head->elems_in_chunk * pool_head->elemsize,
                                           pool_head->state);
        YAKSU_ERR_CHKANDJUMP(!chunk->slab, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);

        /* keep track of the allocated chunk */
        DL_APPEND(pool_head->chunks, chunk);

        /* append each element to the free list */
        for (int i = 0; i < pool_head->elems_in_chunk; i++) {
            elem_s *el = (elem_s *) malloc(sizeof(elem_s));
            YAKSU_ERR_CHKANDJUMP(!el, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);

            el->buf = (char *) chunk->slab + (i * pool_head->elemsize);
            DL_APPEND(pool_head->free_elems, el);
        }

        pool_head->current_num_chunks++;
    }

    /* we should now have some free elements */
    assert(pool_head->free_elems);

    /* delete the first element in the free list and add it to the
     * used hash */
    elem_s *el;
    el = pool_head->free_elems;
    DL_DELETE(pool_head->free_elems, el);
    HASH_ADD_PTR(pool_head->used_elems, buf, el);

    *elem = el->buf;

  fn_exit:
    pthread_mutex_unlock(&pool_head->mutex);
    return rc;
  fn_fail:
    if (chunk) {
        free(chunk->slab);
        free(chunk);
    }
    goto fn_exit;
}

int yaksu_buffer_pool_elem_free(yaksu_buffer_pool_s pool, void *elem)
{
    int rc = YAKSA_SUCCESS;
    pool_head_s *pool_head = (pool_head_s *) pool;

    pthread_mutex_lock(&pool_head->mutex);

    /* find the element in the used hashmap */
    elem_s *el;
    HASH_FIND_PTR(pool_head->used_elems, &elem, el);
    assert(el);

    /* delete the element from the used hashmap and add it to the free
     * list */
    HASH_DEL(pool_head->used_elems, el);
    DL_PREPEND(pool_head->free_elems, el);

    pthread_mutex_unlock(&pool_head->mutex);
    return rc;
}
