/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#ifndef _ZM_HZDPTR_H_
#define _ZM_HZDPTR_H_
#include "common/zm_common.h"
#include "list/zm_sdlist.h"

/* several lock-free objects only require one or two hazard pointers,
 * thus the current default value. We might increase it in the future
 * in case it is no longer sufficient. */
#define ZM_HZDPTR_NUM 2 /* K */

typedef zm_ptr_t zm_hzdptr_t;
/* Forward declaration(s) */
typedef struct zm_hzdptr_lnode zm_hzdptr_lnode_t;

/* List to store hazard pointers. This list simulates a dynamic array.
 * It is lock-free and only supports the push_back() operation */
struct zm_hzdptr_lnode {
    zm_hzdptr_t hzdptrs[ZM_HZDPTR_NUM];
    zm_atomic_flag_t active;
    zm_sdlist_t rlist;
    zm_atomic_ptr_t next;
};

extern zm_atomic_ptr_t zm_hzdptr_list; /* head of the list*/
extern zm_atomic_uint_t zm_hplist_length; /* N: correlates with the number
                                        of threads */

/* per-thread pointer to its own hazard pointer node */
extern zm_thread_local zm_hzdptr_lnode_t* zm_my_hplnode;

static inline void zm_hzdptr_allocate() {
    zm_hzdptr_lnode_t *cur_hplnode = (zm_hzdptr_lnode_t*) zm_atomic_load(&zm_hzdptr_list, zm_memord_acquire);
    zm_ptr_t old_hplhead;
    while ((zm_ptr_t)cur_hplnode != ZM_NULL) {
        if(zm_atomic_flag_test_and_set(&cur_hplnode->active, zm_memord_acq_rel)) {
            cur_hplnode = (zm_hzdptr_lnode_t*)zm_atomic_load(&cur_hplnode->next, zm_memord_acquire);
            continue;
        }
        zm_my_hplnode = cur_hplnode;
        return;
    }
    zm_atomic_fetch_add(&zm_hplist_length, 1, zm_memord_acq_rel);
    /* Allocate and initialize a new node */
    cur_hplnode = malloc (sizeof *cur_hplnode);
    zm_atomic_flag_test_and_set(&cur_hplnode->active, zm_memord_acq_rel);
    zm_atomic_store(&cur_hplnode->next, ZM_NULL, zm_memord_release);
    do {
        old_hplhead = (zm_ptr_t) zm_atomic_load(&zm_hzdptr_list, zm_memord_acquire);
        zm_atomic_store(&cur_hplnode->next, old_hplhead, zm_memord_release);
    } while(!zm_atomic_compare_exchange_weak(&zm_hzdptr_list,
                                             &old_hplhead,
                                             (zm_ptr_t)cur_hplnode,
                                             zm_memord_release,
                                             zm_memord_acquire));
    zm_my_hplnode = cur_hplnode;
    for(int i=0; i<ZM_HZDPTR_NUM; i++)
        zm_my_hplnode->hzdptrs[i] = ZM_NULL;
    zm_sdlist_init(&zm_my_hplnode->rlist);
}

static inline void zm_hzdptr_scan() {
    zm_sdlist_t plist;
    /* stage 1: pull non-null values from hzdptr_list */
    zm_sdlist_init(&plist);
    zm_hzdptr_lnode_t *cur_hplnode = (zm_hzdptr_lnode_t*) zm_atomic_load(&zm_hzdptr_list, zm_memord_acquire);;
    while((zm_ptr_t)cur_hplnode != ZM_NULL) {
        for(int i=0; i<ZM_HZDPTR_NUM; i++) {
            if(cur_hplnode->hzdptrs[i] != ZM_NULL)
                zm_sdlist_push_back(&plist, (void*)cur_hplnode->hzdptrs[i]);
        }
        cur_hplnode = (zm_hzdptr_lnode_t*)zm_atomic_load(&cur_hplnode->next, zm_memord_acquire);
    }
    /* stage 2: search plist */
    zm_sdlnode_t *node = zm_sdlist_begin(zm_my_hplnode->rlist);
    while (node != NULL) {
        zm_sdlnode_t *next = zm_sdlist_next(*node);
        if(!zm_sdlist_remove(&plist, node->data))
            /* TODO: reuse instead of freeing */
            zm_sdlist_rmnode(&zm_my_hplnode->rlist, node);
        node = next;
    }
    zm_sdlist_free(&plist);
}

static inline void zm_hzdptr_helpscan() {
    zm_hzdptr_lnode_t *cur_hplnode = (zm_hzdptr_lnode_t*) zm_hzdptr_list;
    while((zm_ptr_t)cur_hplnode != ZM_NULL) {
        if(zm_atomic_flag_test_and_set(&cur_hplnode->active, zm_memord_acq_rel)) {
            cur_hplnode = (zm_hzdptr_lnode_t*)zm_atomic_load(&cur_hplnode->next, zm_memord_acquire);
            continue;
        }
        while(zm_sdlist_length(cur_hplnode->rlist) > 0) {
            zm_sdlnode_t *node = zm_sdlist_pop_front(&cur_hplnode->rlist);
            zm_sdlist_push_back(&zm_my_hplnode->rlist, node);
            if(zm_sdlist_length(zm_my_hplnode->rlist) >=
               2 * zm_atomic_load(&zm_hplist_length, zm_memord_acquire))
                zm_hzdptr_scan();
        }
        zm_atomic_flag_clear(&cur_hplnode->active, zm_memord_release);
        cur_hplnode = (zm_hzdptr_lnode_t*)zm_atomic_load(&cur_hplnode->next, zm_memord_acquire);
    }
}

static inline void zm_hzdptr_retire(zm_ptr_t node) {
    zm_sdlist_push_back(&zm_my_hplnode->rlist, (void*)node);
    if(zm_sdlist_length(zm_my_hplnode->rlist) >=
       2 * zm_atomic_load(&zm_hplist_length, zm_memord_acquire)) {
        zm_hzdptr_scan();
        zm_hzdptr_helpscan();
    }
}

static inline zm_hzdptr_t* zm_hzdptr_get() {
    if(zm_unlikely(zm_my_hplnode == NULL))
        zm_hzdptr_allocate();
    return zm_my_hplnode->hzdptrs;
}

#endif /* _ZM_HZDPTR_H_ */
