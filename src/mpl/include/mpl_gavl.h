/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPL_GAVL_H_INCLUDED
#define MPL_GAVL_H_INCLUDED

typedef void *MPL_gavl_tree_t;

int MPL_gavl_tree_create(void (*free_fn) (void *), MPL_gavl_tree_t * gavl_tree);
int MPL_gavl_tree_insert(MPL_gavl_tree_t gavl_tree, const void *addr, uintptr_t len,
                         const void *val);
int MPL_gavl_tree_search(MPL_gavl_tree_t gavl_tree, const void *addr, uintptr_t len, void **val);
int MPL_gavl_tree_free(MPL_gavl_tree_t gavl_tree);
int MPL_gavl_tree_delete(MPL_gavl_tree_t gavl_tree, const void *addr, uintptr_t len);

#endif /* MPL_GAVL_H_INCLUDED  */
