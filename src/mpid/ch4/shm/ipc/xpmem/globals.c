/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "xpmem_pre.h"
#include "xpmem_types.h"

MPIDI_XPMEMI_global_t MPIDI_XPMEMI_global;

#ifdef MPL_USE_DBG_LOGGING
MPL_dbg_class MPIDI_XPMEMI_DBG_GENERAL;
#endif

/* Preallocated segment objects */
/* TODO: should use memory pool API. Direct pool objects is not needed. */
MPIDI_XPMEMI_seg_t MPIDI_XPMEMI_seg_mem_direct[MPIDI_XPMEMI_SEG_PREALLOC];

MPIR_Object_alloc_t MPIDI_XPMEMI_seg_mem = { 0, 0, 0, 0, MPIR_INTERNAL,
    sizeof(MPIDI_XPMEMI_seg_t), MPIDI_XPMEMI_seg_mem_direct,
    MPIDI_XPMEMI_SEG_PREALLOC
};
