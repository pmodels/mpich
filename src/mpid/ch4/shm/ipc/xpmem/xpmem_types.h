/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef XPMEM_TYPES_H_INCLUDED
#define XPMEM_TYPES_H_INCLUDED

#include "mpidimpl.h"
#include <xpmem.h>

#define MPIDI_XPMEMI_PERMIT_VALUE ((void *)0600)

typedef struct MPIDI_XPMEMI_seg {
    uintptr_t remote_align_addr;
    uintptr_t att_vaddr;
} MPIDI_XPMEMI_seg_t;

typedef struct {
    xpmem_segid_t remote_segid;
    xpmem_apid_t apid;
    MPL_gavl_tree_t segcache_ubuf;      /* AVL tree based segment cache for user buffer */
} MPIDI_XPMEMI_segmap_t;

typedef struct {
    xpmem_segid_t segid;        /* my local segid associated with entire address space */
    MPIDI_XPMEMI_segmap_t *segmaps;     /* remote seg info for every local processes. */
    size_t sys_page_sz;
} MPIDI_XPMEMI_global_t;

extern MPIDI_XPMEMI_global_t MPIDI_XPMEMI_global;

#ifdef MPL_USE_DBG_LOGGING
extern MPL_dbg_class MPIDI_XPMEMI_DBG_GENERAL;
#endif
#define XPMEM_TRACE(...) \
    MPL_DBG_MSG_FMT(MPIDI_XPMEMI_DBG_GENERAL,VERBOSE,(MPL_DBG_FDEST, "XPMEM "__VA_ARGS__))

#endif /* XPMEM_TYPES_H_INCLUDED */
