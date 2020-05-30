/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef XPMEM_PRE_H_INCLUDED
#define XPMEM_PRE_H_INCLUDED

#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
#include <xpmem.h>
#endif

typedef struct MPIDI_XPMEMI_seg MPIDI_XPMEMI_seg_t;

/* memory handle definition */
typedef struct {
    uint64_t src_offset;
} MPIDI_XPMEM_mem_handle_t;

typedef struct {
    MPIDI_XPMEMI_seg_t *seg_ptr;
} MPIDI_XPMEM_mem_seg_t;

/* request extension definition */
typedef struct {
    int dummy;
} MPIDI_XPMEM_am_request_t;

#endif /* XPMEM_PRE_H_INCLUDED */
