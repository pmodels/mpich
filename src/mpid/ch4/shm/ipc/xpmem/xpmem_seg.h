/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef XPMEM_SEG_H_INCLUDED
#define XPMEM_SEG_H_INCLUDED

#include "xpmem_types.h"

int MPIDI_XPMEMI_segtree_init(MPIDI_XPMEMI_segtree_t * tree);
int MPIDI_XPMEMI_segtree_delete_all(MPIDI_XPMEMI_segtree_t * tree);
int MPIDI_XPMEMI_seg_regist(int node_rank, size_t size,
                            void *remote_vaddr, void **vaddr, MPIDI_XPMEMI_segtree_t * segcache);

#endif /* XPMEM_SEG_H_INCLUDED */
