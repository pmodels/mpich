/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef XPMEM_SEG_H_INCLUDED
#define XPMEM_SEG_H_INCLUDED

#include "xpmem_types.h"

int MPIDI_XPMEMI_segtree_init(MPL_gavl_tree_t * tree);
int MPIDI_XPMEMI_seg_regist(int node_rank, uintptr_t size,
                            void *remote_vaddr, void **vaddr, MPL_gavl_tree_t segcache);
void MPIDI_XPMEM_seg_free(void *seg);

#endif /* XPMEM_SEG_H_INCLUDED */
