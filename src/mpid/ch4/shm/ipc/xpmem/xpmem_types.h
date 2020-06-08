/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef XPMEM_TYPES_H_INCLUDED
#define XPMEM_TYPES_H_INCLUDED

#include <xpmem.h>

#define MPIDI_XPMEMI_PERMIT_VALUE ((void *)0600)
#define MPIDI_XPMEMI_SEG_PREALLOC 8     /* Number of segments to preallocate in the "direct" block */

typedef struct MPIDI_XPMEMI_seg {
    MPIR_OBJECT_HEADER;
    /* AVL-tree internal components start */
    struct MPIDI_XPMEMI_seg *parent;
    struct MPIDI_XPMEMI_seg *left;
    struct MPIDI_XPMEMI_seg *right;
    uint64_t height;            /* height of this subtree */
    /* AVL-tree internal components end */

    uint64_t low;               /* page aligned low address of remote seg */
    uint64_t high;              /* page aligned high address of remote seg */
    void *vaddr;                /* virtual address attached in current process */
} MPIDI_XPMEMI_seg_t;

typedef struct MPIDI_XPMEMI_segtree {
    MPIDI_XPMEMI_seg_t *root;
    int tree_size;
    MPID_Thread_mutex_t lock;
} MPIDI_XPMEMI_segtree_t;

typedef struct {
    xpmem_segid_t remote_segid;
    xpmem_apid_t apid;
    MPIDI_XPMEMI_segtree_t segcache_ubuf;       /* AVL tree based segment cache for user buffer */
} MPIDI_XPMEMI_segmap_t;

typedef struct {
    xpmem_segid_t segid;        /* my local segid associated with entire address space */
    MPIDI_XPMEMI_segmap_t *segmaps;     /* remote seg info for every local processes. */
    size_t sys_page_sz;
} MPIDI_XPMEMI_global_t;

extern MPIDI_XPMEMI_global_t MPIDI_XPMEMI_global;
extern MPIR_Object_alloc_t MPIDI_XPMEMI_seg_mem;

#ifdef MPL_USE_DBG_LOGGING
extern MPL_dbg_class MPIDI_XPMEMI_DBG_GENERAL;
#endif
#define XPMEM_TRACE(...) \
    MPL_DBG_MSG_FMT(MPIDI_XPMEMI_DBG_GENERAL,VERBOSE,(MPL_DBG_FDEST, "XPMEM "__VA_ARGS__))

#endif /* XPMEM_TYPES_H_INCLUDED */
